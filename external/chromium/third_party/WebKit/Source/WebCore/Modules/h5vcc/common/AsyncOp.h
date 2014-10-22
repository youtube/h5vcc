/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SOURCE_WEBCORE_MODULES_H5VCC_COMMON_ASYNCOP_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_COMMON_ASYNCOP_H_

#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

//////////////////////////////////////////////////////////////////////////////
// This file defines WebCore::AsyncOp and WebCore::AsyncOpProxy that are
// used to support asynchronous calls from JavaScript into the H5vcc layer.
//
// An asynchronous operation in H5vcc will be given an instance of
// AsyncOpProxy which it will use to notify WebKit once the operation was
// completed successfully or some error has occurred by calling:
//   AsyncOpProxy::success(result)
//   AsyncOpProxy::error(reason)
// Optionally, the operation can choose to update WebKit with its progress
// by calling:
//   AsyncOpProxy::progress(current, total)
// The above operations must be called from the WebKit main thread
//
// A WebKit object of type Class that is responsible for exposing an
// asynchronous function to JavaScript will create an instance of AsyncOp
// by calling:
//    OwnPtr<AsyncOp<SuccessType> > asyncOp = MakeAsyncOp(
//        this, adoptPtr(new DataContainer(...)),
//        &Class::onSuccess, &Class:onError, &Class::OnProgress);
//
// where SuccessType is the type of the result argument of Class::OnSuccess
//       DataContainer is an object that contains extra data like references
//       to JavaScript callbacks
//
// Then the WebKit object should return a AsyncOpProxy object to the H5vcc
// layer by calling:
//    return asyncOp.getProxy();
//
// It should be safe to destroy asyncOp at any point in time since
// AsyncOpProxy maintains a weak link to asyncOp
//
// The callbacks are expected to have the following types:
// onSuccess:  void (ClassType::*)(AsyncOp<SuccessType>*, SuccessType)
// onError:    void (ClassType::*)(AsyncOp<SuccessType>*, const WTF::String&)
// onProgress: void (ClassType::*)(AsyncOp<SuccessType>*, unsigned, unsigned)
//
// Each instance of AsyncOp can be given a user defined data container that
// will be owned by the instance. This allows the user to associate extra
// data with the operation that can be later on used when executing the
// callbacks. A typical example is storing references to JavaScript callbacks
// as shown in the following example:
//
// void ClassType::OnSuccess(AsyncOp<SuccessType>* asyncOp, SuccessType result)
// {
//    DataContainer* container = asyncOp->getContainer<DataContainer>();
//    container->jsSuccess->invoke(result);
// }
//////////////////////////////////////////////////////////////////////////////

template <typename SuccessT>
class AsyncOp;

template <typename SuccessT>
class AsyncOpProxy;

//////////////////////////////////////////////////////////////////////////////
// WebCore::internal helper utilities
//////////////////////////////////////////////////////////////////////////////

namespace internal {

// This is the error type that should be used by call error callbacks
struct ErrorArgType {
  typedef const WTF::String& type;
};

// Helper utility to extract type traits from a success callback signature
template <typename Sig>
struct FunctionTraits;

template <typename C, typename Op, typename Arg>
struct FunctionTraits<void (C::*)(Op, Arg)> {
  typedef C ClassType;
  typedef Arg SuccessType;
  typedef ErrorArgType::type ErrorType;

  typedef void (ClassType::*SuccessCallbackType) (Op, SuccessType);
  typedef void (ClassType::*ErrorCallbackType)   (Op, ErrorType);
  typedef void (ClassType::*ProgressCallbackType)(Op, unsigned, unsigned);
};

}  // namespace internal

