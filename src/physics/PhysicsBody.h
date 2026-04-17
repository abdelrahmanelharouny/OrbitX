#pragma once

#include <glm/vec3.hpp>

namespace agss {

// Pure physics data container (no rendering concerns).
struct PhysicsBody {
  double mass = 1.0;          // solar masses (can be negative if you want "negative mass" experiments)
  glm::dvec3 position{0.0};   // AU
  glm::dvec3 velocity{0.0};   // AU / day
  glm::dvec3 acceleration{0.0}; // AU / day^2
  double radiusAu = 0.0;      // collision radius in AU (0 disables collision)

  // Debug / diagnostics
  glm::dvec3 accumulatedForce{0.0}; // (solar_mass * AU / day^2) in our scaled units
  glm::dvec3 lastForce{0.0};
};

} // namespace agss

