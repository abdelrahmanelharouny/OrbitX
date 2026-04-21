#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <optional>

namespace agss {

// Forward declarations
class CelestialBody;
class PhysicsBody;

/**
 * @brief Navigation state machine modes
 */
enum class NavigationMode {
  FreeFlight,      // Manual control with physics-based movement
  OrbitMode,       // Automatic orbit around selected target
  FocusMode,       // Camera locked to object, no movement
  AutoPilot        // Automatic navigation to target
};

/**
 * @brief Warp speed states
 */
enum class WarpState {
  Inactive,        // Normal speed
  Accelerating,    // Entering warp
  Active,          // At warp speed
  Decelerating     // Exiting warp
};

/**
 * @brief Spaceship configuration for movement tuning
 */
struct SpaceshipConfig {
  // Movement parameters (in AU/day units)
  double maxSpeed = 5.0;           // Maximum speed cap
  double acceleration = 2.0;       // Acceleration rate
  double deceleration = 3.0;       // Natural deceleration/damping
  double rotationSpeed = 1.5;      // Rotation speed (radians/day)
  
  // Warp parameters
  double warpMinDistance = 10.0;   // Minimum distance to activate warp
  double warpMaxDistance = 100.0;  // Distance for full warp
  double warpSpeedMultiplier = 50.0; // Warp speed multiplier
  double warpDecelerationDist = 5.0; // Distance to start decelerating
  
  // Proximity/Landing parameters
  double proximityThreshold = 2.0;  // Distance to trigger proximity mode
  double landingThreshold = 0.5;    // Distance for landing mode
  double landingSpeedLimit = 0.1;   // Max speed in landing mode
  
  // Input sensitivity
  double inputSensitivity = 1.0;    // Multiplier for input response
  
  // Damping
  double linearDamping = 0.95;      // Velocity damping per frame
  double angularDamping = 0.90;     // Angular velocity damping
};

/**
 * @brief Navigation target information
 */
struct NavigationTarget {
  int bodyIndex = -1;               // Index of target body (-1 if none)
  std::string name;                 // Target name
  glm::dvec3 position{0.0};         // Current target position
  glm::dvec3 velocity{0.0};         // Current target velocity
  double radius = 0.0;              // Target physical radius (AU)
  
  bool IsValid() const { return bodyIndex >= 0; }
};

/**
 * @brief Collision response options
 */
enum class CollisionResponse {
  None,            // No collision handling
  SoftStop,        // Gradually slow down and stop
  Bounce,          // Bounce off surface
  Slide            // Slide along surface
};

/**
 * @brief Navigation state output for UI feedback
 */
struct NavigationState {
  NavigationMode mode = NavigationMode::FreeFlight;
  WarpState warpState = WarpState::Inactive;
  
  glm::dvec3 position{0.0};         // Current spaceship position
  glm::dvec3 velocity{0.0};         // Current velocity
  glm::dvec3 forward{0.0, 0.0, 1.0}; // Forward direction
  glm::dvec3 up{0.0, 1.0, 0.0};      // Up direction
  
  double currentSpeed = 0.0;        // Current speed magnitude
  double distanceToTarget = 0.0;    // Distance to current target
  std::string targetName;           // Name of current target
  
  bool isWarping = false;
  bool isLanding = false;
  bool isInProximity = false;
  bool hasTarget = false;
  
  // Collision info
  bool collisionDetected = false;
  std::string collisionObjectName;
};

/**
 * @brief Body information provider interface
 * 
 * Abstracts access to celestial body data so navigation
 * doesn't depend on specific entity implementation.
 */
class IBodyProvider {
public:
  virtual ~IBodyProvider() = default;
  
  virtual size_t GetBodyCount() const = 0;
  virtual std::string GetBodyName(size_t index) const = 0;
  virtual glm::dvec3 GetBodyPosition(size_t index) const = 0;
  virtual glm::dvec3 GetBodyVelocity(size_t index) const = 0;
  virtual double GetBodyRadius(size_t index) const = 0;
  virtual double GetBodyMass(size_t index) const = 0;
};

/**
 * @brief Main Spaceship Navigation System
 * 
 * Handles all spaceship movement, auto-pilot, warp travel,
 * and camera-independent navigation logic.
 * 
 * Architecture principles:
 * - Separates spaceship entity from camera
 * - Uses physics-based movement with inertia
 * - Modular design for extensibility (VR, multiplayer)
 * - Independent from planetary physics simulation
 */
class SpaceshipNavigation {
public:
  SpaceshipNavigation();
  ~SpaceshipNavigation();
  
  // === Initialization & Configuration ===
  
  void Initialize(const SpaceshipConfig& config = SpaceshipConfig{});
  void SetBodyProvider(const IBodyProvider* provider);
  
  // === Mode Management ===
  
