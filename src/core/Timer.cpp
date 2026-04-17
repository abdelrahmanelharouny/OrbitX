#include "core/Timer.h"

namespace agss {

double Timer::NowSeconds() {
  const auto now = Clock::now().time_since_epoch();
  return std::chrono::duration<double>(now).count();
}

} // namespace agss

