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
#ifndef SRC_LB_CONSOLE_VALUES_H_
#define SRC_LB_CONSOLE_VALUES_H_

#include <assert.h>
#include <pthread.h>
#include <stdint.h>

#if defined(__LB_LINUX__) || defined(__LB_ANDROID__)
#include <map>
#include <set>
#define CValMapType std::map
#define CValSetType std::set
#else
#include <hash_map>
#include <hash_set>
#define CValMapType std::hash_map
#define CValSetType std::hash_set
#endif

#include <set>
#include <sstream>
#include <string>
#include <vector>

// The console value system allows you to mark certain variables to be part
// of the console system and therefore analyzable and trackable by other
// systems.  All modifications to marked variables will trigger an event
// within a ConsoleValueManager class, which will in turn trigger an event
// in all attached hooks (e.g. memory logging systems, tracing systems, etc...).

// Each marked variable is known as a CVal, and every CVal has an associated
// name so that it can be looked up at run time.  Thus, you can only ever
// have one CVal of the same name existing at a time.

// You can mark a variable:
//
//   int memory_counter;
//
// as a CVal by wrapping the type with the CVal template type:
//
//   LB::CVal<int> memory_counter("Memory Counter", 0, "My memory counter");
//
// The first constructor parameter is the name of the CVal, the second is the
// initial value for the variable, and the third is the description.
//
// For the system to function, the singleton ConsoleValueManager class must
// be instantiated.  If a CVal is created before a ConsoleValueManager is
// instantiated, it will attach itself to the ConsoleValueManager the next
// time it is modified.  Thus, global CVal variables can be supported, but they
// will not be tracked until they are modified for the first time after
// the ConsoleValueManager is created.  At this point, while CVals will be
// tracked, you will only be notified of changes to CVals.
//
// To hook in to the ConsoleValueManager in order to receive CVal changed
// notifications, you should create a class that subclasses
// ConsoleValueManager::OnChangedHook and implements the OnValueChanged
// virtual method.  When this class is instantiated, it will automatically
// hook in to the singleton ConsoleValueManager object, which must exist
// at that point.
//

namespace LB {

// Most of the console value functionality will be unavailable without the
// console
#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace CValDetail {
class CValBase;
}

// An enumeration to allow CVals to track the type that they hold in a run-time
// variable.
enum ConsoleValType {
  kU32,
  kU64,
  kS32,
  kS64,
  kFloat,
  kDouble,
  kString,
};

