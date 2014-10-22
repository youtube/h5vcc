// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/tools/test_shell/simple_dom_storage_system.h"

#include "base/auto_reset.h"
#include "base/message_loop.h"
#include "googleurl/src/gurl.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebURL.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebStorageArea.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebStorageEventDispatcher.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebStorageNamespace.h"
#include "webkit/database/database_util.h"
#include "webkit/dom_storage/dom_storage_area.h"
#include "webkit/dom_storage/dom_storage_host.h"
#include "webkit/dom_storage/dom_storage_task_runner.h"

using dom_storage::DomStorageContext;
using dom_storage::DomStorageHost;
using dom_storage::DomStorageSession;
using webkit_database::DatabaseUtil;
using WebKit::WebStorageArea;
using WebKit::WebStorageNamespace;
using WebKit::WebStorageEventDispatcher;
using WebKit::WebString;
using WebKit::WebURL;

namespace {
const int kInvalidNamespaceId = -1;
}

class SimpleDomStorageSystem::NamespaceImpl : public WebStorageNamespace {
 public:
  explicit NamespaceImpl(const base::WeakPtr<SimpleDomStorageSystem>& parent);
  NamespaceImpl(const base::WeakPtr<SimpleDomStorageSystem>& parent,
                int session_namespace_id);
  virtual ~NamespaceImpl();
  virtual WebStorageArea* createStorageArea(const WebString& origin) OVERRIDE;
  virtual WebStorageNamespace* copy() OVERRIDE;
  virtual bool isSameNamespace(const WebStorageNamespace&) const OVERRIDE;

 private:
  DomStorageContext* Context() {
    if (!parent_.get())
      return NULL;
    return parent_->context_.get();
  }

  base::WeakPtr<SimpleDomStorageSystem> parent_;
  int namespace_id_;
};

class SimpleDomStorageSystem::AreaImpl : public WebStorageArea {
 public:
  AreaImpl(const base::WeakPtr<SimpleDomStorageSystem>& parent,
          int namespace_id, const GURL& origin);
  virtual ~AreaImpl();
  virtual unsigned length() OVERRIDE;
  virtual WebString key(unsigned index) OVERRIDE;
  virtual WebString getItem(const WebString& key) OVERRIDE;
  virtual void setItem(const WebString& key, const WebString& newValue,
                       const WebURL& pageUrl, Result&) OVERRIDE;
  virtual void removeItem(const WebString& key,
                          const WebURL& pageUrl) OVERRIDE;
  virtual void clear(const WebURL& pageUrl) OVERRIDE;

 private:
  DomStorageHost* Host() {
    if (!parent_.get())
      return NULL;
    return parent_->host_.get();
  }

  base::WeakPtr<SimpleDomStorageSystem> parent_;
  int connection_id_;
};

class SimpleDomStorageSystem::TaskRunnerImpl
    : public dom_storage::DomStorageTaskRunner {
 public:
  TaskRunnerImpl();

  virtual bool PostDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay) OVERRIDE;

  virtual bool PostShutdownBlockingTask(
      const tracked_objects::Location& from_here,
      SequenceID sequence_id,
      const base::Closure& task) OVERRIDE;

  virtual bool IsRunningOnSequence(SequenceID sequence_id) const OVERRIDE;

 protected:
  virtual ~TaskRunnerImpl();

 private:
  MessageLoop *message_loop_;
};

// NamespaceImpl -----------------------------

SimpleDomStorageSystem::NamespaceImpl::NamespaceImpl(
    const base::WeakPtr<SimpleDomStorageSystem>& parent)
    : parent_(parent),
      namespace_id_(dom_storage::kLocalStorageNamespaceId) {
}

SimpleDomStorageSystem::NamespaceImpl::NamespaceImpl(
    const base::WeakPtr<SimpleDomStorageSystem>& parent,
    int session_namespace_id)
    : parent_(parent),
      namespace_id_(session_namespace_id) {
}

SimpleDomStorageSystem::NamespaceImpl::~NamespaceImpl() {
  if (namespace_id_ == dom_storage::kLocalStorageNamespaceId ||
      namespace_id_ == kInvalidNamespaceId || !Context()) {
    return;
  }
  Context()->DeleteSessionNamespace(namespace_id_, false);
}

