/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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
#include "lb_console_values.h"

#include <sstream>

namespace LB {

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace {
class AutoPthreadLock {
 public:
  explicit AutoPthreadLock(pthread_mutex_t* mutex)
      : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }
  ~AutoPthreadLock() {
    pthread_mutex_unlock(mutex_);
  }
 private:
  pthread_mutex_t* mutex_;
};
}  // namespace

ConsoleValueManager* ConsoleValueManager::instance_ = NULL;
pthread_mutex_t ConsoleValueManager::instance_lock_ = PTHREAD_MUTEX_INITIALIZER;

ConsoleValueManager::ConsoleValueManager() {
  pthread_mutex_init(&hooks_mutex_, NULL);
  pthread_mutex_init(&cvals_mutex_, NULL);

  // Allocate these dynamically since ConsoleValueManager may live across
  // DLL boundaries and so this allows its size to be more consistent (and
  // avoids compiler warnings on some platforms).
  registered_vars_ = new NameVarMap();
  on_changed_hook_set_ = new CValSetType<OnChangedHook*>();

  AutoPthreadLock lock(&instance_lock_);
  assert(instance_ == NULL);
  instance_ = this;
}

ConsoleValueManager::~ConsoleValueManager() {
  {
    AutoPthreadLock lock(&instance_lock_);
    assert(instance_ == this);
    instance_ = NULL;
  }

  delete on_changed_hook_set_;
  delete registered_vars_;

  pthread_mutex_destroy(&cvals_mutex_);
  pthread_mutex_destroy(&hooks_mutex_);
}

void ConsoleValueManager::RegisterCVal(const CValDetail::CValBase* cval) {
  AutoPthreadLock lock(&cvals_mutex_);

  // CVals cannot share name.  Ff this assert is triggered, we are trying to
  // register more than one CVal with the same name, which this system is
  // not designed to handle.
  assert(registered_vars_->find(cval->GetName()) == registered_vars_->end());

  (*registered_vars_)[cval->GetName()] = cval;
}

void ConsoleValueManager::UnregisterCVal(const CValDetail::CValBase* cval) {
  AutoPthreadLock lock(&cvals_mutex_);

  NameVarMap::iterator found = registered_vars_->find(cval->GetName());
  assert(found != registered_vars_->end());
  if (found != registered_vars_->end()) {
    registered_vars_->erase(found);
  }
}

void ConsoleValueManager::PushValueChangedEvent(
    const CValDetail::CValBase* cval, const ConsoleGenericValue& value) {
  AutoPthreadLock lock(&hooks_mutex_);

  // Iterate through the hooks sending each of them the value changed event.
  for (OnChangeHookSet::iterator iter = on_changed_hook_set_->begin();
       iter != on_changed_hook_set_->end(); ++iter) {
    (*iter)->OnValueChanged(cval->GetName(), value);
  }
}

ConsoleValueManager::OnChangedHook::OnChangedHook() {
  ConsoleValueManager* cvm = ConsoleValueManager::GetInstance();

  AutoPthreadLock lock(&cvm->hooks_mutex_);

  // Register this hook with the ConsolveValueManager
  bool was_inserted = cvm->on_changed_hook_set_->insert(this).second;
  assert(was_inserted);
}

ConsoleValueManager::OnChangedHook::~OnChangedHook() {
  ConsoleValueManager* cvm = ConsoleValueManager::GetInstance();

  AutoPthreadLock lock(&cvm->hooks_mutex_);

  // Deregister this hook with the ConsolveValueManager
  size_t values_erased = cvm->on_changed_hook_set_->erase(this);
  assert(values_erased == 1);
}

std::set<std::string> ConsoleValueManager::GetOrderedCValNames() {
  std::set<std::string> ret;
  AutoPthreadLock lock(&cvals_mutex_);

  // Return all registered CVal names, as an std::set to show they are sorted.
  for (NameVarMap::const_iterator iter = registered_vars_->begin();
       iter != registered_vars_->end(); ++iter) {
    ret.insert(iter->first);
  }

  return ret;
}

ConsoleValueManager::ValueQueryResults ConsoleValueManager::GetCValStringValue(
    const std::string& name, bool pretty) {
  ValueQueryResults ret;

  AutoPthreadLock lock(&cvals_mutex_);

  // Return the value of a CVal, if it exists.  If it does not exist,
  // indicate that it does not exist.
  NameVarMap::const_iterator found = registered_vars_->find(name);
  if (found == registered_vars_->end()) {
    ret.valid = false;
  } else {
    ret.valid = true;
    ret.value = (pretty
                 ? found->second->GetValueAsPrettyString()
                 : found->second->GetValueAsString());
  }

  return ret;
}

ConsoleValueManager::ValueQueryResults ConsoleValueManager::GetValueAsString(
    const std::string& name) {
  return GetCValStringValue(name, false);
}

ConsoleValueManager::ValueQueryResults
ConsoleValueManager::GetValueAsPrettyString(
    const std::string& name) {
  return GetCValStringValue(name, true);
}

#endif  // #if defined(__LB_SHELL__ENABLE_CONSOLE__)

}  // namespace LB