  void SetMode(NavigationMode mode);
  NavigationMode GetMode() const { return m_state.mode; }
  
  // === Target Selection ===
  
  void SetTarget(int bodyIndex);
  void ClearTarget();
  NavigationTarget GetTarget() const { return m_target; }
  bool HasTarget() const { return m_target.IsValid(); }
  
  // === Movement Control (Free Flight Mode) ===
  
  void SetThrottle(float throttle);  // -1.0 to 1.0
  void SetStrafe(float strafe);       // -1.0 to 1.0 (left/right)
  void SetVertical(float vertical);   // -1.0 to 1.0 (up/down)
  void SetYaw(float yaw);             // -1.0 to 1.0
  void SetPitch(float pitch);         // -1.0 to 1.0
  void SetRoll(float roll);           // -1.0 to 1.0
  
  // === Auto-Pilot Control ===
  
  void EngageAutoPilot();
  void DisengageAutoPilot();
  bool IsAutoPilotEngaged() const;
  
  // === Warp Control ===
  
  void EngageWarp();
  void DisengageWarp();
  bool IsWarping() const { return m_state.isWarping; }
  WarpState GetWarpState() const { return m_state.warpState; }
  
  // === Landing Control ===
  
  void EngageLandingMode();
  void DisengageLandingMode();
  bool IsInLandingMode() const { return m_state.isLanding; }
  
  // === Update Loop ===
  
  /**
   * @brief Update navigation state
   * @param deltaTime Time delta in days
   * @return Updated navigation state for UI/camera
   */
  NavigationState Update(double deltaTime);
  
  // === State Access ===
  
  const NavigationState& GetState() const { return m_state; }
  const SpaceshipConfig& GetConfig() const { return m_config; }
  void SetConfig(const SpaceshipConfig& config) { m_config = config; }
  
  // === Position & Orientation ===
  
  glm::dvec3 GetPosition() const { return m_state.position; }
  glm::dvec3 GetVelocity() const { return m_state.velocity; }
  glm::dvec3 GetForward() const { return m_state.forward; }
  glm::dvec3 GetUp() const { return m_state.up; }
  
  void SetPosition(const glm::dvec3& pos) { m_state.position = pos; }
  void SetVelocity(const glm::dvec3& vel) { m_state.velocity = vel; }
  void SetOrientation(const glm::dvec3& forward, const glm::dvec3& up);
  
  // === Collision Handling ===
  
  void SetCollisionResponse(CollisionResponse response);
  CollisionResponse GetCollisionResponse() const { return m_collisionResponse; }
  
  // === Camera Integration ===
  
  /**
   * @brief Calculate ideal camera position based on spaceship and mode
   * @param cameraOffset Desired offset from spaceship
   * @param smoothFactor Smoothing factor (0-1) for interpolation
   * @return Interpolated camera position
   */
  glm::dvec3 CalculateCameraPosition(const glm::dvec3& cameraOffset, 
                                      double smoothFactor = 0.1) const;
  
  /**
   * @brief Calculate camera look-at target
   * @return Point camera should look at
   */
  glm::dvec3 CalculateCameraLookAt() const;

private:
  // === Internal Update Methods ===
  
  void UpdateFreeFlight(double deltaTime);
  void UpdateOrbitMode(double deltaTime);
  void UpdateFocusMode(double deltaTime);
  void UpdateAutoPilot(double deltaTime);
  
  void UpdateWarpSystem(double deltaTime);
  void UpdateProximitySystem(double deltaTime);
  void HandleCollisions(double deltaTime);
  
  // === Helper Functions ===
  
  double CalculateDistanceToTarget() const;
  glm::dvec3 CalculateOrbitVelocity(const NavigationTarget& target, 
                                     double orbitRadius) const;
  void ApplyDamping(double deltaTime);
  void ClampVelocity();
  void UpdateStateFromPhysics();
  
  // === Member Variables ===
  
  SpaceshipConfig m_config;
  NavigationState m_state;
  NavigationTarget m_target;
  
  const IBodyProvider* m_bodyProvider = nullptr;
  
  // Input state
  float m_throttle = 0.0f;
  float m_strafe = 0.0f;
  float m_vertical = 0.0f;
  float m_yaw = 0.0f;
  float m_pitch = 0.0f;
  float m_roll = 0.0f;
  
  // Auto-pilot internal state
  bool m_autoPilotEngaged = false;
  double m_autoPilotApproachSpeed = 0.0;
  
  // Warp internal state
  WarpState m_warpStateInternal = WarpState::Inactive;
  double m_warpSpeedFactor = 1.0;
  
  // Landing internal state
  bool m_landingModeEngaged = false;
  
  // Collision handling
  CollisionResponse m_collisionResponse = CollisionResponse::SoftStop;
  
  // Orientation tracking
  glm::dvec3 m_right{1.0, 0.0, 0.0};
};

} // namespace agss
