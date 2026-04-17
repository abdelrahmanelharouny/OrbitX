#pragma once

#include "debug/OrbitTrailsSystem.h"

#include <glm/vec3.hpp>
#include <vector>

namespace agss {

// Minimal debug renderer (top-down XZ view) that draws:
// - orbit trails (line strips with fading alpha)
// - velocity vectors
// - force vectors
//
// This is intentionally isolated from physics; it consumes already-built debug data.
class DebugRender2D {
public:
  void SetEnabled(bool enabled) { m_enabled = enabled; }
  bool IsEnabled() const { return m_enabled; }

  void SetDrawTrails(bool v) { m_drawTrails = v; }
  void SetDrawVelocity(bool v) { m_drawVelocity = v; }
  void SetDrawForce(bool v) { m_drawForce = v; }

  void Render(int viewportW,
              int viewportH,
              const std::vector<glm::dvec3>& bodyPositions,
              const std::vector<glm::dvec3>& bodyVelocities,
              const std::vector<glm::dvec3>& bodyForces,
              const std::vector<TrailRenderData>& trails) const;

  // World-to-screen scale (AU to pixels).
  void SetScale(double pixelsPerAu) { m_pixelsPerAu = pixelsPerAu; }
  double GetScale() const { return m_pixelsPerAu; }

  // Camera-like controls (top-down)
  void SetCenterWorldXZ(const glm::dvec2& c) { m_centerWorldXZ = c; }
  const glm::dvec2& GetCenterWorldXZ() const { return m_centerWorldXZ; }

  // Selection highlight (index into body arrays; -1 disables)
  void SetSelectedIndex(int idx) { m_selectedIndex = idx; }

  // Coordinate conversion for picking
  glm::dvec2 ScreenToWorldXZ(double sx, double sy, int w, int h) const;
  glm::dvec2 WorldXZToScreen(const glm::dvec3& p, int w, int h) const;

private:
  bool m_enabled = true;
  bool m_drawTrails = true;
  bool m_drawVelocity = true;
  bool m_drawForce = false;
  double m_pixelsPerAu = 220.0;
  glm::dvec2 m_centerWorldXZ{0.0, 0.0};
  int m_selectedIndex = -1;
};

} // namespace agss

