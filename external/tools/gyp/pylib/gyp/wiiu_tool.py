#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions for WIIU builds.

These functions are executed via gyp-wiiu-tool when using the ninja generator.
"""

import os
import re
import shutil
import subprocess
import sys


def main(args):
  executor = WiiuTool()
  exit_code = executor.Dispatch(args)
  if exit_code is not None:
    sys.exit(exit_code)


class WiiuTool(object):
  """This class performs all the WiiU tooling steps. The methods can either
  be executed directly, or dispatched from an argument list."""

  def Dispatch(self, args):
    """Dispatches a string command to a method."""
    if len(args) < 1:
      raise Exception("Not enough arguments")

    method = "Exec%s" % self._CommandifyName(args[0])
    return getattr(self, method)(*args[1:])

  def _CommandifyName(self, name_string):
    """Transforms a tool name like recursive-mirror to RecursiveMirror."""
    return name_string.title().replace('-', '')

  def ExecDepNormalize(self, obj_file, dep_file):
    """GHS compiler doesn't expose a flag to specify the name of the .d file it
    emits. it is constructed by replacing the extension of the output .o file
    with .d"""
    base,ext = os.path.splitext(obj_file)
    src = base + ".d"

    src_file = open(src, 'r')
    src_contents = src_file.read()
    src_file.close()

    # Start normalizing slash characters after the colon. Ninja expects the
    # target's path in the depfile to be identical to the target path passed
    # to ninja so we cannot modify it
    colon = src_contents.find(':')
    dest_contents = src_contents[:colon]

    # match exactly one slash, not followed by new line, or quote character
    slash_re = re.compile('\\\\{1}([^\n\"\'])')
    dest_contents += slash_re.sub('/\g<1>', src_contents[colon:])

    dest_file = open(dep_file, 'w')
    dest_file.write(dest_contents)
    dest_file.close()

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
