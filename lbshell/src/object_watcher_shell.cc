/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "object_watcher_shell.h"

#include <sys/poll.h>

#include <map>
#include <stack>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/simple_thread.h"
#include "lb_memory_manager.h"
#include "lb_shell/lb_shell_constants.h"

namespace base {
namespace steel {

struct Watch {
  ObjectWatcher * watcher;            // associated ObjectWatcher instance
  int object;                         // the file descriptor being watched
  int watch_handle;                   // used for uniquely identifying watches
  MessageLoop * origin_loop;          // the origin thread's message loop
  ObjectWatcher::Delegate * delegate; // delegate to notify when signaled
  bool did_signal;                    // set when DoneWaiting is called
  MessagePumpShell::Mode mode;       // callback on read, write or both?

  void Run() {
    // The watcher may have already been torn down, in which case we need to
    // just get out of dodge.
    if (!watcher)
      return;

    DCHECK(did_signal);
    watcher->StopWatching();

    delegate->OnObjectSignaled(object);
  }
};

// To emulate the behavior of the former Task system:
static void WatchTask(Watch * watch) {
  watch->Run();
  delete watch;
}

// -----------------------------------------------------------------------------
// ObjectWatchMultiplexer
// this object runs an internal thread to block on the aggregate of all watched
// objects and poll them for activity.  the best emulation of callback-driven
// io I can muster on the PS3.
class ObjectWatchMultiplexer : public base::SimpleThread {
 public:
  ObjectWatchMultiplexer() :
      base::SimpleThread("ObjectWatchMultiplexer Thread",
          base::SimpleThread::Options(kObjectWatcherThreadStackSize,
                                      kObjectWatcherThreadPriority,
                                      kNetworkIOThreadAffinity)),
      non_empty_list_event_(true, false),
      max_watch_handle_(0),
      should_recompose_pollfd_array_(true),
      exit_(false) {
  }

  ~ObjectWatchMultiplexer() {
  }

  // blocking call to exit the internal thread
  void Join() {
    exit_ = true;
    // wake up the thread in case it's waiting on the empty list event
    non_empty_list_event_.Signal();
    // in any event it should timeout eventually and see the exit flag..
    base::SimpleThread::Join();
  }

  // although you can add different watches with the same file descriptor all day long
  // please don't add the same watch twice it will create an error.
  void AddWatch(Watch * watch) {
    DCHECK(watch);
    DCHECK(watch->watch_handle == 0);
    // grab the mutex to modify the watch map et al
    base::AutoLock lock(add_watch_list_lock_);
    should_recompose_pollfd_array_ = true;
    watch->watch_handle = ++max_watch_handle_;
    new_watch_map_.insert(WatchMap::value_type(watch->object, watch));
    non_empty_list_event_.Signal();
  }

  void RemoveWatch(Watch * watch) {
    DCHECK(watch);
    base::AutoLock lock(remove_watch_list_lock_);
    WatchMap::iterator start = watch_map_.lower_bound(watch->object);
    WatchMap::iterator end = watch_map_.upper_bound(watch->object);
    WatchMap::iterator it;
    // search within those elements having the same file descriptor
    for (it = start; it != end; it++)
      if (it->second->watch_handle == watch->watch_handle)
        break;
    if (it != end) {
      watch_map_.erase(it);
      should_recompose_pollfd_array_ = true;
    } else {
      base::AutoLock lock(add_watch_list_lock_);
      // not found, search new_watch_map_
      WatchMap::iterator start = new_watch_map_.lower_bound(watch->object);
      WatchMap::iterator end = new_watch_map_.upper_bound(watch->object);
      for (it = start; it != end; it++)
      if (it->second->watch_handle == watch->watch_handle)
        break;
      if (it != end) {
        new_watch_map_.erase(it);
        should_recompose_pollfd_array_ = true;
      }
    }
  }

