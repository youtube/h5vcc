#!/usr/bin/python
# Copyright 2014 Google Inc. All Rights Reserved.
"""Package application for the given platform and build configs.

Depending on platform, this will create a package suitable for distribution.
If the buildbot is running the script, it will be uploaded to the
buildbot staging area.

Usage varies depending on the platform and the user.
If the buildbot is running the script, no parameters should be required
other than the platform and the path to the staging area.
However, users who want to build their own packages can specify options on the
command line.
"""
import argparse
import importlib
import logging
import os
import platform
import sys
import textwrap

import package_utils


def _ParseCommandLine(args):
  """Parse command line and return options."""
  parser = argparse.ArgumentParser(
      formatter_class=argparse.RawDescriptionHelpFormatter,
      description=textwrap.dedent(__doc__))

  if platform.system() == 'Linux':
    valid_platforms = ('Android', 'Linux')
  else:
    valid_platforms = ('PS3', 'PS4', 'WiiU', 'XB1')

  packagers = {}
  # Import each packager module for the given platform, and
  # store the platform-specific Packager class in a dict.
  for plat in valid_platforms:
    packagers[plat] = importlib.import_module(
        '%s.packager' % plat.lower()).Packager

  valid_configs = (
      'Debug',
      'Devel',
      'QA',
      'Gold',
  )

  subparsers = parser.add_subparsers(dest='platform', help='Platform name')

  # We allow each platform to add its own command line arguments,
  # as well as the common ones. Add the common args to each sub-parser
  # to avoid confusing ordering requirements.
  # So the user invokes this like $ package_application.py PLATFORM <args>
  for plat, packager in packagers.iteritems():
    sub_parser = subparsers.add_parser(plat)
    packager.AddCommandLineArguments(sub_parser)
    sub_parser.add_argument(
        '-c', '--config',
        dest='config_list',
        required=not package_utils.IsBuildbot(),
        choices=valid_configs, action='append',
        help='Build config. May be specified multiple times.'
        'For automated builds, the set of configs will be specified in'
        'the packager script.')
    # The buildbot tells us the path to the staging directory, since it's
    # based on the branch, time of day and buildnumber.
    sub_parser.add_argument('-s', '--staging',
                            required=package_utils.IsBuildbot(),
                            help='Path to staging area on buildmaster. '
                            '(For use by buildbot.)')
    sub_parser.add_argument('-u', '--user',
                            help='Override user for testing staging.')
    sub_parser.add_argument('-v', '--verbose',
                            required=False, action='store_true')
    sub_parser.set_defaults(packager=packager)

  return parser.parse_args(args)


def main(args):
  options = _ParseCommandLine(args)
  if options.verbose:
    logging_level = logging.DEBUG
  else:
    logging_level = logging.INFO
  logging_format = '%(asctime)s %(levelname)-8s %(message)s'

  logging.basicConfig(level=logging_level,
                      format=logging_format,
                      datefmt='%m-%d %H:%M')

  packager = options.packager(options)
  try:
    deployment_files = packager.PackageApplication()
  except RuntimeError as e:
    logging.error(e)
    return 1

  logging.debug('Paths for deployment: %s', deployment_files)

  rc = 0
  if package_utils.IsBuildbot() or options.user:
    build_info = os.path.join(packager.GetOutDir(), 'build_info.txt')
    if os.path.exists(build_info):
      deployment_files.append(build_info)
    else:
      logging.error('%s not found.', build_info)

    rc |= package_utils.DeployToStaging(
        deployment_files, options.staging, options.user)
  return rc

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
