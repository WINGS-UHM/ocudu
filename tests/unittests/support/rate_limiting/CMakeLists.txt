# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


add_executable(rate_limiting_test token_bucket_test.cpp)
target_link_libraries(rate_limiting_test ocudu_support ocudulog gtest GTest::gtest_main)
target_include_directories(rate_limiting_test PRIVATE ${CMAKE_SOURCE_DIR})
add_test(rate_limiting_test rate_limiting_test)
