#include "navigation/NavigationUI.h"
#include <cstdio>

namespace agss {

void NavigationUI::Render(const NavigationSystem& nav, bool* isOpen) {
    if (!m_visible) return;
    
    ImGui::Begin("Navigation", isOpen);
    
    RenderStatusPanel(nav);
    ImGui::Separator();
    RenderTargetInfo(nav);
    ImGui::Separator();
    RenderControlsHelp();
    
    ImGui::End();
}

void NavigationUI::RenderStatusPanel(const NavigationSystem& nav) {
    const auto& state = nav.GetState();
    const auto& config = nav.GetConfig();
    
    // Mode indicator
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Mode: %s", 
                       NavigationSystem::GetModeName(state.mode));
    
    // Speed indicator (in AU/day and km/s for reference)
    double speedAuPerDay = state.currentSpeed;
    // 1 AU/day ≈ 1731.5 km/s
    double speedKmPerSec = speedAuPerDay * 1731.5;
    
    ImGui::Text("Speed: %.4f AU/day (%.2f km/s)", speedAuPerDay, speedKmPerSec);
    
    // Speed bar visualization
    float speedRatio = static_cast<float>(speedAuPerDay / config.maxSpeed);
    speedRatio = glm::clamp(speedRatio, 0.0f, 1.0f);
    ImGui::ProgressBar(speedRatio, ImVec2(200, 20), "");
    
    // Position (compressed for display if far)
    glm::dvec3 pos = nav.GetPosition();
    ImGui::Text("Position: (%.2f, %.2f, %.2f) AU", pos.x, pos.y, pos.z);
    
    // Camera mode
    ImGui::Text("Camera: %s", state.isThirdPerson ? "Third-Person" : "First-Person");
}

void NavigationUI::RenderTargetInfo(const NavigationSystem& nav) {
    ImGui::Text("Target Information");
    
    const auto& target = nav.GetState().currentTarget;
    
    if (target.isValid) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Target: %s", target.name.c_str());
        ImGui::Text("Distance: %.4f AU", target.distance);
        ImGui::Text("Index: %d", target.bodyIndex);
        
        // Distance bar
        float distRatio = 1.0f - static_cast<float>(glm::min(target.distance / 100.0, 1.0));
        ImGui::ProgressBar(distRatio, ImVec2(200, 20), "Proximity");
        
        // Approaching warning
        if (nav.IsApproachingBody(target.bodyIndex, 2.0)) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                               "⚠ Approaching target!");
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No target locked");
        ImGui::Text("Press 'T' to lock nearest target");
    }
}

void NavigationUI::RenderControlsHelp() {
    ImGui::Text("Controls");
    
    ImGui::BulletText("W/S: Forward/Backward");
    ImGui::BulletText("A/D: Strafe Left/Right");
    ImGui::BulletText("Q/E: Up/Down");
    ImGui::BulletText("Shift: Boost speed");
    ImGui::BulletText("Ctrl: Slow movement");
    ImGui::BulletText("Mouse: Look around");
    ImGui::BulletText("Scroll: Zoom (third-person)");
    ImGui::BulletText("C: Toggle camera mode");
    ImGui::BulletText("T: Lock nearest target");
    ImGui::BulletText("O: Toggle orbit mode");
    ImGui::BulletText("F: Toggle focus mode");
    ImGui::BulletText("N: Cycle navigation modes");
}

} // namespace agss
