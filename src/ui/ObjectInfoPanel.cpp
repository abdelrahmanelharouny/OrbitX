#include "ui/ObjectInfoPanel.h"
#include "imgui.h"
#include <cstdio>

namespace agss {

ObjectInfoPanel::ObjectInfoPanel() 
    : m_cachedMass(0.0), m_cachedSpeed(0.0), m_cachedDistance(0.0), m_lastSelectedIndex(-1) {}

void ObjectInfoPanel::UpdateCachedInfo() {
    if (!m_sim || !m_sim->HasSelection()) {
        return;
    }

    auto info = m_sim->GetSelectedBodyInfo();
    m_cachedName = info.name;
    m_cachedMass = info.mass;
    m_cachedSpeed = info.speed;
    m_cachedDistance = info.distanceFromPrimary;
}

void ObjectInfoPanel::Render(bool& isOpen) {
    if (!isOpen) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(GetPreferredWidth(), GetPreferredMinHeight()), 
                                         ImVec2(400.0f, 500.0f));

    if (ImGui::Begin("Object Information", &isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        if (!m_sim) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Simulation not initialized");
            ImGui::End();
            return;
        }

        // Check if selection changed
        int currentIndex = m_sim->GetSelectedBodyIndex();
        if (currentIndex != m_lastSelectedIndex) {
            m_lastSelectedIndex = currentIndex;
            UpdateCachedInfo();
        }

        if (!m_sim->HasSelection()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No object selected");
            ImGui::Spacing();
            ImGui::TextWrapped("Click on a celestial body in the viewport to view its information.");
            
            ImGui::Separator();
            ImGui::Text("Tip: Use right-click + drag to pan, scroll to zoom");
            
        } else {
            // Body name header
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
            ImGui::TextLarge("%s", m_cachedName.c_str());
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            ImGui::Spacing();

            // Physical properties
            ImGui::Text("Physical Properties:");
            ImGui::Indent();
            
            ImGui::Text("Mass:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.6e kg", m_cachedMass);
            
            ImGui::Text("Velocity:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.6e AU/day", m_cachedSpeed);
            
            // Convert to km/s for better understanding
            double velocityKms = m_cachedSpeed * 149597870.7 / 86400.0; // AU/day to km/s
            ImGui::Text("         ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "(%.2f km/s)", velocityKms);
            
            ImGui::Text("Distance from Star:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.6f AU", m_cachedDistance);
            
            // Convert to kilometers
            double distanceKm = m_cachedDistance * 149597870.7; // AU to km
            ImGui::Text("         ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "(%.2e km)", distanceKm);
            
            ImGui::Unindent();
            
            ImGui::Separator();
            ImGui::Spacing();

            // Action buttons
            ImGui::Text("Actions:");
            ImGui::PushItemWidth(-1);
            
            if (ImGui::Button("🎯 Focus Camera", ImVec2(-1, 30))) {
                // Trigger focus action - will be handled by parent
                const auto snap = m_sim->GetPhysicsSnapshot();
                if (currentIndex >= 0 && static_cast<size_t>(currentIndex) < snap.positions.size()) {
                    // This would trigger a camera focus callback in full implementation
                }
            }
            
            ImGui::Spacing();
            
            if (ImGui::Button("📊 Show Orbit Trail", ImVec2(-1, 30))) {
                m_sim->ToggleDebugTrails();
            }
            
            if (ImGui::Button("⚡ Show Velocity Vector", ImVec2(-1, 30))) {
                m_sim->ToggleDebugVelocity();
            }
            
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Additional info (placeholder for future expansion)
            if (ImGui::CollapsingHeader("Orbital Elements (Coming Soon)")) {
                ImGui::Text("Semi-major axis: --");
                ImGui::Text("Eccentricity: --");
                ImGui::Text("Orbital period: --");
            }
        }

    }
    ImGui::End();
}

} // namespace agss
