#pragma once

#include "physics/PhysicsConfig.h"

#include <glm/vec3.hpp>
#include <vector>

namespace agss {

struct PhysicsBody;

struct EnergyReport {
  double kinetic = 0.0;
  double potential = 0.0;
  double total = 0.0;
};

class EnergyMonitor {
public:
  // Uses softened potential consistent with the force softening (r^2 + eps^2).
  static EnergyReport Compute(const std::vector<PhysicsBody*>& bodies, const PhysicsConfig& cfg);
};

} // namespace agss

