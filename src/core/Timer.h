#pragma once

#include <chrono>

namespace agss {

class Timer {
public:
  using Clock = std::chrono::high_resolution_clock;

  static double NowSeconds();
};

} // namespace agss

