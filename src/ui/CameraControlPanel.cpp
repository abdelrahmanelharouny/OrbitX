#include "ui/CameraControlPanel.h"
#include "imgui.h"

namespace agss {

CameraControlPanel::CameraControlPanel() {}

void CameraControlPanel::Render(bool& isOpen) {
    if (!isOpen) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(GetPreferredWidth(), 150.0f), ImVec2(350.0f, 400.0f));

    if (ImGui::Begin("Camera Controls", &isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        // === Camera Focus Controls ===
        if (ImGui::CollapsingHeader("Focus Controls", &m_showCameraControls,
                                     ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);

            // Focus on selected object
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            
            if (ImGui::Button("🎯 Focus Selected", ImVec2(-1, 32))) {
                if (m_onFocus) {
                    m_onFocus();
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::Spacing();

            // Navigate between bodies
            ImGui::Text("Navigate Bodies:");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

            if (ImGui::Button("◀ Previous Body", ImVec2(-1, 28))) {
                if (m_onPrevBodyCallback) {
                    m_onPrevBodyCallback();
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            
            if (ImGui::Button("Next Body ▶", ImVec2(-1, 28))) {
                if (m_onNextBodyCallback) {
                    m_onNextBodyCallback();
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::Separator();
            ImGui::Spacing();

            // Reset camera
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
            
            if (ImGui::Button("↻ Reset Camera View", ImVec2(-1, 32))) {
                if (m_onReset) {
                    m_onReset();
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::PopItemWidth();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === Camera Info ===
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Camera Controls:");
        ImGui::BulletText("Right-click + drag: Pan view");
        ImGui::BulletText("Scroll wheel: Zoom in/out");
        ImGui::BulletText("Click on body: Select & focus");
        ImGui::BulletText("'F' key: Focus on selected");

    }
    ImGui::End();
}

} // namespace agss
