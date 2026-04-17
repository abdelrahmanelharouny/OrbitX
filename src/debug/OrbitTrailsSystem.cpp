#include "debug/OrbitTrailsSystem.h"

#include "physics/PhysicsBody.h"

#include <algorithm>

namespace agss {

void OrbitTrailsSystem::Configure(size_t maxPointsPerBody, bool enabled) {
  m_maxPoints = (maxPointsPerBody == 0) ? 1 : maxPointsPerBody;
  m_enabled = enabled;
}

void OrbitTrailsSystem::Reset(size_t bodyCount) {
  m_history.clear();
  m_history.resize(bodyCount);
}

void OrbitTrailsSystem::PushSamples(const std::vector<PhysicsBody*>& bodies) {
  if (!m_enabled) return;
  if (m_history.size() != bodies.size()) {
    Reset(bodies.size());
  }

  for (size_t i = 0; i < bodies.size(); ++i) {
    auto* b = bodies[i];
    if (!b) continue;

    auto& h = m_history[i];
    h.push_back(b->position);
    if (h.size() > m_maxPoints) {
      h.erase(h.begin(), h.begin() + (h.size() - m_maxPoints));
    }
  }
}

TrailRenderData OrbitTrailsSystem::BuildRenderDataForBody(size_t bodyIndex, float alphaMin) const {
  TrailRenderData out{};
  if (bodyIndex >= m_history.size()) return out;

  const auto& h = m_history[bodyIndex];
  if (h.empty()) return out;

  out.vertices.reserve(h.size());

  const float a0 = std::clamp(alphaMin, 0.0f, 1.0f);
  const size_t n = h.size();
  for (size_t i = 0; i < n; ++i) {
    const float t = (n <= 1) ? 1.0f : static_cast<float>(i) / static_cast<float>(n - 1);
    const float alpha = a0 + (1.0f - a0) * t;
    out.vertices.push_back(TrailVertex{glm::vec3(h[i]), alpha});
  }
  return out;
}

} // namespace agss

