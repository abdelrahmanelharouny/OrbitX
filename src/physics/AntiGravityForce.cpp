#include "physics/AntiGravityForce.h"

#include "physics/PhysicsBody.h"

#include <glm/geometric.hpp>

namespace agss {

AntiGravityForce::AntiGravityForce(double strength, double effectRadiusAu)
    : m_strength(strength), m_effectRadiusAu(effectRadiusAu) {}

void AntiGravityForce::Apply(std::vector<PhysicsBody*>& bodies, double /*dtDays*/) {
  const double eps = 1e-9;
  const double radius2 = m_effectRadiusAu * m_effectRadiusAu;

  const size_t n = bodies.size();
  for (size_t i = 0; i < n; ++i) {
    auto* a = bodies[i];
    if (!a) continue;
    for (size_t j = i + 1; j < n; ++j) {
      auto* b = bodies[j];
      if (!b) continue;

      const glm::dvec3 r = b->position - a->position;
      const double dist2 = glm::dot(r, r) + eps;
      if (dist2 > radius2) {
        continue;
      }

      const double invDist = 1.0 / std::sqrt(dist2);
      const glm::dvec3 dir = r * invDist;

      // Repulsive "anti-gravity": same shape as gravity but opposite sign and scaled by strength.
      // Force magnitude uses the same G constant as GravityForce (kept in SimulationManager).
      const double forceMag = m_strength * (a->mass * b->mass) / dist2;
      const glm::dvec3 force = -forceMag * dir;

      a->accumulatedForce += force;
      b->accumulatedForce -= force;
    }
  }
}

} // namespace agss

