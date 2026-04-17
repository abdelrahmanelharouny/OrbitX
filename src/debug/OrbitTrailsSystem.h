#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace agss {

struct PhysicsBody;

struct TrailVertex {
  glm::vec3 position; // float for rendering
  float alpha;        // 0..1 fade
};

struct TrailRenderData {
  std::vector<TrailVertex> vertices;
};

class OrbitTrailsSystem {
public:
  void Configure(size_t maxPointsPerBody, bool enabled);
  bool IsEnabled() const { return m_enabled; }

  void Reset(size_t bodyCount);
  void PushSamples(const std::vector<PhysicsBody*>& bodies);

  // Builds per-body vertices with fading (oldest -> alphaMin, newest -> 1.0)
  TrailRenderData BuildRenderDataForBody(size_t bodyIndex, float alphaMin = 0.05f) const;

private:
  size_t m_maxPoints = 2048;
  bool m_enabled = true;
  std::vector<std::vector<glm::dvec3>> m_history;
};

} // namespace agss

