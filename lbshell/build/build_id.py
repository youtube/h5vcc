#!/usr/bin/env python
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

import datetime
import os
import subprocess
import sys
import time

template = """
#ifndef _STEEL_BUILD_ID_H_
#define _STEEL_BUILD_ID_H_

#define STEEL_BUILD_ID "%s"

#endif
"""

def main(working_dir, output_path, git_hash):
  os.chdir(working_dir)
  ts = datetime.datetime.fromtimestamp(time.time()).strftime('%Y%m%d')
  build_id = git_hash + "-" + ts
  open(output_path, 'w').write(template % build_id)

if __name__ == '__main__':
  main(sys.argv[1], sys.argv[2], sys.argv[3])

