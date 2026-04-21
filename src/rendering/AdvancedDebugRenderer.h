#pragma once

#include "visualization/AdvancedVisualization.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace agss {

// ============================================================================
// Advanced Debug Renderer (OpenGL 2.1 compatible)
// Extends DebugRender2D with Phase 7 visualization features
// ============================================================================

class AdvancedDebugRenderer {
public:
  bool Initialize();
  void Shutdown();
  
  // Configuration
  void SetVisualizationSystem(AdvancedVisualizationSystem* visSystem);
  
  // Render all enabled visualization layers
  void Render(int viewportW, int viewportH,
              const std::vector<glm::dvec3>& bodyPositions,
              const std::vector<glm::dvec3>& bodyVelocities,
              const std::vector<glm::dvec3>& bodyForces,
              const std::vector<TrailRenderData>& trails,
              const std::vector<glm::vec3>& bodyColors);
  
  // Individual layer rendering (for selective control)
  void RenderGravityField(int viewportW, int viewportH) const;
  void RenderSpacetimeGrid(int viewportW, int viewportH) const;
  void RenderStarfield(int viewportW, int viewportH) const;
  void RenderEventHighlights(int viewportW, int viewportH,
                             const std::vector<glm::dvec3>& positions) const;
  void RenderScientificOverlays(int viewportW, int viewportH,
                                const std::vector<glm::dvec3>& positions) const;
  void RenderEnhancedOrbitTrails(int viewportW, int viewportH) const;
  
  // Camera integration
  void SetCamera(const CinematicCamera& camera);
  
  // World-to-screen conversion
  glm::dvec2 WorldXZToScreen(const glm::dvec3& p, int w, int h) const;
  
  // View controls
  void SetScale(double pixelsPerAu) { m_pixelsPerAu = pixelsPerAu; }
  double GetScale() const { return m_pixelsPerAu; }
  void SetCenterWorldXZ(const glm::dvec2& c) { m_centerWorldXZ = c; }
  const glm::dvec2& GetCenterWorldXZ() const { return m_centerWorldXZ; }
  
  // Toggle individual visualization layers
  void SetRenderGravityField(bool v) { m_renderGravityField = v; }
  void SetRenderSpacetimeGrid(bool v) { m_renderSpacetimeGrid = v; }
  void SetRenderStarfield(bool v) { m_renderStarfield = v; }
  void SetRenderEventHighlights(bool v) { m_renderEventHighlights = v; }
  void SetRenderScientificOverlays(bool v) { m_renderScientificOverlays = v; }
  void SetRenderEnhancedTrails(bool v) { m_renderEnhancedTrails = v; }
  
  bool IsRenderGravityField() const { return m_renderGravityField; }
  bool IsRenderSpacetimeGrid() const { return m_renderSpacetimeGrid; }
  bool IsRenderStarfield() const { return m_renderStarfield; }
  bool IsRenderEventHighlights() const { return m_renderEventHighlights; }
  bool IsRenderScientificOverlays() const { return m_renderScientificOverlays; }
  bool IsRenderEnhancedTrails() const { return m_renderEnhancedTrails; }
  
private:
  AdvancedVisualizationSystem* m_visSystem = nullptr;
  
  // View state
  double m_pixelsPerAu = 220.0;
  glm::dvec2 m_centerWorldXZ{0.0, 0.0};
  CinematicCamera m_camera;
  
  // Layer toggles
  bool m_renderGravityField = false;
  bool m_renderSpacetimeGrid = false;
  bool m_renderStarfield = true;
  bool m_renderEventHighlights = true;
  bool m_renderScientificOverlays = false;
  bool m_renderEnhancedTrails = true;
  
  // Helper rendering methods
  void DrawArrow(const glm::vec3& position, const glm::vec3& direction,
                 float magnitude, const glm::vec4& color,
                 int viewportW, int viewportH) const;
  void DrawFieldLine(const std::vector<glm::vec3>& points,
                     const glm::vec4& color,
                     int viewportW, int viewportH) const;
  void DrawGlowEffect(const glm::dvec2& screenPos, float radius,
                      const glm::vec4& color,
                      int viewportW, int viewportH) const;
};

} // namespace agss
