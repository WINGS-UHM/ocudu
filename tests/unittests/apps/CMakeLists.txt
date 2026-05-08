# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


add_subdirectory(units)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fbs/CMakeLists.txt")
    add_subdirectory(fbs)
endif ()
