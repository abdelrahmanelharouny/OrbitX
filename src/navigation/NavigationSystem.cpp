#include "navigation/NavigationSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace agss {

NavigationSystem::NavigationSystem() = default;
NavigationSystem::~NavigationSystem() = default;

bool NavigationSystem::Init(const NavigationConfig& config, IBodyDataProvider* bodyProvider) {
    m_config = config;
    m_bodyProvider = bodyProvider;
    
    // Initialize spaceship at a reasonable starting position
    m_state.position = glm::dvec3(0.0, 5.0, 20.0);
    m_state.orientation = glm::dquat(1.0, 0.0, 0.0, 0.0);
    m_state.velocity = glm::dvec3(0.0);
    m_state.mode = NavigationMode::FreeFlight;
    m_state.isThirdPerson = true;
    m_state.cameraDistance = m_config.thirdPersonDistance;
    
    // Find a target to start with (e.g., the sun at index 0)
    if (m_bodyProvider && m_bodyProvider->HasBody(0)) {
        LockTarget(0);
    }
    
    return true;
}

void NavigationSystem::Update(double deltaTime) {
    // Reset mouse deltas for this frame
    m_state.mouseDeltaX = 0.0f;
    m_state.mouseDeltaY = 0.0f;
    
    // Update target information
    UpdateTargetInfo();
    
    // Handle mode-specific updates
    switch (m_state.mode) {
        case NavigationMode::FreeFlight:
            UpdateMovement(deltaTime);
            UpdateRotation(deltaTime);
            break;
        case NavigationMode::Orbit:
            UpdateOrbitMode(deltaTime);
            break;
        case NavigationMode::Focus:
            UpdateFocusMode(deltaTime);
            break;
    }
    
    // Clamp speed to maximum
    ClampSpeed();
    
    // Update camera position
    UpdateCamera(deltaTime);
    
    // Update current speed magnitude
    m_state.currentSpeed = glm::length(m_state.velocity);
}

void NavigationSystem::UpdateMovement(double deltaTime) {
    // Calculate speed multiplier based on boost/slow
    double speedMultiplier = 1.0;
    if (m_state.inputBoost) {
        speedMultiplier = m_config.boostMultiplier;
    } else if (m_state.inputSlow) {
        speedMultiplier = m_config.slowMultiplier;
    }
    
    // Calculate movement direction in local space
    glm::dvec3 moveDirection(0.0);
    
    if (m_state.inputForward) {
        moveDirection += GetLocalForward();
    }
    if (m_state.inputBackward) {
        moveDirection -= GetLocalForward();
    }
    if (m_state.inputRight) {
        moveDirection += GetLocalRight();
    }
    if (m_state.inputLeft) {
        moveDirection -= GetLocalRight();
    }
    if (m_state.inputUp) {
        moveDirection += GetLocalUp();
    }
    if (m_state.inputDown) {
        moveDirection -= GetLocalUp();
    }
    
    // Normalize if there's input in multiple directions
    if (glm::length(moveDirection) > 0.0) {
        moveDirection = glm::normalize(moveDirection);
    }
    
    // Apply acceleration in the movement direction
    if (glm::length(moveDirection) > 0.0) {
        double effectiveAcceleration = m_config.acceleration * speedMultiplier;
        glm::dvec3 accel = moveDirection * effectiveAcceleration;
        ApplyAcceleration(accel, deltaTime);
    } else {
        // Apply natural deceleration when no input
        // In real space, there's no friction, but we add some for gameplay
        double decel = m_config.deceleration;
        if (m_state.currentSpeed > 0.0) {
            glm::dvec3 damping = -glm::normalize(m_state.velocity) * decel;
            ApplyAcceleration(damping, deltaTime);
            
            // Don't let damping reverse velocity
            if (glm::dot(m_state.velocity, damping) > 0.0) {
                m_state.velocity = glm::dvec3(0.0);
            }
        }
    }
    
    // Update position based on velocity
    m_state.position += m_state.velocity * deltaTime;
}

