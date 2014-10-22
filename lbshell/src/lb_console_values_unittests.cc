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

#include "lb_console_values.h"

#include <string>

#include "external/chromium/testing/gmock/include/gmock/gmock.h"
#include "external/chromium/testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

TEST(ConsoleValueTest, CValGetSet) {
  LB::CVal<int> cv_int("cv_int", 10, "");
  EXPECT_EQ(cv_int, 10);
  cv_int = 5;
  EXPECT_EQ(cv_int, 5);

  LB::CVal<float> cv_float("cv_float", 10.0f, "");
  EXPECT_EQ(cv_float, 10.0f);
  cv_float = 0.05f;
  EXPECT_EQ(cv_float, 0.05f);

  LB::CVal<std::string> cv_string("cv_string", "foo", "");
  EXPECT_EQ(static_cast<std::string>(cv_string), std::string("foo"));
  cv_string = "bar";
  EXPECT_EQ(static_cast<std::string>(cv_string), std::string("bar"));
}

TEST(ConsoleValueTest, HookReceiver) {
  LB::ConsoleValueManager manager;

  LB::CVal<int32_t> cval_a("cval_a", 10, "");

  {
    class TestHook : public LB::ConsoleValueManager::OnChangedHook {
     public:
      TestHook() : value_changed_count_(0) {}

      virtual void OnValueChanged(
          const std::string& name,
          const LB::ConsoleGenericValue& value) {
        EXPECT_EQ(name, "cval_a");
        EXPECT_EQ(value.AsString(), "15");

        ++value_changed_count_;
      }

      int value_changed_count_;
    };
    TestHook test_hook;

    cval_a = 15;

    EXPECT_EQ(test_hook.value_changed_count_, 1);
  }

  // Make sure there's no interference from the previous hook which should
  // be completely non-existant now
  {
    class TestHook : public LB::ConsoleValueManager::OnChangedHook {
     public:
      TestHook() : value_changed_count_(0) {}

      virtual void OnValueChanged(
          const std::string& name,
          const LB::ConsoleGenericValue& value) {
        EXPECT_EQ(name, "cval_a");
        EXPECT_EQ(value.AsString(), "20");

        ++value_changed_count_;
      }

      int value_changed_count_;
    };
    TestHook test_hook;

    cval_a = 20;

    EXPECT_EQ(test_hook.value_changed_count_, 1);
  }

  // Try modifying a new variable with a different name.  Also checks that
  // past hooks will not be checked.
  {
    class TestHook : public LB::ConsoleValueManager::OnChangedHook {
     public:
      TestHook() : value_changed_count_(0) {}

      virtual void OnValueChanged(
          const std::string& name,
          const LB::ConsoleGenericValue& value) {
        EXPECT_EQ(name, "cval_b");
        EXPECT_EQ(value.AsString(), "bar");

        ++value_changed_count_;
      }

      int value_changed_count_;
    };
    TestHook test_hook;

    LB::CVal<std::string> cval_b("cval_b", "foo", "");
    cval_b = "bar";

    EXPECT_EQ(test_hook.value_changed_count_, 1);
  }
}

TEST(ConsoleValueTest, ConvertFromInt) {
  LB::ConsoleValueManager manager;
  class TestIntHook : public LB::ConsoleValueManager::OnChangedHook {
   public:
    TestIntHook() : value_changed_count_(0) {}

    virtual void OnValueChanged(
        const std::string& name,
        const LB::ConsoleGenericValue& value) {
      ASSERT_TRUE(value.CanConvert<int32_t>());
      EXPECT_EQ(value.Convert<int32_t>(), 15);
      ASSERT_TRUE(value.CanConvert<uint32_t>());
      EXPECT_EQ(value.Convert<uint32_t>(), 15);
      ASSERT_TRUE(value.CanConvert<int64_t>());
      EXPECT_EQ(value.Convert<int64_t>(), 15);
      ASSERT_TRUE(value.CanConvert<uint64_t>());
      EXPECT_EQ(value.Convert<uint64_t>(), 15);
      ASSERT_TRUE(value.CanConvert<size_t>());
      EXPECT_EQ(value.Convert<size_t>(), 15);
      ASSERT_TRUE(value.CanConvert<float>());
      EXPECT_EQ(value.Convert<float>(), 15.0f);
      ASSERT_TRUE(value.CanConvert<double>());
      EXPECT_EQ(value.Convert<double>(), 15.0);

      EXPECT_FALSE(value.CanConvert<std::vector<int> >());
      EXPECT_FALSE(value.CanConvert<std::string>());

      ++value_changed_count_;
    }

    int value_changed_count_;
  };
  TestIntHook test_hook;

  std::vector<int> foo = static_cast<std::vector<int> >(5);

  LB::CVal<int32_t> cv_int32_t("cv_int32_t", 10, "");
  cv_int32_t = 15;

  LB::CVal<int64_t> cv_int64_t("cv_int64_t", 10, "");
  cv_int64_t = 15;

  LB::CVal<uint32_t> cv_uint32_t("cv_uint32_t", 10, "");
  cv_uint32_t = 15;

  LB::CVal<uint64_t> cv_uint64_t("cv_uint64_t", 10, "");
  cv_uint64_t = 15;

  LB::CVal<int> cv_int("cv_int", 10, "");
  cv_int = 15;

  LB::CVal<unsigned int> cv_unsigned_int("cv_unsigned_int", 10, "");
  cv_unsigned_int = 15;

  LB::CVal<size_t> cv_size_t("cv_size_t", 10, "");
  cv_size_t = 15;

  EXPECT_EQ(test_hook.value_changed_count_, 7);
}

