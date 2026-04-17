#pragma once

#include <glm/vec3.hpp>
#include <vector>

namespace agss {

struct PhysicsBody;

enum class CollisionResponseMode {
  MergeConserveMomentum
};

struct CollisionConfig {
  bool enabled = true;
  CollisionResponseMode response = CollisionResponseMode::MergeConserveMomentum;
};

struct CollisionPair {
  size_t a = 0;
  size_t b = 0;
};

class CollisionSystem {
public:
  static std::vector<CollisionPair> Detect(const std::vector<PhysicsBody*>& bodies);

  // Applies response and removes merged bodies by returning a "keep mask"
  // (SimulationManager owns actual entity deletion; this keeps physics decoupled).
  static void ResolveMergeConserveMomentum(const std::vector<PhysicsBody*>& bodies,
                                          const std::vector<CollisionPair>& collisions,
                                          std::vector<bool>& keepMask);
};

} // namespace agss

