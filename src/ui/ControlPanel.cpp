#include "ui/ControlPanel.h"
#include "imgui.h"

namespace agss {

ControlPanel::ControlPanel() : m_gravitySliderValue(0.0f), m_timePresetIndex(1) {}

void ControlPanel::Render(bool& isOpen) {
    if (!isOpen || !m_sim) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(GetPreferredWidth(), 300.0f), ImVec2(500.0f, 800.0f));
    
    if (ImGui::Begin("Control Panel", &isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        // === Simulation Controls ===
        if (ImGui::CollapsingHeader("Simulation Controls", &m_showSimulationControls, 
                                     ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            
            // Play/Pause button
            bool isPaused = m_sim->IsPaused();
            ImVec4 buttonColor = isPaused ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(0.9f, 0.7f, 0.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 
                                  isPaused ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.8f, 0.1f, 1.0f));
            
            if (ImGui::Button(isPaused ? "▶ Play" : "⏸ Pause", ImVec2(-1, 35))) {
                m_sim->TogglePaused();
            }
            ImGui::PopStyleColor(2);
            
            ImGui::SameLine();
            
            // Reset button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("↻ Reset", ImVec2(-1, 35))) {
                m_sim->ResetToPreset("assets/data/solar_system.json");
            }
            ImGui::PopStyleColor(2);
            
            // Time scale slider with presets
            ImGui::Separator();
            ImGui::Text("Time Scale");
            
            const char* speedLabels[] = {"x0.1", "x1", "x10", "x100", "x1000"};
            int currentPreset = static_cast<int>(m_sim->GetSpeedPresetIndex());
            if (currentPreset < 0) currentPreset = 0;
            if (currentPreset > 4) currentPreset = 4;
            
            if (ImGui::Combo("##TimeScale", &currentPreset, speedLabels, IM_ARRAYSIZE(speedLabels))) {
                m_sim->SetSpeedPresetIndex(static_cast<size_t>(currentPreset));
            }
            
            ImGui::Text("Current: %.3fx", m_sim->GetTimeScale());
            
            // Fine-tune time scale
            float timeScale = static_cast<float>(m_sim->GetTimeScale());
            if (ImGui::SliderFloat("Fine Adjust", &timeScale, 0.01f, 2000.0f, "%.2fx")) {
                // Find closest preset or set custom
                m_sim->SetTimeScale(static_cast<double>(timeScale));
            }
            
            ImGui::PopItemWidth();
        }
        
        // === Physics Controls ===
        if (ImGui::CollapsingHeader("Physics Controls", &m_showPhysicsControls,
                                     ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            
            // Anti-gravity toggle
            bool antiGravity = m_sim->IsAntiGravityEnabled();
            ImVec4 agColor = antiGravity ? ImVec4(0.7f, 0.3f, 0.9f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_CheckBox, agColor);
            ImGui::PushStyleColor(ImGuiCol_CheckBoxHovered, 
                                  antiGravity ? ImVec4(0.8f, 0.4f, 1.0f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            
            if (ImGui::Checkbox("Anti-Gravity (Repulsive)", &antiGravity)) {
                m_sim->ToggleAntiGravity();
            }
            ImGui::PopStyleColor(2);
            
            // Gravitational constant slider
            ImGui::Separator();
            ImGui::Text("Gravitational Constant (G)");
            
            double g = m_sim->GetGravityConstant();
            m_gravitySliderValue = static_cast<float>(g);
            
            if (ImGui::SliderFloat("##Gravity", &m_gravitySliderValue, 0.0f, 0.002f, "%.6f")) {
                m_sim->SetGravityConstant(static_cast<double>(m_gravitySliderValue));
            }
            
            ImGui::Text("Current G: %.7f", g);
            
            // Quick G adjustments
            ImGui::Spacing();
            ImGui::Text("Quick Adjust:");
            ImGui::SameLine();
            if (ImGui::SmallButton("-10%")) {
                m_sim->SetGravityConstant(g * 0.9);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("+10%")) {
                m_sim->SetGravityConstant(g * 1.1);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset")) {
                m_sim->SetGravityConstant(6.674e-11); // Default value
            }
            
            // Integrator selector (advanced)
            if (ImGui::CollapsingHeader("Integrator (Advanced)", &m_showIntegratorSelector)) {
                const char* integratorNames[] = {"Euler", "RK4"};
                int currentIntegrator = static_cast<int>(m_sim->GetIntegrator());
                
                if (ImGui::Combo("Integration Method", &currentIntegrator, 
                                 integratorNames, IM_ARRAYSIZE(integratorNames))) {
                    m_sim->SetIntegrator(static_cast<IntegratorType>(currentIntegrator));
                }
            }
            
            ImGui::PopItemWidth();
        }
        
        // === Collision Settings ===
        if (ImGui::CollapsingHeader("Collision Settings")) {
            ImGui::Text("Collision detection status will be shown here");
            // Future: Add collision toggle and visualization options
        }
        
    }
    ImGui::End();
}

} // namespace agss