namespace CValDetail {

// Introduce a Traits class so that we can convert from C++ type to
// ConsoleValType.
template <typename T>
struct Traits {
  // If you get a compiler error here, you must add a Traits class
  // specific to the variable type you would like to support.
  int UnsupportedCValType[0];
};
template <>
struct Traits<uint32_t> {
  static const ConsoleValType kTypeVal = kU32;
  static const bool kIsNumerical = true;
};
template <>
struct Traits<uint64_t> {
  static const ConsoleValType kTypeVal = kU64;
  static const bool kIsNumerical = true;
};
template <>
struct Traits<int32_t> {
  static const ConsoleValType kTypeVal = kS32;
  static const bool kIsNumerical = true;
};
template <>
struct Traits<int64_t> {
  static const ConsoleValType kTypeVal = kS64;
  static const bool kIsNumerical = true;
};
template <>
struct Traits<float> {
  static const ConsoleValType kTypeVal = kFloat;
  static const bool kIsNumerical = true;
};
template <>
struct Traits<double> {
  static const ConsoleValType kTypeVal = kDouble;
  static const bool kIsNumerical = true;
};
template <>
struct Traits<std::string> {
  static const ConsoleValType kTypeVal = kString;
  static const bool kIsNumerical = false;
};

// Provide methods to convert from an arbitrary type to a string, useful for
// systems that want to read the value of a CVal without caring about its
// type.
template <typename T>
std::string ValToString(const T& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <>
inline std::string ValToString<std::string>(const std::string& value) {
  return value;
}

// Helper function to implement the numerical branch of ValToPrettyString
template <typename T>
std::string NumericalValToPrettyString(const T& value) {
  struct {
    T threshold;
    T divide_by;
    const char* postfix;
  } thresholds[] = {
    {static_cast<T>(10000000), static_cast<T>(1000000), "M"},
    {static_cast<T>(10000), static_cast<T>(1000), "K"},
  };

  T divided_value = value;
  const char* postfix = "";

  for (int i = 0; i < sizeof(thresholds) / sizeof(thresholds[0]); ++i) {
    if (value >= thresholds[i].threshold) {
      divided_value = value / thresholds[i].divide_by;
      postfix = thresholds[i].postfix;
      break;
    }
  }

  std::ostringstream oss;
  oss << divided_value << postfix;
  return oss.str();
}

// Helper function for the subsequent ValToPrettyString function.
template <typename T, bool IsNumerical>
class ValToPrettyStringHelper {
};
template <typename T>
class ValToPrettyStringHelper<T, true> {
 public:
  static std::string Call(const T& value) {
    return NumericalValToPrettyString(value);
  }
};
template <typename T>
class ValToPrettyStringHelper<T, false> {
 public:
  static std::string Call(const T& value) {
    return ValToString(value);
  }
};

// As opposed to ValToString, here we take the opportunity to clean up the
// number a bit to make it easier for humans to digest.  For example, if a
// number is much larger than one million, we can divide it by one million
// and postfix it with an 'M'.
template <typename T>
std::string ValToPrettyString(const T& value) {
  return ValToPrettyStringHelper<T, Traits<T>::kIsNumerical>::Call(value);
}

// Helper function to emulate running a template function with a runtime
// type parameter by switching on the runtime type and calling the appropriate
// template function.
template <typename F>
void DispatchCValTypeToCompilerType(ConsoleValType type, F* functor) {
  switch (type) {
    case kU32: {
      functor->template Call<uint32_t>();
    } break;
    case kU64: {
      functor->template Call<uint64_t>();
    } break;
    case kS32: {
      functor->template Call<int32_t>();
    } break;
    case kS64: {
      functor->template Call<int64_t>();
    } break;
    case kFloat: {
      functor->template Call<float>();
    } break;
    case kDouble: {
      functor->template Call<double>();
    } break;
    case kString: {
      functor->template Call<std::string>();
    } break;
    default: {
      assert(false);  // Unknown CVal type.
    }
  }
}

// Use SFINAE to determine if one type is convertible to another.
// This code for is_convertible is copied from
// external/chromium/base/template_util.h.  Please see that file
// for more information.
namespace base {

template<class T, T v>
struct integral_constant {
  static const T value = v;
  typedef T value_type;
  typedef integral_constant<T, v> type;
};

typedef char YesType;

struct NoType {
  YesType dummy[2];
};

struct ConvertHelper {
  template <typename To>
  static YesType Test(To);

  template <typename To>
  static NoType Test(...);

  template <typename From>
  static From& Create();
};

template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(ConvertHelper::Test<To>(
                            ConvertHelper::Create<From>())) ==
                        sizeof(YesType)> {
};

}  // namespace base

// Helper class that will convert a value from type IN to OUT only if
// template argument DoCast is true.  If DoCast is false, this call
// will be a NOP.
template <typename IN_TYPE, typename OUT_TYPE, bool DoCast>
class ConvertIfPossible {
 public:
  static void Convert(const IN_TYPE&, OUT_TYPE*) {
  }
};
template <typename IN_TYPE, typename OUT_TYPE>
class ConvertIfPossible<IN_TYPE, OUT_TYPE, true> {
 public:
  static void Convert(const IN_TYPE& in, OUT_TYPE* out) {
    *out = OUT_TYPE(in);
  }
};

// Helper class to perform an implicit conversion on the stored type from
// IN_TYPE (passed in to Call() via CValTypeSwitcher) to OUT_TYPE, storing
// the result in out_value_ assuming the conversion is possible.  If the
// conversion is not possible, can_convert_ will be false.
template <typename OUT_TYPE>
class Converter {
 public:
  Converter(ConsoleValType type, void* value)
      : value_(value), can_convert_(false) {
    DispatchCValTypeToCompilerType(type, this);
  }
  template <typename IN_TYPE>
  void Call() {
    can_convert_ = CValDetail::base::is_convertible<IN_TYPE, OUT_TYPE>::value;
    // This next line collapses in to a NOP if the type is not convertible.
    LB::CValDetail::ConvertIfPossible<
        IN_TYPE, OUT_TYPE,
        CValDetail::base::is_convertible<IN_TYPE, OUT_TYPE>::value>
        ::Convert(*static_cast<IN_TYPE*>(value_), &out_value_);
  }

  bool can_convert() const { return can_convert_; }
  OUT_TYPE out_value() const { return out_value_; }

 private:
  void* value_;
  bool can_convert_;
  OUT_TYPE out_value_;
};

}  // namespace CValDetail

// This class is passed back to the CVal modified event handlers so that they
// can examine the new CVal value.  This class knows its type, which can be
// checked through GetType, and then the actual value can be retrieved by
// calling ConsoleGenericValue::AsNativeType<TYPE>() with the correct type.  If
// you don't care about the type, you can call ConsoleGenericValue::ToString()
// and retrieve the string version of the value.
class LB_BASE_EXPORT ConsoleGenericValue {
 public:
  // Return the value casted to the specified type.  The requested type must
  // match the contained value's actual native type.
  template<typename T>
  const T& AsNativeType() const {
    assert(IsNativeType<T>());
    return *static_cast<T*>(generic_value_);
  }

