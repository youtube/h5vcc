import os
import sys
import shutil
import time
import optparse
import diff_images
import glob
import re
from layout_exceptions import *


default_layouttest_dir = \
  "../../external/chromium/third_party/WebKit/LayoutTests"

tests_complete_re = re.compile(
    r'LBShellLayoutTestRunner -- All tests complete\.')


def main(options, args):
  platform = args[0]
  tests = args[1:]

  # The arguments cannot be absolute paths, they must be relative to
  # TESTROOTDIR
  for test in tests:
    if os.path.isabs(test):
      print 'Paths to tests must be descendants of TESTROOTDIR, and ' + \
        'relative to it.'
      print '(TESTROOTDIR: "' + options.test_root_dir + '")'
      print 'The first problematic argument was "' + test + '".'
      return -1

  # Echo out the main program options/parameters passed in
  print 'Running layout tests with the following parameters:'
  print 'Platform: ' + platform
  for (k,v) in options.__dict__.items():
    print str(k) + ': ' + str(v)
  print ''

  test_runner_process = None
  try:
    Plat = GetPlatformModule(platform)

    # Remove extensions from the input files, we'll add them back as needed
    # later.
    noext_tests = [ os.path.splitext(x)[0] for x in tests ]

    work_dir = Plat.GetWorkDirectory(options.use_build)

    # Copy input test files in to a location accessible by the platform
    SetupTestEnvironment(work_dir,
                         options.test_root_dir,
                         noext_tests)

    work_dir_tests = [os.path.join(work_dir, x) for x in noext_tests]

    print 'Running platform layout test executable...'
    # Actually get the platform to start running tests

    launcher = Plat.GetTestRunnerLauncher(tests, options.use_build)
    devnull = open(os.devnull, 'w')
    launcher.SetOutputFile(devnull)
    launcher.AddExitRegex(tests_complete_re)
    launcher.SetTimeout(50 + 10 * len(tests))
    class Results:
      def __init__(self):
        self._failures = 0
      def AddFailure(self):
        self._failures = self._failures + 1
      def failures(self):
        return self._failures
    test_results = Results()
    launcher.SetOutputCallback(
        lambda x: ProcessLine(x, work_dir, 50, 10, options, test_results))
  except TestClientError as e:
    print e.strerror
    return -1
  except TestFileError as e:
    print e.strerror
    return -1

  launcher.Run()

  if test_results.failures() > 0 or launcher.returncode != 0:
    return -1

  return 0

def GetDefaultExpectedOutputDir(platform):
  return os.path.join(default_layouttest_dir, "platform/lbshell-" + platform)

def GetOptionsParser():
  def GetOptionsParserDetail(platform=None):
    usage_msg = 'Usage: %prog [options] PLATFORM'
    if sys.argv[0] == 'run_layout_tests.py':
     usage_msg += ' (Tests...)'
    usage_msg += '\n'
    usage_msg += GetSupportedPlatformsStr(GetSupportedPlatforms()) + '\n'

    parser = optparse.OptionParser(usage=usage_msg)

    parser.add_option("-t", "--testrootdir", dest="test_root_dir",
      default = default_layouttest_dir,
      help="Search for specified layout tests rooted in TESTROOTDIR. " + \
        "The default directory is %default",
      metavar="TESTROOTDIR")

    # When defining the expected output directory option, we may not know
    # the platform.  We learn it by parsing the options with this option
    # parser, so we parse twice, once to learn the platform, and then again
    # to apply it to the expected output dir default.
    parser.add_option("-e", "--expectedoutputdir", dest="expected_output_dir",
      default = GetDefaultExpectedOutputDir(platform if platform != None
        else 'PLATFORM'),
      help="Use EXPECTEDOUTPUTDIR as a root when searching for expected " + \
        "output files.  The default directory is %default",
      metavar="EXPECTEDOUTPUTDIR")

    parser.add_option("-r", "--rebasetodir", dest="rebase_to_dir",
      help="When specified, will write rebased files to REBASETODIR.",
      metavar="REBASETODIR")
    parser.add_option("-o", "--outputdir", dest="output_dir",
      help="If specified, output test results will be placed in OUTPUTDIR",
      metavar="OUTPUTDIR")
    parser.add_option("-b", "--usebuild", dest="use_build",
      help="If specified, the given build will be used for testing. If it" + \
        " is not specified, the script will search for a build.  The" + \
        " actual build to run is platform specific.  For example: " + \
        " '-b Linux_Debug'.",
      metavar="USEBUILD")

    (options, args) = parser.parse_args()

    return parser

  # Get an initial parser before we know the platform...  The parser will
  # then be used to parse the command line arguments and determine the platform,
  # after which we will make the parser again with the platform info and return
  # that.
  parser = GetOptionsParserDetail()
  (options, parsed_args) = parser.parse_args()
  if len(parsed_args) == 0:
    # This is likely an error, as a platform has not been specified,
    # but we'll let the caller detect and deal with that
    return parser

  # Re-parse the options now that we have a platform
  parser = GetOptionsParserDetail(parsed_args[0])
  return parser


