#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Formats as a .json file that can be used to localize Google Chrome
extensions."""

import re
import types

from grit import util
from grit.node import message

def Format(root, lang='en', output_dir='.'):
  """Format the messages as JSON."""
  yield '{\n'

  format = ('  "%s": {\n'
            '    "message": "%s"\n'
            '  }')
  first = True
  for child in root.ActiveDescendants():
    if isinstance(child, message.MessageNode):
      loc_message = child.Translate(lang)
      loc_message = re.sub(r'\\', r'\\\\', loc_message)
      loc_message = re.sub(r'"', r'\"', loc_message)

      id = child.attrs['name']
      if id.startswith('IDR_'):
        id = id[4:]

      if not first:
        yield ',\n'
      first = False
      yield format % (id, loc_message)

  yield '\n}\n'