//////////////////////////////////////////////////////////////////////////////
// WebCore::AsyncOp<SuccessT> definition
// --------------------------------------
// This is a base class that defines an interface for calling the callbacks.
// It also holds a reference to AsyncOpProxy which will be notified
// when the object is destroyed.
// AsyncOp should be privately owned by the WebKit class that implements
// the callbacks.
//
// The H5vcc layer should be given the result of AsyncOp::getProxy() which
// returns a reference to a AsyncOpProxy object. The returned object maintains
// a weak link to AsyncOp and it's safe to use it even if AsyncOp was already
// destroyed.
//
// AsyncOp also owns an instance of a user data container which can be retrieved
// by calling AsyncOp::getContainer<ContainerType>. Note that the returned
// pointer is borrowed and that the container will remain valid as long as
// AsyncOp is alive.
//////////////////////////////////////////////////////////////////////////////

template <typename SuccessT>
class AsyncOp {
 public:
  typedef SuccessT SuccessType;
  typedef internal::ErrorArgType::type ErrorType;
  typedef AsyncOpProxy<SuccessType> ProxyType;

  virtual ~AsyncOp();

  PassRefPtr<ProxyType> getProxy() const { return m_proxy; }

  template <typename ContainerType>
  ContainerType* getContainer() const {
    return reinterpret_cast<ContainerType*>(getContainerImpl());
  }

 protected:
  AsyncOp();

  virtual void success(SuccessType result) = 0;
  virtual void error(ErrorType reason) = 0;
  virtual void progress(unsigned current, unsigned total) = 0;
  virtual void* getContainerImpl() const = 0;

 private:
  RefPtr<ProxyType> m_proxy;

  // Allows AsyncOpProxy to access the protected methods
  friend class AsyncOpProxy<SuccessType>;
};

namespace internal {

//////////////////////////////////////////////////////////////////////////////
// WebCore::AsyncOpImpl<Functor, ContainerType> definition
// -------------------------------------------------------
// This class is created by MakeAsyncOp and is designed to implement the
// callback interface that was declared in AsyncOp.
//////////////////////////////////////////////////////////////////////////////

template <typename Functor, typename ContainerType>
class AsyncOpImpl
    : public AsyncOp<
          typename internal::FunctionTraits<Functor>::SuccessType> {
  typedef typename internal::FunctionTraits<Functor>::SuccessCallbackType
      SuccessCallbackType;
  typedef typename internal::FunctionTraits<Functor>::ErrorCallbackType
      ErrorCallbackType;
  typedef typename internal::FunctionTraits<Functor>::ProgressCallbackType
      ProgressCallbackType;
  typedef typename internal::FunctionTraits<Functor>::ClassType ClassType;
  typedef typename internal::FunctionTraits<Functor>::SuccessType SuccessType;
  typedef typename internal::FunctionTraits<Functor>::ErrorType ErrorType;

 public:
  AsyncOpImpl(ClassType* classInst,
              PassOwnPtr<ContainerType> container,
              SuccessCallbackType onSuccess,
              ErrorCallbackType onError)
      : m_classInst(classInst), m_container(container)
      , m_onSuccess(onSuccess), m_onError(onError), m_onProgress(NULL) {}

  AsyncOpImpl(ClassType* classInst,
              PassOwnPtr<ContainerType> container,
              SuccessCallbackType onSuccess,
              ErrorCallbackType onError,
              ProgressCallbackType onProgress)
      : m_classInst(classInst), m_container(container)
      , m_onSuccess(onSuccess), m_onError(onError), m_onProgress(onProgress) {}

 private:
  virtual void success(SuccessType result) {
    (m_classInst->*m_onSuccess)(this, result);
  }

  virtual void error(ErrorType reason) {
    (m_classInst->*m_onError)(this, reason);
  }

  virtual void progress(unsigned current, unsigned total) {
    if (m_onProgress)
      (m_classInst->*m_onProgress)(this, current, total);
  }

  virtual void* getContainerImpl() const {
    return m_container.get();
  }

  ClassType* m_classInst;
  OwnPtr<ContainerType> m_container;
  SuccessCallbackType m_onSuccess;
  ErrorCallbackType m_onError;
  ProgressCallbackType m_onProgress;
};

}  // namespace internal

