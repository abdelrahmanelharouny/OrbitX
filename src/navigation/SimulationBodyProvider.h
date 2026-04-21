#pragma once

#include "navigation/SpaceshipNavigation.h"

#include <cstddef>
#include <string>
#include <vector>

namespace agss {

class SimulationManager;

/**
 * @brief Adapter that provides body data from SimulationManager to NavigationSystem
 * 
 * This bridges the gap between the navigation system and the physics simulation,
 * maintaining separation of concerns while allowing navigation to access
 * celestial body information.
 */
class SimulationBodyProvider : public IBodyProvider {
public:
  explicit SimulationBodyProvider(const SimulationManager* simManager);
  
  size_t GetBodyCount() const override;
  std::string GetBodyName(size_t index) const override;
  glm::dvec3 GetBodyPosition(size_t index) const override;
  glm::dvec3 GetBodyVelocity(size_t index) const override;
  double GetBodyRadius(size_t index) const override;
  double GetBodyMass(size_t index) const override;
  
private:
  const SimulationManager* m_simManager;
};

} // namespace agss
