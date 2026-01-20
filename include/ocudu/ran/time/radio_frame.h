/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include <chrono>

namespace ocudu {

/// Time unit for subframes. One subframe is 1 millisecond.
using subframes = std::chrono::milliseconds;

/// Time unit of radio frames. One radio frame is 10 milliseconds.
using radio_frames = std::chrono::duration<std::chrono::milliseconds::rep, std::ratio<1, 100>>;

/// Time unit of hyper system frames. One hyper frame is 10 * 1024 = 10240 milliseconds.
using hyper_frames = std::chrono::duration<std::chrono::milliseconds::rep, std::ratio<1024, 100>>;

} // namespace ocudu
