#pragma once

#include <string>
#include <vector>

namespace agss {

struct PhysicsBody;

class IForceGenerator {
public:
  virtual ~IForceGenerator() = default;

  virtual void Apply(std::vector<PhysicsBody*>& bodies, double dtDays) = 0;
  virtual std::string GetName() const = 0;

  bool IsEnabled() const { return m_enabled; }
  void SetEnabled(bool enabled) { m_enabled = enabled; }

private:
  bool m_enabled = true;
};

} // namespace agss

