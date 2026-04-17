#pragma once

#include "debug/EnergyMonitor.h"
#include "debug/OrbitTrailsSystem.h"
#include "entities/SolarSystemFactory.h"
#include "physics/PhysicsConfig.h"
#include "physics/PhysicsEngine.h"
#include "simulation/CollisionSystem.h"
#include "time/TimeController.h"

#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <vector>

namespace agss {

class GravityForce;

class SimulationManager {
public:
  bool Init(const std::string& solarSystemJsonPath);

  void TickFrame();
  void ResetToPreset(const std::string& solarSystemJsonPath);

  void TogglePaused() { m_time.TogglePause(); }
  bool IsPaused() const { return m_time.IsPaused(); }
  void ToggleAntiGravity();

  bool IsAntiGravityEnabled() const;

  // Runtime controls (Phase 2 requirements)
  void SetGravityConstant(double g);
  double GetGravityConstant() const;

  void SetTimeScale(double scale);
  double GetTimeScale() const { return m_time.GetTimeScale(); }
  void IncreaseSpeedPreset() { m_time.IncreasePreset(); }
  void DecreaseSpeedPreset() { m_time.DecreasePreset(); }
  size_t GetSpeedPresetIndex() const { return m_time.GetPresetIndex(); }
  void SetSpeedPresetIndex(size_t idx) { m_time.SetPresetIndex(idx); }

  void SetIntegrator(IntegratorType type);
  IntegratorType GetIntegrator() const { return m_physicsConfig.integrator; }

  const PhysicsConfig& GetPhysicsConfig() const { return m_physicsConfig; }

  // Debug data access (renderer can visualize without touching physics)
  struct DebugVectors {
    // Direction is in world space; magnitude is raw in simulation units.
    // Rendering can scale/normalize as needed.
    glm::dvec3 velocity{0.0};
    glm::dvec3 force{0.0};
  };
  DebugVectors GetDebugVectorsForBody(size_t index) const;

  const EnergyReport& GetEnergyReport() const { return m_energy; }

  struct PhysicsSnapshot {
    std::vector<glm::dvec3> positions;
    std::vector<glm::dvec3> velocities;
    std::vector<glm::dvec3> forces;
  };
  PhysicsSnapshot GetPhysicsSnapshot() const;

  // Selection / interaction
  void SetSelectedBodyIndex(int idx);
  int GetSelectedBodyIndex() const { return m_selectedBody; }
  bool HasSelection() const { return m_selectedBody >= 0 && static_cast<size_t>(m_selectedBody) < m_system.bodiesRaw.size(); }

  struct SelectedBodyInfo {
    std::string name;
    double mass = 0.0;
    double speed = 0.0;              // AU/day
    double distanceFromPrimary = 0.0; // AU
  };
  SelectedBodyInfo GetSelectedBodyInfo() const;

  // Build render-ready trail data with fading (per body).
  std::vector<TrailRenderData> BuildTrailRenderData(float alphaMin = 0.05f) const;

  void SetDebugEnabled(bool enabled) { m_debugEnabled = enabled; }
  bool IsDebugEnabled() const { return m_debugEnabled; }
  void ToggleDebugEnabled() { m_debugEnabled = !m_debugEnabled; }
  void ToggleDebugTrails() { m_debugTrails = !m_debugTrails; m_trails.Configure(m_trailMaxPoints, m_debugTrails); }
  void ToggleDebugVelocity() { m_debugVelocity = !m_debugVelocity; }
  void ToggleDebugForces() { m_debugForces = !m_debugForces; }

  bool DebugDrawTrails() const { return m_debugTrails; }
  bool DebugDrawVelocity() const { return m_debugVelocity; }
  bool DebugDrawForces() const { return m_debugForces; }

private:
  void StepFixed(double fixedDtDays);
  void SyncPhysicsConfigToForces();
  void UpdateMonitoringAndDebug();
  void ResolveCollisionsIfNeeded();

  SolarSystemBuildResult m_system;
  PhysicsEngine m_physics;

  GravityForce* m_gravity = nullptr;

  TimeController m_time{};

  PhysicsConfig m_physicsConfig{};

  // Debug / monitoring
  EnergyReport m_energy{};
  OrbitTrailsSystem m_trails{};
  CollisionConfig m_collisionConfig{};

  bool m_debugEnabled = true;
  bool m_debugTrails = true;
  bool m_debugVelocity = true;
  bool m_debugForces = false;

  // Orbit trail settings
  size_t m_trailMaxPoints = 4096;

  // Fixed-timestep accumulator (seconds wall-clock)
  double m_prevWallSeconds = 0.0;
  double m_accumulatorSeconds = 0.0;

  int m_selectedBody = -1;
  size_t m_primaryBody = 0; // index of "star" for distance queries; default 0
};

} // namespace agss

