#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittest for android_xml.py."""

import os
import StringIO
import sys
import unittest

if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

from grit import util
from grit.tool import build


class AndroidXmlUnittest(unittest.TestCase):

  def testMessages(self):
    root = util.ParseGrdForUnittest(ur"""
        <messages>
          <message name="IDS_SIMPLE" desc="A vanilla string">
            Martha
          </message>
          <message name="IDS_ONE_LINE" desc="On one line">sat and wondered</message>
          <message name="IDS_QUOTES" desc="A string with quotation marks">
            out loud, "Why don't I build a flying car?"
          </message>
          <message name="IDS_MULTILINE" desc="A string split over several lines">
            She gathered
wood, charcoal, and
a sledge hammer.
          </message>
          <message name="IDS_WHITESPACE" desc="A string with extra whitespace.">
            '''   How old fashioned  --  she thought. '''
          </message>
          <message name="IDS_PRODUCT_SPECIFIC_product_nosdcard"
              desc="A string that only applies if there's no sdcard">
            Lasers will probably be helpful.
          </message>
          <message name="IDS_PLACEHOLDERS" desc="A string with placeholders">
            I'll buy a <ph name="WAVELENGTH">%d<ex>200</ex></ph> nm laser at <ph name="STORE_NAME">%s<ex>the grocery store</ex></ph>.
          </message>
        </messages>
        """)

    buf = StringIO.StringIO()
    build.RcBuilder.ProcessNode(root, DummyOutput('android', 'en'), buf)
    output = buf.getvalue()
    expected = ur"""
<?xml version="1.0" encoding="utf-8"?>
<resources xmlns:android="http://schemas.android.com/apk/res/android">
<string name="simple">"Martha"</string>
<string name="one_line">"sat and wondered"</string>
<string name="quotes">"out loud, \"Why don\'t I build a flying car?\""</string>
<string name="multiline">"She gathered
wood, charcoal, and
a sledge hammer."</string>
<string name="whitespace">"   How old fashioned  --  she thought. "</string>
<string name="product_specific" product="nosdcard">"Lasers will probably be helpful."</string>
<string name="placeholders">"I\'ll buy a %d nm laser at %s."</string>
</resources>
"""
    self.assertEqual(output.strip(), expected.strip())


class DummyOutput(object):

  def __init__(self, type, language):
    self.type = type
    self.language = language

  def GetType(self):
    return self.type

  def GetLanguage(self):
    return self.language

  def GetOutputFilename(self):
    return 'hello.gif'

if __name__ == '__main__':
  unittest.main()
