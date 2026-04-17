#pragma once

#include "physics/IForceGenerator.h"

namespace agss {

// Kept for future experimentation; Phase 2 prefers toggling repulsion in GravityForce.
class AntiGravityForce final : public IForceGenerator {
public:
  AntiGravityForce(double strength = 1.0, double effectRadiusAu = 5.0);

  void Apply(std::vector<PhysicsBody*>& bodies, double dtDays) override;
  std::string GetName() const override { return "AntiGravityForce"; }

  void SetStrength(double s) { m_strength = s; }
  double GetStrength() const { return m_strength; }

  void SetEffectRadius(double r) { m_effectRadiusAu = r; }
  double GetEffectRadius() const { return m_effectRadiusAu; }

private:
  double m_strength;
  double m_effectRadiusAu;
};

} // namespace agss

