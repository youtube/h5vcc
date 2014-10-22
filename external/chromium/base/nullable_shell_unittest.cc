#include "base/nullable_shell.h"

#include "testing/gtest/include/gtest/gtest.h"

typedef base::Nullable<int> intN;

TEST(NullableTest, Null) {
  intN test;
  EXPECT_TRUE(test.is_null());
}

TEST(NullableTest, Value) {
  intN test(2);
  EXPECT_FALSE(test.is_null());
  EXPECT_EQ(2, test.value());
}

TEST(NullableTest, Clear) {
  intN test(2);
  test.Clear();
  EXPECT_TRUE(test.is_null());
}

TEST(NullableTest, InitializeValue) {
  intN test = 2;
  EXPECT_FALSE(test.is_null());
  EXPECT_EQ(2, test.value());
}

TEST(NullableTest, InitializeValue2) {
  intN test(2);
  EXPECT_FALSE(test.is_null());
  EXPECT_EQ(2, test.value());
}

TEST(NullableTest, InitializeNull) {
  intN test = intN::Null();
  EXPECT_TRUE(test.is_null());
}

TEST(NullableTest, InitializeNull2) {
  intN test(intN::Null());
  EXPECT_TRUE(test.is_null());
}

TEST(NullableTest, Assignment) {
  intN a;
  intN b = 2;
  intN c;
  a = b;
  EXPECT_FALSE(a.is_null());
  EXPECT_EQ(2, a.value());
  a = c;
  EXPECT_TRUE(a.is_null());
  b = intN::Null();
  EXPECT_TRUE(b.is_null());
}

TEST(NullableTest, Comparions) {
  intN test1 = 1;
  intN test2 = 2;
  intN test3;
  intN test4 = 2;

  EXPECT_FALSE(test1 == test2);
  EXPECT_FALSE(test2 == test1);
  EXPECT_FALSE(test1 == test3);
  EXPECT_FALSE(test3 == test1);
  EXPECT_FALSE(2 == test1);
  EXPECT_FALSE(test1 == 2);
  EXPECT_EQ(1, test1);
  EXPECT_EQ(test1, 1);
  EXPECT_EQ(test4, test2);
  EXPECT_EQ(test2, test4);

  EXPECT_NE(test1, test2);
  EXPECT_NE(test2, test1);
  EXPECT_NE(test1, test3);
  EXPECT_NE(test3, test1);
  EXPECT_NE(2, test1);
  EXPECT_NE(test1, 2);
  EXPECT_FALSE(1 != test1);
  EXPECT_FALSE(test1 != 1);
  EXPECT_FALSE(test4 != test2);
  EXPECT_FALSE(test2 != test4);
}
