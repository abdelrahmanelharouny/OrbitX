#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>

namespace agss {

// ============================================================================
// Phase 7: Advanced Visualization & Cinematic Effects
// ============================================================================
// Architecture:
// - Visualization systems are separate from physics engine
// - All effects are toggleable for performance
// - Modular post-processing pipeline
// - Scientific visualization overlays

// ----------------------------------------------------------------------------
// 1. Gravitational Field Visualization
// ----------------------------------------------------------------------------

struct GravityFieldConfig {
  bool enabled = false;
  bool useVectorField = true;  // true = arrows, false = field lines
  float arrowSpacing = 2.0f;   // AU between sample points
  float maxArrowLength = 1.0f; // visual scaling
  float colorIntensityScale = 1.0f;
  int gridResolution = 32;     // for field lines
  float fieldLineThreshold = 0.1f;
};

class GravityFieldVisualizer {
public:
  void Configure(const GravityFieldConfig& config);
  bool IsEnabled() const { return m_config.enabled; }
  
  // Generate field visualization data from gravity sources
  void Update(const std::vector<glm::dvec3>& positions, 
              const std::vector<double>& masses,
              double G,
              const glm::dvec3& viewCenter,
              float viewRadius);
  
  struct ArrowData {
    glm::vec3 position;
    glm::vec3 direction;
    float magnitude;
    glm::vec4 color;
  };
  
  struct FieldLineData {
    std::vector<glm::vec3> points;
    float intensity;
  };
  
  const std::vector<ArrowData>& GetArrows() const { return m_arrows; }
  const std::vector<FieldLineData>& GetFieldLines() const { return m_fieldLines; }
  
private:
  GravityFieldConfig m_config;
  std::vector<ArrowData> m_arrows;
  std::vector<FieldLineData> m_fieldLines;
  
  glm::dvec3 ComputeGravityField(const glm::dvec3& point,
                                  const std::vector<glm::dvec3>& positions,
                                  const std::vector<double>& masses,
                                  double G) const;
};

// ----------------------------------------------------------------------------
// 2. Enhanced Orbit Path System
// ----------------------------------------------------------------------------

struct OrbitTrailConfig {
  bool enabled = true;
  int maxPoints = 4096;
  float fadeRate = 0.001f;      // alpha decay per point
  float minAlpha = 0.05f;
  bool smoothCurves = true;     // use Catmull-Rom splines
  glm::vec3 customColor{1.0f};  // if set, overrides body color
  bool useCustomColor = false;
};

class EnhancedOrbitTrails {
public:
  void Configure(int bodyIndex, const OrbitTrailConfig& config);
  void Reset(int bodyIndex);
  
  void PushSample(int bodyIndex, const glm::dvec3& position, const glm::vec3& color);
  
  struct SmoothTrailVertex {
    glm::vec3 position;
    glm::vec4 color;
    float age;  // 0 = newest, 1 = oldest
  };
  
  // Get interpolated smooth curve points
  std::vector<SmoothTrailVertex> GetRenderData(int bodyIndex) const;
  
  // Generate Catmull-Rom spline points
  static std::vector<glm::vec3> GenerateSpline(const std::vector<glm::vec3>& controlPoints,
                                                int segmentsPerSpan = 4);
  
private:
  struct TrailData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<float> ages;
    OrbitTrailConfig config;
  };
  
  std::vector<TrailData> m_trails;

  glm::vec3 CatmullRomInterpolate(const glm::vec3& p0, const glm::vec3& p1,
                                   const glm::vec3& p2, const glm::vec3& p3,
                                   float t) const;
};

// ----------------------------------------------------------------------------
// 3. Space-Time Distortion Grid
// ----------------------------------------------------------------------------

struct SpacetimeGridConfig {
  bool enabled = false;
  int gridSize = 40;          // number of cells per axis
  float gridExtent = 50.0f;   // AU
  float distortionStrength = 1.0f;
  float falloffExponent = 2.0f;
  glm::vec4 gridColor{0.0f, 0.5f, 0.3f, 0.4f};
  bool renderHorizontal = true;
  bool renderVertical = true;
};

class SpacetimeDistortionGrid {
public:
  void Configure(const SpacetimeGridConfig& config);
  bool IsEnabled() const { return m_config.enabled; }
  