WebStorageArea* SimpleDomStorageSystem::NamespaceImpl::createStorageArea(
    const WebString& origin) {
  return new AreaImpl(parent_, namespace_id_, GURL(origin));
}

WebStorageNamespace* SimpleDomStorageSystem::NamespaceImpl::copy() {
  DCHECK_NE(dom_storage::kLocalStorageNamespaceId, namespace_id_);
  int new_id = kInvalidNamespaceId;
  if (Context()) {
    new_id = Context()->AllocateSessionId();
    Context()->CloneSessionNamespace(namespace_id_, new_id, std::string());
  }
  return new NamespaceImpl(parent_, new_id);
}

bool SimpleDomStorageSystem::NamespaceImpl::isSameNamespace(
    const WebStorageNamespace& other) const {
  const NamespaceImpl* other_impl = static_cast<const NamespaceImpl*>(&other);
  return namespace_id_ == other_impl->namespace_id_;
}

// AreaImpl -----------------------------

SimpleDomStorageSystem::AreaImpl::AreaImpl(
    const base::WeakPtr<SimpleDomStorageSystem>& parent,
    int namespace_id, const GURL& origin)
    : parent_(parent),
      connection_id_(0) {
  if (Host()) {
    connection_id_ = (parent_->next_connection_id_)++;
    Host()->OpenStorageArea(connection_id_, namespace_id, origin);
  }
}

SimpleDomStorageSystem::AreaImpl::~AreaImpl() {
  if (Host())
    Host()->CloseStorageArea(connection_id_);
}

unsigned SimpleDomStorageSystem::AreaImpl::length() {
  if (Host())
    return Host()->GetAreaLength(connection_id_);
  return 0;
}

WebString SimpleDomStorageSystem::AreaImpl::key(unsigned index) {
  if (Host())
    return Host()->GetAreaKey(connection_id_, index);
  return NullableString16(true);
}

WebString SimpleDomStorageSystem::AreaImpl::getItem(const WebString& key) {
  if (Host())
    return Host()->GetAreaItem(connection_id_, key);
  return NullableString16(true);
}

void SimpleDomStorageSystem::AreaImpl::setItem(
    const WebString& key, const WebString& newValue,
    const WebURL& pageUrl, Result& result) {
  result = ResultBlockedByQuota;
  if (!Host())
    return;

  base::AutoReset<AreaImpl*> auto_reset(&parent_->area_being_processed_, this);
  NullableString16 unused;
  if (!Host()->SetAreaItem(connection_id_, key, newValue, pageUrl,
                           &unused))
    return;

  result = ResultOK;
}

void SimpleDomStorageSystem::AreaImpl::removeItem(
    const WebString& key, const WebURL& pageUrl) {
  if (!Host())
    return;

  base::AutoReset<AreaImpl*> auto_reset(&parent_->area_being_processed_, this);
  string16 notused;
  Host()->RemoveAreaItem(connection_id_, key, pageUrl, &notused);
}

void SimpleDomStorageSystem::AreaImpl::clear(const WebURL& pageUrl) {
  if (!Host())
    return;

  base::AutoReset<AreaImpl*> auto_reset(&parent_->area_being_processed_, this);
  Host()->ClearArea(connection_id_, pageUrl);
}

// TaskRunnerImpl -----------------------------

SimpleDomStorageSystem::TaskRunnerImpl::TaskRunnerImpl()
    : message_loop_(MessageLoop::current()) {
}

SimpleDomStorageSystem::TaskRunnerImpl::~TaskRunnerImpl() {
}

bool SimpleDomStorageSystem::TaskRunnerImpl::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  message_loop_->PostDelayedTask(from_here, task, delay);
  return true;
}

bool SimpleDomStorageSystem::TaskRunnerImpl::PostShutdownBlockingTask(
    const tracked_objects::Location&,
    SequenceID,
    const base::Closure& task) {
  // In lieu of shutdown-blocking functionality in MessageLoop, we will
  // ensure that this is called from the main thread as expected and run
  // the shutdown task synchronously.
  DCHECK(message_loop_ == MessageLoop::current());
  task.Run();
  return true;
}

