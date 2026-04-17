#include "physics/Integrators.h"

#include "physics/PhysicsConfig.h"

#include <glm/geometric.hpp>

namespace agss {

void EulerExplicitIntegrator::Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) {
  // Explicit Euler:
  //   x_{t+dt} = x_t + v_t * dt
  //   v_{t+dt} = v_t + a_t * dt
  for (auto* b : bodies) {
    if (!b) continue;
    b->position += b->velocity * dtDays;
    b->velocity += b->acceleration * dtDays;
  }
}

void SemiImplicitEulerIntegrator::Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) {
  // Semi-implicit (symplectic) Euler:
  //   v_{t+dt} = v_t + a_t * dt
  //   x_{t+dt} = x_t + v_{t+dt} * dt
  for (auto* b : bodies) {
    if (!b) continue;
    b->velocity += b->acceleration * dtDays;
    b->position += b->velocity * dtDays;
  }
}

void RK4Integrator::Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) {
  (void)dtDays;
  // Placeholder: we keep the type for API stability; not yet implemented.
  // For now, fall back to semi-implicit Euler so behavior is reasonable.
  SemiImplicitEulerIntegrator fallback;
  fallback.Integrate(bodies, dtDays);
}

std::unique_ptr<IIntegrator> CreateIntegrator(IntegratorType type) {
  switch (type) {
  case IntegratorType::EulerExplicit:
    return std::make_unique<EulerExplicitIntegrator>();
  case IntegratorType::SemiImplicitEuler:
    return std::make_unique<SemiImplicitEulerIntegrator>();
  case IntegratorType::RK4:
    return std::make_unique<RK4Integrator>();
  default:
    return std::make_unique<EulerExplicitIntegrator>();
  }
}

} // namespace agss

