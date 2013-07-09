#!/bin/sh

# This script exists mainly to append the additional step of zipping up layout
# test output to the execution of the layout tests.

# Exit if any command fails
set -e

if [ $# -ne 3 ]; then
  echo "Usage: run_layout_tests.sh <platform> <build> <zipped_output>"
  exit 1
fi

platform=$1
build=$2
zipped_output=$3

# Put test results in a known output location
layout_test_output_dir="../../out/lb_shell/$build/layout_test_output"

# Actually run the layout tests
(
  cd ../layout_tests;
  python run_lb_shell_tests.py "$platform" --usebuild="$build" --outputdir="$layout_test_output_dir"
)

# Zip the test results for later inspection
(
  # Execute zip in the test output directory so that it is rooted there
  cd "$layout_test_output_dir";
  zip -r Output.zip * > /dev/null
)

# Make sure the directories leading up to the zip file exist
mkdir -p "$(dirname "$zipped_output")"

# Finally copy the zip file in to its place
mv  "$layout_test_output_dir/Output.zip" "$zipped_output"
