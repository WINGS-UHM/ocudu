#!/bin/bash
#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

# make sure all commands are echoed
#set -x
set -o pipefail

# check for apps
app1=$(which git)
app2=$(which colordiff)
if [ ! -x "$app1" ] || [ ! -x "$app2" ]; then
  echo "Please install git and colordiff"
  exit 1
fi

# Get diff
diff=$(git diff)

if [ "$diff" ]; then
  # If diff is not empty, print it, save it as a patch and exit with errors
  echo "The following changes were detected:"
  echo "${diff}" | colordiff
  echo "${diff}" >ci.patch
  exit 1
else
  echo "No changes detected."
fi
