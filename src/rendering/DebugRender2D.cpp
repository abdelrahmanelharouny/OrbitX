#include "rendering/DebugRender2D.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace agss {

static void SetupOrtho2D(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, static_cast<double>(w), static_cast<double>(h), 0.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

glm::dvec2 DebugRender2D::WorldXZToScreen(const glm::dvec3& p, int w, int h) const {
  const double cx = w * 0.5;
  const double cy = h * 0.5;
  const double x = (p.x - m_centerWorldXZ.x) * m_pixelsPerAu;
  const double z = (p.z - m_centerWorldXZ.y) * m_pixelsPerAu;
  return glm::dvec2(cx + x, cy + z);
}

glm::dvec2 DebugRender2D::ScreenToWorldXZ(double sx, double sy, int w, int h) const {
  const double cx = w * 0.5;
  const double cy = h * 0.5;
  const double x = (sx - cx) / m_pixelsPerAu + m_centerWorldXZ.x;
  const double z = (sy - cy) / m_pixelsPerAu + m_centerWorldXZ.y;
  return glm::dvec2(x, z);
}

void DebugRender2D::Render(int viewportW,
                           int viewportH,
                           const std::vector<glm::dvec3>& bodyPositions,
                           const std::vector<glm::dvec3>& bodyVelocities,
                           const std::vector<glm::dvec3>& bodyForces,
                           const std::vector<TrailRenderData>& trails) const {
  if (!m_enabled) return;

  SetupOrtho2D(viewportW, viewportH);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Trails
  if (m_drawTrails) {
    for (const auto& t : trails) {
      if (t.vertices.size() < 2) continue;
      glBegin(GL_LINE_STRIP);
      for (const auto& v : t.vertices) {
        glColor4f(1.0f, 1.0f, 1.0f, v.alpha);
        // Interpret TrailVertex.position as (x,y,z) in world; use x/z in screen.
        const auto s = WorldXZToScreen(glm::dvec3(v.position), viewportW, viewportH);
        glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
      }
      glEnd();
    }
  }

  // Bodies as points (and selected highlight)
  const size_t nBodies = bodyPositions.size();
  if (nBodies > 0) {
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < nBodies; ++i) {
      const auto s = WorldXZToScreen(bodyPositions[i], viewportW, viewportH);
      if (static_cast<int>(i) == m_selectedIndex) {
        glColor4f(1.0f, 1.0f, 0.2f, 1.0f);
      } else {
        glColor4f(0.9f, 0.9f, 0.9f, 0.9f);
      }
      glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
    }
    glEnd();

    if (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < nBodies) {
      const auto s = WorldXZToScreen(bodyPositions[static_cast<size_t>(m_selectedIndex)], viewportW, viewportH);
      glColor4f(1.0f, 1.0f, 0.2f, 0.9f);
      glBegin(GL_LINE_LOOP);
      const float r = 10.0f;
      for (int k = 0; k < 24; ++k) {
        const float a = static_cast<float>(k) * 6.2831853f / 24.0f;
        glVertex2f(static_cast<float>(s.x) + r * std::cos(a), static_cast<float>(s.y) + r * std::sin(a));
      }
      glEnd();
    }
  }

  // Vectors (velocity / force) as lines from body position.
  const size_t n = bodyPositions.size();
  for (size_t i = 0; i < n; ++i) {
    const auto s0 = WorldXZToScreen(bodyPositions[i], viewportW, viewportH);

    if (m_drawVelocity && i < bodyVelocities.size()) {
      const glm::dvec3 v = bodyVelocities[i];
      const glm::dvec3 tipWorld = bodyPositions[i] + v * 2.0; // arbitrary visual scale (2 days)
      const auto s1 = WorldXZToScreen(tipWorld, viewportW, viewportH);
      glColor4f(0.2f, 0.8f, 1.0f, 0.9f);
      glBegin(GL_LINES);
      glVertex2f(static_cast<float>(s0.x), static_cast<float>(s0.y));
      glVertex2f(static_cast<float>(s1.x), static_cast<float>(s1.y));
      glEnd();
    }

    if (m_drawForce && i < bodyForces.size()) {
      const glm::dvec3 f = bodyForces[i];
      const glm::dvec3 tipWorld = bodyPositions[i] + f * 500.0; // arbitrary visual scale
      const auto s1 = WorldXZToScreen(tipWorld, viewportW, viewportH);
      glColor4f(1.0f, 0.35f, 0.2f, 0.9f);
      glBegin(GL_LINES);
      glVertex2f(static_cast<float>(s0.x), static_cast<float>(s0.y));
      glVertex2f(static_cast<float>(s1.x), static_cast<float>(s1.y));
      glEnd();
    }
  }
}

} // namespace agss

