#pragma once

#include <glm/vec3.hpp>
#include <glm/glm.hpp>

namespace agss {

/**
 * Configuration constants for the navigation system
 */
struct NavigationConfig {
    // Movement speeds (AU/day for consistency with physics)
    double baseSpeed = 0.5;           // Base forward/backward speed
    double boostMultiplier = 3.0;     // Speed multiplier when boosting
    double slowMultiplier = 0.3;      // Speed multiplier when in slow mode
    double strafeSpeed = 0.4;         // Lateral movement speed
    double verticalSpeed = 0.4;       // Up/down movement speed
    
    // Rotation sensitivity
    float mouseSensitivity = 0.002f;  // Radians per pixel
    
    // Physics parameters
    double acceleration = 0.8;        // Acceleration rate (AU/day^2)
    double deceleration = 0.5;        // Natural deceleration (if enabled)
    double maxSpeed = 5.0;            // Maximum speed clamp (AU/day)
    double rotationSpeed = 1.5;       // Angular velocity (radians/day)
    
    // Camera settings
    float thirdPersonDistance = 10.0f; // Distance behind ship in third-person
    float cameraSmoothFactor = 0.15f;  // Lerp factor for smooth camera
    float minZoom = 1.0f;              // Minimum zoom distance
    float maxZoom = 100.0f;            // Maximum zoom distance
    
    // Scale compression for large distances
    double scaleCompressionThreshold = 100.0; // AU - start compressing beyond this
    double scaleCompressionFactor = 0.1;      // Compression ratio for distant objects
    
    // Interaction settings
    double targetLockDistance = 50.0;   // Max distance to lock onto target
    double orbitRadius = 5.0;           // Default orbit radius around target
    double orbitSpeed = 0.5;            // Orbital angular speed
    
    // Mode transition smoothing
    float modeTransitionSpeed = 5.0f;   // Interpolation speed for mode changes
};

} // namespace agss
