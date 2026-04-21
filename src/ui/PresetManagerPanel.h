#pragma once

#include <imgui.h>
#include <functional>
#include <string>
#include <vector>

namespace agss {

/**
 * Preset Manager Panel - Scenario selection and management
 * Presets: Solar System, Chaos Mode, Binary Star, Anti-Gravity
 */
class PresetManagerPanel {
public:
    PresetManagerPanel();

    // Set callback for preset selection
    void SetOnPresetSelected(std::function<void(const std::string&)> callback) {
        m_onPresetSelected = callback;
    }

    // Render the preset manager panel
    void Render(bool& isOpen);

    // Get preferred size
    float GetPreferredWidth() const { return 240.0f; }

private:
    std::function<void(const std::string&)> m_onPresetSelected;

    // Preset definitions
    struct PresetInfo {
        std::string name;
        std::string path;
        std::string description;
    };

    std::vector<PresetInfo> m_presets;
    int m_selectedPresetIndex = 0;

    // Initialize preset list
    void InitializePresets();
};

} // namespace agss
