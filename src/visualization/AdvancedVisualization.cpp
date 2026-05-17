#include "visualization/AdvancedVisualization.h"

#include <glm/geometric.hpp>
#include <glm/gtc/random.hpp>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace agss {

// ============================================================================
// GravityFieldVisualizer Implementation
// ============================================================================

void GravityFieldVisualizer::Configure(const GravityFieldConfig& config) {
  m_config = config;
}

glm::dvec3 GravityFieldVisualizer::ComputeGravityField(
    const glm::dvec3& point,
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses,
    double G) const {
  
  glm::dvec3 totalField{0.0};
  const double softening = 0.01; // Prevent singularity
  
  for (size_t i = 0; i < positions.size(); ++i) {
    glm::dvec3 r = positions[i] - point;
    double distSq = glm::dot(r, r) + softening * softening;
    double dist = std::sqrt(distSq);
    
    // F = GM/r^2, direction toward mass
    double fieldMagnitude = G * masses[i] / distSq;
    totalField += glm::normalize(r) * fieldMagnitude;
  }
  
  return totalField;
}

void GravityFieldVisualizer::Update(
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses,
    double G,
    const glm::dvec3& viewCenter,
    float viewRadius) {
  
  if (!m_config.enabled) {
    m_arrows.clear();
    m_fieldLines.clear();
    return;
  }
  
  m_arrows.clear();
  m_fieldLines.clear();
  
  if (m_config.useVectorField) {
    // Generate arrow grid in XZ plane (top-down view)
    int arrowsPerAxis = static_cast<int>(viewRadius * 2.0f / m_config.arrowSpacing);
    arrowsPerAxis = std::min(arrowsPerAxis, 20); // Limit for performance
    
    float halfExtent = arrowsPerAxis * m_config.arrowSpacing * 0.5f;
    
    for (int x = -arrowsPerAxis / 2; x <= arrowsPerAxis / 2; ++x) {
      for (int z = -arrowsPerAxis / 2; z <= arrowsPerAxis / 2; ++z) {
        glm::dvec3 samplePoint{
          viewCenter.x + x * m_config.arrowSpacing,
          viewCenter.y,
          viewCenter.z + z * m_config.arrowSpacing
        };
        
        glm::dvec3 field = ComputeGravityField(samplePoint, positions, masses, G);
        float magnitude = static_cast<float>(glm::length(field));
        
        if (magnitude > 1e-10f) {
          ArrowData arrow;
          arrow.position = glm::vec3(samplePoint);
          arrow.direction = glm::normalize(glm::vec3(field));
          arrow.magnitude = magnitude;
          
          // Color by intensity (blue = weak, red = strong)
          float normalizedIntensity = std::min(magnitude * m_config.colorIntensityScale, 1.0f);
          arrow.color = glm::mix(
            glm::vec4(0.2f, 0.4f, 1.0f, 0.8f),
            glm::vec4(1.0f, 0.2f, 0.2f, 0.8f),
            normalizedIntensity
          );
          
          m_arrows.push_back(arrow);
        }
      }
    }
  } else {
    // Field lines mode - trace from seeds near massive objects
    for (size_t i = 0; i < positions.size(); ++i) {
      if (masses[i] < 0.01) continue; // Skip small bodies
      
      // Emit field lines in multiple directions
      int numLines = 8;
      for (int j = 0; j < numLines; ++j) {
        float angle = static_cast<float>(j) * 6.2831853f / numLines;
        glm::dvec3 dir{std::cos(angle), 0.0, std::sin(angle)};
        
        FieldLineData line;
        line.intensity = static_cast<float>(masses[i]);
        
        glm::dvec3 currentPos = positions[i] + dir * 0.1; // Start slightly offset
        
        for (int step = 0; step < 50; ++step) {
          line.points.push_back(glm::vec3(currentPos));
          
          glm::dvec3 field = ComputeGravityField(currentPos, positions, masses, G);
          float fieldMag = glm::length(field);
          
          if (fieldMag < 1e-8f) break;
          
          // Step along field line
          double stepSize = 0.05 / (1.0 + fieldMag * 10.0);
          currentPos += glm::normalize(field) * stepSize;
          
          // Check bounds
          if (glm::distance(currentPos, viewCenter) > viewRadius * 1.5) break;
        }
        
        if (line.points.size() > 2) {
          m_fieldLines.push_back(line);
        }
      }
    }
  }
}

