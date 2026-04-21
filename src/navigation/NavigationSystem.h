#pragma once

#include "navigation/NavigationConfig.h"
#include "physics/PhysicsBody.h"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <functional>

namespace agss {

/**
 * Navigation mode enumeration
 */
enum class NavigationMode {
    FreeFlight,      // Standard 6DOF spaceship movement
    Orbit,           // Auto-orbit around selected target
    Focus            // Locked camera facing target
};

/**
 * Target information for locking and interaction
 */
struct NavigationTarget {
    int bodyIndex = -1;               // Index in simulation body list
    std::string name;                 // Target name
    glm::dvec3 position{0.0};         // Current world position
    double distance = 0.0;            // Distance from spaceship
    bool isValid = false;             // Whether target is currently valid
};

/**
 * Spaceship state containing all navigation physics data
 * This is independent from the main physics simulation
 */
struct SpaceshipState {
    // Position and orientation
    glm::dvec3 position{0.0, 5.0, 20.0};  // Start above and behind origin
    glm::dquat orientation{1.0, 0.0, 0.0, 0.0}; // Identity quaternion
    
    // Velocity (linear)
    glm::dvec3 velocity{0.0};
    glm::dvec3 acceleration{0.0};
    
    // Angular velocity (for rotation)
    glm::dvec3 angularVelocity{0.0};
    
    // Input state (current frame)
    bool inputForward = false;
    bool inputBackward = false;
    bool inputLeft = false;
    bool inputRight = false;
    bool inputUp = false;
    bool inputDown = false;
    bool inputBoost = false;
    bool inputSlow = false;
    
    // Mouse look deltas (accumulated per frame)
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    
    // Current speed magnitude
    double currentSpeed = 0.0;
    
    // Navigation mode
    NavigationMode mode = NavigationMode::FreeFlight;
    
    // Current target
    NavigationTarget currentTarget;
    
    // Camera mode
    bool isThirdPerson = true;
    float cameraDistance = 10.0f;
};

/**
 * Interface for getting celestial body data from simulation
 * This decouples navigation from the physics simulation
 */
class IBodyDataProvider {
public:
    virtual ~IBodyDataProvider() = default;
    
    virtual size_t GetBodyCount() const = 0;
    virtual std::string GetBodyName(size_t index) const = 0;
    virtual glm::dvec3 GetBodyPosition(size_t index) const = 0;
    virtual double GetBodyRadius(size_t index) const = 0;
    virtual bool HasBody(size_t index) const = 0;
};

/**
 * Main Navigation System class
 * Handles spaceship movement, camera control, and target interaction
 * 
 * Architecture:
 * - Independent from physics simulation
 * - Receives input through abstracted interface
 * - Provides camera transform for rendering
 * - Manages navigation modes and transitions
 */
class NavigationSystem {
public:
    NavigationSystem();
    ~NavigationSystem();
    
    /**
     * Initialize with configuration and body data provider
     */
    bool Init(const NavigationConfig& config, IBodyDataProvider* bodyProvider);
    
    /**
     * Update navigation state (call once per frame)
     * @param deltaTime Time elapsed since last frame (in days for consistency)
     */
    void Update(double deltaTime);
    
    /**
     * Get the view matrix for rendering
     */
    glm::dmat4 GetViewMatrix() const;
    
    /**
     * Get the projection matrix (can be overridden by application)
     */
    glm::dmat4 GetProjectionMatrix(double aspectRatio) const;
    
    /**
     * Get current spaceship state (const access)
     */
    const SpaceshipState& GetState() const { return m_state; }
    
    /**
     * Get spaceship position in world space
     */
    glm::dvec3 GetPosition() const { return m_state.position; }
    
    /**
     * Get spaceship forward direction
     */
    glm::dvec3 GetForwardDirection() const;
    
    /**
     * Get spaceship right direction
     */
    glm::dvec3 GetRightDirection() const;
    
    /**
     * Get spaceship up direction
     */
    glm::dvec3 GetUpDirection() const;
    
    // === Input Handling ===
    
    /**
     * Set keyboard input state
     */
    void SetKeyboardInput(bool forward, bool backward, bool left, bool right,
                          bool up, bool down, bool boost, bool slow);
    
    /**
     * Process mouse movement (called when mouse moves)
     * @param deltaX Mouse movement in X (pixels)
     * @param deltaY Mouse movement in Y (pixels)
     */
    void ProcessMouseMovement(float deltaX, float deltaY);
    
    /**
     * Process scroll input for zoom
     * @param delta Scroll wheel delta
     */
    void ProcessScroll(float delta);
    
    /**
     * Toggle between first and third person camera
     */
    void ToggleCameraMode();
    
    // === Mode Management ===
    
    /**
     * Set navigation mode
     */
    void SetNavigationMode(NavigationMode mode);
    
    /**
     * Get current navigation mode
     */
    NavigationMode GetNavigationMode() const { return m_state.mode; }
    
    /**
     * Get mode name as string for UI
     */
    static const char* GetModeName(NavigationMode mode);
    
    // === Target Management ===
    
    /**
     * Lock onto a target by body index
     * @param bodyIndex Index of the celestial body to target
     * @return True if target was successfully locked
     */
    bool LockTarget(int bodyIndex);
    
    /**
     * Clear current target
     */
    void ClearTarget();
    
    /**
     * Find nearest target within range
     * @param maxRange Maximum search range (AU)
     * @return Index of nearest body, or -1 if none found
     */
    int FindNearestTarget(double maxRange = 100.0) const;
    
    /**
     * Get distance to current target
     */
    double GetDistanceToTarget() const;
    
    // === Configuration ===
    
    /**
     * Update navigation configuration at runtime
     */
    void SetConfig(const NavigationConfig& config) { m_config = config; }
    
    /**
     * Get current configuration
     */
    const NavigationConfig& GetConfig() const { return m_config; }
    
    // === Scale Compression ===
    
    /**
     * Apply scale compression to a world position for rendering
     * This helps visualize vast distances
     * @param worldPos World space position
     * @return Compressed position relative to ship
     */
    glm::dvec3 ApplyScaleCompression(const glm::dvec3& worldPos) const;
    
    /**
     * Check if approaching a planet (for smooth transition effects)
     * @param bodyIndex Body to check
     * @param threshold Distance threshold (AU)
     * @return True if within threshold
     */
    bool IsApproachingBody(int bodyIndex, double threshold = 1.0) const;
    
private:
    // Internal update methods
    void UpdateMovement(double deltaTime);
    void UpdateRotation(double deltaTime);
    void UpdateCamera(double deltaTime);
    void UpdateOrbitMode(double deltaTime);
    void UpdateFocusMode(double deltaTime);
    void UpdateTargetInfo();
    void ClampSpeed();
    
    // Helper functions
    glm::dvec3 GetLocalForward() const;
    glm::dvec3 GetLocalRight() const;
    glm::dvec3 GetLocalUp() const;
    void ApplyAcceleration(const glm::dvec3& accel, double deltaTime);
    
    // State
    SpaceshipState m_state;
    NavigationConfig m_config;
    IBodyDataProvider* m_bodyProvider = nullptr;
    
    // Smooth interpolation targets
    glm::dvec3 m_cameraTarget{0.0};
    glm::dquat m_orientationTarget{1.0, 0.0, 0.0, 0.0};
    
    // Mode transition state
    float m_modeTransitionAlpha = 1.0f;
    NavigationMode m_previousMode = NavigationMode::FreeFlight;
    
    // Orbit state
    double m_orbitAngle = 0.0;
    glm::dvec3 m_orbitAxis{0.0, 1.0, 0.0};
};

} // namespace agss
