#include "simulation/SimulationManager.h"

#include "core/Timer.h"
#include "entities/CelestialBody.h"
#include "physics/GravityForce.h"
#include "simulation/CollisionSystem.h"

#include <cstdio>
#include <glm/geometric.hpp>

namespace agss {

bool SimulationManager::Init(const std::string& solarSystemJsonPath) {
  ResetToPreset(solarSystemJsonPath);
  m_prevWallSeconds = Timer::NowSeconds();
  return true;
}

void SimulationManager::ResetToPreset(const std::string& solarSystemJsonPath) {
  try {
    m_system = SolarSystemFactory::LoadFromJsonFile(solarSystemJsonPath);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "%s\n", e.what());
    return;
  }

  m_physics = PhysicsEngine{};
  m_gravity = nullptr;

  for (auto* b : m_system.bodiesRaw) {
    b->CommitPreviousPhysicsState();
    m_physics.RegisterBody(&b->Physics());
  }

  // Default Phase 2 params (runtime adjustable)
  m_physicsConfig.gravityConstantG = 0.00029591220828559104;
  m_physicsConfig.repulsiveGravity = false;
  m_physicsConfig.softeningEpsilon = 1e-6;
  m_physicsConfig.maxSpeed = 0.0; // disabled by default
  m_physicsConfig.integrator = IntegratorType::EulerExplicit;

  m_physics.SetConfig(m_physicsConfig);

  auto grav = std::make_unique<GravityForce>(m_physicsConfig.gravityConstantG);
  m_gravity = grav.get();
  SyncPhysicsConfigToForces();
  m_physics.AddForceGenerator(std::move(grav));

  m_collisionConfig.enabled = true;

  TimeControlConfig tcfg;
  tcfg.presetScales = {0.1, 1.0, 10.0, 100.0, 1000.0};
  tcfg.allowReverse = false;
  tcfg.maxSimStepDays = 0.002;
  tcfg.maxSubStepsPerFixedTick = 128;
  m_time.SetConfig(tcfg);
  m_time.SetPresetIndex(1); // x1

  m_trails.Configure(m_trailMaxPoints, m_debugTrails);
  m_trails.Reset(m_physics.GetBodies().size());
  UpdateMonitoringAndDebug();

  // Default selection: none. Primary body defaults to 0.
  m_selectedBody = -1;
  m_primaryBody = 0;
}

void SimulationManager::TickFrame() {
  const double now = Timer::NowSeconds();
  double frameSeconds = now - m_prevWallSeconds;
  m_prevWallSeconds = now;

  // Avoid spiral of death if the debugger pauses.
  if (frameSeconds > 0.25) {
    frameSeconds = 0.25;
  }

  m_accumulatorSeconds += frameSeconds;

  // Physics runs in simulation "days". We'll use a 60 Hz fixed timestep in wall seconds
  // and scale it by timeScale to advance simulation faster/slower.
  constexpr double fixedWallDtSeconds = 1.0 / 60.0;
  constexpr double secondsPerDay = 86400.0;

  while (m_accumulatorSeconds >= fixedWallDtSeconds) {
    m_accumulatorSeconds -= fixedWallDtSeconds;

    const double desiredSimDtDays = (fixedWallDtSeconds * m_time.GetTimeScale()) / secondsPerDay;
    size_t steps = 0;
    double subDt = 0.0;
    if (m_time.ComputeSubSteps(desiredSimDtDays, steps, subDt)) {
      for (size_t i = 0; i < steps; ++i) {
        StepFixed(subDt);
      }
    }
  }

  const double alpha = m_accumulatorSeconds / fixedWallDtSeconds;
  (void)alpha; // used later for render interpolation
}

void SimulationManager::StepFixed(double fixedDtDays) {
  SyncPhysicsConfigToForces();
  m_physics.Step(fixedDtDays);
  ResolveCollisionsIfNeeded();
  UpdateMonitoringAndDebug();
}

void SimulationManager::ToggleAntiGravity() {
  m_physicsConfig.repulsiveGravity = !m_physicsConfig.repulsiveGravity;
  m_physics.SetConfig(m_physicsConfig);
  SyncPhysicsConfigToForces();
}

bool SimulationManager::IsAntiGravityEnabled() const {
  return m_physicsConfig.repulsiveGravity;
}

void SimulationManager::SetGravityConstant(double g) {
  m_physicsConfig.gravityConstantG = g;
  if (m_gravity) {
    m_gravity->SetG(g);
  }
}

double SimulationManager::GetGravityConstant() const {
  return m_gravity ? m_gravity->GetG() : m_physicsConfig.gravityConstantG;
}

void SimulationManager::SetTimeScale(double scale) {
  m_time.SetTimeScale(scale);
}

void SimulationManager::SetIntegrator(IntegratorType type) {
  m_physicsConfig.integrator = type;
  m_physics.SetConfig(m_physicsConfig);
}

