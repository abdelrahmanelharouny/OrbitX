#include "ui/StatusBar.h"
#include "imgui.h"
#include <cstdio>

namespace agss {

StatusBar::StatusBar() 
    : m_fps(0.0f), m_simulationTime(0.0), m_bodyCount(0), m_isPaused(false), 
      m_timeScale(1.0), m_currentPreset("Solar System") {}

void StatusBar::Update(float fps) {
    m_fps = fps;
    
    if (m_sim) {
        m_isPaused = m_sim->IsPaused();
        m_timeScale = m_sim->GetTimeScale();
        
        auto snapshot = m_sim->GetPhysicsSnapshot();
        m_bodyCount = static_cast<int>(snapshot.positions.size());
    }
}

void StatusBar::Render() {
    // Status bar at bottom of screen
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 30));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 30));
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20, 0));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNav;
    
    if (ImGui::Begin("StatusBar", nullptr, flags)) {
        
        // Simulation state indicator
        if (m_isPaused) {
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.0f, 1.0f), "⏸ PAUSED");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "▶ Running");
        }
        
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        
        // Time scale
        ImGui::Text("Time Scale:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%.2fx", m_timeScale);
        
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        
        // Body count
        ImGui::Text("Bodies:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%d", m_bodyCount);
        
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        
        // FPS
        ImGui::Text("FPS:");
        ImGui::SameLine();
        
        ImVec4 fpsColor;
        if (m_fps >= 55.0f) {
            fpsColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
        } else if (m_fps >= 30.0f) {
            fpsColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f);
        } else {
            fpsColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        }
        ImGui::TextColored(fpsColor, "%.1f", m_fps);
        
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        
        // Current preset
        ImGui::Text("Scenario:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%s", m_currentPreset.c_str());
        
        // Right-aligned info
        float cursorX = ImGui::GetCursorPosX();
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(cursorX + availWidth - 150);
        
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Anti-Gravity Solar System v0.1");
        
    }
    ImGui::End();
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

} // namespace agss