  // Returns true if the type of this value is exactly T.
  template<typename T>
  bool IsNativeType() const {
    return type_ == CValDetail::Traits<T>::kTypeVal;
  }

  // This function will perform an implicit type conversion or promotion on the
  // internal value (whatever type it may be) to the specified output type.
  // This can simplify code that deals with many different but similar types.
  // To be precise, it will return the value of OUT_TYPE(value).
  template<typename OUT_TYPE>
  OUT_TYPE Convert() const {
    CValDetail::Converter<OUT_TYPE> converter(type_, generic_value_);
    assert(converter.can_convert());
    return converter.out_value();
  }

  // Returns true if the type can be implicitly converted to the specified
  // OUT_TYPE.
  template<typename OUT_TYPE>
  bool CanConvert() const {
    CValDetail::Converter<OUT_TYPE> converter(type_, generic_value_);
    return converter.can_convert();
  }

  virtual std::string AsString() const = 0;
  virtual std::string AsPrettyString() const = 0;

 protected:
  ConsoleGenericValue(ConsoleValType type, void* value_mem)
      : type_(type)
      , generic_value_(value_mem) {
  }
  virtual ~ConsoleGenericValue() {}

 private:
  ConsoleValType type_;
  void* generic_value_;
};

namespace CValDetail {

// Internally used to store the actual typed value that a ConsoleGenericValue
// may contain.  This value actually stores the data as a copy and then
// passes a pointer (as a void*) to its parent class ConsoleGenericValue so
// that the value can be accessed from the parent class.  This class is
// intended to snapshot a CVal value before passing it along to event handlers
// ensuring that they all receive the same value.
template <typename T>
class SpecificValue : public ConsoleGenericValue {
 public:
  explicit SpecificValue(const T& value)
      : ConsoleGenericValue(Traits<T>::kTypeVal, &value_)
      , value_(value) {
  }
  SpecificValue(const SpecificValue<T>& other)
      : ConsoleGenericValue(Traits<T>::kTypeVal, &value_)
      , value_(other.value_) {
  }
  virtual ~SpecificValue() {}

  std::string AsString() const {
    return ValToString(value_);
  }

  std::string AsPrettyString() const {
    return ValToPrettyString(value_);
  }

