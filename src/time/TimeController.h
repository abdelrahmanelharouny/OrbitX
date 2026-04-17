#pragma once

#include <cstddef>
#include <vector>

namespace agss {

struct TimeControlConfig {
  // Allowed preset speeds (slider/stepper will snap to these).
  // Values are multipliers applied to wall-clock dt.
  std::vector<double> presetScales{0.1, 1.0, 10.0, 100.0, 1000.0};

  bool allowReverse = false;

  // Stability: cap absolute dt per physics step (simulation days).
  double maxSimStepDays = 0.002; // ~172.8 seconds

  // Safety: prevent huge substep loops.
  size_t maxSubStepsPerFixedTick = 128;
};

class TimeController {
public:
  void SetConfig(TimeControlConfig cfg);
  const TimeControlConfig& GetConfig() const { return m_cfg; }

  void Play() { m_paused = false; }
  void Pause() { m_paused = true; }
  void TogglePause() { m_paused = !m_paused; }
  bool IsPaused() const { return m_paused; }

  void SetTimeScale(double scale);
  double GetTimeScale() const { return m_timeScale; }

  // Preset stepping helpers
  void SetPresetIndex(size_t idx);
  size_t GetPresetIndex() const { return m_presetIndex; }
  void IncreasePreset();
  void DecreasePreset();

  // Compute substep count and per-substep sim dt based on the configured maxSimStepDays.
  // Returns false if paused (no simulation advance).
  bool ComputeSubSteps(double desiredSimDtDays, size_t& outSteps, double& outSubDtDays) const;

private:
  TimeControlConfig m_cfg{};
  bool m_paused = false;
  double m_timeScale = 1.0;
  size_t m_presetIndex = 1; // default to x1

  void SnapToNearestPresetIfClose();
};

} // namespace agss

