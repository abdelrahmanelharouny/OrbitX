#include "debug/EnergyMonitor.h"

#include "physics/PhysicsBody.h"

#include <glm/geometric.hpp>
#include <cmath>

namespace agss {

EnergyReport EnergyMonitor::Compute(const std::vector<PhysicsBody*>& bodies, const PhysicsConfig& cfg) {
  EnergyReport out{};

  // KE = 1/2 m v^2
  for (auto* b : bodies) {
    if (!b) continue;
    const double v2 = glm::dot(b->velocity, b->velocity);
    out.kinetic += 0.5 * b->mass * v2;
  }

  // PE = -/+ G m1 m2 / r   (softened: r = sqrt(r^2 + eps^2))
  const double eps2 = cfg.softeningEpsilon * cfg.softeningEpsilon;
  const double sign = cfg.repulsiveGravity ? +1.0 : -1.0; // repulsive has positive potential energy

  const size_t n = bodies.size();
  for (size_t i = 0; i < n; ++i) {
    auto* a = bodies[i];
    if (!a) continue;
    for (size_t j = i + 1; j < n; ++j) {
      auto* b = bodies[j];
      if (!b) continue;
      const glm::dvec3 r = b->position - a->position;
      const double dist = std::sqrt(glm::dot(r, r) + eps2);
      out.potential += sign * cfg.gravityConstantG * (a->mass * b->mass) / dist;
    }
  }

  out.total = out.kinetic + out.potential;
  return out;
}

} // namespace agss

