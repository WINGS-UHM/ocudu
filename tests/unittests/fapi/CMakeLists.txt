# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


add_subdirectory(common)
add_subdirectory(p7)

add_library(ocudu_fapi_message_builder_test_helpers STATIC message_builder_helpers.cpp)
