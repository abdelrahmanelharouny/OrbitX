#pragma once

#include "simulation/SimulationManager.h"
#include <imgui.h>
#include <string>

namespace agss {

/**
 * Status Bar - Bottom bar showing simulation stats
 * Displays: Time, FPS, Body count, Simulation state
 */
class StatusBar {
public:
    StatusBar();

    void SetSimulationManager(SimulationManager* sim) { m_sim = sim; }

    // Render the status bar (full width at bottom)
    void Render();

    // Update internal state
    void Update(float fps);

private:
    SimulationManager* m_sim = nullptr;

    // Display values
    float m_fps = 0.0f;
    double m_simulationTime = 0.0;
    int m_bodyCount = 0;
    bool m_isPaused = false;
    double m_timeScale = 1.0;
    std::string m_currentPreset;
};

} // namespace agss
