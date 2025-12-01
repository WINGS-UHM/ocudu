#!/bin/bash
#
# Copyright 2013-2022 Software Radio Systems Limited
#
# This file is part of srsRAN
#
# srsRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
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
