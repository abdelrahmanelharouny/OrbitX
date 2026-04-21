#include "rendering/AdvancedDebugRenderer.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace agss {

bool AdvancedDebugRenderer::Initialize() {
  // Initialize OpenGL state if needed
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return true;
}

void AdvancedDebugRenderer::Shutdown() {
  // Cleanup if needed
}

void AdvancedDebugRenderer::SetVisualizationSystem(AdvancedVisualizationSystem* visSystem) {
  m_visSystem = visSystem;
}

void AdvancedDebugRenderer::SetCamera(const CinematicCamera& camera) {
  m_camera = camera;
}

glm::dvec2 AdvancedDebugRenderer::WorldXZToScreen(const glm::dvec3& p, int w, int h) const {
  const double cx = w * 0.5;
  const double cy = h * 0.5;
  const double x = (p.x - m_centerWorldXZ.x) * m_pixelsPerAu;
  const double z = (p.z - m_centerWorldXZ.y) * m_pixelsPerAu;
  return glm::dvec2(cx + x, cy + z);
}

void AdvancedDebugRenderer::Render(int viewportW, int viewportH,
                                    const std::vector<glm::dvec3>& bodyPositions,
                                    const std::vector<glm::dvec3>& bodyVelocities,
                                    const std::vector<glm::dvec3>& bodyForces,
                                    const std::vector<TrailRenderData>& trails,
                                    const std::vector<glm::vec3>& bodyColors) {
  
  // Render order matters for proper blending:
  // 1. Starfield (background)
  // 2. Spacetime grid
  // 3. Orbit trails
  // 4. Gravity field
  // 5. Bodies
  // 6. Event highlights
  // 7. Scientific overlays
  
  if (m_renderStarfield && m_visSystem) {
    RenderStarfield(viewportW, viewportH);
  }
  
  if (m_renderSpacetimeGrid && m_visSystem) {
    RenderSpacetimeGrid(viewportW, viewportH);
  }
  
  if (m_renderEnhancedTrails && m_visSystem) {
    RenderEnhancedOrbitTrails(viewportW, viewportH);
  } else {
    // Fallback to basic trails
    if (m_visSystem && m_visSystem->GetConfig().orbitTrails.enabled) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      
      for (size_t i = 0; i < trails.size(); ++i) {
        const auto& t = trails[i];
        if (t.vertices.size() < 2) continue;
        
        glBegin(GL_LINE_STRIP);
        for (const auto& v : t.vertices) {
          glColor4f(1.0f, 1.0f, 1.0f, v.alpha);
          const auto s = WorldXZToScreen(glm::dvec3(v.position), viewportW, viewportH);
          glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
        }
        glEnd();
      }
    }
  }
  
  if (m_renderGravityField && m_visSystem) {
    RenderGravityField(viewportW, viewportH);
  }
  
  // Render bodies as points
  const size_t nBodies = bodyPositions.size();
  if (nBodies > 0) {
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < nBodies; ++i) {
      const auto s = WorldXZToScreen(bodyPositions[i], viewportW, viewportH);
      glm::vec3 color = (i < bodyColors.size()) ? bodyColors[i] : glm::vec3(0.9f);
      glColor4f(color.r, color.g, color.b, 1.0f);
      glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
    }
    glEnd();
  }
  
  if (m_renderEventHighlights && m_visSystem) {
    RenderEventHighlights(viewportW, viewportH, bodyPositions);
  }
  
  if (m_renderScientificOverlays && m_visSystem) {
    RenderScientificOverlays(viewportW, viewportH, bodyPositions);
  }
}

void AdvancedDebugRenderer::RenderStarfield(int viewportW, int viewportH) const {
  if (!m_visSystem) return;
  
  auto& starfield = m_visSystem->GetStarfield();
  const auto& stars = starfield.GetStars();
  
  glDisable(GL_DEPTH_TEST);
  
  glBegin(GL_POINTS);
  for (const auto& star : stars) {
    // Apply parallax offset based on camera position
    glm::dvec3 camPos = m_camera.GetPosition();
    float parallaxOffset = star.position.w * 0.001f;
    
    float screenX = viewportW * 0.5f + star.position.x * viewportW * 0.4f 
                    - static_cast<float>(camPos.x * parallaxOffset);
    float screenY = viewportH * 0.5f + star.position.y * viewportH * 0.4f 
                    + static_cast<float>(camPos.y * parallaxOffset);
    
    // Clamp to screen
    if (screenX < 0 || screenX > viewportW || screenY < 0 || screenY > viewportH) {
      continue;
    }
    
    glColor4f(star.color.r * star.brightness,
              star.color.g * star.brightness,
              star.color.b * star.brightness,
              star.brightness);
    glVertex2f(screenX, screenY);
  }
  glEnd();
}

