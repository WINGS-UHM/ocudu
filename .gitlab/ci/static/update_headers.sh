#!/bin/bash
#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set -e

echo "=================="
echo "= Update headers ="
echo "=================="

# for CMake/YML files
find . -type f -\( -name "CMakeLists.txt" -o -name "*.cmake" -o -name "*.yml" -o -name "*.sh" -o -name "*.py" -o -name "*.toml" -o -name "Dockerfile" -o -name "ocudu_performance" -o -name ".gdbinit" \) \
    ! -path "*/build*/*" ! -path "*/.tox/*" ! -path "*/docker/open5gs/*" ! -name "FindBackward.cmake" ! -name "sbom.cmake" ! -path "*/node_modules*/*" \
    -print0 | while IFS= read -r -d '' file; do

    # Check header format
    found_header=false
    while IFS= read -r line; do
        if [ -z "$line" ] && [ "$found_header" = false ]; then
            continue # Ignore empty lines before first comment block
        elif [[ "$line" =~ ^#.*$ ]]; then
            found_header=true
            continue # This line start with #. Keep reading
        elif [[ -z "$line" ]]; then
            break # Empty line after the header block. The format is valid and exit.
        else
            echo "$file: Header (or empty line after it) is missing."
            exit 1
        fi
    done <"$file"

    # Replace header
    perl -0777 -pi -e "s/#[^!][\s\S]*?(?=\n.*?=|\n\n)/#
# Copyright 2021-$(date +%Y) Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#/" "$file"

done

# for actual source and header files
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.h.in" \) \
    ! -path "*/external/*" ! -name "rfnoc_test.cc" \
    -exec perl -0777 -pi -e "s{/\*.*?\*/}{/*
 *
 * Copyright 2021-$(date +%Y) Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */}s" {} \;

# for matlab files (for the OCUDU-matlab supplementary repo): in matlab, the header
# is the second comment "%" block, as the first contains the file documentation
find . -type f -name "*.m" ! -name "hSkipWeakTimingOffset.m" \
    ! -name "HARQEntity.m" \
    -exec perl -0777 -pi -e "s/((?:%.*\n)+\n)(?:%.*\n)+/\$1%
%   Copyright 2021-2025 Software Radio Systems Limited
%
%   By using this file, you agree to the terms and conditions set
%   forth in the LICENSE file which can be found at the top level of
%   the distribution.
%
/" {} \;