void NavigationSystem::UpdateRotation(double deltaTime) {
    // Apply mouse look
    if (m_state.mouseDeltaX != 0.0f || m_state.mouseDeltaY != 0.0f) {
        float yawDelta = -m_state.mouseDeltaX * m_config.mouseSensitivity;
        float pitchDelta = -m_state.mouseDeltaY * m_config.mouseSensitivity;
        
        // Create rotation quaternions
        glm::dquat yawRot = glm::angleAxis(static_cast<double>(yawDelta), 
                                            glm::dvec3(0.0, 1.0, 0.0));
        glm::dquat pitchRot = glm::angleAxis(static_cast<double>(pitchDelta), 
                                              GetLocalRight());
        
        // Apply rotations (yaw first, then pitch)
        m_state.orientation = yawRot * m_state.orientation;
        m_state.orientation = pitchRot * m_state.orientation;
        
        // Normalize to prevent drift
        m_state.orientation = glm::normalize(m_state.orientation);
    }
}

void NavigationSystem::UpdateCamera(double deltaTime) {
    if (m_state.isThirdPerson) {
        // Third-person: camera follows behind ship
        glm::dvec3 backDirection = -GetLocalForward();
        glm::dvec3 targetPos = m_state.position + backDirection * m_state.cameraDistance;
        
        // Smooth interpolation
        float t = m_config.cameraSmoothFactor;
        m_cameraTarget = glm::mix(m_cameraTarget, targetPos, t);
    } else {
        // First-person: camera at ship position
        m_cameraTarget = m_state.position;
    }
}

void NavigationSystem::UpdateOrbitMode(double deltaTime) {
    if (!m_state.currentTarget.isValid) {
        // No valid target, switch back to free flight
        SetNavigationMode(NavigationMode::FreeFlight);
        return;
    }
    
    // Update orbit angle
    m_orbitAngle += m_config.orbitSpeed * deltaTime;
    
    // Calculate orbit position around target
    glm::dvec3 targetPos = m_state.currentTarget.position;
    
    // Create orbit offset using the orbit axis
    glm::dvec3 rightDir = glm::normalize(glm::cross(m_orbitAxis, GetLocalForward()));
    glm::dvec3 upDir = glm::normalize(glm::cross(rightDir, m_orbitAxis));
    
    double cosAngle = glm::cos(m_orbitAngle);
    double sinAngle = glm::sin(m_orbitAngle);
    
    glm::dvec3 orbitOffset = rightDir * (m_config.orbitRadius * cosAngle) +
                             upDir * (m_config.orbitRadius * sinAngle);
    
    m_state.position = targetPos + orbitOffset;
    
    // Orient ship to face the target
    glm::dvec3 toTarget = glm::normalize(targetPos - m_state.position);
    glm::dvec3 forward = GetLocalForward();
    
    // Create rotation to face target
    glm::dquat targetRot = glm::quatLookAt(-toTarget, glm::dvec3(0.0, 1.0, 0.0));
    
    // Smooth rotation interpolation
    float t = m_config.modeTransitionSpeed * static_cast<float>(deltaTime);
    m_state.orientation = glm::slerp(m_state.orientation, targetRot, t);
    m_state.orientation = glm::normalize(m_state.orientation);
    
    // Reset velocity in orbit mode (ship is positioned by orbit calculation)
    m_state.velocity = glm::dvec3(0.0);
}

void NavigationSystem::UpdateFocusMode(double deltaTime) {
    if (!m_state.currentTarget.isValid) {
        // No valid target, switch back to free flight
        SetNavigationMode(NavigationMode::FreeFlight);
        return;
    }
    
    // In focus mode, maintain distance but always face the target
    glm::dvec3 targetPos = m_state.currentTarget.position;
    glm::dvec3 toTarget = targetPos - m_state.position;
    double currentDist = glm::length(toTarget);
    
    // Maintain orbit radius
    if (currentDist > 0.0) {
        glm::dvec3 desiredPos = targetPos - glm::normalize(toTarget) * m_config.orbitRadius;
        
        // Smooth movement to maintain distance
        glm::dvec3 moveDir = desiredPos - m_state.position;
        m_state.position += moveDir * m_config.modeTransitionSpeed * deltaTime;
    }
    
    // Face the target
    glm::dquat targetRot = glm::quatLookAt(glm::normalize(targetPos - m_state.position),
                                            glm::dvec3(0.0, 1.0, 0.0));
    float t = m_config.modeTransitionSpeed * static_cast<float>(deltaTime);
    m_state.orientation = glm::slerp(m_state.orientation, targetRot, t);
    m_state.orientation = glm::normalize(m_state.orientation);
    
    m_state.velocity = glm::dvec3(0.0);
}