// ============================================================================
// EnhancedOrbitTrails Implementation
// ============================================================================

void EnhancedOrbitTrails::Configure(int bodyIndex, const OrbitTrailConfig& config) {
  while (static_cast<int>(m_trails.size()) <= bodyIndex) {
    m_trails.emplace_back();
  }
  m_trails[static_cast<size_t>(bodyIndex)].config = config;
}

void EnhancedOrbitTrails::Reset(int bodyIndex) {
  if (bodyIndex >= 0 && static_cast<size_t>(bodyIndex) < m_trails.size()) {
    m_trails[static_cast<size_t>(bodyIndex)] = TrailData{};
    m_trails[static_cast<size_t>(bodyIndex)].config = m_trails[static_cast<size_t>(bodyIndex)].config;
  }
}

void EnhancedOrbitTrails::PushSample(int bodyIndex, const glm::dvec3& position, const glm::vec3& color) {
  while (static_cast<int>(m_trails.size()) <= bodyIndex) {
    m_trails.emplace_back();
  }
  
  auto& trail = m_trails[static_cast<size_t>(bodyIndex)];
  if (!trail.config.enabled) return;
  
  trail.positions.push_back(glm::vec3(position));
  trail.colors.push_back(color);
  trail.ages.push_back(0.0f);
  
  // Enforce max points
  while (static_cast<int>(trail.positions.size()) > trail.config.maxPoints) {
    trail.positions.erase(trail.positions.begin());
    trail.colors.erase(trail.colors.begin());
    trail.ages.erase(trail.ages.begin());
  }
  
  // Age all points
  for (auto& age : trail.ages) {
    age += trail.config.fadeRate;
  }
}

glm::vec3 EnhancedOrbitTrails::CatmullRomInterpolate(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3,
    float t) const {
  
  float t2 = t * t;
  float t3 = t2 * t;
  
  return 0.5f * (
    (2.0f * p1) +
    (-p0 + p2) * t +
    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
  );
}

std::vector<glm::vec3> EnhancedOrbitTrails::GenerateSpline(
    const std::vector<glm::vec3>& controlPoints,
    int segmentsPerSpan) {
  
  std::vector<glm::vec3> result;
  if (controlPoints.size() < 2) return controlPoints;
  
  for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
    // Get surrounding points for Catmull-Rom
    size_t p0Idx = (i > 0) ? (i - 1) : 0;
    size_t p1Idx = i;
    size_t p2Idx = i + 1;
    size_t p3Idx = (i + 2 < controlPoints.size()) ? (i + 2) : (controlPoints.size() - 1);
    
    const auto& p0 = controlPoints[p0Idx];
    const auto& p1 = controlPoints[p1Idx];
    const auto& p2 = controlPoints[p2Idx];
    const auto& p3 = controlPoints[p3Idx];
    
    for (int s = 0; s < segmentsPerSpan; ++s) {
      float t = static_cast<float>(s) / segmentsPerSpan;
      result.push_back(CatmullRomInterpolate(p0, p1, p2, p3, t));
    }
  }
  
  // Add last point
  result.push_back(controlPoints.back());
  
  return result;
}

// Static wrapper for GenerateSpline to be called without object
std::vector<glm::vec3> EnhancedOrbitTrails::GenerateSplineStatic(
    const std::vector<glm::vec3>& controlPoints,
    int segmentsPerSpan) {
  // Create a temporary instance to call the non-static method
  EnhancedOrbitTrails temp;
  return temp.GenerateSpline(controlPoints, segmentsPerSpan);
}

