#include "physics/PhysicsEngine.h"

#include "physics/PhysicsBody.h"

#include <algorithm>
#include <glm/geometric.hpp>

namespace agss {

PhysicsEngine::PhysicsEngine() : m_integrator(CreateIntegrator(m_config.integrator)) {}

void PhysicsEngine::RegisterBody(PhysicsBody* body) {
  if (!body) {
    return;
  }
  if (std::find(m_bodies.begin(), m_bodies.end(), body) != m_bodies.end()) {
    return;
  }
  m_bodies.push_back(body);
}

void PhysicsEngine::UnregisterBody(PhysicsBody* body) {
  m_bodies.erase(std::remove(m_bodies.begin(), m_bodies.end(), body), m_bodies.end());
}

void PhysicsEngine::AddForceGenerator(std::unique_ptr<IForceGenerator> gen) {
  if (!gen) {
    return;
  }
  m_generators.emplace_back(std::move(gen));
}

void PhysicsEngine::RemoveForceGenerator(const std::string& name) {
  m_generators.erase(
      std::remove_if(m_generators.begin(), m_generators.end(),
                     [&](const std::unique_ptr<IForceGenerator>& g) { return g && g->GetName() == name; }),
      m_generators.end());
}

IForceGenerator* PhysicsEngine::FindForceGenerator(const std::string& name) {
  for (auto& g : m_generators) {
    if (g && g->GetName() == name) {
      return g.get();
    }
  }
  return nullptr;
}

void PhysicsEngine::SetConfig(const PhysicsConfig& cfg) {
  m_config = cfg;
  m_integrator = CreateIntegrator(m_config.integrator);
}

void PhysicsEngine::ComputeForces(double dtDays) {
  (void)dtDays;
  for (auto* b : m_bodies) {
    if (!b) continue;
    b->accumulatedForce = glm::dvec3(0.0);
  }

  for (auto& gen : m_generators) {
    if (gen && gen->IsEnabled()) {
      gen->Apply(m_bodies, dtDays);
    }
  }

  for (auto* b : m_bodies) {
    if (!b) continue;
    b->lastForce = b->accumulatedForce;
    b->acceleration = (b->mass != 0.0) ? (b->accumulatedForce / b->mass) : glm::dvec3(0.0);
  }
}

void PhysicsEngine::UpdateBodies(double dtDays) {
  if (!m_integrator) {
    m_integrator = CreateIntegrator(m_config.integrator);
  }
  m_integrator->Integrate(m_bodies, dtDays);
  ApplyPostIntegrationStability();
}

void PhysicsEngine::ApplyPostIntegrationStability() {
  if (m_config.maxSpeed <= 0.0) {
    return;
  }

  for (auto* b : m_bodies) {
    if (!b) continue;
    const double speed = glm::length(b->velocity);
    if (speed > m_config.maxSpeed) {
      b->velocity = b->velocity * (m_config.maxSpeed / speed);
    }
  }
}

void PhysicsEngine::Step(double dtDays) {
  ComputeForces(dtDays);
  UpdateBodies(dtDays);
  m_simulationTimeDays += dtDays;
}

} // namespace agss

