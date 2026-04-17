#pragma once

#include "physics/IForceGenerator.h"
#include "physics/Integrators.h"
#include "physics/PhysicsConfig.h"

#include <memory>
#include <string>
#include <vector>

namespace agss {

struct PhysicsBody;

class PhysicsEngine {
public:
  PhysicsEngine();

  void RegisterBody(PhysicsBody* body);
  void UnregisterBody(PhysicsBody* body);

  void AddForceGenerator(std::unique_ptr<IForceGenerator> gen);
  void RemoveForceGenerator(const std::string& name);
  IForceGenerator* FindForceGenerator(const std::string& name);

  void SetConfig(const PhysicsConfig& cfg);
  const PhysicsConfig& GetConfig() const { return m_config; }

  // Phase 2 API
  void ComputeForces(double dtDays);
  void UpdateBodies(double dtDays);

  // Convenience (typical per-frame/per-tick entrypoint)
  void Step(double dtDays);

  double GetSimulationTimeDays() const { return m_simulationTimeDays; }

  const std::vector<PhysicsBody*>& GetBodies() const { return m_bodies; }

private:
  void ApplyPostIntegrationStability();

  std::vector<PhysicsBody*> m_bodies;
  std::vector<std::unique_ptr<IForceGenerator>> m_generators;
  double m_simulationTimeDays = 0.0;

  PhysicsConfig m_config{};
  std::unique_ptr<IIntegrator> m_integrator;
};

} // namespace agss