 private:
  T value_;
};

}  // namespace CValDetail

// Manager class required for the CVal tracking system to function.  This is a
// singleton class that can be created anywhere, though only one of them can
// exist at a time.
class LB_BASE_EXPORT ConsoleValueManager {
 public:
  ConsoleValueManager();
  ~ConsoleValueManager();
  static ConsoleValueManager* GetInstance() {
    return instance_;
  }

  // In order for a system to receive notifications when a console variable
  // changes, it should create a subclass of OnChangedHook with OnValueChanged.
  // When instantiated, OnChangedHook will be called whenever a CVal changes.
  class LB_BASE_EXPORT OnChangedHook {
   public:
    OnChangedHook();
    virtual ~OnChangedHook();

    virtual void OnValueChanged(const std::string& name,
                                const ConsoleGenericValue& value) = 0;
  };

  // Returns a set containing the sorted names of all CVals currently registered
  std::set<std::string> GetOrderedCValNames();


  struct ValueQueryResults {
    bool valid;
    std::string value;
  };

  // Returns the value of the given CVal as a string.  The return value is a
  // pair, the first element is true if the CVal exists, in which case the
  // second element is the value (as a string).  If the CVal does not exist,
  // the value of the second element is undefined.
  ValueQueryResults GetValueAsString(const std::string& name);

  // Similar to the above, but formatting may be done on numerical values.
  // For example, large numbers may have metric postfixes appended to them.
  // i.e. 100000000 = 100M
  ValueQueryResults GetValueAsPrettyString(const std::string& name);


 private:
  // Called in CVal constructors to register/deregister themselves with the
  // system.
  void RegisterCVal(const CValDetail::CValBase* cval);
  void UnregisterCVal(const CValDetail::CValBase* cval);

  // Called whenever a CVal is modified, and does the work of notifying all
  // hooks.
  void PushValueChangedEvent(const CValDetail::CValBase* cval,
                             const ConsoleGenericValue& value);

  // Helper function to remove code duplication between GetValueAsString
  // and GetValueAsPrettyString.
  ValueQueryResults GetCValStringValue(const std::string& name,
                                       bool pretty);

  static ConsoleValueManager* instance_;
  // Guards access to instance_
  static pthread_mutex_t instance_lock_;  // TODO: (Optimize) Make a RW lock

  // Scoped reference to ConsoleValueManager such that ConsoleValueManager
  // is guaranteed not to be destroyed (if it exists in the first place) while
  // you hold this object.
  class LockedReference {
   public:
    LockedReference() {
      pthread_mutex_lock(&instance_lock_);
      manager_ = ConsoleValueManager::GetInstance();
      if (!manager_) {
        pthread_mutex_unlock(&ConsoleValueManager::instance_lock_);
      }
    }
    ~LockedReference() {
      if (manager_) {
        pthread_mutex_unlock(&ConsoleValueManager::instance_lock_);
      }
    }
    ConsoleValueManager* get() { return manager_; }

   private:
    ConsoleValueManager* manager_;
  };

  // Mutex that protects against changes to hooks
  pthread_mutex_t hooks_mutex_;  // TODO: (Optimize) Make a RW lock

  // Mutex that protects against CVals being registered/deregistered
  pthread_mutex_t cvals_mutex_;  // TODO: (Optimize) Make a RW lock

  // The actual value registry, mapping CVal name to actual CVal object.
  typedef CValMapType<std::string, const CValDetail::CValBase*> NameVarMap;
  NameVarMap* registered_vars_;

  // The set of hooks that we should notify whenever a CVal is modified.
  typedef CValSetType<OnChangedHook*> OnChangeHookSet;
  OnChangeHookSet* on_changed_hook_set_;

  template <typename T>
  friend class CVal;
};

namespace CValDetail {

class CValBase {
 public:
  CValBase(const std::string& name, ConsoleValType type,
           const std::string& description)
      : name_(name)
      , description_(description)
      , type_(type) {
  }