std::vector<EnhancedOrbitTrails::SmoothTrailVertex> EnhancedOrbitTrails::GetRenderData(int bodyIndex) const {
  std::vector<SmoothTrailVertex> result;
  
  if (bodyIndex < 0 || static_cast<size_t>(bodyIndex) >= m_trails.size()) {
    return result;
  }
  
  const auto& trail = m_trails[static_cast<size_t>(bodyIndex)];
  if (trail.positions.size() < 2) return result;
  
  std::vector<glm::vec3> smoothPoints;
  if (trail.config.smoothCurves && trail.positions.size() >= 4) {
    smoothPoints = GenerateSpline(trail.positions, 2);
  } else {
    smoothPoints = std::vector<glm::vec3>(trail.positions.begin(), trail.positions.end());
  }
  
  // Interpolate colors and ages
  for (size_t i = 0; i < smoothPoints.size(); ++i) {
    float t = static_cast<float>(i) / std::max(1u, static_cast<unsigned>(smoothPoints.size() - 1));
    size_t idx = static_cast<size_t>(t * (trail.positions.size() - 1));
    
    SmoothTrailVertex vertex;
    vertex.position = smoothPoints[i];
    
    // Color interpolation
    if (trail.config.useCustomColor) {
      vertex.color = glm::vec4(trail.config.customColor, 1.0f);
    } else if (idx < trail.colors.size()) {
      vertex.color = glm::vec4(trail.colors[idx], 1.0f);
    } else {
      vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Alpha based on age
    float age = trail.ages.empty() ? 0.0f : trail.ages[idx];
    float alpha = 1.0f - std::min(age, 1.0f);
    alpha = std::max(alpha, trail.config.minAlpha);
    vertex.color.a = alpha;
    vertex.age = age;
    
    result.push_back(vertex);
  }
  
  return result;
}

// ============================================================================
// SpacetimeDistortionGrid Implementation
// ============================================================================

void SpacetimeDistortionGrid::Configure(const SpacetimeGridConfig& config) {
  m_config = config;
}

float SpacetimeDistortionGrid::ComputeDistortionAtPoint(
    const glm::vec3& point,
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses) const {
  
  float totalDistortion = 0.0f;
  
  for (size_t i = 0; i < positions.size(); ++i) {
    float dist = static_cast<float>(glm::distance(glm::dvec3(point), positions[i]));
    float massFactor = static_cast<float>(masses[i]);
    
    // Distortion falls off with distance
    float distortion = massFactor / std::pow(dist + 0.1f, m_config.falloffExponent);
    totalDistortion += distortion;
  }
  
  return totalDistortion * m_config.distortionStrength;
}

glm::vec3 SpacetimeDistortionGrid::ApplyDistortion(
    const glm::vec3& originalPoint,
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses) const {
  
  glm::vec3 distorted = originalPoint;
  
  for (size_t i = 0; i < positions.size(); ++i) {
    glm::vec3 toMass = glm::vec3(positions[i]) - originalPoint;
    float distSq = glm::dot(toMass, toMass);
    float dist = std::sqrt(distSq);
    
    if (dist > 0.01f) {
      // Pull grid toward mass
      float pullStrength = static_cast<float>(masses[i]) * m_config.distortionStrength;
      float pull = pullStrength / (distSq + 0.1f);
      distorted += glm::normalize(toMass) * pull;
    }
  }
  
  return distorted;
}

void SpacetimeDistortionGrid::Update(
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses) {
  
  if (!m_config.enabled) {
    m_horizontalLines.clear();
    m_verticalLines.clear();
    return;
  }
  
  m_horizontalLines.clear();
  m_verticalLines.clear();
  
  float step = m_config.gridExtent / m_config.gridSize;
  
  // Horizontal lines (varying Z)
  if (m_config.renderHorizontal) {
    for (int z = 0; z <= m_config.gridSize; ++z) {
      GridLine line;
      line.color = m_config.gridColor;
      
      for (int x = 0; x <= m_config.gridSize; ++x) {
        glm::vec3 originalPoint{
          -m_config.gridExtent * 0.5f + x * step,
          0.0f,
          -m_config.gridExtent * 0.5f + z * step
        };
        
        glm::vec3 distorted = ApplyDistortion(originalPoint, positions, masses);
        line.vertices.push_back(distorted);
      }
      
      if (line.vertices.size() > 1) {
        m_horizontalLines.push_back(line);
      }
    }
  }
  
  // Vertical lines (varying X)
  if (m_config.renderVertical) {
    for (int x = 0; x <= m_config.gridSize; ++x) {
      GridLine line;
      line.color = m_config.gridColor;
      
      for (int z = 0; z <= m_config.gridSize; ++z) {
        glm::vec3 originalPoint{
          -m_config.gridExtent * 0.5f + x * step,
          0.0f,
          -m_config.gridExtent * 0.5f + z * step
        };
        
        glm::vec3 distorted = ApplyDistortion(originalPoint, positions, masses);
        line.vertices.push_back(distorted);
      }
      
      if (line.vertices.size() > 1) {
        m_verticalLines.push_back(line);
      }
    }
  }
}

// ============================================================================
// PostProcessPipeline Implementation
// ============================================================================

const std::vector<PostProcessEffectType> PostProcessPipeline::s_heavyEffects = {
  PostProcessEffectType::DepthOfField,
  PostProcessEffectType::MotionBlur,
  PostProcessEffectType::Bloom
};

void PostProcessPipeline::EnableEffect(PostProcessEffectType type, const PostProcessEffectParams& params) {
  if (!IsEffectEnabled(type)) {
    m_activeEffects.push_back(type);
  }
  m_effectParams[type] = params;
}

void PostProcessPipeline::DisableEffect(PostProcessEffectType type) {
  auto it = std::find(m_activeEffects.begin(), m_activeEffects.end(), type);
  if (it != m_activeEffects.end()) {
    m_activeEffects.erase(it);
  }
}

bool PostProcessPipeline::IsEffectEnabled(PostProcessEffectType type) const {
  return std::find(m_activeEffects.begin(), m_activeEffects.end(), type) != m_activeEffects.end();
}

const PostProcessEffectParams& PostProcessPipeline::GetEffectParams(PostProcessEffectType type) const {
  static PostProcessEffectParams defaultParams;
  auto it = m_effectParams.find(type);
  return (it != m_effectParams.end()) ? it->second : defaultParams;
}

void PostProcessPipeline::SetEffectParams(PostProcessEffectType type, const PostProcessEffectParams& params) {
  m_effectParams[type] = params;
}

bool PostProcessPipeline::HasHeavyEffects() const {
  for (PostProcessEffectType effect : m_activeEffects) {
    if (std::find(s_heavyEffects.begin(), s_heavyEffects.end(), effect) != s_heavyEffects.end()) {
      return true;
    }
  }
  return false;
}

// ============================================================================
// ProceduralStarfield Implementation
// ============================================================================

void ProceduralStarfield::Configure(const StarfieldConfig& config) {
  m_config = config;
  Regenerate();
}

void ProceduralStarfield::Regenerate() {
  m_stars.clear();
  GenerateStars();
}

glm::vec3 ProceduralStarfield::StarColorFromTemperature(float temperature) const {
  // Simplified blackbody color approximation
  // Temperature range: 1000K (red) to 40000K (blue)
  float t = temperature / 1000.0f;
  
  glm::vec3 color;
  if (t < 66.0f) {
    color.r = 1.0f;
    color.g = std::clamp(0.390068f * std::log(t) - 0.631781f, 0.0f, 1.0f);
    color.b = (t >= 19.0f) ? std::clamp(0.543206f * std::log(t - 10.0f) - 1.196254f, 0.0f, 1.0f) : 0.0f;
  } else {
    color.r = std::clamp(1.292936f * std::pow(t - 60.0f, -0.133204f), 0.0f, 1.0f);
    color.g = std::clamp(1.129311f * std::pow(t - 60.0f, -0.091089f), 0.0f, 1.0f);
    color.b = 1.0f;
  }
  
  return color;
}

void ProceduralStarfield::GenerateStars() {
  std::srand(m_config.seed);
  
  for (int i = 0; i < m_config.starCount; ++i) {
    Star star;
    
    // Random direction on sphere
    float theta = static_cast<float>(std::rand()) / RAND_MAX * 6.2831853f;
    float phi = std::acos(2.0f * static_cast<float>(std::rand()) / RAND_MAX - 1.0f);
    
    star.position.x = std::sin(phi) * std::cos(theta);
    star.position.y = std::sin(phi) * std::sin(theta);
    star.position.z = std::cos(phi);
    
    // Layer assignment for parallax
    star.layer = std::rand() % m_config.numLayers;
    star.position.w = 100.0f + star.layer * 50.0f; // Depth
    
    // Random brightness and size
    star.brightness = m_config.minBrightness + 
                      static_cast<float>(std::rand()) / RAND_MAX * (m_config.maxBrightness - m_config.minBrightness);
    star.size = 0.5f + star.brightness * 2.0f;
    
    // Color from temperature (random stellar temperatures)
    float temperature = 2000.0f + static_cast<float>(std::rand()) / RAND_MAX * 30000.0f;
    star.color = StarColorFromTemperature(temperature);
    
    m_stars.push_back(star);
  }
}

void ProceduralStarfield::UpdateParallax(const glm::dvec3& cameraPosition) {
  // Parallax is applied during rendering based on camera position
  // This method can be used to precompute offsets if needed
  (void)cameraPosition;
}

// ============================================================================
// CinematicCamera Implementation
// ============================================================================

void CinematicCamera::Configure(const CinematicCameraConfig& config) {
  m_config = config;
}

void CinematicCamera::SetTarget(const glm::dvec3& position, const glm::dvec3& lookAt) {
  m_targetPosition = position;
  m_targetLookAt = lookAt;
}

void CinematicCamera::SetFollowTarget(int bodyIndex) {
  m_followBodyIndex = bodyIndex;
  m_config.mode = CameraMode::Follow;
}

void CinematicCamera::Update(double deltaTime, const std::vector<glm::dvec3>& bodyPositions) {
  // Smooth transition to target
  float lerpFactor = static_cast<float>(1.0 - std::exp(-m_config.transitionSpeed * deltaTime));
  
  m_position = glm::mix(m_position, m_targetPosition, lerpFactor);
  m_lookAt = glm::mix(m_lookAt, m_targetLookAt, lerpFactor);
  
  // Mode-specific updates
  switch (m_config.mode) {
    case CameraMode::Follow:
      UpdateFollowMode(bodyPositions);
      break;
    case CameraMode::AutoFrame:
      UpdateAutoFrameMode();
      break;
    default:
      break;
  }
  
  // Apply shake
  if (m_shakeRemainingTime > 0.0f) {
    ApplyShake(deltaTime);
  }
  
  // Auto-rotation
  if (m_config.autoRotate) {
    float angle = static_cast<float>(m_config.autoRotateSpeed * deltaTime);
    glm::dvec3 offset = m_position - m_lookAt;
    glm::dvec3 rotatedOffset{
      offset.x * std::cos(angle) - offset.z * std::sin(angle),
      offset.y,
      offset.x * std::sin(angle) + offset.z * std::cos(angle)
    };
    m_position = m_lookAt + rotatedOffset;
  }
}

void CinematicCamera::UpdateFollowMode(const std::vector<glm::dvec3>& bodyPositions) {
  if (m_followBodyIndex < 0 || static_cast<size_t>(m_followBodyIndex) >= bodyPositions.size()) {
    return;
  }
  
  glm::dvec3 targetPos = bodyPositions[static_cast<size_t>(m_followBodyIndex)];
  
  // Maintain offset from target
  glm::dvec3 offset = m_position - m_lookAt;
  m_targetPosition = targetPos + offset;
  m_targetLookAt = targetPos;
}

void CinematicCamera::UpdateAutoFrameMode() {
  // Auto-framing handled by AutoFrame method
}

void CinematicCamera::AutoFrame(const std::vector<glm::dvec3>& positions,
                                 const std::vector<int>& indicesToFrame) {
  if (indicesToFrame.empty()) return;
  
  // Calculate bounding box of targets
  glm::dvec3 minPos = positions[static_cast<size_t>(indicesToFrame[0])];
  glm::dvec3 maxPos = minPos;
  
  for (int idx : indicesToFrame) {
    if (idx >= 0 && static_cast<size_t>(idx) < positions.size()) {
      minPos = glm::min(minPos, positions[static_cast<size_t>(idx)]);
      maxPos = glm::max(maxPos, positions[static_cast<size_t>(idx)]);
    }
  }
  
  glm::dvec3 center = (minPos + maxPos) * 0.5;
  glm::dvec3 extent = maxPos - minPos;
  float maxExtent = std::max({extent.x, extent.y, extent.z});
  
  // Position camera to frame all objects
  float fovRad = m_config.fov * 3.14159265f / 180.0f;
  float distance = maxExtent / (2.0f * std::tan(fovRad * 0.5f));
  
  m_targetLookAt = center;
  m_targetPosition = center + glm::dvec3(0.0, maxExtent * 0.5, distance * 1.5);
}

void CinematicCamera::TriggerShake(float intensity, float duration) {
  m_shakeIntensity = intensity;
  m_shakeRemainingTime = duration;
  m_shakePhase = 0.0f;
}

void CinematicCamera::ApplyShake(double deltaTime) {
  m_shakeRemainingTime -= static_cast<float>(deltaTime);
  m_shakePhase += static_cast<float>(m_config.shakeFrequency * deltaTime * 6.2831853);
  
  if (m_shakeRemainingTime <= 0.0f) {
    m_shakeIntensity = 0.0f;
    m_shakeRemainingTime = 0.0f;
    return;
  }
  
  // Perlin-like noise approximation using sine waves
  float shakeX = std::sin(m_shakePhase) * std::sin(m_shakePhase * 2.3f);
  float shakeY = std::cos(m_shakePhase * 1.7f) * std::sin(m_shakePhase * 0.9f);
  float shakeZ = std::sin(m_shakePhase * 3.1f) * std::cos(m_shakePhase * 1.4f);
  
  m_position.x += shakeX * m_shakeIntensity * 0.1;
  m_position.y += shakeY * m_shakeIntensity * 0.1;
  m_position.z += shakeZ * m_shakeIntensity * 0.1;
}

// ============================================================================
// EventHighlightSystem Implementation
// ============================================================================

void EventHighlightSystem::Configure(const EventDetectionConfig& config) {
  m_config = config;
}

void EventHighlightSystem::DetectEvents(
    const std::vector<glm::dvec3>& positions,
    const std::vector<glm::dvec3>& velocities,
    const std::vector<double>& masses,
    double currentTime) {
  
  CheckCloseEncounters(positions, masses, currentTime);
  CheckHighSpeed(velocities, currentTime);
}

void EventHighlightSystem::CheckCloseEncounters(
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses,
    double currentTime) {
  
  for (size_t i = 0; i < positions.size(); ++i) {
    for (size_t j = i + 1; j < positions.size(); ++j) {
      float dist = static_cast<float>(glm::distance(positions[i], positions[j]));
      
      if (dist < m_config.closeEncounterDistance) {
        // Highlight both bodies
        TriggerEvent(EventType::CloseEncounter, static_cast<int>(i), 2.0f,
                     glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        TriggerEvent(EventType::CloseEncounter, static_cast<int>(j), 2.0f,
                     glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
      }
      
      if (dist < m_config.collisionWarningDistance) {
        TriggerEvent(EventType::Collision, static_cast<int>(i), 3.0f,
                     glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        TriggerEvent(EventType::Collision, static_cast<int>(j), 3.0f,
                     glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
      }
    }
  }
}

void EventHighlightSystem::CheckHighSpeed(
    const std::vector<glm::dvec3>& velocities,
    double currentTime) {
  
  for (size_t i = 0; i < velocities.size(); ++i) {
    float speed = static_cast<float>(glm::length(velocities[i]));
    
    if (speed > m_config.highSpeedThreshold) {
      TriggerEvent(EventType::HighSpeedMotion, static_cast<int>(i), 1.5f,
                   glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
    }
  }
}

void EventHighlightSystem::TriggerEvent(EventType type, int bodyIndex, float duration,
                                         const glm::vec4& color) {
  // Check if event already exists for this body
  for (auto& event : m_pendingEvents) {
    if (event.bodyIndex == bodyIndex && event.type == type && event.active) {
      event.duration = std::max(event.duration, duration);
      return;
    }
  }
  
  EventHighlight event;
  event.type = type;
  event.bodyIndex = bodyIndex;
  event.startTime = static_cast<float>(m_currentTime);
  event.duration = duration;
  event.intensity = 1.0f;
  event.highlightColor = color;
  event.active = true;
  
  m_pendingEvents.push_back(event);
}

void EventHighlightSystem::Update(double currentTime) {
  m_currentTime = currentTime;
  m_activeHighlights.clear();
  
  // Process pending events
  for (auto& event : m_pendingEvents) {
    if (!event.active) continue;
    
    float elapsed = static_cast<float>(currentTime) - event.startTime;
    float remaining = event.duration - elapsed;
    
    if (remaining <= 0.0f) {
      event.active = false;
      continue;
    }
    
    ActiveHighlight highlight;
    highlight.bodyIndex = event.bodyIndex;
    highlight.type = event.type;
    highlight.remainingTime = remaining;
    highlight.intensity = remaining / event.duration;
    highlight.color = event.highlightColor;
    highlight.color.a *= highlight.intensity;
    
    m_activeHighlights.push_back(highlight);
  }
  
  // Clean up inactive events periodically
  m_pendingEvents.erase(
    std::remove_if(m_pendingEvents.begin(), m_pendingEvents.end(),
                   [](const EventHighlight& e) { return !e.active; }),
    m_pendingEvents.end()
  );
}

// ============================================================================
// ScientificVisualizer Implementation
// ============================================================================

void ScientificVisualizer::Configure(const SciVisConfig& config) {
  m_config = config;
}

void ScientificVisualizer::ToggleOverlay(SciVisOverlay overlay) {
  auto it = std::find(m_config.activeOverlays.begin(), m_config.activeOverlays.end(), overlay);
  if (it != m_config.activeOverlays.end()) {
    m_config.activeOverlays.erase(it);
  } else {
    m_config.activeOverlays.push_back(overlay);
  }
}

bool ScientificVisualizer::IsOverlayActive(SciVisOverlay overlay) const {
  return std::find(m_config.activeOverlays.begin(), m_config.activeOverlays.end(), overlay)
         != m_config.activeOverlays.end();
}

void ScientificVisualizer::Update(
    const std::vector<glm::dvec3>& positions,
    const std::vector<glm::dvec3>& velocities,
    const std::vector<glm::dvec3>& accelerations,
    const std::vector<double>& masses,
    const std::vector<std::string>& names) {
  
  if (!m_config.enabled) return;
  
  m_velocityField.clear();
  m_accelerationField.clear();
  m_energyData.clear();
  
  // Velocity vectors
  if (IsOverlayActive(SciVisOverlay::VelocityVectors)) {
    for (size_t i = 0; i < positions.size(); ++i) {
      VectorFieldData data;
      data.position = glm::vec3(positions[i]);
      data.vector = glm::vec3(velocities[i]) * m_config.vectorScale;
      data.magnitude = static_cast<float>(glm::length(velocities[i]));
      
      // Color by speed
      float normalizedSpeed = std::min(data.magnitude * 10.0f, 1.0f);
      data.color = glm::mix(
        glm::vec4(0.2f, 0.8f, 1.0f, 1.0f),
        glm::vec4(1.0f, 0.8f, 0.2f, 1.0f),
        normalizedSpeed
      );
      
      m_velocityField.push_back(data);
    }
  }
  
  // Acceleration vectors
  if (IsOverlayActive(SciVisOverlay::AccelerationVectors)) {
    for (size_t i = 0; i < positions.size(); ++i) {
      VectorFieldData data;
      data.position = glm::vec3(positions[i]);
      data.vector = glm::vec3(accelerations[i]) * m_config.vectorScale;
      data.magnitude = static_cast<float>(glm::length(accelerations[i]));
      
      data.color = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
      m_accelerationField.push_back(data);
    }
  }
  
  // Energy visualization
  if (IsOverlayActive(SciVisOverlay::EnergyVisualization)) {
    for (size_t i = 0; i < positions.size(); ++i) {
      EnergyData energy;
      double v2 = glm::dot(velocities[i], velocities[i]);
      energy.kinetic = 0.5 * masses[i] * v2;
      
      // Simplified potential (would need pairwise calculation for accuracy)
      energy.potential = 0.0;
      for (size_t j = 0; j < positions.size(); ++j) {
        if (i == j) continue;
        double r = glm::distance(positions[i], positions[j]);
        if (r > 0.001) {
          energy.potential -= masses[i] * masses[j] / r;
        }
      }
      energy.potential *= 0.5; // Avoid double counting
      
      energy.total = energy.kinetic + energy.potential;
      
      // Color: green = bound (negative total), red = unbound
      if (energy.total < 0.0) {
        energy.color = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);
      } else {
        energy.color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);
      }
      
      m_energyData.push_back(energy);
    }
  }
}

std::vector<glm::vec3> ScientificVisualizer::CalculateLagrangePoints(
    const glm::dvec3& pos1, double mass1,
    const glm::dvec3& pos2, double mass2) {
  
  std::vector<glm::vec3> points;
  
  glm::dvec3 r = pos2 - pos1;
  double d = glm::length(r);
  glm::dvec3 n = glm::normalize(r);
  
  double mu = mass2 / (mass1 + mass2);
  double r13 = d * std::cbrt(mu / 3.0);  // Approximate L1
  double r23 = d * std::cbrt(mu / 3.0);  // Approximate L2
  
  // L1 (between bodies)
  points.push_back(glm::vec3(pos1 + n * (d - r13)));
  
  // L2 (beyond smaller body)
  points.push_back(glm::vec3(pos2 + n * r23));
  
  // L3 (opposite side of larger body)
  points.push_back(glm::vec3(pos1 - n * d * (1.0 + 5.0/12.0 * mu)));
  
  // L4 and L5 (equilateral triangles)
  glm::dvec3 perp = glm::dvec3(-n.z, 0.0, n.x);
  if (glm::length(perp) < 0.001) {
    perp = glm::dvec3(0.0, 1.0, 0.0);
  }
  perp = glm::normalize(perp);
  
  glm::dvec3 midpoint = (pos1 + pos2) * 0.5;
  double height = d * std::sqrt(3.0) / 2.0;
  
  points.push_back(glm::vec3(midpoint + perp * height));  // L4
  points.push_back(glm::vec3(midpoint - perp * height));  // L5
  
  return points;
}

double ScientificVisualizer::CalculateHillSphere(double primaryMass, double secondaryMass, double semiMajorAxis) {
  return semiMajorAxis * std::cbrt(secondaryMass / (3.0 * primaryMass));
}

// ============================================================================
// AdvancedVisualizationSystem Implementation
// ============================================================================

bool AdvancedVisualizationSystem::Initialize() {
  m_starfield.Regenerate();
  return true;
}

void AdvancedVisualizationSystem::Shutdown() {
  // Cleanup if needed
}

void AdvancedVisualizationSystem::Configure(const VisualizationConfig& config) {
  m_config = config;
  
  m_gravityField.Configure(config.gravityField);
  m_spacetimeGrid.Configure(config.spacetimeGrid);
  m_camera.Configure(config.camera);
  m_eventHighlights.Configure(config.eventDetection);
  m_scientificVis.Configure(config.scientificVis);
  m_starfield.Configure(config.starfield);
}

void AdvancedVisualizationSystem::Update(
    double deltaTime,
    const std::vector<glm::dvec3>& positions,
    const std::vector<glm::dvec3>& velocities,
    const std::vector<glm::dvec3>& accelerations,
    const std::vector<double>& masses,
    const std::vector<std::string>& bodyNames,
    double G,
    double currentTime) {
  
  m_currentTime = currentTime;
  
  // Update gravity field visualization
  if (ShouldRenderGravityField()) {
    glm::dvec3 viewCenter = m_camera.GetLookAt();
    m_gravityField.Update(positions, masses, G, viewCenter, 20.0f);
  }
  
  // Update spacetime grid
  if (ShouldRenderSpacetimeGrid()) {
    m_spacetimeGrid.Update(positions, masses);
  }
  
  // Update scientific visualization
  if (m_config.scientificVis.enabled) {
    m_scientificVis.Update(positions, velocities, accelerations, masses, bodyNames);
  }
  
  // Update event highlights
  m_eventHighlights.DetectEvents(positions, velocities, masses, currentTime);
  m_eventHighlights.Update(currentTime);
  
  // Update camera
  m_camera.Update(deltaTime, positions);
  
  // Update starfield parallax
  if (ShouldRenderStarfield()) {
    m_starfield.UpdateParallax(m_camera.GetPosition());
  }
}

bool AdvancedVisualizationSystem::ShouldRenderGravityField() const {
  return m_config.gravityField.enabled || 
         m_config.scientificVis.enabled;
}

bool AdvancedVisualizationSystem::ShouldRenderSpacetimeGrid() const {
  return m_config.spacetimeGrid.enabled && !m_config.performanceMode;
}

bool AdvancedVisualizationSystem::ShouldRenderStarfield() const {
  return m_config.starfield.enabled;
}

} // namespace agss