 private:
  // the internal thread runloop
  void Run() {
    // we keep a stack of pointers to watches needing a callback and
    // subsequent removal from the map
    std::stack<WatchMap::iterator> watch_removal_stack;
    // separate stack of callbacks to process after releasing the
    // WatchMap mutex
    std::stack<Watch*> watch_callback_stack;

    while (!exit_) {
      // We need both locks here. Order is important
      remove_watch_list_lock_.Acquire();
      add_watch_list_lock_.Acquire();
      if (should_recompose_pollfd_array_)
        RecomposePollFDArray();
      add_watch_list_lock_.Release();
      remove_watch_list_lock_.Release();

      if (!pollfd_array_.empty()) {
        // timeout on a 100ms to wake up and see if we need
        // to modify the array of fds or exit..
        int event_count = poll(&pollfd_array_.front(),
                                     pollfd_array_.size(), kPollTimeout);
        // negative value returned means error...

        // we need the mutex to access the map. AddWatch() is on a different
        // map object so we dont need the add_watch_list_lock_ here.
        remove_watch_list_lock_.Acquire();
        DCHECK_LE(event_count, static_cast<int>(pollfd_array_.size()));
        for(int j = 0; j < pollfd_array_.size() && event_count > 0; ++j) {
          if (pollfd_array_[j].revents != 0) {
            event_count--;
            WatchMap::iterator start =
                watch_map_.lower_bound(pollfd_array_[j].fd);
            WatchMap::iterator stop =
                watch_map_.upper_bound(pollfd_array_[j].fd);
            for (WatchMap::iterator it = start; it != stop; it++) {
              // oh STL, your capacity for obfuscation never ceases to amaze.
              // this if/else block tries to match interest for each fd event
              // to the associated watchers.  If a watcher was watching for reads
              // and a read happened, then we add it to the stack of callbacks
              // to process.  Else if it was watching for writes and a write
              // happened then we do the same.
              // note that signaled watches are going to become the property
              // of their respectively owned MessageLoops and will be deleted
              // once executed, and so will be removed from the hash_map
              if ((pollfd_array_[j].revents & POLLIN) &&
                (it->second->mode & MessagePumpShell::WATCH_READ)) {
                watch_removal_stack.push(it);
                watch_callback_stack.push(it->second);
              }
              else if ((pollfd_array_[j].revents & POLLOUT) &&
                (it->second->mode & MessagePumpShell::WATCH_WRITE)) {
                watch_removal_stack.push(it);
                watch_callback_stack.push(it->second);
              }
            }
          }
        }  // for(int j = 0; j < pollfd_array_.size() && event_count > 0; ++j)

        // process all callbacks.  this must unfortunately be done inside
        // of the mutex to prevent the contention on watch objects being
        // deleted due to a call to ObjectWatcher::StopWatching that could
        // occur while the watch is on this callback stack
        while (!watch_callback_stack.empty()) {
          Watch * watch = watch_callback_stack.top();
          watch_callback_stack.pop();
          // signal this watch to run in the original thread context
          // of the calling thread message loop.  The watch will be
          // posted as a task to the MessageLoop which will take
          // ownership of the object and delete it
          watch->did_signal = true;
          watch->origin_loop->PostTask(FROM_HERE, base::Bind(WatchTask, watch));
        }
        // if we're going to signal something we should recompose
        // the array as we are going to be removing watches
        if (!watch_removal_stack.empty())
          should_recompose_pollfd_array_ = true;
        // now remove the callback elements from the current hash_map
        while (!watch_removal_stack.empty()) {
          watch_map_.erase(watch_removal_stack.top());
          watch_removal_stack.pop();
        }

        remove_watch_list_lock_.Release();
      } else {  // if (!pollfd_array_.empty())
        // sleep until flagged that something has been added to the list
        DCHECK(watch_map_.empty());
        // NOTE: You can't DCHECK(new_watch_map_.empty()) because we let go
        // of the lock already and new stuff could have been added before we
        // get here.  If that happens, the event will have already been
        // signaled and we won't have to wait.
        non_empty_list_event_.Wait();
      }  // if (!pollfd_array_.empty())
    } // while (!exit_)
  }

  // it is assumed that any call to this function is within the context
  // of a lock on watch_list_lock. This function touches both watch_map_
  // and new_watch_map_ so that both locks are needed.
  void RecomposePollFDArray() {
    remove_watch_list_lock_.AssertAcquired();
    add_watch_list_lock_.AssertAcquired();
    // process new requests and move them into working map
    WatchMap::iterator it;
    for (it = new_watch_map_.begin(); it != new_watch_map_.end(); it++) {
      watch_map_.insert(*it);
    }
    new_watch_map_.clear();
    // count the number of unique file descriptors in the map
    int count = 0;
    int last_fd = -1;
    for (it = watch_map_.begin(); it != watch_map_.end(); it++) {
      if (it->first != last_fd) {
          last_fd = it->first;
          count++;
      }
    }
    pollfd_array_.resize(count);

    // init pollfd_array_
    if (!pollfd_array_.empty()) {
      int j = -1;
      last_fd = -1;
      for (it = watch_map_.begin(); it != watch_map_.end(); it++) {
        if (it->first != last_fd) {
          // we keep j pointing at this current element for ORing in the watch
          // flags
          j++;
          last_fd = it->first;
          pollfd_array_[j].fd = last_fd;
          // assume we care about both, we will filter based on mode on callback
          pollfd_array_[j].events = 0;
          if (it->second->mode & MessagePumpShell::WATCH_READ)
            pollfd_array_[j].events |= POLLIN;
          if (it->second->mode & MessagePumpShell::WATCH_WRITE)
            pollfd_array_[j].events |= POLLOUT;
          pollfd_array_[j].revents = 0;
        } else {  // if (it->first != last_fd)
          // OR in the watch flags to the already initialized pollfd struct
          if (it->second->mode & MessagePumpShell::WATCH_READ)
            pollfd_array_[j].events |= POLLIN;
          if (it->second->mode & MessagePumpShell::WATCH_WRITE)
            pollfd_array_[j].events |= POLLOUT;
        }  // if (it->first != last_fd)
      }  // for (it = watch_map_.begin(); it != watch_map_.end(); it++)
    } else {  // if (!pollfd_array_.empty())
      // reset the empty event if we've completely emptied the watch list
      DCHECK(watch_map_.empty());
      DCHECK(new_watch_map_.empty());
      non_empty_list_event_.Reset();
    }  // if (!pollfd_array_.empty())
    // reset flag
    should_recompose_pollfd_array_ = false;
  }  // void RecomposePollFDArray()

#if defined(__LB_WIIU__)
  // 33ms is a good balance between quickly servicing new
  // sockets to poll, and not overloading the IO pipeline.
  static const int kPollTimeout = 33;
#else
  // TODO: Investigate whether decreasing this on other platforms shows
  // improvement in performance
  static const int kPollTimeout = 100;
#endif

