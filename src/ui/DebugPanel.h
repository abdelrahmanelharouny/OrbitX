#pragma once

#include "simulation/SimulationManager.h"
#include <imgui.h>

namespace agss {

/**
 * Debug Panel - Advanced debugging and visualization toggles
 * Collapsible panel with debug draw options and statistics
 */
class DebugPanel {
public:
    DebugPanel();

    void SetSimulationManager(SimulationManager* sim) { m_sim = sim; }

    // Render the debug panel
    void Render(bool& isOpen);

    // Get preferred size
    float GetPreferredWidth() const { return 280.0f; }

private:
    SimulationManager* m_sim = nullptr;

    // Debug draw toggles (mirrored from simulation)
    bool m_drawTrails = true;
    bool m_drawVelocity = true;
    bool m_drawForces = false;
    bool m_drawCollisions = false;

    // Statistics display
    int m_bodyCount = 0;
    double m_totalEnergy = 0.0;
    double m_kineticEnergy = 0.0;
    double m_potentialEnergy = 0.0;

    // Update stats from simulation
    void UpdateStatistics();
};

} // namespace agss