  void Update(const std::vector<glm::dvec3>& positions,
              const std::vector<double>& masses);
  
  struct GridLine {
    std::vector<glm::vec3> vertices;
    glm::vec4 color;
  };
  
  const std::vector<GridLine>& GetHorizontalLines() const { return m_horizontalLines; }
  const std::vector<GridLine>& GetVerticalLines() const { return m_verticalLines; }
  
private:
  SpacetimeGridConfig m_config;
  std::vector<GridLine> m_horizontalLines;
  std::vector<GridLine> m_verticalLines;
  
  glm::vec3 ApplyDistortion(const glm::vec3& originalPoint,
                            const std::vector<glm::dvec3>& positions,
                            const std::vector<double>& masses) const;
  
  float ComputeDistortionAtPoint(const glm::vec3& point,
                                  const std::vector<glm::dvec3>& positions,
                                  const std::vector<double>& masses) const;
};

// ----------------------------------------------------------------------------
// 4. Post-Processing Effects Framework
// ----------------------------------------------------------------------------

enum class PostProcessEffectType {
  Bloom,
  HDRToneMapping,
  DepthOfField,
  MotionBlur,
  ChromaticAberration,
  Vignette,
  ColorGrading
};

struct PostProcessEffectParams {
  bool enabled = false;
  float intensity = 1.0f;
  
  // Bloom-specific
  float bloomThreshold = 0.8f;
  float bloomRadius = 4.0f;
  int bloomIterations = 4;
  
  // Tone mapping
  float exposure = 1.0f;
  float gamma = 2.2f;
  
  // Depth of field
  float focalDistance = 10.0f;
  float aperture = 0.1f;
  
  // Motion blur
  float shutterAngle = 180.0f;
};

class PostProcessPipeline {
public:
  void EnableEffect(PostProcessEffectType type, const PostProcessEffectParams& params);
  void DisableEffect(PostProcessEffectType type);
  bool IsEffectEnabled(PostProcessEffectType type) const;
  
  const PostProcessEffectParams& GetEffectParams(PostProcessEffectType type) const;
  void SetEffectParams(PostProcessEffectType type, const PostProcessEffectParams& params);
  
  // Get all active effects for rendering
  const std::vector<PostProcessEffectType>& GetActiveEffects() const { return m_activeEffects; }
  
  // Performance optimization: check if any heavy effects are enabled
  bool HasHeavyEffects() const;
  
private:
  std::vector<PostProcessEffectType> m_activeEffects;
  std::map<PostProcessEffectType, PostProcessEffectParams> m_effectParams;
  
  static const std::vector<PostProcessEffectType> s_heavyEffects;
};

// ----------------------------------------------------------------------------
// 5. Procedural Starfield System
// ----------------------------------------------------------------------------

struct StarfieldConfig {
  bool enabled = true;
  int starCount = 2000;
  float minBrightness = 0.1f;
  float maxBrightness = 1.0f;
  float parallaxStrength = 0.5f;
  int numLayers = 3;  // for parallax depth
  bool showNebula = false;
  glm::vec3 nebulaColor{0.2f, 0.1f, 0.4f};
  float nebulaDensity = 0.3f;
  unsigned int seed = 12345;
};

class ProceduralStarfield {
public:
  void Configure(const StarfieldConfig& config);
  void Regenerate();
  
  struct Star {
    glm::vec4 position;  // xyz = direction (normalized), w = depth
    float brightness;
    float size;
    glm::vec3 color;
    int layer;  // for parallax
  };
  
  const std::vector<Star>& GetStars() const { return m_stars; }
  
  // Render with parallax based on camera position
  void UpdateParallax(const glm::dvec3& cameraPosition);
  
private:
  StarfieldConfig m_config;
  std::vector<Star> m_stars;
  
  void GenerateStars();
  glm::vec3 StarColorFromTemperature(float temperature) const;
};

// ----------------------------------------------------------------------------
// 6. Cinematic Camera System
// ----------------------------------------------------------------------------

enum class CameraMode {
  Free,
  Follow,
  AutoFrame,
  CinematicTour,
  TopDown
};

struct CinematicCameraConfig {
  CameraMode mode = CameraMode::Free;
  float transitionSpeed = 2.0f;
  float shakeIntensity = 0.0f;
  float shakeFrequency = 5.0f;
  bool autoRotate = false;
  float autoRotateSpeed = 0.1f;
  float fov = 60.0f;
  float nearPlane = 0.01f;
  float farPlane = 1000.0f;
};

