#include "ui/PresetManagerPanel.h"
#include "imgui.h"

namespace agss {

PresetManagerPanel::PresetManagerPanel() : m_selectedPresetIndex(0) {
    InitializePresets();
}

void PresetManagerPanel::InitializePresets() {
    m_presets = {
        {"Solar System", "assets/data/solar_system.json", 
         "Our solar system with Sun and planets"},
        {"Binary Star", "assets/data/presets/binary_star.json", 
         "Two stars orbiting their common center of mass"},
        {"Chaos Mode", "assets/data/presets/chaos.json", 
         "Randomized bodies with chaotic trajectories"},
        {"Anti-Gravity Demo", "assets/data/presets/anti_grav.json", 
         "Demonstration of repulsive gravitational forces"}
    };
}

void PresetManagerPanel::Render(bool& isOpen) {
    if (!isOpen) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(GetPreferredWidth(), 200.0f), ImVec2(400.0f, 500.0f));

    if (ImGui::Begin("Scenario Presets", &isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Select a Scenario:");
        ImGui::Separator();
        ImGui::Spacing();

        // Preset list with descriptions
        ImGui::PushItemWidth(-1);
        
        for (size_t i = 0; i < m_presets.size(); ++i) {
            const auto& preset = m_presets[i];
            
            // Create a selectable item with description
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.4f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.5f, 0.6f, 1.0f));
            
            bool isSelected = (static_cast<int>(i) == m_selectedPresetIndex);
            
            // Begin a group for each preset item
            ImGui::PushID(static_cast<int>(i));
            
            // Create a button-like selectable
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
            
            char labelBuffer[256];
            std::snprintf(labelBuffer, sizeof(labelBuffer), "%s", preset.name.c_str());
            
            if (ImGui::Selectable(labelBuffer, isSelected, ImGuiSelectableFlags_AllowDoubleClick, 
                                  ImVec2(0, 45))) {
                m_selectedPresetIndex = static_cast<int>(i);
                
                // Trigger callback to load preset
                if (m_onPresetSelected) {
                    m_onPresetSelected(preset.path);
                }
            }
            
            // Show description below the name
            if (ImGui::IsItemVisible()) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("%s", preset.description.c_str());
                ImGui::PopStyleColor();
            }
            
            ImGui::PopStyleVar();
            ImGui::PopID();
            ImGui::PopStyleColor(3);
            
            // Add a small separator between items
            if (i < m_presets.size() - 1) {
                ImGui::Separator();
            }
        }
        
        ImGui::PopItemWidth();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Quick load buttons (alternative UI)
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Quick Load:");
        ImGui::Spacing();
        
        ImGui::PushItemWidth(-1);
        for (size_t i = 0; i < m_presets.size(); ++i) {
            char btnLabel[64];
            std::snprintf(btnLabel, sizeof(btnLabel), "%zu) %s", i + 1, m_presets[i].name.c_str());
            
            if (ImGui::Button(btnLabel, ImVec2(-1, 28))) {
                m_selectedPresetIndex = static_cast<int>(i);
                if (m_onPresetSelected) {
                    m_onPresetSelected(m_presets[i].path);
                }
            }
        }
        ImGui::PopItemWidth();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Info text
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                          "Note: Loading a preset will reset the simulation.");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                          "You can also use number keys (1-4) for quick access.");

    }
    ImGui::End();
}

} // namespace agss
