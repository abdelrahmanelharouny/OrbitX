#pragma once

#include "physics/PhysicsBody.h"

#include <memory>
#include <vector>

namespace agss {

class IIntegrator {
public:
  virtual ~IIntegrator() = default;
  virtual void Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) = 0;
};

class EulerExplicitIntegrator final : public IIntegrator {
public:
  void Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) override;
};

class SemiImplicitEulerIntegrator final : public IIntegrator {
public:
  void Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) override;
};

// RK4 is wired for future use; implemented as a placeholder so the engine API is stable.
class RK4Integrator final : public IIntegrator {
public:
  void Integrate(std::vector<PhysicsBody*>& bodies, double dtDays) override;
};

std::unique_ptr<IIntegrator> CreateIntegrator(IntegratorType type);

} // namespace agss

