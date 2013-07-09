#!/bin/bash
# Copyright 2012 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

build=$(dirname $0)
if test "$build" = "." && ! echo "$0" | grep -q '^./'; then
  echo "This script cannot be run from \$PATH."
  exit 1
fi
lbshell=$(realpath "$build/../..")
cd "$lbshell"

echo "cleaning build directories..."
rm -rf out # current output for both msvs and make
echo "cleaning gyp-generated build files..."
find \( -name '*.gyp_lb*' -or -name '*.gyp_jsc*' -or -name '*.pyc' \) -delete
rm -f lbshell/build/depends_dict.*

