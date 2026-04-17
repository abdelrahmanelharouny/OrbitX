#pragma once

#include "entities/SceneNode.h"
#include "physics/PhysicsBody.h"

#include <glm/vec3.hpp>

namespace agss {

class CelestialBody final : public SceneNode {
public:
  explicit CelestialBody(std::string name);

  double radius_km = 1.0;

  glm::vec3 baseColor{1.0f};

  PhysicsBody& Physics() { return m_physics; }
  const PhysicsBody& Physics() const { return m_physics; }

  // Rendering interpolation helper (render system will use this later).
  glm::dvec3 GetRenderPosition(double alpha) const;
  void CommitPreviousPhysicsState();

private:
  PhysicsBody m_physics{};
  glm::dvec3 m_prevPosition{0.0};
};

} // namespace agss

