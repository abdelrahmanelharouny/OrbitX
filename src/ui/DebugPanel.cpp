#include "ui/DebugPanel.h"
#include "imgui.h"

namespace agss {

DebugPanel::DebugPanel() 
    : m_drawTrails(true), m_drawVelocity(true), m_drawForces(false), m_drawCollisions(false),
      m_bodyCount(0), m_totalEnergy(0.0), m_kineticEnergy(0.0), m_potentialEnergy(0.0) {}

void DebugPanel::UpdateStatistics() {
    if (!m_sim) {
        return;
    }

    // Get energy report
    const auto& energy = m_sim->GetEnergyReport();
    m_kineticEnergy = energy.kinetic;
    m_potentialEnergy = energy.potential;
    m_totalEnergy = energy.total;

    // Get body count from physics snapshot
    auto snapshot = m_sim->GetPhysicsSnapshot();
    m_bodyCount = static_cast<int>(snapshot.positions.size());
}

void DebugPanel::Render(bool& isOpen) {
    if (!isOpen || !m_sim) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(GetPreferredWidth(), 250.0f), ImVec2(450.0f, 600.0f));

    if (ImGui::Begin("Debug Panel", &isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        // Update statistics each frame
        UpdateStatistics();

        // === Debug Visualization Toggles ===
        if (ImGui::CollapsingHeader("Visualization Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);

            // Orbit trails
            bool trailsEnabled = m_sim->DebugDrawTrails();
            if (ImGui::Checkbox("Orbit Trails", &trailsEnabled)) {
                m_sim->ToggleDebugTrails();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "(T)");

            // Velocity vectors
            bool velocityEnabled = m_sim->DebugDrawVelocity();
            if (ImGui::Checkbox("Velocity Vectors", &velocityEnabled)) {
                m_sim->ToggleDebugVelocity();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "(V)");

            // Force vectors
            bool forcesEnabled = m_sim->DebugDrawForces();
            if (ImGui::Checkbox("Force Vectors", &forcesEnabled)) {
                m_sim->ToggleDebugForces();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "(F)");

            // Collision visualization (placeholder for future)
            if (ImGui::Checkbox("Collision Bounds", &m_drawCollisions)) {
                // Future: Toggle collision visualization
            }

            ImGui::PopItemWidth();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // === System Statistics ===
        if (ImGui::CollapsingHeader("System Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);

            ImGui::Text("Celestial Bodies:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "%d", m_bodyCount);

            ImGui::Separator();

            ImGui::Text("Energy Analysis:");
            ImGui::Indent();

            ImGui::Text("Kinetic Energy:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.6e J", m_kineticEnergy);

            ImGui::Text("Potential Energy:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.6e J", m_potentialEnergy);

            ImGui::Separator();

            ImGui::Text("Total Mechanical Energy:");
            ImGui::SameLine();
            
            // Color-code energy conservation status
            float energyDrift = static_cast<float>(m_totalEnergy / (m_kineticEnergy + std::abs(m_potentialEnergy)));
            ImVec4 energyColor;
            if (std::abs(energyDrift) < 0.01f) {
                energyColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f); // Good conservation
            } else if (std::abs(energyDrift) < 0.05f) {
                energyColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f); // Acceptable
            } else {
                energyColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); // Poor conservation
            }
            
            ImGui::TextColored(energyColor, "%.6e J", m_totalEnergy);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Drift: %.4f%%)", energyDrift * 100.0f);

            ImGui::Unindent();
            ImGui::PopItemWidth();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // === Physics Configuration ===
        if (ImGui::CollapsingHeader("Physics Configuration")) {
            const auto& config = m_sim->GetPhysicsConfig();

            ImGui::Text("Integrator:");
            ImGui::SameLine();
            const char* integratorName = config.integrator == IntegratorType::RK4 ? "RK4" : "Euler";
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", integratorName);

            ImGui::Text("Fixed Timestep:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.6f days", config.fixedTimestepDays);

            ImGui::Text("G Constant:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.7f", config.gravitationalConstant);
        }

        ImGui::Separator();
        ImGui::Spacing();

        // === Performance Info ===
        if (ImGui::CollapsingHeader("Performance")) {
            ImGui::Text("FPS:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "%.1f", ImGui::GetIO().Framerate);

            ImGui::Text("Frame Time:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "%.2f ms", 1000.0f / ImGui::GetIO().Framerate);
        }

    }
    ImGui::End();
}

} // namespace agss
