Steel Layout Tests
------------------
In their current state, the Steel layout test framework can run a subset
of the WebKit layout tests found in:

external/chromium/third_party/WebKit/LayoutTests

In particular, it is only setup to run a test, take a screenshot when it is
loaded, and then image diff the results.  Thus, all render tree dump diffs
are ignored, and no window.* javascript functionality is available.

Running the Tests
-----------------
Enter the directory lbshell/layout_tests and enter the command:

  python run_lb_shell_tests.py PLATFORM

Where PLATFORM is the platform you'd like to run the tests on (i.e. 'ps3').
For a list of supported platforms, omit the PLATFORM parameter.

The script 'run_lb_shell_tests.py' determines the list of layout tests that will
be run as well as which directory contains the expected output files, and feeds
this in to 'run_layout_tests.py' which actually does all the work.
You can call 'run_layout_tests.py' directly and pass in your own set of tests
to run.  It has good command-line help output when passing "--help".
'run_lb_shell_tests.py' determines which tests to run by scanning the contents
of the platform specific expected output directory (see the section
"Adding a New Platform") and running a test for each expected output file found.
Thus, to declare to 'run_lb_shell_tests.py' that you would like a test
to be run as part of the layout tests, add an expected output file for it
to its expected output directory.

Adding a New Platform
---------------------
To add a new platform, you will need to create a "plat_PLAT.py" file with PLAT
set to the name of your platform.  This file should define a function
implementation: GetWorkDirectory() and a class definition for
TestRunnerProcess.  See existing "plat_PLAT.py" files for examples.

You should then add expected output files to the directory:

external/chromium/third_party/WebKit/LayoutTests/platform/steel-PLAT

which you will have to create.  This directory should have the same directory
structure as the layout tests directory.  Existing expected output can be found
in other platform layout test folders.  Another way of obtaining expected output
files is by doing a rebase.  See the "--rebasetodir" option when calling
"--help".  Essentially, the layout tests are run and the produced output is
saved as expected output files in the directory specified, which can then be
used as an expected output folder for subsequent layout test runs.

A special file titled "TestExpectations.txt" may be found in the root
expected output directory.  If it exists, it identifies whether certain
tests are expected to fail or not.  If they are, it will not execute them
as part of the layout tests, even if they have expected output.  The WebKit
layout tests have a formal TestExpectations file format that can be found here:

http://trac.webkit.org/wiki/TestExpectations

However currently the Steel layout tests recognize a very small subset of that
language; enough to allow us to indicate that tests are expected to fail.
If a test is listed in this file and has '[ Failure ]' after it on its line,
it will not be run.

Adding a New Test
-----------------
A new layout test directory for Steel-specific tests has been created and
can be found here:

external/chromium/third_party/WebKit/LayoutTests/steel

Hence, any new tests that are specific to Steel should be placed here,
and then the corresponding expected outputs created and placed in to
the platform-specific expected output directories.

Layout Test System Design
-------------------------
The main entry point to the layout test system is run_layout_tests.py.
The idea behind this script is that it will copy the tests to run (passed
as parameters) to a temporary work directory that is visible to the Steel
shell (i.e. in the content/local directory for the PS3).  The script will
then launch a "test runner" version of the steel shell that will read each of
the tests, and output the result back to the temporary work directory.  The
script will, in the mean time, watch for these output results to be produced
by the shell and when they are, it will analyze them to see if the test passed
or not.

All platform-specific code is placed in plat_*.py, where * is the platform.

