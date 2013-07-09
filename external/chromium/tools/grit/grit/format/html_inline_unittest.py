#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for grit.format.html_inline'''


import os
import re
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import unittest

from grit import util
from grit.format import html_inline


class HtmlInlineUnittest(unittest.TestCase):
  '''Unit tests for HtmlInline.'''

  def testGetResourceFilenames(self):
    '''Tests that all included files are returned by GetResourceFilenames.'''

    files = {
      'index.html': '''
      <!DOCTYPE HTML>
      <html>
        <head>
          <link rel="stylesheet" href="test.css">
        </head>
        <body>
          <include src="test.html">
        </body>
      </html>
      ''',

      'test.html': '''
      <include src="test2.html">
      ''',

      'test2.html': '''
      <!-- This second level resource should also be included. -->
      ''',

      'test.css': '''
      .image {
        background: url('test.png');
      }
      ''',

      'test.png': 'PNG DATA',
    }

    source_resources = set()
    tmp_dir = util.TempDir(files)
    for filename in files:
      source_resources.add(tmp_dir.GetPath(filename))

    resources = html_inline.GetResourceFilenames(tmp_dir.GetPath('index.html'))
    resources.add(tmp_dir.GetPath('index.html'))
    self.failUnlessEqual(resources, source_resources)
    tmp_dir.CleanUp()

if __name__ == '__main__':
  unittest.main()