SimulationManager::DebugVectors SimulationManager::GetDebugVectorsForBody(size_t index) const {
  DebugVectors out{};
  if (index >= m_system.bodiesRaw.size()) {
    return out;
  }
  const auto& pb = m_system.bodiesRaw[index]->Physics();
  out.velocity = pb.velocity;
  out.force = pb.lastForce;
  return out;
}

SimulationManager::PhysicsSnapshot SimulationManager::GetPhysicsSnapshot() const {
  PhysicsSnapshot out{};
  const auto& bodies = m_physics.GetBodies();
  out.positions.reserve(bodies.size());
  out.velocities.reserve(bodies.size());
  out.forces.reserve(bodies.size());
  for (auto* b : bodies) {
    if (!b) continue;
    out.positions.push_back(b->position);
    out.velocities.push_back(b->velocity);
    out.forces.push_back(b->lastForce);
  }
  return out;
}

void SimulationManager::SetSelectedBodyIndex(int idx) {
  if (idx < 0 || static_cast<size_t>(idx) >= m_system.bodiesRaw.size()) {
    m_selectedBody = -1;
  } else {
    m_selectedBody = idx;
  }
}

SimulationManager::SelectedBodyInfo SimulationManager::GetSelectedBodyInfo() const {
  SelectedBodyInfo out{};
  if (!HasSelection()) return out;

  const auto* body = m_system.bodiesRaw[static_cast<size_t>(m_selectedBody)];
  if (!body) return out;

  out.name = body->GetName();
  out.mass = body->Physics().mass;
  out.speed = glm::length(body->Physics().velocity);

  if (m_primaryBody < m_system.bodiesRaw.size() && m_system.bodiesRaw[m_primaryBody]) {
    const glm::dvec3 d = body->Physics().position - m_system.bodiesRaw[m_primaryBody]->Physics().position;
    out.distanceFromPrimary = glm::length(d);
  }

  return out;
}

std::vector<TrailRenderData> SimulationManager::BuildTrailRenderData(float alphaMin) const {
  std::vector<TrailRenderData> out;
  if (!m_debugEnabled || !m_debugTrails) {
    return out;
  }
  out.reserve(m_physics.GetBodies().size());
  for (size_t i = 0; i < m_physics.GetBodies().size(); ++i) {
    out.push_back(m_trails.BuildRenderDataForBody(i, alphaMin));
  }
  return out;
}

void SimulationManager::SyncPhysicsConfigToForces() {
  if (!m_gravity) {
    return;
  }
  m_gravity->SetG(m_physicsConfig.gravityConstantG);
  m_gravity->SetRepulsive(m_physicsConfig.repulsiveGravity);
  m_gravity->SetSofteningEpsilon(m_physicsConfig.softeningEpsilon);
}

void SimulationManager::UpdateMonitoringAndDebug() {
  // Energy monitoring (always computed; cheap for small N, UI can consume)
  m_energy = EnergyMonitor::Compute(m_physics.GetBodies(), m_physicsConfig);

  if (!m_debugEnabled) {
    return;
  }
  m_trails.Configure(m_trailMaxPoints, m_debugTrails);
  m_trails.PushSamples(m_physics.GetBodies());
}

void SimulationManager::ResolveCollisionsIfNeeded() {
  if (!m_collisionConfig.enabled) {
    return;
  }

  const auto bodies = m_physics.GetBodies();
  const auto collisions = CollisionSystem::Detect(bodies);
  if (collisions.empty()) {
    return;
  }

  std::vector<bool> keepMask;
  CollisionSystem::ResolveMergeConserveMomentum(bodies, collisions, keepMask);

  // Apply keepMask to entity ownership lists (SimulationManager owns entities).
  // We keep the "a" body and delete the "b" body for each merge.
  std::vector<std::unique_ptr<CelestialBody>> newOwned;
  std::vector<CelestialBody*> newRaw;
  newOwned.reserve(m_system.bodiesOwned.size());
  newRaw.reserve(m_system.bodiesRaw.size());

  for (size_t i = 0; i < m_system.bodiesOwned.size(); ++i) {
    if (i < keepMask.size() && keepMask[i]) {
      newRaw.push_back(m_system.bodiesOwned[i].get());
      newOwned.emplace_back(std::move(m_system.bodiesOwned[i]));
    }
  }

  m_system.bodiesOwned = std::move(newOwned);
  m_system.bodiesRaw = std::move(newRaw);

  // Rebuild physics body registration to match new arrays.
  m_physics = PhysicsEngine{};
  m_physics.SetConfig(m_physicsConfig);

  auto grav = std::make_unique<GravityForce>(m_physicsConfig.gravityConstantG);
  m_gravity = grav.get();
  SyncPhysicsConfigToForces();
  m_physics.AddForceGenerator(std::move(grav));

  for (auto* b : m_system.bodiesRaw) {
    m_physics.RegisterBody(&b->Physics());
  }

  // Reset debug history to avoid mismatch after merges.
  m_trails.Reset(m_physics.GetBodies().size());
}

} // namespace agss

