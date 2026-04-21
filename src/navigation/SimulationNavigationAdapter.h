#pragma once

#include "navigation/NavigationSystem.h"
#include "simulation/SimulationManager.h"
#include <memory>

namespace agss {

/**
 * Adapter class that bridges NavigationSystem with SimulationManager
 * Implements IBodyDataProvider to provide celestial body data to navigation
 */
class SimulationNavigationAdapter : public IBodyDataProvider {
public:
    explicit SimulationNavigationAdapter(SimulationManager* sim) 
        : m_simulation(sim) {}
    
    // IBodyDataProvider implementation
    size_t GetBodyCount() const override {
        if (!m_simulation) return 0;
        auto snapshot = m_simulation->GetPhysicsSnapshot();
        return snapshot.positions.size();
    }
    
    std::string GetBodyName(size_t index) const override {
        if (!m_simulation || index >= GetBodyCount()) return "";
        // Get body info through simulation manager
        // For now, return generic name - could be enhanced with actual names
        return "Body_" + std::to_string(index);
    }
    
    glm::dvec3 GetBodyPosition(size_t index) const override {
        if (!m_simulation || index >= GetBodyCount()) {
            return glm::dvec3(0.0);
        }
        auto snapshot = m_simulation->GetPhysicsSnapshot();
        if (index < snapshot.positions.size()) {
            return snapshot.positions[index];
        }
        return glm::dvec3(0.0);
    }
    
    double GetBodyRadius(size_t index) const override {
        if (!m_simulation || index >= GetBodyCount()) return 0.0;
        // Could be enhanced to return actual body radii
        return 0.1; // Default placeholder
    }
    
    bool HasBody(size_t index) const override {
        return index < GetBodyCount();
    }
    
    /**
     * Lock target on simulation's selected body
     */
    bool LockToSelectedBody(NavigationSystem& nav) {
        if (!m_simulation || !m_simulation->HasSelection()) {
            return false;
        }
        int idx = m_simulation->GetSelectedBodyIndex();
        return nav.LockTarget(idx);
    }
    
private:
    SimulationManager* m_simulation = nullptr;
};

} // namespace agss
