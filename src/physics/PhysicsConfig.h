#pragma once

namespace agss {

enum class IntegratorType {
  EulerExplicit,
  SemiImplicitEuler,
  RK4
};

struct PhysicsConfig {
  double gravityConstantG = 0.00029591220828559104; // 4π² / (365.25²)
  bool repulsiveGravity = false;                    // anti-gravity mode (repulsive force law)

  // Stability knobs
  double softeningEpsilon = 1e-6; // AU; used as epsilon^2 in inverse-square force
  double maxSpeed = 0.0;          // AU/day; 0 disables clamp

  IntegratorType integrator = IntegratorType::EulerExplicit;
};

} // namespace agss

