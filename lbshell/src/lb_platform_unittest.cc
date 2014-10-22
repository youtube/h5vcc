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

#include <stdio.h>
#include <stdlib.h>

#include "gtest/gtest.h"

#include "lb_platform.h"

static unsigned char data_stream[8] = {
  'A', 'B', 'C', 'D',
  'E', 'F', 'G', 'H'
};

static unsigned char output_stream[8] = {0};

TEST(EndianSwapTest, Load16Big) {
  uint16_t expected_val = 'A' << 8 | 'B';
  uint16_t loaded_val = LB::Platform::load_uint16_big_endian(data_stream);
  EXPECT_EQ(expected_val, loaded_val);
}

TEST(EndianSwapTest, Load16Little) {
  uint16_t expected_val = 'B' << 8 | 'A';
  uint16_t loaded_val = LB::Platform::load_uint16_little_endian(data_stream);
  EXPECT_EQ(expected_val, loaded_val);
}

TEST(EndianSwapTest, Load32Big) {
  uint32_t expected_val = 'A' << 24 | 'B' << 16 | 'C' << 8 | 'D';
  uint32_t loaded_val = LB::Platform::load_uint32_big_endian(data_stream);
  EXPECT_EQ(expected_val, loaded_val);
}

TEST(EndianSwapTest, Load32Little) {
  uint32_t expected_val = 'D' << 24 | 'C' << 16 | 'B' << 8 |  'A';
  uint32_t loaded_val = LB::Platform::load_uint32_little_endian(data_stream);
  EXPECT_EQ(expected_val, loaded_val);
}

TEST(EndianSwapTest, Load64Big) {
  uint64_t high_word = 'A' << 24 | 'B' << 16 | 'C' << 8 | 'D';
  uint64_t low_word = 'E' << 24 | 'F' << 16 | 'G' << 8 | 'H';
  uint64_t expected_val =  high_word << 32 | low_word;
  uint64_t loaded_val = LB::Platform::load_uint64_big_endian(data_stream);
  EXPECT_EQ(expected_val, loaded_val);
}

TEST(EndianSwapTest, Load64Little) {
  uint64_t high_word = 'H' << 24 | 'G' << 16 | 'F' << 8 | 'E';
  uint64_t low_word = 'D' << 24 | 'C' << 16 | 'B' << 8 | 'A';
  uint64_t expected_val =  high_word << 32 | low_word;
  uint64_t loaded_val = LB::Platform::load_uint64_little_endian(data_stream);
  EXPECT_EQ(expected_val, loaded_val);
}

TEST(EndianSwapTest, Store16Big) {
  uint16_t val = 'A' << 8 | 'B';
  LB::Platform::store_uint16_big_endian(val, output_stream);
  EXPECT_EQ(output_stream[0], 'A');
  EXPECT_EQ(output_stream[1], 'B');
}

TEST(EndianSwapTest, Store16Little) {
  uint16_t val = 'A' << 8 | 'B';
  LB::Platform::store_uint16_little_endian(val, output_stream);
  EXPECT_EQ(output_stream[0], 'B');
  EXPECT_EQ(output_stream[1], 'A');
}

TEST(EndianSwapTest, Store32Big) {
  uint32_t val = 'A' << 24 | 'B' << 16 | 'C' << 8 | 'D';
  LB::Platform::store_uint32_big_endian(val, output_stream);
  EXPECT_EQ(output_stream[0], 'A');
  EXPECT_EQ(output_stream[1], 'B');
  EXPECT_EQ(output_stream[2], 'C');
  EXPECT_EQ(output_stream[3], 'D');
}

TEST(EndianSwapTest, Store32Little) {
  uint32_t val = 'A' << 24 | 'B' << 16 | 'C' << 8 | 'D';
  LB::Platform::store_uint32_little_endian(val, output_stream);
  EXPECT_EQ(output_stream[0], 'D');
  EXPECT_EQ(output_stream[1], 'C');
  EXPECT_EQ(output_stream[2], 'B');
  EXPECT_EQ(output_stream[3], 'A');
}

TEST(EndianSwapTest, Store64Big) {
  uint64_t high_word = 'A' << 24 | 'B' << 16 | 'C' << 8 | 'D';
  uint64_t low_word = 'E' << 24 | 'F' << 16 | 'G' << 8 | 'H';
  uint64_t val = high_word << 32 | low_word;
  LB::Platform::store_uint64_big_endian(val, output_stream);
  EXPECT_EQ(output_stream[0], 'A');
  EXPECT_EQ(output_stream[1], 'B');
  EXPECT_EQ(output_stream[2], 'C');
  EXPECT_EQ(output_stream[3], 'D');
  EXPECT_EQ(output_stream[4], 'E');
  EXPECT_EQ(output_stream[5], 'F');
  EXPECT_EQ(output_stream[6], 'G');
  EXPECT_EQ(output_stream[7], 'H');
}

TEST(EndianSwapTest, Store64Little) {
  uint64_t high_word = 'A' << 24 | 'B' << 16 | 'C' << 8 | 'D';
  uint64_t low_word = 'E' << 24 | 'F' << 16 | 'G' << 8 | 'H';
  uint64_t val = high_word << 32 | low_word;
  LB::Platform::store_uint64_little_endian(val, output_stream);
  EXPECT_EQ(output_stream[0], 'H');
  EXPECT_EQ(output_stream[1], 'G');
  EXPECT_EQ(output_stream[2], 'F');
  EXPECT_EQ(output_stream[3], 'E');
  EXPECT_EQ(output_stream[4], 'D');
  EXPECT_EQ(output_stream[5], 'C');
  EXPECT_EQ(output_stream[6], 'B');
  EXPECT_EQ(output_stream[7], 'A');
}
