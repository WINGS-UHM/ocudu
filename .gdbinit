#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

############################################
#            Pretty-Printers
############################################

# If you add a pretty printer, please create the corresponding test in
# tests/utils/gdb/pretty_printers

python

sys.path.append('./utils/gdb/python/')

from ocudu.printers import register_printers
register_printers(gdb.current_objfile())
