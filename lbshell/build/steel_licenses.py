#!/usr/bin/python
# Copyright 2014 Google Inc. All Rights Reserved.

"""License checking utility.

Utility for checking and processing licensing information in third_party
directories.

Usage: steel_licenses.py <command> <build_dir> [<output_file>]

build_dir is the directory where build products are generated.

Commands:
  scan     scan third_party directories, verifying that we have licensing info
  license  generate license HTML information to output file, or stdout.

"""

import fnmatch
import os
import re
import subprocess
import sys


def NativePath(path):
  if sys.platform == 'cygwin':
    import cygpath  # pylint: disable=g-import-not-at-top
    return cygpath.to_nt(path)
  else:
    return path


script_dir = os.path.abspath(os.path.dirname(__file__))
toplevel_dir = os.path.dirname(os.path.dirname(script_dir))
tools_dir = os.path.join(toplevel_dir, 'external', 'chromium', 'tools')
sys.path.insert(0, tools_dir)

import licenses  # pylint: disable=g-import-not-at-top

build_dir = ''

# LayoutTests contains thousands of non-code directories, skip it.
licenses.PRUNE_DIRS += ('.repo', 'LayoutTests')

STEEL_ADDITIONAL_PATHS = (
    os.path.join(toplevel_dir, 'external', 'chromium'),
    os.path.join(toplevel_dir, 'external', 'openssl'),
)

# Paths from the root of the tree to directories to skip.
STEEL_PRUNE_PATHS = set([
    # Same module occurs in both the top-level third_party and others.
    os.path.join('third_party', 'jemalloc'),

    # Assume we will never use anything from chrome or v8.
    'chrome',
    'v8',
])

licenses.ADDITIONAL_PATHS += STEEL_ADDITIONAL_PATHS

# Exclude this hardcoded path from licenses.py.
licenses.ADDITIONAL_PATHS = [
    p for p in licenses.ADDITIONAL_PATHS
    if p != os.path.join('v8', 'strongtalk')
]

licenses.PRUNE_PATHS |= STEEL_PRUNE_PATHS

# Directories where we check out directly from upstream, and therefore
# can't provide a README.chromium.  Please prefer a README.chromium
# wherever possible.
# Default License File is "LICENSE"

STEEL_SPECIAL_CASES = {
    os.path.join(toplevel_dir, 'external', 'chromium'): {
        'Name': 'Chromium',
        'URL': 'http://chromium.org',
        'License': 'BSD',
    },
    os.path.join('third_party', 'freetype'): {
        'Name': 'FreeType',
        'URL': 'http://freetype.org',
        'License': 'The FreeType Project LICENSE',
        'License File': 'docs/FTL.TXT',
    },
    os.path.join('third_party', 'skia'): {
        'Name': 'Skia',
        'URL': 'http://chromium.org',
        'License': 'BSD',
    },
}

# we don't use v8
del licenses.SPECIAL_CASES[os.path.join('v8')]

licenses.SPECIAL_CASES = dict(
    licenses.SPECIAL_CASES.items() + STEEL_SPECIAL_CASES.items())


def FindThirdPartyDependencies():
  """Scan Makefile deps to find all third_party uses."""

  third_party_deps = set()

  # Match third_party/<foo>/...
  third_party_re = re.compile(r'third_party[\\/]([^ \t\n\r\f\v/\\]*)')

  if not os.path.exists(build_dir):
    print >> sys.stderr, 'Build directory', build_dir, 'does not exist'

  for path, _, files in os.walk(build_dir):
    for dep in fnmatch.filter(files, '*.d'):
      f = open(os.path.join(path, dep), 'r')
      dep_text = f.read()
      matches = third_party_re.findall(dep_text)
      for m in matches:
        third_party_deps.add(m)

  # Query the ninja deps database (this is mutually exclusive with .d files).
  # Use -n to avoid trying to open the deps log for write. ninja is probably
  # already running.
  ninja_deps = subprocess.check_output(
      ['ninja', '-n', '-C', NativePath(build_dir), '-t', 'deps'])
  matches = third_party_re.findall(ninja_deps)
  for m in matches:
    third_party_deps.add(m)

  return third_party_deps


def CreateSteelPruneList(third_party_dirs):
  """Generate a list of third_party directories we don't use."""

  third_party_deps = FindThirdPartyDependencies()
  if not third_party_deps:
    raise licenses.LicenseError(
        'No dependencies found. Check the build directory.')

  prune_paths = set()
  for f in third_party_dirs:
    if f.find('third_party') != -1:
      for d in third_party_deps:
        pattern = re.compile(d)
        found = 0
        if pattern.search(f):
          found = 1
          break
      if not found:
        prune_paths.add(f)

  return prune_paths


def GenerateLicense(output_filename=None):
  """Generate list of licenses in html form.

  Dumps the result to output file.

  Args:
    output_filename: Filename to write the license.html to.
                     Writes to stdout if None.
  Returns:
    0 on success.
  """

  chromium_root = os.path.join(toplevel_dir, 'external', 'chromium')

  third_party_dirs = set(licenses.FindThirdPartyDirs(licenses.PRUNE_PATHS,
                                                     chromium_root))
  prune_paths = CreateSteelPruneList(third_party_dirs)

  third_party_dirs -= prune_paths
  entries = []
  for path in sorted(third_party_dirs):
    try:
      metadata = licenses.ParseDir(path, chromium_root)
    except licenses.LicenseError:
      print >> sys.stderr, ('WARNING: licensing info for ' + path +
                            ' is incomplete, skipping.')
      continue

    if metadata['License File'] != 'NOT_SHIPPED':
      env = {
          'name': metadata['Name'],
          'url': metadata['URL'],
          'license': open(metadata['License File'], 'rb').read(),
      }
      entries.append(env)

  if output_filename:
    output_file = open(output_filename, 'wb')
  else:
    output_file = sys.stdout

  for e in entries:
    output_file.write('<h4>\n')
    output_file.write(e['name'] + '\n')
    output_file.write('</h4>\n')
    output_file.write('<pre>\n')
    output_file.write(e['license'] + '\n')
    output_file.write('</pre>\n')

  return 0


def ScanDirectories():
  """Verify all directories (that we use) have valid license information."""

  # The functions we call in licenses.py assume we are at the root.
  os.chdir(toplevel_dir)

  # Add all the directories we don't use to PRUNE_PATHS.
  third_party_dirs = set(
      licenses.FindThirdPartyDirs(licenses.PRUNE_PATHS, os.getcwd()))
  prune_paths = CreateSteelPruneList(third_party_dirs)
  licenses.PRUNE_PATHS = set(licenses.PRUNE_PATHS) | prune_paths

  # Now verify the presence of a license file in all our third party dirs.
  if licenses.ScanThirdPartyDirs():
    print 'scan successful.'
    return 1


def main():
  command = 'help'

  if len(sys.argv) > 2:
    command = sys.argv[1]
    global build_dir
    build_dir = os.path.abspath(sys.argv[2])

  if command == 'scan':
    if not ScanDirectories():
      return 1

  elif command == 'license':
    if len(sys.argv) > 3:
      output_filename = sys.argv[3]
    else:
      output_filename = None

    return GenerateLicense(output_filename)
  else:
    print __doc__
    return 1


if __name__ == '__main__':
  sys.exit(main())