//////////////////////////////////////////////////////////////////////////////
// WebCore::AsyncOpProxy<SuccessT> definition
// ---------------------------------------------
// This class is created by calling AsyncOp::proxy() and used by the H5vcc
// layer to notify WebKit when an asynchronous operation is completed.
// Since AsyncOpProxy maintains a weak link to the parent AsyncOp object,
// it should be safe to use it even if AsyncOp was already destroyed.
// AsyncOpProxy::success/error should be called only once.
//////////////////////////////////////////////////////////////////////////////

template <typename SuccessT>
class AsyncOpProxy
    : public WTF::ThreadSafeRefCounted<
          AsyncOpProxy<SuccessT> > {
 public:
  typedef SuccessT SuccessType;
  typedef internal::ErrorArgType::type ErrorType;
  typedef AsyncOp<SuccessType> AsyncOpType;

  void success(SuccessType result) {
    if (m_async) {
      m_async->success(result);
      disconnect();
    }
  }

  void error(ErrorType reason) {
    if (m_async) {
      m_async->error(reason);
      disconnect();
    }
  }

  void progress(unsigned current, unsigned total) {
    if (m_async) {
      m_async->progress(current, total);
    }
  }

 private:
  explicit AsyncOpProxy(AsyncOpType* async)
      : m_async(async) {}

  void disconnect() {
    m_async = NULL;
  }

  AsyncOpType* m_async;

  // Allows AsyncOp to call AsyncOpProxy::disconnect
  friend class AsyncOp<SuccessType>;
};

//////////////////////////////////////////////////////////////////////////////
// Additional WebCore::AsyncOp<SuccessT> definitions
// --------------------------------------------------
// The following function are defined here since they need to follow the
// AsyncOpProxy definition
//////////////////////////////////////////////////////////////////////////////

template <typename SuccessT>
AsyncOp<SuccessT>::AsyncOp() {
  m_proxy = adoptRef(new ProxyType(this));
}

template <typename SuccessT>
AsyncOp<SuccessT>::~AsyncOp() {
  // The object is about to get destroyed so make sure that we disconnect
  // the proxy. m_proxy should always be valid since we are holding a
  // reference.
  m_proxy->disconnect();
}

//////////////////////////////////////////////////////////////////////////////
// WebCore::MakeAsyncOp helper functions
// --------------------------------------
// MakeAsyncOp returns an OwnPtr to AsyncOp<SuccessType>
// This object should be stored in one of the WebKit object and a proxy
// to AsyncOp should be returned to H5vcc instead.
// There are two variants of MakeAsyncOp, one that supports an onProgress
// callback and one that does not.
//////////////////////////////////////////////////////////////////////////////

template <typename Functor, typename ContainerType>
PassOwnPtr<
    AsyncOp<
        typename internal::FunctionTraits<Functor>::SuccessType> >
MakeAsyncOp(
    typename internal::FunctionTraits<Functor>::ClassType* inst,
    PassOwnPtr<ContainerType> container,
    Functor onSuccess,
    typename internal::FunctionTraits<Functor>::ErrorCallbackType onError,
    typename internal::FunctionTraits<
        Functor>::ProgressCallbackType onProgress) {
  return adoptPtr(
      new internal::AsyncOpImpl<Functor, ContainerType>(
            inst, container, onSuccess, onError, onProgress));
}

template <typename Functor, typename ContainerType>
PassOwnPtr<
    AsyncOp<
        typename internal::FunctionTraits<Functor>::SuccessType> >
MakeAsyncOp(
    typename internal::FunctionTraits<Functor>::ClassType* inst,
    PassOwnPtr<ContainerType> container,
    Functor onSuccess,
    typename internal::FunctionTraits<Functor>::ErrorCallbackType onError) {
  return adoptPtr(
      new internal::AsyncOpImpl<Functor, ContainerType>(
          inst, container, onSuccess, onError));
}

}  // namespace WebCore

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_COMMON_ASYNCOP_H_
