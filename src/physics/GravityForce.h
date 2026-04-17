#pragma once

#include "physics/IForceGenerator.h"
#include "physics/PhysicsConfig.h"

namespace agss {

class GravityForce final : public IForceGenerator {
public:
  // In the plan, G is "scaled" in AU / (solar mass · day^2).
  // Here we default to a Kepler-friendly constant: 4π² / (365.25²) AU³ / (M☉·day²)
  explicit GravityForce(double GScaled = 0.00029591220828559104);

  void Apply(std::vector<PhysicsBody*>& bodies, double dtDays) override;
  std::string GetName() const override { return "GravityForce"; }

  void SetG(double g) { m_G = g; }
  double GetG() const { return m_G; }

  void SetRepulsive(bool repulsive) { m_repulsive = repulsive; }
  bool IsRepulsive() const { return m_repulsive; }

  void SetSofteningEpsilon(double eps) { m_softeningEpsilon = eps; }

private:
  double m_G;
  bool m_repulsive = false;
  double m_softeningEpsilon = 1e-6;
};

} // namespace agss

