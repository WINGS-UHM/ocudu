# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


set_directory_properties(PROPERTIES LABELS "ofh")

add_executable(ofh_compression_benchmark ofh_compression_benchmark.cpp)
target_link_libraries(ofh_compression_benchmark ocudulog ocudu_ofh_compression)
