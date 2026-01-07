#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(transform_precoding_sources
        transform_precoder_dft_impl.cpp
        transform_precoding_factories.cpp)

add_library(ocudu_transform_precoding STATIC ${transform_precoding_sources})

add_to_exported_libs(ocudu_transform_precoding)