def GetPlatformModule(platform):
  supported_platforms = GetSupportedPlatforms()
  if not platform in supported_platforms:
    raise TestClientError('Specified platform "' + platform + \
      '" is not supported.\n' + GetSupportedPlatformsStr(supported_platforms))

  plat_module = 'plat_' + platform
  assert(os.path.exists(plat_module + '.py'))
  return __import__(plat_module)

def GetSupportedPlatforms():
  supported_platform_files = glob.glob('plat_*.py')
  plat_name_extractor = re.compile(r"^plat_(.*)\.py")
  supported_platforms = [ plat_name_extractor.match(x).group(1) for x in
                          supported_platform_files]
  return supported_platforms

def GetSupportedPlatformsStr(supported_platforms):
  ret = 'Supported platforms:\n'
  for plat in supported_platforms:
    ret += '  ' + plat + '\n'
  return ret

test_start_re = re.compile(r'LBShellLayoutTestRunner -- Test Start \((.*)\)\.')
test_end_re = re.compile(r'LBShellLayoutTestRunner -- Test Done \((.*)\)\.')

def ProcessLine(line,
                work_dir,
                first_test_timeout,
                test_timeout,
                options,
                test_results):
  """Process a line of output from the console test runner and Possibly
  kick off a diff test because of it."""
  test_start_match = test_start_re.search(line)
  if test_start_match:
    print 'Waiting for results from "' + test_start_match.group(1) + '"...'
    return

  test_end_match = test_end_re.search(line)
  if test_end_match:
    test_path_from_local = test_end_match.group(1)
    find_str = options.test_root_dir + '/'
    find_index = test_path_from_local.find(find_str)
    if find_index != -1:
      cur_test = test_path_from_local[find_index+len(find_str):]
    else:
      cur_test = test_path_from_local
    print 'Consuming ' + cur_test + '...'
    ConsumeTestOutput(cur_test, work_dir, options, test_results)

def ConsumeTestOutput(cur_test,
                      work_dir,
                      options,
                      test_results):
  test_output = {}
  outputImagePath = \
    os.path.abspath(os.path.join(work_dir, cur_test+'-actual.png'))
  if os.path.exists(outputImagePath):
    test_output['image_file'] = outputImagePath
  outputTextPath = \
    os.path.abspath(os.path.join(work_dir, cur_test+'-actual.txt'))
  if os.path.exists(outputTextPath):
    test_output['text_file'] = outputTextPath

  # Test results as they are generated, to increase efficiency so that we
  # may compare current results while the test runner is generating new results
  test_result = CompareResults(options.expected_output_dir, cur_test, work_dir, test_output)
  (did_pass, msg) = TestSuccessful(test_output, test_result)
  out_msg = ''
  if did_pass:
    out_msg = 'PASS: ' + cur_test
  else:
    out_msg = 'FAIL: ' + cur_test + '\n' + \
      '  ' + msg + '\n'
    test_results.AddFailure()

  print out_msg

  # Possibly copy test ouput to an output directory
  if options.output_dir:
    WriteTestOutput(options.output_dir,
                    cur_test,
                    work_dir,
                    test_output,
                    test_result,
                    out_msg)

  if options.rebase_to_dir:
    WriteRebaseOutput(options.rebase_to_dir, cur_test, test_output)

  return did_pass

