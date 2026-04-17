#include "entities/CelestialBody.h"

#include <glm/common.hpp>

namespace agss {

CelestialBody::CelestialBody(std::string name) : SceneNode(std::move(name)) {}

glm::dvec3 CelestialBody::GetRenderPosition(double alpha) const {
  return glm::mix(m_prevPosition, m_physics.position, alpha);
}

void CelestialBody::CommitPreviousPhysicsState() {
  m_prevPosition = m_physics.position;
}

} // namespace agss