void AdvancedDebugRenderer::RenderSpacetimeGrid(int viewportW, int viewportH) const {
  if (!m_visSystem) return;
  
  auto& grid = m_visSystem->GetSpacetimeGrid();
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  // Render horizontal lines
  for (const auto& line : grid.GetHorizontalLines()) {
    if (line.vertices.size() < 2) continue;
    
    glBegin(GL_LINE_STRIP);
    glColor4f(line.color.r, line.color.g, line.color.b, line.color.a);
    
    for (const auto& v : line.vertices) {
      const auto s = WorldXZToScreen(glm::dvec3(v), viewportW, viewportH);
      glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
    }
    glEnd();
  }
  
  // Render vertical lines
  for (const auto& line : grid.GetVerticalLines()) {
    if (line.vertices.size() < 2) continue;
    
    glBegin(GL_LINE_STRIP);
    glColor4f(line.color.r, line.color.g, line.color.b, line.color.a);
    
    for (const auto& v : line.vertices) {
      const auto s = WorldXZToScreen(glm::dvec3(v), viewportW, viewportH);
      glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
    }
    glEnd();
  }
}

void AdvancedDebugRenderer::RenderGravityField(int viewportW, int viewportH) const {
  if (!m_visSystem) return;
  
  auto& gravityField = m_visSystem->GetGravityField();
  const auto& config = m_visSystem->GetConfig().gravityField;
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  if (config.useVectorField) {
    // Render arrows
    for (const auto& arrow : gravityField.GetArrows()) {
      DrawArrow(arrow.position, arrow.direction, arrow.magnitude,
                arrow.color, viewportW, viewportH);
    }
  } else {
    // Render field lines
    for (const auto& line : gravityField.GetFieldLines()) {
      if (line.points.size() < 2) continue;
      
      glm::vec4 lineColor = glm::mix(
        glm::vec4(0.2f, 0.4f, 1.0f, 0.5f),
        glm::vec4(1.0f, 0.2f, 0.2f, 0.5f),
        std::min(line.intensity * 0.5f, 1.0f)
      );
      
      DrawFieldLine(line.points, lineColor, viewportW, viewportH);
    }
  }
}

void AdvancedDebugRenderer::RenderEventHighlights(int viewportW, int viewportH,
                                                   const std::vector<glm::dvec3>& positions) const {
  if (!m_visSystem) return;
  
  auto& eventSystem = m_visSystem->GetEventHighlights();
  const auto& highlights = eventSystem.GetActiveHighlights();
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  for (const auto& highlight : highlights) {
    if (highlight.bodyIndex < 0 || 
        static_cast<size_t>(highlight.bodyIndex) >= positions.size()) {
      continue;
    }
    
    const auto& pos = positions[static_cast<size_t>(highlight.bodyIndex)];
    auto screenPos = WorldXZToScreen(pos, viewportW, viewportH);
    
    // Draw glow ring
    float baseRadius = 15.0f * highlight.intensity;
    DrawGlowEffect(screenPos, baseRadius, highlight.color, viewportW, viewportH);
    
    // Draw pulsing outer ring
    float pulsePhase = static_cast<float>(glfwGetTime() * 5.0);
    float pulseRadius = baseRadius * (1.0f + 0.3f * std::sin(pulsePhase));
    
    glColor4f(highlight.color.r, highlight.color.g, highlight.color.b,
              highlight.color.a * 0.5f * highlight.intensity);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; ++i) {
      float angle = static_cast<float>(i) * 6.2831853f / 32.0f;
      float x = static_cast<float>(screenPos.x) + pulseRadius * std::cos(angle);
      float y = static_cast<float>(screenPos.y) + pulseRadius * std::sin(angle);
      glVertex2f(x, y);
    }
    glEnd();
  }
}