class CinematicCamera {
public:
  void Configure(const CinematicCameraConfig& config);
  
  void SetTarget(const glm::dvec3& position, const glm::dvec3& lookAt);
  void SetFollowTarget(int bodyIndex);
  
  void Update(double deltaTime, const std::vector<glm::dvec3>& bodyPositions);
  
  // Get current camera state
  glm::dvec3 GetPosition() const { return m_position; }
  glm::dvec3 GetLookAt() const { return m_lookAt; }
  glm::dvec3 GetUp() const { return m_up; }
  float GetFOV() const { return m_config.fov; }
  
  // Auto-framing: compute optimal view for selected bodies
  void AutoFrame(const std::vector<glm::dvec3>& positions,
                 const std::vector<int>& indicesToFrame);
  
  // Add subtle shake effect
  void TriggerShake(float intensity, float duration);
  
private:
  CinematicCameraConfig m_config;
  glm::dvec3 m_position{0.0, 5.0, 10.0};
  glm::dvec3 m_lookAt{0.0, 0.0, 0.0};
  glm::dvec3 m_up{0.0, 1.0, 0.0};
  glm::dvec3 m_targetPosition;
  glm::dvec3 m_targetLookAt;
  int m_followBodyIndex = -1;
  
  float m_shakeRemainingTime = 0.0f;
  float m_shakeIntensity = 0.0f;
  float m_shakePhase = 0.0f;
  
  void UpdateFollowMode(const std::vector<glm::dvec3>& bodyPositions);
  void UpdateAutoFrameMode();
  void ApplyShake(double deltaTime);
};

// ----------------------------------------------------------------------------
// 7. Event Highlight System
// ----------------------------------------------------------------------------

enum class EventType {
  CloseEncounter,
  Collision,
  HighSpeedMotion,
  EscapeTrajectory,
  OrbitalResonance,
  Custom
};

struct EventHighlight {
  EventType type;
  int bodyIndex;
  float startTime;
  float duration;
  float intensity;
  glm::vec4 highlightColor{1.0f, 1.0f, 0.0f, 1.0f};
  bool active = true;
};

struct EventDetectionConfig {
  float closeEncounterDistance = 0.5f;  // AU
  float highSpeedThreshold = 0.1f;       // AU/day
  float collisionWarningDistance = 0.01f;
};

class EventHighlightSystem {
public:
  void Configure(const EventDetectionConfig& config);
  
  // Check for events and create highlights
  void DetectEvents(const std::vector<glm::dvec3>& positions,
                    const std::vector<glm::dvec3>& velocities,
                    const std::vector<double>& masses,
                    double currentTime);
  
  // Manually trigger an event highlight
  void TriggerEvent(EventType type, int bodyIndex, float duration = 3.0f,
                    const glm::vec4& color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  
  void Update(double currentTime);
  
  struct ActiveHighlight {
    int bodyIndex;
    EventType type;
    float remainingTime;
    float intensity;
    glm::vec4 color;
  };
  
  const std::vector<ActiveHighlight>& GetActiveHighlights() const { return m_activeHighlights; }
  
private:
  EventDetectionConfig m_config;
  std::vector<EventHighlight> m_pendingEvents;
  std::vector<ActiveHighlight> m_activeHighlights;
  double m_currentTime = 0.0;  // Track simulation time for event timestamps
  
  void CheckCloseEncounters(const std::vector<glm::dvec3>& positions,
                            const std::vector<double>& masses,
                            double currentTime);
  void CheckHighSpeed(const std::vector<glm::dvec3>& velocities, double currentTime);
};

// ----------------------------------------------------------------------------
// 8. Scientific Visualization Mode
// ----------------------------------------------------------------------------

enum class SciVisOverlay {
  GravitationalField,
  VelocityVectors,
  AccelerationVectors,
  EnergyVisualization,
  AngularMomentum,
  HillSphere,
  LagrangePoints,
  None
};

struct SciVisConfig {
  bool enabled = false;
  std::vector<SciVisOverlay> activeOverlays;
  bool colorCodeByMass = true;
  bool colorCodeByEnergy = false;
  bool showLabels = true;
  float vectorScale = 1.0f;
  float labelScale = 1.0f;
};

class ScientificVisualizer {
public:
  void Configure(const SciVisConfig& config);
  void ToggleOverlay(SciVisOverlay overlay);
  bool IsOverlayActive(SciVisOverlay overlay) const;
  