  // used to let the thread block until it actually has _something_ to poll
  base::WaitableEvent non_empty_list_event_;

  // this lock protects watch_map_, should_recompose_pollfd_array_, and
  // max_watch_handle_ which is used to generate unique watch handles to
  // disambiguate watches on the same file descriptor
  base::Lock      add_watch_list_lock_;
  base::Lock      remove_watch_list_lock_;
  bool should_recompose_pollfd_array_;
  typedef std::multimap<int,Watch*> WatchMap;
  WatchMap watch_map_;      // current working watches
  WatchMap new_watch_map_;  // new watches
  int max_watch_handle_;

  std::vector<pollfd>  pollfd_array_;

  bool exit_;
};

// our singleton instance pointer
static ObjectWatchMultiplexer * OWMuxInstance = NULL;

// who watches the watchers?
// static
void ObjectWatcher::InitializeObjectWatcherSystem() {
  OWMuxInstance = LB_NEW ObjectWatchMultiplexer();
  OWMuxInstance->Start();
}

// static
void ObjectWatcher::TeardownObjectWatcherSystem() {
  OWMuxInstance->Join();
  delete OWMuxInstance;
  OWMuxInstance = NULL;
}

//-----------------------------------------------------------------------------

ObjectWatcher::ObjectWatcher() : watch_(NULL) {
}

ObjectWatcher::~ObjectWatcher() {
  StopWatching();
}

bool ObjectWatcher::StartWatching(int object,
                                  MessagePumpShell::Mode mode,
                                  Delegate * delegate) {
  DCHECK(OWMuxInstance != NULL);
  if (watch_) {
    NOTREACHED() << "Already watching an object";
    return false;
  }

  watch_ = LB_NEW Watch;
  watch_->watcher = this;
  watch_->object = object;
  watch_->mode = mode;
  watch_->origin_loop = MessageLoop::current();
  watch_->delegate = delegate;
  watch_->did_signal = false;
  watch_->watch_handle = 0;

  OWMuxInstance->AddWatch(watch_);

  // We need to know if the current message loop is going away so we can
  // prevent the wait thread from trying to access a dead message loop.
  MessageLoop::current()->AddDestructionObserver(this);
  return true;
}

bool ObjectWatcher::StopWatching() {
  DCHECK(OWMuxInstance != NULL);
  if (!watch_)
    return false;

  // make sure stop call happens on same thread as start call
  DCHECK(watch_->origin_loop == MessageLoop::current());

  // this will block until this watch has been removed from the mux
  OWMuxInstance->RemoveWatch(watch_);
  // let the watch know that the watcher has died, in case the watch is 
  // still sitting in a message loop.  See Watch::Run() to see that it
  // will bug out in that event.
  watch_->watcher = NULL;

  // if the watch was signaled, then it is now property of the MessageLoop
  // and will be deleted there.  It has also been safely removed from the
  // internal list of watches in the ObjectWatchMux.  Therefore if it hasn't
  // been signaled we delete it here to avoid leaking the watch
  if (!watch_->did_signal)
    delete watch_;

  watch_ = NULL;

  MessageLoop::current()->RemoveDestructionObserver(this);
  return true;
}

void ObjectWatcher::WillDestroyCurrentMessageLoop() {
  // Need to shutdown the watch so that we don't try to access the MessageLoop
  // after this point.
  StopWatching();
}

} // namespace steel
} // namespace base
