#!/bin/bash
#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

# for actual source and header files
find . -type f \( -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.h.in" \) \( ! -path "*/bundled/*" ! -path "*/external/*" \) -exec sed -i 's/0x[0-9a-fA-F]*/\L&/g' {} \;