bool SimpleDomStorageSystem::TaskRunnerImpl::IsRunningOnSequence(
    SequenceID) const {
  return MessageLoop::current() == message_loop_;
}

// SimpleDomStorageSystem -----------------------------

SimpleDomStorageSystem* SimpleDomStorageSystem::g_instance_;

SimpleDomStorageSystem::SimpleDomStorageSystem()
    : weak_factory_(this),
      context_(new DomStorageContext(FilePath(), FilePath(), NULL,
                                     new TaskRunnerImpl())),
      host_(new DomStorageHost(context_)),
      area_being_processed_(NULL),
      next_connection_id_(1) {
  DCHECK(!g_instance_);
  g_instance_ = this;
  context_->AddEventObserver(this);
}

SimpleDomStorageSystem::~SimpleDomStorageSystem() {
  g_instance_ = NULL;
  host_.reset();
  context_->RemoveEventObserver(this);
  context_->Shutdown();
}

WebStorageNamespace* SimpleDomStorageSystem::CreateLocalStorageNamespace() {
  return new NamespaceImpl(weak_factory_.GetWeakPtr());
}

WebStorageNamespace* SimpleDomStorageSystem::CreateSessionStorageNamespace() {
  int id = context_->AllocateSessionId();
  context_->CreateSessionNamespace(id, std::string());
  return new NamespaceImpl(weak_factory_.GetWeakPtr(), id);
}

void SimpleDomStorageSystem::OnDomStorageItemSet(
    const dom_storage::DomStorageArea* area,
    const string16& key,
    const string16& new_value,
    const NullableString16& old_value,
    const GURL& page_url) {
  DispatchDomStorageEvent(area, page_url,
                          NullableString16(key, false),
                          NullableString16(new_value, false),
                          old_value);
}

void SimpleDomStorageSystem::OnDomStorageItemRemoved(
    const dom_storage::DomStorageArea* area,
    const string16& key,
    const string16& old_value,
    const GURL& page_url) {
  DispatchDomStorageEvent(area, page_url,
                          NullableString16(key, false),
                          NullableString16(true),
                          NullableString16(old_value, false));
}

void SimpleDomStorageSystem::OnDomStorageAreaCleared(
    const dom_storage::DomStorageArea* area,
    const GURL& page_url) {
  DispatchDomStorageEvent(area, page_url,
                          NullableString16(true),
                          NullableString16(true),
                          NullableString16(true));
}

void SimpleDomStorageSystem::DispatchDomStorageEvent(
    const dom_storage::DomStorageArea* area,
    const GURL& page_url,
    const NullableString16& key,
    const NullableString16& new_value,
    const NullableString16& old_value) {
  DCHECK(area_being_processed_);
  if (area->namespace_id() == dom_storage::kLocalStorageNamespaceId) {
    WebStorageEventDispatcher::dispatchLocalStorageEvent(
        key,
        old_value,
        new_value,
        area->origin(),
        page_url,
        area_being_processed_,
        true  /* originatedInProcess */);
  } else {
    NamespaceImpl session_namespace_for_event_dispatch(
        base::WeakPtr<SimpleDomStorageSystem>(), area->namespace_id());
    WebStorageEventDispatcher::dispatchSessionStorageEvent(
        key,
        old_value,
        new_value,
        area->origin(),
        page_url,
        session_namespace_for_event_dispatch,
        area_being_processed_,
        true  /* originatedInProcess */);
  }
}

void SimpleDomStorageSystem::Flush() {
  DCHECK(host_);
  if (host_) {
    host_->Flush();
  }
}

#if defined(__LB_SHELL__)
void SimpleDomStorageSystem::Reset() {
  DCHECK(host_);
  if (host_) {
    context_ = new DomStorageContext(FilePath(), FilePath(), NULL,
                                     new TaskRunnerImpl());
    context_->AddEventObserver(this);
    host_.reset(new DomStorageHost(context_));
  }
}
#endif
