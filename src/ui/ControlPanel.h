#pragma once

#include "simulation/SimulationManager.h"
#include <imgui.h>
#include <string>

namespace agss {

/**
 * Control Panel - Main simulation and physics controls
 * Located in the side panel or top toolbar
 */
class ControlPanel {
public:
    ControlPanel();

    void SetSimulationManager(SimulationManager* sim) { m_sim = sim; }

    // Render the control panel
    void Render(bool& isOpen);

    // Get preferred width for layout
    float GetPreferredWidth() const { return 280.0f; }

private:
    SimulationManager* m_sim = nullptr;

    // UI State
    bool m_showSimulationControls = true;
    bool m_showPhysicsControls = true;
    bool m_showIntegratorSelector = false;

    // Temporary values for sliders
    float m_gravitySliderValue = 0.0f;
    int m_timePresetIndex = 1;
};

} // namespace agss