TEST(ConsoleValueTest, ConvertFromFloat) {
  LB::ConsoleValueManager manager;
  class TestFloatHook : public LB::ConsoleValueManager::OnChangedHook {
   public:
    TestFloatHook() : value_changed_count_(0) {}

    virtual void OnValueChanged(
        const std::string& name,
        const LB::ConsoleGenericValue& value) {
      ASSERT_TRUE(value.CanConvert<float>());
      EXPECT_EQ(value.Convert<float>(), 0.25f);
      ASSERT_TRUE(value.CanConvert<double>());
      EXPECT_EQ(value.Convert<double>(), 0.25);

      EXPECT_FALSE(value.CanConvert<std::vector<int> >());
      EXPECT_FALSE(value.CanConvert<std::string>());

      ++value_changed_count_;
    }

    int value_changed_count_;
  };
  TestFloatHook test_hook;

  LB::CVal<float> cv_float("cv_float", 10.0f, "");
  cv_float = 0.25f;

  LB::CVal<double> cv_double("cv_double", 10.0, "");
  cv_double = 0.25;

  EXPECT_EQ(test_hook.value_changed_count_, 2);
}

TEST(ConsoleValueTest, ConvertFromString) {
  LB::ConsoleValueManager manager;
  class TestStringHook : public LB::ConsoleValueManager::OnChangedHook {
   public:
    TestStringHook() : value_changed_count_(0) {}

    virtual void OnValueChanged(
        const std::string& name,
        const LB::ConsoleGenericValue& value) {
      ASSERT_TRUE(value.CanConvert<std::string>());
      EXPECT_EQ(value.Convert<std::string>(), "bar");

      EXPECT_FALSE(value.CanConvert<std::vector<int> >());
      EXPECT_FALSE(value.CanConvert<int>());
      EXPECT_FALSE(value.CanConvert<unsigned int>());
      EXPECT_FALSE(value.CanConvert<float>());

      ++value_changed_count_;
    }

    int value_changed_count_;
  };
  TestStringHook test_hook;

  LB::CVal<std::string> cv_string("cv_string", "foo", "");
  cv_string = "bar";

  EXPECT_EQ(test_hook.value_changed_count_, 1);
}

TEST(ConsoleValueTest, NativeType) {
  LB::ConsoleValueManager manager;
  class TestInt32Hook : public LB::ConsoleValueManager::OnChangedHook {
   public:
    TestInt32Hook() : value_changed_count_(0) {}

    virtual void OnValueChanged(
        const std::string& name,
        const LB::ConsoleGenericValue& value) {
      EXPECT_TRUE(value.IsNativeType<int32_t>());

      EXPECT_FALSE(value.IsNativeType<uint32_t>());
      EXPECT_FALSE(value.IsNativeType<int64_t>());
      EXPECT_FALSE(value.IsNativeType<uint64_t>());
      EXPECT_FALSE(value.IsNativeType<float>());
      EXPECT_FALSE(value.IsNativeType<double>());
      EXPECT_FALSE(value.IsNativeType<std::string>());

      ASSERT_TRUE(value.IsNativeType<int32_t>());
      EXPECT_EQ(value.AsNativeType<int32_t>(), 15);

      ++value_changed_count_;
    }

    int value_changed_count_;
  };
  TestInt32Hook test_hook;

  LB::CVal<int32_t> cv_int32_t("cv_int32_t", 10, "");
  cv_int32_t = 15;

  EXPECT_EQ(test_hook.value_changed_count_, 1);
}
