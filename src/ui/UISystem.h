#pragma once

#include "simulation/SimulationManager.h"
#include "rendering/DebugRender2D.h"

#include <imgui.h>
#include <glm/vec3.hpp>
#include <string>
#include <functional>
#include <memory>

namespace agss {

// Forward declarations for UI components
class ControlPanel;
class ObjectInfoPanel;
class CameraControlPanel;
class PresetManagerPanel;
class DebugPanel;
class StatusBar;

/**
 * Main UI System - Orchestrates all UI components
 * Implements a clean separation between UI rendering and simulation logic
 */
class UISystem {
public:
    UISystem();
    ~UISystem();

    bool Init();
    void Shutdown();

    // Main render call - renders all UI panels
    void Render(int viewportW, int viewportH);

    // Set communication layer to simulation
    void SetSimulationManager(SimulationManager* sim) { m_sim = sim; }
    void SetDebugRenderer(DebugRender2D* debugRenderer) { m_debugRenderer = debugRenderer; }

    // UI State management
    void SetSelectedBodyIndex(int idx);
    int GetSelectedBodyIndex() const { return m_selectedBodyIndex; }

    // Panel visibility toggles
    void ToggleControlPanel() { m_showControlPanel = !m_showControlPanel; }
    void ToggleObjectInfoPanel() { m_showObjectInfoPanel = !m_showObjectInfoPanel; }
    void ToggleCameraPanel() { m_showCameraPanel = !m_showCameraPanel; }
    void TogglePresetPanel() { m_showPresetPanel = !m_showPresetPanel; }
    void ToggleDebugPanel() { m_showDebugPanel = !m_showDebugPanel; }
    void ToggleStatusBar() { m_showStatusBar = !m_showStatusBar; }

    // Theme management
    void ApplyDarkTheme();
    void ApplySpaceTheme();

private:
    SimulationManager* m_sim = nullptr;
    DebugRender2D* m_debugRenderer = nullptr;

    // Panel instances
    std::unique_ptr<ControlPanel> m_controlPanel;
    std::unique_ptr<ObjectInfoPanel> m_objectInfoPanel;
    std::unique_ptr<CameraControlPanel> m_cameraPanel;
    std::unique_ptr<PresetManagerPanel> m_presetPanel;
    std::unique_ptr<DebugPanel> m_debugPanel;
    std::unique_ptr<StatusBar> m_statusBar;

    // Panel visibility
    bool m_showControlPanel = true;
    bool m_showObjectInfoPanel = true;
    bool m_showCameraPanel = false;
    bool m_showPresetPanel = false;
    bool m_showDebugPanel = false;
    bool m_showStatusBar = true;

    // Selection state
    int m_selectedBodyIndex = -1;

    // Callbacks for UI → Simulation communication
    std::function<void(const std::string&)> m_onPresetSelected;
    std::function<void(int)> m_onBodyFocused;

    // UI State
    bool m_isInitialized = false;
};

} // namespace agss