void AdvancedDebugRenderer::RenderScientificOverlays(int viewportW, int viewportH,
                                                      const std::vector<glm::dvec3>& positions) const {
  if (!m_visSystem) return;
  
  auto& sciVis = m_visSystem->GetScientificVis();
  const auto& config = m_visSystem->GetConfig().scientificVis;
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  // Render velocity vectors
  if (sciVis.IsOverlayActive(SciVisOverlay::VelocityVectors)) {
    const auto& velField = sciVis.GetVelocityField();
    for (const auto& data : velField) {
      DrawArrow(data.position, glm::normalize(data.vector), data.magnitude,
                data.color, viewportW, viewportH);
    }
  }
  
  // Render acceleration vectors
  if (sciVis.IsOverlayActive(SciVisOverlay::AccelerationVectors)) {
    const auto& accField = sciVis.GetAccelerationField();
    for (const auto& data : accField) {
      DrawArrow(data.position, glm::normalize(data.vector), data.magnitude,
                data.color, viewportW, viewportH);
    }
  }
  
  // Render energy indicators
  if (sciVis.IsOverlayActive(SciVisOverlay::EnergyVisualization)) {
    const auto& energyData = sciVis.GetEnergyData();
    
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < positions.size() && i < energyData.size(); ++i) {
      const auto& energy = energyData[i];
      const auto s = WorldXZToScreen(positions[i], viewportW, viewportH);
      glColor4f(energy.color.r, energy.color.g, energy.color.b, energy.color.a);
      glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
    }
    glEnd();
  }
}

void AdvancedDebugRenderer::RenderEnhancedOrbitTrails(int viewportW, int viewportH) const {
  if (!m_visSystem) return;
  
  // Enhanced trails are managed by the visualization system
  // This would integrate with the EnhancedOrbitTrails class
  // For now, we use the existing trail system as a placeholder
  (void)viewportW;
  (void)viewportH;
}

void AdvancedDebugRenderer::DrawArrow(const glm::vec3& position, const glm::vec3& direction,
                                       float magnitude, const glm::vec4& color,
                                       int viewportW, int viewportH) const {
  const float arrowScale = 2.0f;
  float length = std::min(magnitude * arrowScale, 1.5f);
  
  glm::vec3 start = position;
  glm::vec3 end = position + direction * length;
  
  auto s0 = WorldXZToScreen(glm::dvec3(start), viewportW, viewportH);
  auto s1 = WorldXZToScreen(glm::dvec3(end), viewportW, viewportH);
  
  glColor4f(color.r, color.g, color.b, color.a);
  
  // Draw line
  glBegin(GL_LINES);
  glVertex2f(static_cast<float>(s0.x), static_cast<float>(s0.y));
  glVertex2f(static_cast<float>(s1.x), static_cast<float>(s1.y));
  glEnd();
  
  // Draw arrowhead
  float headSize = 0.15f * arrowScale;
  glm::vec2 lineDir = glm::normalize(glm::vec2(s1.x - s0.x, s1.y - s0.y));
  glm::vec2 perp{-lineDir.y, lineDir.x};
  
  glm::vec2 h0 = glm::vec2(s1.x, s1.y);
  glm::vec2 h1 = glm::vec2(s1.x, s1.y) - lineDir * headSize * 10.0f + perp * headSize * 5.0f;
  glm::vec2 h2 = glm::vec2(s1.x, s1.y) - lineDir * headSize * 10.0f - perp * headSize * 5.0f;
  
  glColor4f(color.r, color.g, color.b, color.a * 0.8f);
  glBegin(GL_TRIANGLES);
  glVertex2f(h0.x, h0.y);
  glVertex2f(h1.x, h1.y);
  glVertex2f(h2.x, h2.y);
  glEnd();
}

void AdvancedDebugRenderer::DrawFieldLine(const std::vector<glm::vec3>& points,
                                           const glm::vec4& color,
                                           int viewportW, int viewportH) const {
  if (points.size() < 2) return;
  
  glBegin(GL_LINE_STRIP);
  glColor4f(color.r, color.g, color.b, color.a);
  
  for (const auto& p : points) {
    const auto s = WorldXZToScreen(glm::dvec3(p), viewportW, viewportH);
    glVertex2f(static_cast<float>(s.x), static_cast<float>(s.y));
  }
  glEnd();
}

void AdvancedDebugRenderer::DrawGlowEffect(const glm::dvec2& screenPos, float radius,
                                            const glm::vec4& color,
                                            int viewportW, int viewportH) const {
  (void)viewportW;
  (void)viewportH;
  
  // Draw multiple concentric rings with decreasing alpha
  int numRings = 5;
  for (int i = 0; i < numRings; ++i) {
    float ringRadius = radius * (1.0f - static_cast<float>(i) / numRings);
    float alpha = color.a * (1.0f - static_cast<float>(i) / numRings) * 0.5f;
    
    glColor4f(color.r, color.g, color.b, alpha);
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < 32; ++j) {
      float angle = static_cast<float>(j) * 6.2831853f / 32.0f;
      float x = static_cast<float>(screenPos.x) + ringRadius * std::cos(angle);
      float y = static_cast<float>(screenPos.y) + ringRadius * std::sin(angle);
      glVertex2f(x, y);
    }
    glEnd();
  }
}

} // namespace agss