void NavigationSystem::UpdateTargetInfo() {
    if (m_state.currentTarget.bodyIndex >= 0 && m_bodyProvider) {
        size_t idx = static_cast<size_t>(m_state.currentTarget.bodyIndex);
        if (m_bodyProvider->HasBody(idx)) {
            m_state.currentTarget.position = m_bodyProvider->GetBodyPosition(idx);
            m_state.currentTarget.name = m_bodyProvider->GetBodyName(idx);
            m_state.currentTarget.distance = glm::distance(m_state.position, 
                                                            m_state.currentTarget.position);
            m_state.currentTarget.isValid = true;
        } else {
            m_state.currentTarget.isValid = false;
        }
    } else {
        m_state.currentTarget.isValid = false;
    }
}

void NavigationSystem::ClampSpeed() {
    double speed = glm::length(m_state.velocity);
    if (speed > m_config.maxSpeed) {
        m_state.velocity = glm::normalize(m_state.velocity) * m_config.maxSpeed;
    }
}

void NavigationSystem::ApplyAcceleration(const glm::dvec3& accel, double deltaTime) {
    m_state.velocity += accel * deltaTime;
    m_state.acceleration = accel;
}

glm::dvec3 NavigationSystem::GetForwardDirection() const {
    return GetLocalForward();
}

glm::dvec3 NavigationSystem::GetRightDirection() const {
    return GetLocalRight();
}

glm::dvec3 NavigationSystem::GetUpDirection() const {
    return GetLocalUp();
}

glm::dvec3 NavigationSystem::GetLocalForward() const {
    // Quaternion rotates forward vector (0, 0, -1)
    return m_state.orientation * glm::dvec3(0.0, 0.0, -1.0);
}

glm::dvec3 NavigationSystem::GetLocalRight() const {
    return m_state.orientation * glm::dvec3(1.0, 0.0, 0.0);
}

glm::dvec3 NavigationSystem::GetLocalUp() const {
    return m_state.orientation * glm::dvec3(0.0, 1.0, 0.0);
}

glm::dmat4 NavigationSystem::GetViewMatrix() const {
    glm::dvec3 eye = m_state.isThirdPerson ? m_cameraTarget : m_state.position;
    glm::dvec3 center = eye + GetLocalForward();
    glm::dvec3 up = GetLocalUp();
    
    return glm::lookAt(eye, center, up);
}

glm::dmat4 NavigationSystem::GetProjectionMatrix(double aspectRatio) const {
    double fov = glm::radians(60.0);
    double nearPlane = 0.01;
    double farPlane = 10000.0;
    
    return glm::perspective(fov, aspectRatio, nearPlane, farPlane);
}

void NavigationSystem::SetKeyboardInput(bool forward, bool backward, bool left, bool right,
                                         bool up, bool down, bool boost, bool slow) {
    m_state.inputForward = forward;
    m_state.inputBackward = backward;
    m_state.inputLeft = left;
    m_state.inputRight = right;
    m_state.inputUp = up;
    m_state.inputDown = down;
    m_state.inputBoost = boost;
    m_state.inputSlow = slow;
}

void NavigationSystem::ProcessMouseMovement(float deltaX, float deltaY) {
    m_state.mouseDeltaX += deltaX;
    m_state.mouseDeltaY += deltaY;
}