def CompareResults(expected_output_dir, test, work_dir, test_output):
  """Compares a single test's output to expected results"""
  test_result = {}
  expected_image_file = os.path.join(expected_output_dir, test) + \
                        '-expected.png'
  if os.path.exists(expected_image_file):
    test_result['expected_image_file'] = expected_image_file
    if test_output.has_key('image_file'):
      # We have an expected image and an actual image, compare the results
      # to see if they're different
      try:
        diff_file = os.path.join(work_dir, test) + '-diff.png'
        num_diff_pixels = \
          diff_images.DiffImageFiles(expected_image_file, test_output['image_file'], diff_file)
        test_result['diff_num_non_zero_pixels'] = num_diff_pixels
        test_result['image_diff_file'] = diff_file
      except Exception as e:
        # If an exception is raised, it is enough to leave test_results
        # with no 'image_diff_file' field in order to indicate that an error
        # occurred.
        test_result['error'] = 'Error diffing image "' + test + '": ' + str(e)

  return test_result

def TestSuccessful(test_output, test_result):
  """Returns true if the return value of CompareResults is a test success"""
  if test_result.has_key('error'):
    return (False, test_result['error'])
  if not test_output.has_key('image_file'):
    return (False, 'No output image file produced.')
  if not test_result.has_key('expected_image_file'):
    return (False, 'No expected image results file found.  For Steel, all ' + \
                    'tests must have associated "*-expected.png" files.')
  if not test_result.has_key('image_diff_file'):
    return (False, 'Could not diff expected/actual images because they had ' + \
                    'different dimensions or formats.')
  if test_result['diff_num_non_zero_pixels'] > 0:
    return (False, 'Image diff failed, there is at least one pixel difference')

  return (True, '')


def WriteTestOutput(output_dir, cur_test, work_dir, test_output, test_result,
                    out_msg):
  """ Copies test output and results to a specified output directory """

  if not os.path.exists(output_dir):
    os.makedirs(output_dir)

  test_dir = os.path.join(output_dir, os.path.dirname(cur_test))
  if not os.path.exists(test_dir):
    os.makedirs(test_dir)

  # Write out the output files associated with this test
  for output_file in [test_output['image_file']]:
    shutil.copy(output_file,
      os.path.join(test_dir, os.path.basename(output_file)))

  # Write out the diff image
  if test_result.has_key('image_diff_file'):
    shutil.copy(test_result['image_diff_file'],
      os.path.join(output_dir, cur_test + '-diff.png'))

  # Write out test comparison results
  result_filename = os.path.join(output_dir, cur_test + '-results.txt')
  result_file = open(result_filename, 'w')
  result_file.write(out_msg)


def WriteRebaseOutput(rebase_dir, cur_test, test_output):
  if not os.path.exists(rebase_dir):
    os.makedirs(rebase_dir)

  test_dir = os.path.join(rebase_dir, os.path.dirname(cur_test))
  if not os.path.exists(test_dir):
    os.makedirs(test_dir)

  img_filename = os.path.join(rebase_dir, cur_test + '-expected.png')
  shutil.copy(test_output['image_file'], img_filename)


def SetupTestEnvironment(work_dir, test_root_dir, tests):
  """Create a platform test runner accessible directory and copy the specified
  tests to it."""

  src_test_files = [os.path.join(test_root_dir, x) + '.html' for x in tests]
  dst_test_files = [os.path.join(work_dir, x) + '.html' for x in tests]

  # Make sure all source tests specified actually exist
  for test_filename in src_test_files:
    if not os.path.exists(test_filename):
      raise TestFileError('Could not find test "' + test_filename + '".')


  # Start by cleaning the work_dir so that it's ready for the input
  problem_removing = False
  try:
    if os.path.exists(work_dir):
      shutil.rmtree(work_dir)
  except:
    problem_removing = True

  if problem_removing or os.path.exists(work_dir):
    raise TestClientError('There was a problem clearing the ' + \
      'temporary work directory:\n  "' + work_dir + '"\n\nPerhaps an ' + \
      'access problem?')

  # Create our fresh work directory
  os.makedirs(work_dir)

  # Copy the original tests to their new locations in the work directory
  print 'Copying test files in to a platform accessible folder...'
  for src_dst_pair in zip(src_test_files, dst_test_files):
    src_file = src_dst_pair[0]
    dst_file = src_dst_pair[1]

    # We already tested for this
    assert(os.path.exists(src_file))

    # Make the destination file's directory if it doesn't already exist
    if not os.path.exists(os.path.dirname(dst_file)):
      os.makedirs(os.path.dirname(dst_file))

    # Copy the file!
    shutil.copy(src_file, dst_file)


if __name__ == '__main__':
  parser = GetOptionsParser()
  (options, args) = parser.parse_args()

  if len(args) < 2:
    parser.print_help()
    sys.exit(-1)

  ret = main(options, args)

  sys.exit(ret)
