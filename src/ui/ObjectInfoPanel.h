#pragma once

#include "simulation/SimulationManager.h"
#include <imgui.h>
#include <string>

namespace agss {

/**
 * Object Information Panel - Displays selected celestial body data
 * Shows: Name, Mass, Velocity, Distance from star
 */
class ObjectInfoPanel {
public:
    ObjectInfoPanel();

    void SetSimulationManager(SimulationManager* sim) { m_sim = sim; }

    // Render the object info panel
    void Render(bool& isOpen);

    // Get preferred size for layout
    float GetPreferredWidth() const { return 260.0f; }
    float GetPreferredMinHeight() const { return 180.0f; }

private:
    SimulationManager* m_sim = nullptr;

    // Cached info for smooth display
    std::string m_cachedName;
    double m_cachedMass = 0.0;
    double m_cachedSpeed = 0.0;
    double m_cachedDistance = 0.0;
    int m_lastSelectedIndex = -1;

    // Update cached info from simulation
    void UpdateCachedInfo();
};

} // namespace agss
