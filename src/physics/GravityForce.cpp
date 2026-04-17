#include "physics/GravityForce.h"

#include "physics/PhysicsBody.h"

#include <glm/geometric.hpp>

namespace agss {

GravityForce::GravityForce(double GScaled) : m_G(GScaled) {}

void GravityForce::Apply(std::vector<PhysicsBody*>& bodies, double /*dtDays*/) {
  const double eps2 = m_softeningEpsilon * m_softeningEpsilon;
  const double sign = m_repulsive ? -1.0 : 1.0;

  const size_t n = bodies.size();
  for (size_t i = 0; i < n; ++i) {
    auto* a = bodies[i];
    if (!a) continue;
    for (size_t j = i + 1; j < n; ++j) {
      auto* b = bodies[j];
      if (!b) continue;

      const glm::dvec3 r = b->position - a->position;
      const double dist2 = glm::dot(r, r) + eps2;
      const double invDist = 1.0 / std::sqrt(dist2);
      const glm::dvec3 dir = r * invDist;

      // Supports negative mass naturally because m1*m2 can be negative.
      const double forceMag = sign * m_G * (a->mass * b->mass) / dist2;
      const glm::dvec3 force = forceMag * dir;

      a->accumulatedForce += force;
      b->accumulatedForce -= force;
    }
  }
}

} // namespace agss

