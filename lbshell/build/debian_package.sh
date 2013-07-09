#!/bin/sh
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

# Wrapper around the steps needed to build a debian package
# and optionally upload it to the given directory.

# Exit if any command fails
set -e

if [ $# -gt 1 ]; then
  echo "Usage: debian_package.sh <destination_directory>"
  exit 1
fi

if [ $# == 1 ]; then
  package_destination=$1
else
  package_destination=""
fi

./update_changelog.sh
./debian/rules clean
fakeroot ./debian/rules binary

if [ -n "$package_destination" ]; then
  mkdir -p $package_destination
  cp ../steel_*amd64.deb $package_destination
fi
