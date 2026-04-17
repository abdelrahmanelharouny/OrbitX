#include "simulation/CollisionSystem.h"

#include "physics/PhysicsBody.h"

#include <glm/geometric.hpp>
#include <algorithm>

namespace agss {

std::vector<CollisionPair> CollisionSystem::Detect(const std::vector<PhysicsBody*>& bodies) {
  std::vector<CollisionPair> out;

  const size_t n = bodies.size();
  for (size_t i = 0; i < n; ++i) {
    auto* a = bodies[i];
    if (!a || a->radiusAu <= 0.0) continue;
    for (size_t j = i + 1; j < n; ++j) {
      auto* b = bodies[j];
      if (!b || b->radiusAu <= 0.0) continue;

      const double rSum = a->radiusAu + b->radiusAu;
      const glm::dvec3 d = b->position - a->position;
      const double dist2 = glm::dot(d, d);
      if (dist2 <= (rSum * rSum)) {
        out.push_back(CollisionPair{i, j});
      }
    }
  }
  return out;
}

void CollisionSystem::ResolveMergeConserveMomentum(const std::vector<PhysicsBody*>& bodies,
                                                  const std::vector<CollisionPair>& collisions,
                                                  std::vector<bool>& keepMask) {
  keepMask.assign(bodies.size(), true);

  // Greedy merge: if a body already merged away, skip.
  for (const auto& c : collisions) {
    if (c.a >= bodies.size() || c.b >= bodies.size()) continue;
    if (!keepMask[c.a] || !keepMask[c.b]) continue;
    auto* a = bodies[c.a];
    auto* b = bodies[c.b];
    if (!a || !b) continue;

    const double mA = a->mass;
    const double mB = b->mass;
    const double m = mA + mB;
    if (m == 0.0) {
      // Degenerate; just delete b.
      keepMask[c.b] = false;
      continue;
    }

    // Conserve momentum: v = (mA vA + mB vB) / (mA + mB)
    const glm::dvec3 newVel = (mA * a->velocity + mB * b->velocity) / m;

    // Center of mass position
    const glm::dvec3 newPos = (mA * a->position + mB * b->position) / m;

    // Volume-additive radius approximation (spheres): r^3 = rA^3 + rB^3
    const double rA = std::max(0.0, a->radiusAu);
    const double rB = std::max(0.0, b->radiusAu);
    const double newR = std::cbrt(rA * rA * rA + rB * rB * rB);

    a->mass = m;
    a->position = newPos;
    a->velocity = newVel;
    a->radiusAu = newR;

    keepMask[c.b] = false;
  }
}

} // namespace agss

