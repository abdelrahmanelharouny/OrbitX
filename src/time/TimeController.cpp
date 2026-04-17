#include "time/TimeController.h"

#include <algorithm>
#include <cmath>

namespace agss {

void TimeController::SetConfig(TimeControlConfig cfg) {
  if (cfg.presetScales.empty()) {
    cfg.presetScales = {1.0};
  }
  for (auto& s : cfg.presetScales) {
    if (s < 0.0) s = -s;
    if (s == 0.0) s = 1.0;
  }
  std::sort(cfg.presetScales.begin(), cfg.presetScales.end());

  m_cfg = std::move(cfg);
  m_presetIndex = std::min(m_presetIndex, m_cfg.presetScales.size() - 1);
  SetPresetIndex(m_presetIndex);
}

void TimeController::SetTimeScale(double scale) {
  if (!m_cfg.allowReverse) {
    if (scale < 0.0) scale = -scale;
  }
  if (scale == 0.0) {
    // For "stopped" prefer Pause instead of zero scale.
    m_paused = true;
    return;
  }
  m_timeScale = scale;
  SnapToNearestPresetIfClose();
}

void TimeController::SetPresetIndex(size_t idx) {
  if (m_cfg.presetScales.empty()) return;
  m_presetIndex = std::min(idx, m_cfg.presetScales.size() - 1);
  const double sign = (m_cfg.allowReverse && m_timeScale < 0.0) ? -1.0 : 1.0;
  m_timeScale = sign * m_cfg.presetScales[m_presetIndex];
}

void TimeController::IncreasePreset() {
  if (m_cfg.presetScales.empty()) return;
  if (m_presetIndex + 1 < m_cfg.presetScales.size()) {
    SetPresetIndex(m_presetIndex + 1);
  }
}

void TimeController::DecreasePreset() {
  if (m_cfg.presetScales.empty()) return;
  if (m_presetIndex > 0) {
    SetPresetIndex(m_presetIndex - 1);
  }
}

bool TimeController::ComputeSubSteps(double desiredSimDtDays, size_t& outSteps, double& outSubDtDays) const {
  if (m_paused) {
    outSteps = 0;
    outSubDtDays = 0.0;
    return false;
  }

  const double absDt = std::abs(desiredSimDtDays);
  if (absDt <= 0.0) {
    outSteps = 0;
    outSubDtDays = 0.0;
    return false;
  }

  const double cap = std::max(1e-12, m_cfg.maxSimStepDays);
  size_t steps = static_cast<size_t>(std::ceil(absDt / cap));
  steps = std::max<size_t>(1, std::min(steps, m_cfg.maxSubStepsPerFixedTick));
  outSteps = steps;
  outSubDtDays = desiredSimDtDays / static_cast<double>(steps);
  return true;
}

void TimeController::SnapToNearestPresetIfClose() {
  if (m_cfg.presetScales.empty()) return;

  const double absScale = std::abs(m_timeScale);
  size_t best = 0;
  double bestErr = std::abs(absScale - m_cfg.presetScales[0]);
  for (size_t i = 1; i < m_cfg.presetScales.size(); ++i) {
    const double err = std::abs(absScale - m_cfg.presetScales[i]);
    if (err < bestErr) {
      bestErr = err;
      best = i;
    }
  }

  // Snap if within 2% of a preset (so wheel/slider feels stable).
  const double target = m_cfg.presetScales[best];
  if (target > 0.0 && (bestErr / target) < 0.02) {
    m_presetIndex = best;
  }
}

} // namespace agss

