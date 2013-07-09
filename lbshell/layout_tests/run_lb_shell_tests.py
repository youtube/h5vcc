import subprocess
import optparse
import sys
import os
import run_layout_tests

# This file specifies the list of tests that we will run, and
# forwards them to run_layout_tests.py

layout_tests_dir = "../../external/chromium/third_party/WebKit/LayoutTests"


# Open up the appropriate TestExpectations file and prune the expected
# failures from the tests that will be run.  Note that this is a simple
# version of the layout test expectations parser found in
# external/chromium/third_party/WebKit/Tools/Scripts/webkitpy/layout_tests
#   /models/test_expectations.py
# The language defined by this simple parser is a very small subset of the
# language defined by the WebKit Test Expectations parser.
# The reason we don't simply use that is because we don't have a WebKit port
# framework setup for Steel, which is a dependency of the WebKit test
# expectations parser, and what seems to be a fair bit of work to implement.
def FilterTestsByExpectationsFile(tests_to_run, expected_output_dir):
  class ParseError(Exception):
    pass

  tests_to_run_set = set(tests_to_run)

  expectations_file_name = os.path.join(expected_output_dir,
    'TestExpectations.txt')

  if not os.path.exists(expectations_file_name):
    # If the file doesn't exist, there is no filtering to be done
    return tests_to_run

  expectations_file = open(expectations_file_name, 'r')
  for line in expectations_file:
    line = line.strip()
    if line == '':
      continue

    space_pos = line.find(' ')
    if space_pos == -1:
      raise ParseError
    test_name = os.path.splitext(line[:space_pos])[0]
    expected_results = line[space_pos+1:]
    if expected_results.upper().find('FAILURE') != -1:
      # This test is expected to fail, remove it from the set of tests to
      # run.
      if test_name in tests_to_run_set:
        tests_to_run_set.remove(test_name)

  expectations_file.close()

  return list(tests_to_run_set)

def GetTestsToRun(expected_output_dir):
  tests_to_run = []

  # Determine which tests to run by looking at what expected output files
  # are available to us.
  for (dir_path, dir_names, file_names) in os.walk(expected_output_dir):
    for file_name in file_names:
      # Look for files that end with -expected.png
      find_substr = '-EXPECTED.PNG'
      expected_pos = file_name.upper().rfind(find_substr)
      if expected_pos == len(file_name) - len(find_substr):
        dir_rel_top_dir = os.path.relpath(dir_path, expected_output_dir)

        # Add the directory and strip the '-expected.png'
        test_name = os.path.join(dir_rel_top_dir, file_name[:-len(find_substr)])

        tests_to_run.append(test_name.replace('\\', '/'))

  # Prune the expected failures from the tests that will be run.
  return sorted(
    FilterTestsByExpectationsFile(tests_to_run, expected_output_dir))


# Parse the 'run_layout_tests' options here and then just pass them through
# to run_layout_tests.main() later, with the list of tests appended, and
# the expected output directory setup for Steel.
parser = run_layout_tests.GetOptionsParser()
(options, args) = parser.parse_args()
if len(args) != 1:
  print 'Usage: ' + sys.argv[0] + ' PLATFORM [options]\n'
  print run_layout_tests.GetSupportedPlatformsStr(
          run_layout_tests.GetSupportedPlatforms())
  exit(-1)

ret = run_layout_tests.main(
  options,
  args + GetTestsToRun(options.expected_output_dir))

sys.exit(ret)