void NavigationSystem::ProcessScroll(float delta) {
    m_state.cameraDistance += delta * 2.0f;
    m_state.cameraDistance = glm::clamp(m_state.cameraDistance, 
                                         m_config.minZoom, 
                                         m_config.maxZoom);
}

void NavigationSystem::ToggleCameraMode() {
    m_state.isThirdPerson = !m_state.isThirdPerson;
}

void NavigationSystem::SetNavigationMode(NavigationMode mode) {
    if (m_state.mode != mode) {
        m_previousMode = m_state.mode;
        m_state.mode = mode;
        m_modeTransitionAlpha = 0.0f;
        
        // Initialize orbit angle when entering orbit mode
        if (mode == NavigationMode::Orbit && m_state.currentTarget.isValid) {
            glm::dvec3 toShip = glm::normalize(m_state.position - m_state.currentTarget.position);
            m_orbitAngle = glm::atan(glm::dot(toShip, GetLocalRight()), 
                                      glm::dot(toShip, GetLocalForward()));
        }
    }
}

const char* NavigationSystem::GetModeName(NavigationMode mode) {
    switch (mode) {
        case NavigationMode::FreeFlight: return "Free Flight";
        case NavigationMode::Orbit: return "Orbit";
        case NavigationMode::Focus: return "Focus";
        default: return "Unknown";
    }
}

bool NavigationSystem::LockTarget(int bodyIndex) {
    if (!m_bodyProvider || !m_bodyProvider->HasBody(static_cast<size_t>(bodyIndex))) {
        return false;
    }
    
    m_state.currentTarget.bodyIndex = bodyIndex;
    m_state.currentTarget.name = m_bodyProvider->GetBodyName(static_cast<size_t>(bodyIndex));
    m_state.currentTarget.position = m_bodyProvider->GetBodyPosition(static_cast<size_t>(bodyIndex));
    m_state.currentTarget.isValid = true;
    
    return true;
}

void NavigationSystem::ClearTarget() {
    m_state.currentTarget.bodyIndex = -1;
    m_state.currentTarget.isValid = false;
    m_state.currentTarget.name.clear();
}

int NavigationSystem::FindNearestTarget(double maxRange) const {
    if (!m_bodyProvider) return -1;
    
    int nearestIdx = -1;
    double nearestDist = std::numeric_limits<double>::max();
    
    for (size_t i = 0; i < m_bodyProvider->GetBodyCount(); ++i) {
        if (!m_bodyProvider->HasBody(i)) continue;
        
        glm::dvec3 bodyPos = m_bodyProvider->GetBodyPosition(i);
        double dist = glm::distance(m_state.position, bodyPos);
        
        if (dist < maxRange && dist < nearestDist) {
            nearestDist = dist;
            nearestIdx = static_cast<int>(i);
        }
    }
    
    return nearestIdx;
}

double NavigationSystem::GetDistanceToTarget() const {
    if (m_state.currentTarget.isValid) {
        return m_state.currentTarget.distance;
    }
    return -1.0;
}

glm::dvec3 NavigationSystem::ApplyScaleCompression(const glm::dvec3& worldPos) const {
    glm::dvec3 relativePos = worldPos - m_state.position;
    double dist = glm::length(relativePos);
    
    if (dist > m_config.scaleCompressionThreshold) {
        // Apply compression factor to distant objects
        double compressedDist = m_config.scaleCompressionThreshold +
                                (dist - m_config.scaleCompressionThreshold) * 
                                 m_config.scaleCompressionFactor;
        if (dist > 0.0) {
            return m_state.position + glm::normalize(relativePos) * compressedDist;
        }
    }
    
    return worldPos;
}

bool NavigationSystem::IsApproachingBody(int bodyIndex, double threshold) const {
    if (!m_bodyProvider || !m_bodyProvider->HasBody(static_cast<size_t>(bodyIndex))) {
        return false;
    }
    
    glm::dvec3 bodyPos = m_bodyProvider->GetBodyPosition(static_cast<size_t>(bodyIndex));
    double dist = glm::distance(m_state.position, bodyPos);
    
    return dist < threshold;
}

} // namespace agss