  void Update(const std::vector<glm::dvec3>& positions,
              const std::vector<glm::dvec3>& velocities,
              const std::vector<glm::dvec3>& accelerations,
              const std::vector<double>& masses,
              const std::vector<std::string>& names);
  
  struct VectorFieldData {
    glm::vec3 position;
    glm::vec3 vector;
    glm::vec4 color;
    float magnitude;
  };
  
  struct EnergyData {
    double kinetic;
    double potential;
    double total;
    glm::vec4 color;  // green = stable, red = unstable
  };
  
  const std::vector<VectorFieldData>& GetVelocityField() const { return m_velocityField; }
  const std::vector<VectorFieldData>& GetAccelerationField() const { return m_accelerationField; }
  const std::vector<EnergyData>& GetEnergyData() const { return m_energyData; }
  
  // Calculate Lagrange points for a two-body system
  static std::vector<glm::vec3> CalculateLagrangePoints(const glm::dvec3& pos1, double mass1,
                                                         const glm::dvec3& pos2, double mass2);
  
  // Calculate Hill sphere radius
  static double CalculateHillSphere(double primaryMass, double secondaryMass, double semiMajorAxis);
  
private:
  SciVisConfig m_config;
  std::vector<VectorFieldData> m_velocityField;
  std::vector<VectorFieldData> m_accelerationField;
  std::vector<EnergyData> m_energyData;
  std::vector<glm::vec3> m_lagrangePoints;
  std::vector<double> m_hillSphereRadii;
};

// ----------------------------------------------------------------------------
// Master Visualization Controller
// ----------------------------------------------------------------------------

struct VisualizationConfig {
  // Global toggles
  bool enableAllEffects = false;
  bool performanceMode = false;  // disables heavy effects
  
  // Individual systems
  GravityFieldConfig gravityField;
  OrbitTrailConfig orbitTrails;
  SpacetimeGridConfig spacetimeGrid;
  PostProcessPipeline postProcess;
  StarfieldConfig starfield;
  CinematicCameraConfig camera;
  EventDetectionConfig eventDetection;
  SciVisConfig scientificVis;
};

class AdvancedVisualizationSystem {
public:
  bool Initialize();
  void Shutdown();
  
  void Configure(const VisualizationConfig& config);
  
  // Main update call
  void Update(double deltaTime,
              const std::vector<glm::dvec3>& positions,
              const std::vector<glm::dvec3>& velocities,
              const std::vector<glm::dvec3>& accelerations,
              const std::vector<double>& masses,
              const std::vector<std::string>& bodyNames,
              double G,
              double currentTime);
  
  // Accessors for rendering
  GravityFieldVisualizer& GetGravityField() { return m_gravityField; }
  EnhancedOrbitTrails& GetOrbitTrails() { return m_orbitTrails; }
  SpacetimeDistortionGrid& GetSpacetimeGrid() { return m_spacetimeGrid; }
  PostProcessPipeline& GetPostProcess() { return m_postProcess; }
  ProceduralStarfield& GetStarfield() { return m_starfield; }
  CinematicCamera& GetCamera() { return m_camera; }
  EventHighlightSystem& GetEventHighlights() { return m_eventHighlights; }
  ScientificVisualizer& GetScientificVis() { return m_scientificVis; }
  
  const VisualizationConfig& GetConfig() const { return m_config; }
  
  // Performance helpers
  bool ShouldRenderGravityField() const;
  bool ShouldRenderSpacetimeGrid() const;
  bool ShouldRenderStarfield() const;
  
private:
  VisualizationConfig m_config;
  
  GravityFieldVisualizer m_gravityField;
  EnhancedOrbitTrails m_orbitTrails;
  SpacetimeDistortionGrid m_spacetimeGrid;
  PostProcessPipeline m_postProcess;
  ProceduralStarfield m_starfield;
  CinematicCamera m_camera;
  EventHighlightSystem m_eventHighlights;
  ScientificVisualizer m_scientificVis;
  
  double m_currentTime = 0.0;
};

} // namespace agss
