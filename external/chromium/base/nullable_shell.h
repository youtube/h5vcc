// Copyright (c) 2014 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NULLABLE_SHELL_H_
#define BASE_NULLABLE_SHELL_H_

#include "base/base_export.h"
#include "base/logging.h"

namespace base {

// This class is a wrapper for any type, adding a null state to it. Naming
// convention suggestion:
//   typedef Nullable<MyType> MyTypeN;
template <typename T>
class BASE_EXPORT Nullable {
 public:
  // Default constructor is null. The value is the default value for the wrapped
  // type.
  Nullable() : is_null_(true) { }

  // As a semi-transparant wrapper, we provide this non-explicit singleton
  // constructor so users can pass in a T wherever a Nullable<T> is
  // expected. Otherwise, usage of Nullable would be syntactically heavy.
  Nullable(const T &value) : value_(value), is_null_(false) { }

  // A descriptive way to create a null Nullable<T>.
  static Nullable<T> Null() { return Nullable<T>(); }

  Nullable<T> &operator=(T rhs) {
    value_ = rhs;
    is_null_ = false;
    return *this;
  }

  // Sets this Nullable to null, if not already null.
  void Clear() {
    value_ = T();
    is_null_ = true;
  }

  bool is_null() const { return is_null_; }

  const T &value() const {
    DCHECK(!is_null_);
    return value_;
  }

 private:
  T value_;
  bool is_null_;
};

template <typename T>
inline bool operator==(const Nullable<T> &lhs, const Nullable<T> &rhs) {
  if (lhs.is_null()) {
    return rhs.is_null();
  }

  return rhs == lhs.value();
}

template <typename T>
inline bool operator!=(const Nullable<T> &lhs, const Nullable<T> &rhs) {
  return !(lhs == rhs);
}

template <typename T>
inline bool operator==(const Nullable<T> &lhs, const T &rhs) {
  return (lhs.is_null() ? false : lhs.value() == rhs);
}

template <typename T>
inline bool operator!=(const Nullable<T> &lhs, const T &rhs) {
  return !(lhs == rhs);
}

template <typename T>
inline bool operator==(const T &lhs, const Nullable<T> &rhs) {
  return rhs == lhs;
}

template <typename T>
inline bool operator!=(const T &lhs, const Nullable<T> &rhs) {
  return rhs != lhs;
}

}  // namespace base

#endif  // BASE_NULLABLE_SHELL_H_
