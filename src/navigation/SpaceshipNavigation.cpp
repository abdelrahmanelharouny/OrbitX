#include "navigation/SpaceshipNavigation.h"

#include <algorithm>
#include <cmath>

namespace agss {

// ============================================================================
// Constants and Utilities
// ============================================================================

namespace {
  constexpr double EPSILON = 1e-9;
  constexpr double ORBIT_DEFAULT_RADIUS_MULTIPLIER = 1.5; // Orbit at 1.5x body radius
  
  // Gravitational constant for orbit calculations (scaled units)
  constexpr double G_ORBITAL = 0.00029591220828559104;
}

static glm::dvec3 NormalizeSafe(const glm::dvec3& v) {
  double len = glm::length(v);
  if (len < EPSILON) return glm::dvec3(0.0, 0.0, 1.0);
  return v / len;
}

static double Clamp(double value, double minVal, double maxVal) {
  return std::max(minVal, std::min(maxVal, value));
}

// ============================================================================
// SpaceshipNavigation Implementation
// ============================================================================

SpaceshipNavigation::SpaceshipNavigation() 
  : m_config{}, m_state{}, m_target{} {
  // Initialize with default orientation
  m_state.forward = glm::dvec3(0.0, 0.0, 1.0);
  m_state.up = glm::dvec3(0.0, 1.0, 0.0);
  m_right = glm::dvec3(1.0, 0.0, 0.0);
}

SpaceshipNavigation::~SpaceshipNavigation() = default;

void SpaceshipNavigation::Initialize(const SpaceshipConfig& config) {
  m_config = config;
  m_state = NavigationState{};
  m_state.forward = glm::dvec3(0.0, 0.0, 1.0);
  m_state.up = glm::dvec3(0.0, 1.0, 0.0);
  m_right = glm::dvec3(1.0, 0.0, 0.0);
  
  m_throttle = 0.0f;
  m_strafe = 0.0f;
  m_vertical = 0.0f;
  m_yaw = 0.0f;
  m_pitch = 0.0f;
  m_roll = 0.0f;
  
  m_autoPilotEngaged = false;
  m_warpStateInternal = WarpState::Inactive;
  m_landingModeEngaged = false;
}

void SpaceshipNavigation::SetBodyProvider(const IBodyProvider* provider) {
  m_bodyProvider = provider;
}

// ============================================================================
// Mode Management
// ============================================================================

void SpaceshipNavigation::SetMode(NavigationMode mode) {
  NavigationMode oldMode = m_state.mode;
  m_state.mode = mode;
  
  // Handle mode transitions
  switch (mode) {
    case NavigationMode::FreeFlight:
      // Clear auto-pilot when switching to manual
      DisengageAutoPilot();
      break;
      
    case NavigationMode::OrbitMode:
      // Auto-engage auto-pilot for orbit
      if (HasTarget()) {
        EngageAutoPilot();
      }
      break;
      
    case NavigationMode::FocusMode:
      // Stop all movement in focus mode
      m_state.velocity = glm::dvec3(0.0);
      DisengageAutoPilot();
      DisengageWarp();
      break;
      
    case NavigationMode::AutoPilot:
      if (!HasTarget()) {
        // Can't engage auto-pilot without target
        m_state.mode = oldMode;
      }
      break;
  }
}

// ============================================================================
// Target Selection
// ============================================================================

void SpaceshipNavigation::SetTarget(int bodyIndex) {
  if (!m_bodyProvider || bodyIndex < 0 || 
      static_cast<size_t>(bodyIndex) >= m_bodyProvider->GetBodyCount()) {
    ClearTarget();
    return;
  }
  
  m_target.bodyIndex = bodyIndex;
  m_target.name = m_bodyProvider->GetBodyName(static_cast<size_t>(bodyIndex));
  m_target.position = m_bodyProvider->GetBodyPosition(static_cast<size_t>(bodyIndex));
  m_target.velocity = m_bodyProvider->GetBodyVelocity(static_cast<size_t>(bodyIndex));
  m_target.radius = m_bodyProvider->GetBodyRadius(static_cast<size_t>(bodyIndex));
  
  m_state.hasTarget = true;
  m_state.targetName = m_target.name;
  
  // Update distance
  m_state.distanceToTarget = CalculateDistanceToTarget();
}

void SpaceshipNavigation::ClearTarget() {
  m_target = NavigationTarget{};
  m_state.hasTarget = false;
  m_state.targetName.clear();
  m_state.distanceToTarget = 0.0;
  DisengageAutoPilot();
}

// ============================================================================
// Movement Control (Free Flight Mode)
// ============================================================================

void SpaceshipNavigation::SetThrottle(float throttle) {
  m_throttle = Clamp(throttle, -1.0f, 1.0f);
}

void SpaceshipNavigation::SetStrafe(float strafe) {
  m_strafe = Clamp(strafe, -1.0f, 1.0f);
}

void SpaceshipNavigation::SetVertical(float vertical) {
  m_vertical = Clamp(vertical, -1.0f, 1.0f);
}

void SpaceshipNavigation::SetYaw(float yaw) {
  m_yaw = Clamp(yaw, -1.0f, 1.0f);
}

void SpaceshipNavigation::SetPitch(float pitch) {
  m_pitch = Clamp(pitch, -1.0f, 1.0f);
}

void SpaceshipNavigation::SetRoll(float roll) {
  m_roll = Clamp(roll, -1.0f, 1.0f);
}

// ============================================================================
// Auto-Pilot Control
// ============================================================================

void SpaceshipNavigation::EngageAutoPilot() {
  if (!HasTarget()) return;
  m_autoPilotEngaged = true;
  m_state.mode = NavigationMode::AutoPilot;
}

void SpaceshipNavigation::DisengageAutoPilot() {
  m_autoPilotEngaged = false;
  m_autoPilotApproachSpeed = 0.0;
  if (m_state.mode == NavigationMode::AutoPilot) {
    m_state.mode = NavigationMode::FreeFlight;
  }
}

bool SpaceshipNavigation::IsAutoPilotEngaged() const {
  return m_autoPilotEngaged;
}

// ============================================================================
// Warp Control
// ============================================================================

void SpaceshipNavigation::EngageWarp() {
  if (!HasTarget()) return;
  
  double dist = CalculateDistanceToTarget();
  if (dist < m_config.warpMinDistance) {
    // Too close to warp
    return;
  }
  
  m_warpStateInternal = WarpState::Accelerating;
  m_state.isWarping = true;
  m_state.warpState = WarpState::Accelerating;
}

void SpaceshipNavigation::DisengageWarp() {
  m_warpStateInternal = WarpState::Decelerating;
  m_state.warpState = WarpState::Decelerating;
}

// ============================================================================
// Landing Control
// ============================================================================

void SpaceshipNavigation::EngageLandingMode() {
  m_landingModeEngaged = true;
  m_state.isLanding = true;
}

void SpaceshipNavigation::DisengageLandingMode() {
  m_landingModeEngaged = false;
  m_state.isLanding = false;
}

// ============================================================================
// Main Update Loop
// ============================================================================

NavigationState SpaceshipNavigation::Update(double deltaTime) {
  // Ensure positive delta time
  if (deltaTime <= 0.0) {
    return m_state;
  }
  
  // Reset collision state each frame
  m_state.collisionDetected = false;
  m_state.collisionObjectName.clear();
  
  // Update target position if we have one
  if (HasTarget() && m_bodyProvider) {
    m_target.position = m_bodyProvider->GetBodyPosition(static_cast<size_t>(m_target.bodyIndex));
    m_target.velocity = m_bodyProvider->GetBodyVelocity(static_cast<size_t>(m_target.bodyIndex));
    m_target.radius = m_bodyProvider->GetBodyRadius(static_cast<size_t>(m_target.bodyIndex));
    m_state.distanceToTarget = CalculateDistanceToTarget();
  }
  
  // Update proximity system (affects all modes)
  UpdateProximitySystem(deltaTime);
  
  // Update warp system (affects speed calculations)
  UpdateWarpSystem(deltaTime);
  
  // Execute mode-specific update
  switch (m_state.mode) {
    case NavigationMode::FreeFlight:
      UpdateFreeFlight(deltaTime);
      break;
    case NavigationMode::OrbitMode:
      UpdateOrbitMode(deltaTime);
      break;
    case NavigationMode::FocusMode:
      UpdateFocusMode(deltaTime);
      break;
    case NavigationMode::AutoPilot:
      UpdateAutoPilot(deltaTime);
      break;
  }
  
  // Handle collisions (navigation layer only)
  HandleCollisions(deltaTime);
  
  // Apply damping and clamp velocity
  ApplyDamping(deltaTime);
  ClampVelocity();
  
  // Update position from velocity
  m_state.position += m_state.velocity * deltaTime;
  
  // Update derived state
  UpdateStateFromPhysics();
  
  return m_state;
}

// ============================================================================
// Mode-Specific Updates
// ============================================================================

void SpaceshipNavigation::UpdateFreeFlight(double deltaTime) {
  // Build acceleration vector from input in local space
  glm::dvec3 localAccel(0.0);
  
  // Forward/backward thrust
  if (std::abs(m_throttle) > EPSILON) {
    localAccel.z += m_throttle * m_config.acceleration * m_config.inputSensitivity;
  }
  
  // Strafe (left/right)
  if (std::abs(m_strafe) > EPSILON) {
    localAccel.x -= m_strafe * m_config.acceleration * m_config.inputSensitivity * 0.5;
  }
  
  // Vertical (up/down)
  if (std::abs(m_vertical) > EPSILON) {
    localAccel.y += m_vertical * m_config.acceleration * m_config.inputSensitivity * 0.5;
  }
  
  // Transform local acceleration to world space
  // Using forward, up, and right vectors as basis
  glm::dvec3 worldAccel = 
    m_right * localAccel.x + 
    m_state.up * localAccel.y + 
    m_state.forward * localAccel.z;
  
  // Apply rotation input
  if (std::abs(m_yaw) > EPSILON || std::abs(m_pitch) > EPSILON || std::abs(m_roll) > EPSILON) {
    double rotAmount = m_config.rotationSpeed * deltaTime * m_config.inputSensitivity;
    
    // Yaw (rotate around up axis)
    if (std::abs(m_yaw) > EPSILON) {
      double yawAngle = m_yaw * rotAmount;
      glm::dmat3 yawMat(
        cos(yawAngle), 0.0, sin(yawAngle),
        0.0, 1.0, 0.0,
        -sin(yawAngle), 0.0, cos(yawAngle)
      );
      m_state.forward = NormalizeSafe(yawMat * m_state.forward);
      m_right = NormalizeSafe(glm::cross(m_state.forward, m_state.up));
    }
    
    // Pitch (rotate around right axis)
    if (std::abs(m_pitch) > EPSILON) {
      double pitchAngle = m_pitch * rotAmount;
      glm::dvec3 newForward = m_state.forward * cos(pitchAngle) + m_state.up * sin(pitchAngle);
      // Clamp pitch to avoid flipping
      if (std::abs(newForward.y) < 0.99) {
        m_state.forward = NormalizeSafe(newForward);
        m_state.up = NormalizeSafe(glm::cross(m_right, m_state.forward));
      }
    }
    
    // Roll (rotate around forward axis)
    if (std::abs(m_roll) > EPSILON) {
      double rollAngle = m_roll * rotAmount;
      glm::dvec3 newUp = m_state.up * cos(rollAngle) + m_right * sin(rollAngle);
      m_state.up = NormalizeSafe(newUp);
      m_right = NormalizeSafe(glm::cross(m_state.forward, m_state.up));
    }
  }
  
  // Apply acceleration to velocity
  m_state.velocity += worldAccel * deltaTime;
}

void SpaceshipNavigation::UpdateOrbitMode(double deltaTime) {
  if (!HasTarget()) {
    // No target, fall back to free flight
    m_state.mode = NavigationMode::FreeFlight;
    return;
  }
  
  // Calculate desired orbit radius based on target size
  double orbitRadius = std::max(m_target.radius * ORBIT_DEFAULT_RADIUS_MULTIPLIER, 
                                 m_config.proximityThreshold);
  
  // Calculate current direction to target
  glm::dvec3 toTarget = m_target.position - m_state.position;
  double currentDist = glm::length(toTarget);
  
  if (currentDist < EPSILON) {
    // At target center, move out slightly
    toTarget = glm::dvec3(orbitRadius, 0.0, 0.0);
    currentDist = orbitRadius;
  }
  
  // Calculate desired orbital velocity (perpendicular to radial direction)
  glm::dvec3 desiredVelocity = CalculateOrbitVelocity(m_target, orbitRadius);
  
  // Smoothly interpolate current velocity toward orbital velocity
  double interpFactor = std::min(1.0, deltaTime * 2.0);
  m_state.velocity = m_state.velocity * (1.0 - interpFactor) + desiredVelocity * interpFactor;
  
  // Adjust position if too far from orbit radius
  double distError = currentDist - orbitRadius;
  if (std::abs(distError) > orbitRadius * 0.1) {
    glm::dvec3 radialDir = NormalizeSafe(toTarget);
    double correctionSpeed = -distError * 0.5; // Proportional control
    m_state.velocity += radialDir * correctionSpeed;
  }
  
  // Orient ship to face direction of travel
  if (glm::length(m_state.velocity) > EPSILON) {
    m_state.forward = NormalizeSafe(m_state.velocity);
    m_state.up = glm::dvec3(0.0, 1.0, 0.0);
    m_right = NormalizeSafe(glm::cross(m_state.forward, m_state.up));
  }
}

void SpaceshipNavigation::UpdateFocusMode(double deltaTime) {
  (void)deltaTime;
  
  // In focus mode, velocity is zero but we track the target
  m_state.velocity = glm::dvec3(0.0);
  
  // Orientation can be set to look at target if we have one
  if (HasTarget()) {
    glm::dvec3 toTarget = m_target.position - m_state.position;
    if (glm::length(toTarget) > EPSILON) {
      m_state.forward = NormalizeSafe(toTarget);
      m_state.up = glm::dvec3(0.0, 1.0, 0.0);
      m_right = NormalizeSafe(glm::cross(m_state.forward, m_state.up));
    }
  }
}

void SpaceshipNavigation::UpdateAutoPilot(double deltaTime) {
  if (!HasTarget()) {
    DisengageAutoPilot();
    return;
  }
  
  double distToTarget = m_state.distanceToTarget;
  
  // Check if we've arrived
  double arrivalThreshold = m_target.radius + m_config.landingThreshold;
  if (distToTarget <= arrivalThreshold) {
    // Arrived at target
    m_state.velocity *= 0.9; // Rapid deceleration
    if (glm::length(m_state.velocity) < EPSILON) {
      m_state.velocity = glm::dvec3(0.0);
    }
    
    // Optionally transition to orbit or landing
    if (m_landingModeEngaged) {
      // Hold position near surface
      glm::dvec3 toTarget = NormalizeSafe(m_target.position - m_state.position);
      glm::dvec3 holdPos = m_target.position + toTarget * (m_target.radius + m_config.landingThreshold);
      
      // Simple proportional controller to maintain position
      glm::dvec3 posError = holdPos - m_state.position;
      m_state.velocity += posError * 0.5 * deltaTime;
    }
    return;
  }
  
  // Calculate approach vector
  glm::dvec3 toTarget = m_target.position - m_state.position;
  glm::dvec3 toTargetNorm = NormalizeSafe(toTarget);
  
  // Predict target movement (lead the target)
  double leadTime = distToTarget / (m_config.maxSpeed * m_warpSpeedFactor + EPSILON);
  glm::dvec3 predictedTargetPos = m_target.position + m_target.velocity * leadTime;
  glm::dvec3 toPredictedTarget = predictedTargetPos - m_state.position;
  glm::dvec3 approachDir = NormalizeSafe(toPredictedTarget);
  
  // Determine desired speed based on distance
  double desiredSpeed;
  if (m_warpStateInternal == WarpState::Active) {
    desiredSpeed = m_config.maxSpeed * m_config.warpSpeedMultiplier;
  } else if (distToTarget < m_config.warpDecelerationDist) {
    // Deceleration zone - linear interpolation to zero
    double t = distToTarget / m_config.warpDecelerationDist;
    desiredSpeed = m_config.maxSpeed * t;
  } else if (distToTarget < m_config.proximityThreshold) {
    // Proximity zone - slow approach
    desiredSpeed = m_config.landingSpeedLimit;
  } else {
    // Normal cruising
    desiredSpeed = m_config.maxSpeed * 0.8;
  }
  
  // Apply warp multiplier if warping
  if (m_state.isWarping && m_warpStateInternal == WarpState::Active) {
    desiredSpeed *= m_config.warpSpeedMultiplier;
  }
  
  // Calculate required acceleration/deceleration
  double currentSpeed = glm::length(m_state.velocity);
  double speedError = desiredSpeed - currentSpeed;
  
  // Accelerate or decelerate along approach direction
  glm::dvec3 accelDir = approachDir;
  if (speedError < 0) {
    // Need to slow down - accelerate opposite to current velocity
    if (currentSpeed > EPSILON) {
      accelDir = -NormalizeSafe(m_state.velocity);
    }
  }
  
  double accelMagnitude = std::abs(speedError) * m_config.acceleration * deltaTime;
  accelMagnitude = std::min(accelMagnitude, m_config.acceleration * deltaTime);
  
  m_state.velocity += accelDir * accelMagnitude;
  
  // Orient ship toward approach direction
  m_state.forward = approachDir;
  m_state.up = glm::dvec3(0.0, 1.0, 0.0);
  m_right = NormalizeSafe(glm::cross(m_state.forward, m_state.up));
}

// ============================================================================
// Warp System
// ============================================================================

void SpaceshipNavigation::UpdateWarpSystem(double deltaTime) {
  if (!HasTarget()) {
    DisengageWarp();
    return;
  }
  
  double distToTarget = m_state.distanceToTarget;
  
  switch (m_warpStateInternal) {
    case WarpState::Inactive:
      m_state.warpState = WarpState::Inactive;
      m_state.isWarping = false;
      m_warpSpeedFactor = 1.0;
      
      // Auto-engage warp if far enough and requested
      if (distToTarget > m_config.warpMinDistance && m_autoPilotEngaged) {
        m_warpStateInternal = WarpState::Accelerating;
      }
      break;
      
    case WarpState::Accelerating:
      m_state.warpState = WarpState::Accelerating;
      m_state.isWarping = true;
      
      // Ramp up warp factor
      m_warpSpeedFactor += deltaTime * 2.0;
      m_warpSpeedFactor = std::min(m_warpSpeedFactor, m_config.warpSpeedMultiplier);
      
      // Transition to active warp when at max
      if (m_warpSpeedFactor >= m_config.warpSpeedMultiplier - EPSILON) {
        m_warpStateInternal = WarpState::Active;
      }
      
      // Cancel if too close
      if (distToTarget < m_config.warpDecelerationDist) {
        m_warpStateInternal = WarpState::Decelerating;
      }
      break;
      
    case WarpState::Active:
      m_state.warpState = WarpState::Active;
      m_state.isWarping = true;
      
      // Start decelerating when approaching target
      if (distToTarget < m_config.warpDecelerationDist) {
        m_warpStateInternal = WarpState::Decelerating;
      }
      break;
      
    case WarpState::Decelerating:
      m_state.warpState = WarpState::Decelerating;
      m_state.isWarping = true;
      
      // Ramp down warp factor
      m_warpSpeedFactor -= deltaTime * 3.0;
      m_warpSpeedFactor = std::max(m_warpSpeedFactor, 1.0);
      
      // Transition to inactive when back to normal
      if (m_warpSpeedFactor <= 1.0 + EPSILON) {
        m_warpSpeedFactor = 1.0;
        m_warpStateInternal = WarpState::Inactive;
        m_state.isWarping = false;
      }
      break;
  }
}

// ============================================================================
// Proximity System
// ============================================================================

void SpaceshipNavigation::UpdateProximitySystem(double deltaTime) {
  (void)deltaTime;
  
  if (!HasTarget()) {
    m_state.isInProximity = false;
    return;
  }
  
  double distToSurface = m_state.distanceToTarget - m_target.radius;
  
  // Check proximity
  m_state.isInProximity = (distToSurface < m_config.proximityThreshold);
  
  // Auto-engage landing mode if very close
  if (distToSurface < m_config.landingThreshold && !m_landingModeEngaged) {
    // Could auto-engage, but for now just flag it
    m_state.isLanding = true;
  }
}

// ============================================================================
// Collision Handling
// ============================================================================

void SpaceshipNavigation::HandleCollisions(double deltaTime) {
  if (m_collisionResponse == CollisionResponse::None) {
    return;
  }
  
  if (!m_bodyProvider) return;
  
  // Check collision with all bodies
  for (size_t i = 0; i < m_bodyProvider->GetBodyCount(); ++i) {
    glm::dvec3 bodyPos = m_bodyProvider->GetBodyPosition(i);
    double bodyRadius = m_bodyProvider->GetBodyRadius(i);
    
    glm::dvec3 toBody = bodyPos - m_state.position;
    double distToCenter = glm::length(toBody);
    double distToSurface = distToCenter - bodyRadius;
    
    // Check if colliding (position inside body radius)
    if (distToSurface < 0.0) {
      m_state.collisionDetected = true;
      m_state.collisionObjectName = m_bodyProvider->GetBodyName(i);
      
      glm::dvec3 surfaceNormal = NormalizeSafe(toBody);
      
      switch (m_collisionResponse) {
        case CollisionResponse::SoftStop:
          // Gradually reduce velocity to zero
          m_state.velocity *= (1.0 - deltaTime * 5.0);
          if (glm::length(m_state.velocity) < EPSILON) {
            m_state.velocity = glm::dvec3(0.0);
          }
          
          // Push out of collision along normal
          double penetrationDepth = -distToSurface;
          m_state.position += surfaceNormal * penetrationDepth * 0.5;
          break;
          
        case CollisionResponse::Bounce:
          // Reflect velocity across normal
          double dotProduct = glm::dot(m_state.velocity, surfaceNormal);
          if (dotProduct < 0) {
            m_state.velocity -= 2.0 * surfaceNormal * dotProduct;
            m_state.velocity *= 0.7; // Energy loss on bounce
          }
          
          // Push out of collision
          m_state.position += surfaceNormal * (-distToSurface + 0.01);
          break;
          
        case CollisionResponse::Slide:
          // Remove normal component of velocity
          double normalVel = glm::dot(m_state.velocity, surfaceNormal);
          if (normalVel < 0) {
            m_state.velocity -= surfaceNormal * normalVel;
          }
          
          // Push out of collision
          m_state.position += surfaceNormal * (-distToSurface + 0.01);
          break;
          
        case CollisionResponse::None:
          break;
      }
      
      // Only handle first collision per frame
      break;
    }
  }
}

// ============================================================================
// Helper Functions
// ============================================================================

double SpaceshipNavigation::CalculateDistanceToTarget() const {
  if (!HasTarget()) return 0.0;
  return glm::length(m_target.position - m_state.position);
}

glm::dvec3 SpaceshipNavigation::CalculateOrbitVelocity(const NavigationTarget& target, 
                                                        double orbitRadius) const {
  // Calculate orbital velocity using vis-viva equation approximation
  // v = sqrt(GM/r) for circular orbit
  
  double centralMass = m_bodyProvider ? m_bodyProvider->GetBodyMass(static_cast<size_t>(target.bodyIndex)) : 1.0;
  double orbitalSpeed = std::sqrt(G_ORBITAL * centralMass / orbitRadius);
  
  // Get direction perpendicular to radial vector
  glm::dvec3 radialDir = NormalizeSafe(target.position - m_state.position);
  
  // Choose an arbitrary perpendicular direction for orbit plane
  glm::dvec3 upVec = glm::dvec3(0.0, 1.0, 0.0);
  if (std::abs(radialDir.y) > 0.99) {
    upVec = glm::dvec3(1.0, 0.0, 0.0);
  }
  
  glm::dvec3 tangentDir = NormalizeSafe(glm::cross(radialDir, upVec));
  glm::dvec3 orbitNormal = NormalizeSafe(glm::cross(tangentDir, radialDir));
  tangentDir = NormalizeSafe(glm::cross(orbitNormal, radialDir));
  
  return tangentDir * orbitalSpeed;
}

void SpaceshipNavigation::ApplyDamping(double deltaTime) {
  // Apply linear damping
  double dampingFactor = std::pow(m_config.linearDamping, deltaTime * 60.0);
  m_state.velocity *= dampingFactor;
  
  // Additional deceleration if no input
  if (std::abs(m_throttle) < EPSILON && std::abs(m_strafe) < EPSILON && 
      std::abs(m_vertical) < EPSILON && !m_autoPilotEngaged) {
    double deccelAmount = m_config.deceleration * deltaTime;
    double currentSpeed = glm::length(m_state.velocity);
    if (currentSpeed > deccelAmount) {
      m_state.velocity = NormalizeSafe(m_state.velocity) * (currentSpeed - deccelAmount);
    } else {
      m_state.velocity = glm::dvec3(0.0);
    }
  }
}

void SpaceshipNavigation::ClampVelocity() {
  double maxVel = m_config.maxSpeed * m_warpSpeedFactor;
  
  // Override max speed in landing mode
  if (m_landingModeEngaged || m_state.isInProximity) {
    maxVel = std::min(maxVel, m_config.landingSpeedLimit);
  }
  
  double currentSpeed = glm::length(m_state.velocity);
  if (currentSpeed > maxVel) {
    m_state.velocity = NormalizeSafe(m_state.velocity) * maxVel;
  }
}

void SpaceshipNavigation::UpdateStateFromPhysics() {
  m_state.currentSpeed = glm::length(m_state.velocity);
  
  // Update warp state display
  m_state.warpState = m_warpStateInternal;
  
  // Update landing state
  m_state.isLanding = m_landingModeEngaged || (m_state.isInProximity && HasTarget());
}

void SpaceshipNavigation::SetOrientation(const glm::dvec3& forward, const glm::dvec3& up) {
  m_state.forward = NormalizeSafe(forward);
  m_state.up = NormalizeSafe(up);
  m_right = NormalizeSafe(glm::cross(m_state.forward, m_state.up));
}

CollisionResponse SpaceshipNavigation::SetCollisionResponse(CollisionResponse response) {
  CollisionResponse old = m_collisionResponse;
  m_collisionResponse = response;
  return old;
}

// ============================================================================
// Camera Integration
// ============================================================================

glm::dvec3 SpaceshipNavigation::CalculateCameraPosition(const glm::dvec3& cameraOffset,
                                                         double smoothFactor) const {
  // Base camera position follows spaceship
  glm::dvec3 targetCamPos = m_state.position + cameraOffset;
  
  // Modify based on navigation mode
  switch (m_state.mode) {
    case NavigationMode::FocusMode:
      // In focus mode, camera looks at target from fixed offset
      if (HasTarget()) {
        glm::dvec3 toTarget = NormalizeSafe(m_target.position - m_state.position);
        targetCamPos = m_target.position - toTarget * glm::length(cameraOffset);
      }
      break;
      
    case NavigationMode::OrbitMode:
      // Camera maintains wider view during orbit
      targetCamPos = m_state.position + cameraOffset * 1.5;
      break;
      
    default:
      break;
  }
  
  // Interpolate for smooth camera movement (avoid snapping)
  // Note: Actual smoothing should be done by camera system with its own state
  // This provides the target position
  return targetCamPos;
}

glm::dvec3 SpaceshipNavigation::CalculateCameraLookAt() const {
  switch (m_state.mode) {
    case NavigationMode::FocusMode:
      // Look at target in focus mode
      if (HasTarget()) {
        return m_target.position;
      }
      return m_state.position + m_state.forward * 10.0;
      
    case NavigationMode::OrbitMode:
      // Look at center of orbit (target)
      if (HasTarget()) {
        return m_target.position;
      }
      return m_state.position + m_state.forward;
      
    default:
      // Look where ship is facing
      return m_state.position + m_state.forward;
  }
}

} // namespace agss