  const std::string& GetName() const { return name_; }
  const std::string& GetDescription() const { return description_; }
  ConsoleValType GetType() const { return type_; }

  virtual std::string GetValueAsString() const = 0;
  virtual std::string GetValueAsPrettyString() const = 0;

 private:
  std::string name_;
  std::string description_;
  ConsoleValType type_;
};

}  // namespace CValDetail

// This is a wrapper class that marks that we wish to track a value through
// the console value system.  If a CVal is created before the singleton
// ConsoleValueManager instance, it will register itself the next time it is
// modified and the ConsoleValueManager instance exists.
template <typename T>
class CVal : public CValDetail::CValBase {
 public:
  CVal(const std::string& name,
       const T& initial_value,
       const std::string& description)
      : CValDetail::CValBase(
            name, CValDetail::Traits<T>::kTypeVal, description)
      , value_(initial_value) {
    CommonConstructor();
  }
  CVal(const std::string& name,
       const std::string& description)
      : CValDetail::CValBase(
            name, CValDetail::Traits<T>::kTypeVal, description) {
    CommonConstructor();
  }
  ~CVal() {
    if (registered_) {
      ConsoleValueManager::LockedReference manager;
      // Unregister ourselves with the system
      if (manager.get()) {
        manager.get()->UnregisterCVal(this);
      }
    }
  }

  operator T() const {
    return value_;
  }
  const CVal<T>& operator =(const T& rhs) {
    if (value_ != rhs) {
      value_ = rhs;
      OnValueChanged();
    }

    return *this;
  }

  const CVal<T>& operator +=(const T& rhs) {
    value_ += rhs;
    OnValueChanged();
    return *this;
  }

  const CVal<T>& operator -=(const T& rhs) {
    value_ -= rhs;
    OnValueChanged();
    return *this;
  }

  std::string GetValueAsString() const {
    // Can be called to get the value of a CVal without knowing the type first.
    return CValDetail::ValToString<T>(value_);
  }

  std::string GetValueAsPrettyString() const {
    // Similar to GetValueAsString(), but it will also format the string to
    // do things like make very large numbers more readable.
    return CValDetail::ValToPrettyString<T>(value_);
  }

 private:
  void CommonConstructor() {
    registered_ = false;
    ConsoleValueManager::LockedReference manager;
    TryToRegisterWithManager(&manager);
  }

  void TryToRegisterWithManager(ConsoleValueManager::LockedReference* manager) {
    if (manager->get()) {
      // Only register if we haven't already.
      if (!registered_) {
        manager->get()->RegisterCVal(this);
        registered_ = true;
      }
    }
  }

  void OnValueChanged() {
    ConsoleValueManager::LockedReference manager;
    // If we're still not attached to a manager, try again now.  This might
    // happen if the CVal is a global variable.
    TryToRegisterWithManager(&manager);
    if (manager.get()) {
      manager.get()->PushValueChangedEvent(
          this, CValDetail::SpecificValue<T>(value_));
    }
  }

  T value_;
  mutable bool registered_;
};

#else  // #if defined(__LB_SHELL__ENABLE_CONSOLE__)

// In Gold builds, CVals will be as similar as possible to their underlying
// types.
template <typename T>
class CVal {
 public:
  CVal(const std::string& name,
       const T& initial_value,
       const std::string& description)
      : value_(initial_value) {}
  CVal(const std::string& name,
       const std::string& description) {}

  operator T() const {
    return value_;
  }
  const CVal<T>& operator =(const T& rhs) {
    value_ = rhs;
    return *this;
  }
  const CVal<T>& operator +=(const T& rhs) {
    value_ += rhs;
    return *this;
  }
  const CVal<T>& operator -=(const T& rhs) {
    value_ -= rhs;
    return *this;
  }

 private:
  T value_;
};

#endif  // #if defined(__LB_SHELL__ENABLE_CONSOLE__)

}  // namespace LB

#endif  // SRC_LB_CONSOLE_VALUES_H_
