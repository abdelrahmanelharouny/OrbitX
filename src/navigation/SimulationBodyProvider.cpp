#include "navigation/SimulationBodyProvider.h"

#include "simulation/SimulationManager.h"
#include "entities/CelestialBody.h"

namespace agss {

SimulationBodyProvider::SimulationBodyProvider(const SimulationManager* simManager)
  : m_simManager(simManager) {}

size_t SimulationBodyProvider::GetBodyCount() const {
  if (!m_simManager) return 0;
  
  auto snapshot = m_simManager->GetPhysicsSnapshot();
  return snapshot.positions.size();
}

std::string SimulationBodyProvider::GetBodyName(size_t index) const {
  if (!m_simManager) return "";
  
  // Get body info through simulation manager
  int savedSelection = m_simManager->GetSelectedBodyIndex();
  
  // Temporarily select the target body to get its info
  const_cast<SimulationManager*>(m_simManager)->SetSelectedBodyIndex(static_cast<int>(index));
  auto info = m_simManager->GetSelectedBodyInfo();
  
  // Restore previous selection
  const_cast<SimulationManager*>(m_simManager)->SetSelectedBodyIndex(savedSelection);
  
  return info.name;
}

glm::dvec3 SimulationBodyProvider::GetBodyPosition(size_t index) const {
  if (!m_simManager) return glm::dvec3(0.0);
  
  auto snapshot = m_simManager->GetPhysicsSnapshot();
  if (index >= snapshot.positions.size()) {
    return glm::dvec3(0.0);
  }
  return snapshot.positions[index];
}

glm::dvec3 SimulationBodyProvider::GetBodyVelocity(size_t index) const {
  if (!m_simManager) return glm::dvec3(0.0);
  
  auto snapshot = m_simManager->GetPhysicsSnapshot();
  if (index >= snapshot.velocities.size()) {
    return glm::dvec3(0.0);
  }
  return snapshot.velocities[index];
}

double SimulationBodyProvider::GetBodyRadius(size_t index) const {
  if (!m_simManager) return 0.0;
  
  // Radius is not directly available from physics snapshot
  // We need to estimate based on mass or use a default
  // For a more accurate implementation, SimulationManager would expose radius
  
  auto snapshot = m_simManager->GetPhysicsSnapshot();
  if (index >= snapshot.positions.size()) {
    return 0.0;
  }
  
  // Approximate radius from mass (very rough approximation)
  // In a real system, this would come from CelestialBody data
  double mass = GetBodyMass(index);
  
  // Simple scaling: radius ~ mass^(1/3) for rocky bodies
  // Using AU units - Sun radius is about 0.00465 AU
  if (mass > 0.1) {
    // Star-like object
    return 0.005 * std::pow(mass, 0.33);
  } else if (mass > 0.0001) {
    // Planet-like object
    return 0.0001 * std::pow(mass / 0.000003, 0.33);
  } else {
    // Small object
    return 0.00001;
  }
}

double SimulationBodyProvider::GetBodyMass(size_t index) const {
  if (!m_simManager) return 0.0;
  
  int savedSelection = m_simManager->GetSelectedBodyIndex();
  
  const_cast<SimulationManager*>(m_simManager)->SetSelectedBodyIndex(static_cast<int>(index));
  auto info = m_simManager->GetSelectedBodyInfo();
  
  const_cast<SimulationManager*>(m_simManager)->SetSelectedBodyIndex(savedSelection);
  
  return info.mass;
}

} // namespace agss
