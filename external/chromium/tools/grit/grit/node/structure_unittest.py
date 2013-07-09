#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for <structure> nodes.
'''

import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import platform
import unittest
import StringIO

from grit import util
from grit.node import structure
from grit.format import rc


class StructureUnittest(unittest.TestCase):
  def testSkeleton(self):
    grd = util.ParseGrdForUnittest('''
        <structures>
          <structure type="dialog" name="IDD_ABOUTBOX" file="klonk.rc" encoding="utf-16-le">
            <skeleton expr="lang == 'fr'" variant_of_revision="1" file="klonk-alternate-skeleton.rc" />
          </structure>
        </structures>''', base_dir=util.PathFromRoot('grit/testdata'))
    grd.SetOutputLanguage('fr')
    grd.RunGatherers()
    transl = ''.join(rc.Format(grd, 'fr', '.'))
    self.failUnless(transl.count('040704') and transl.count('110978'))
    self.failUnless(transl.count('2005",IDC_STATIC'))

  def testRunCommandOnCurrentPlatform(self):
    node = structure.StructureNode()
    node.attrs = node.DefaultAttributes()
    self.failUnless(node.RunCommandOnCurrentPlatform())
    node.attrs['run_command_on_platforms'] = 'Nosuch'
    self.failIf(node.RunCommandOnCurrentPlatform())
    node.attrs['run_command_on_platforms'] = (
        'Nosuch,%s,Othernot' % platform.system())
    self.failUnless(node.RunCommandOnCurrentPlatform())


if __name__ == '__main__':
  unittest.main()
