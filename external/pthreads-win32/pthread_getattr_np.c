#include "pthread.h"
#include "implement.h"

// Get a pthread_attr_t containing attributes of the given pthread_t.
int pthread_getattr_np(pthread_t tid, pthread_attr_t* attr) {
  ptw32_thread_t* thread_ptr;
  if (ptw32_is_attr(attr) != 0) {
    return EINVAL;
  }

  thread_ptr = (ptw32_thread_t*)to_internal(tid).p;
  (*attr)->stacksize = thread_ptr->stackSize;
  (*attr)->stackaddr = thread_ptr->stackAddr;
  (*attr)->detachstate = thread_ptr->detachState;
  (*attr)->param.sched_priority = thread_ptr->sched_priority;
  return 0;
}
