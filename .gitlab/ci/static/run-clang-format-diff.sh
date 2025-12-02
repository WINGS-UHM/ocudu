#!/bin/bash
#
# Copyright 2013-2022 Software Radio Systems Limited
#
# This file is part of OCUDU
#
# OCUDU is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# OCUDU is distributed in the hope that it will be useful,
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

if [ ! "$1" ]; then
  echo "Please call script with target branch name or git hash to perform diff with."
  echo "E.g. ./run-clang-format-diff.sh [HASH]"
  exit 1
fi

# check for apps
app1=$(which clang-format)
app2=$(which git)
if [ ! -x "$app1" ] || [ ! -x "$app2" ]; then
  echo "Please install clang-format and git"
  exit 1
fi

FILE_EXTENSION_REGEX='.*\.(cpp|cc|c\+\+|cxx|c|cl|h|hh|hpp)$'
target=$1

echo "Running code format check between current state and ${target}"

# Get modified files (added, removed or changed) compared with target branch
files=$(git diff --name-only --relative --diff-filter=d "${target}" | grep -E "${FILE_EXTENSION_REGEX}" | tr '\n' ' ')

# Run clang-format for those files and apply changes
[ "$files" ] && clang-format -style=file -i ${files} || echo "No files changed"
