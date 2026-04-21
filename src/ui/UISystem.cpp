#include "ui/UISystem.h"
#include "ui/ControlPanel.h"
#include "ui/ObjectInfoPanel.h"
#include "ui/CameraControlPanel.h"
#include "ui/PresetManagerPanel.h"
#include "ui/DebugPanel.h"
#include "ui/StatusBar.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

namespace agss {

UISystem::UISystem() : m_selectedBodyIndex(-1) {}

UISystem::~UISystem() {
    Shutdown();
}

bool UISystem::Init() {
    if (m_isInitialized) {
        return true;
    }

    // Create all UI panel components
    m_controlPanel = std::make_unique<ControlPanel>();
    m_objectInfoPanel = std::make_unique<ObjectInfoPanel>();
    m_cameraPanel = std::make_unique<CameraControlPanel>();
    m_presetPanel = std::make_unique<PresetManagerPanel>();
    m_debugPanel = std::make_unique<DebugPanel>();
    m_statusBar = std::make_unique<StatusBar>();

    // Connect panels to simulation
    if (m_sim) {
        m_controlPanel->SetSimulationManager(m_sim);
        m_objectInfoPanel->SetSimulationManager(m_sim);
        m_debugPanel->SetSimulationManager(m_sim);
        m_statusBar->SetSimulationManager(m_sim);
    }

    // Setup callbacks for camera panel
    m_cameraPanel->SetOnFocusCallback([this]() {
        if (m_sim && m_sim->HasSelection()) {
            const auto snap = m_sim->GetPhysicsSnapshot();
            int idx = m_sim->GetSelectedBodyIndex();
            if (idx >= 0 && static_cast<size_t>(idx) < snap.positions.size()) {
                // Trigger focus - this would be handled by the application
                if (m_onBodyFocused) {
                    m_onBodyFocused(idx);
                }
            }
        }
    });

    m_cameraPanel->SetOnResetCallback([this]() {
        // Reset camera to default view
        if (m_debugRenderer) {
            m_debugRenderer->SetCenterWorldXZ(glm::dvec2(0.0, 0.0));
        }
    });

    m_cameraPanel->SetOnNextBodyCallback([this]() {
        if (m_sim) {
            int currentIdx = m_sim->GetSelectedBodyIndex();
            auto snapshot = m_sim->GetPhysicsSnapshot();
            int nextIdx = (currentIdx + 1) % static_cast<int>(snapshot.positions.size());
            m_sim->SetSelectedBodyIndex(nextIdx);
            m_selectedBodyIndex = nextIdx;
            
            if (m_onBodyFocused) {
                m_onBodyFocused(nextIdx);
            }
        }
    });

    m_cameraPanel->SetOnPrevBodyCallback([this]() {
        if (m_sim) {
            int currentIdx = m_sim->GetSelectedBodyIndex();
            auto snapshot = m_sim->GetPhysicsSnapshot();
            int prevIdx = currentIdx - 1;
            if (prevIdx < 0) {
                prevIdx = static_cast<int>(snapshot.positions.size()) - 1;
            }
            m_sim->SetSelectedBodyIndex(prevIdx);
            m_selectedBodyIndex = prevIdx;
            
            if (m_onBodyFocused) {
                m_onBodyFocused(prevIdx);
            }
        }
    });

    // Setup callback for preset panel
    m_presetPanel->SetOnPresetSelected([this](const std::string& path) {
        if (m_onPresetSelected) {
            m_onPresetSelected(path);
        }
    });

    // Apply space/dark theme
    ApplySpaceTheme();

    m_isInitialized = true;
    return true;
}

void UISystem::Shutdown() {
    m_controlPanel.reset();
    m_objectInfoPanel.reset();
    m_cameraPanel.reset();
    m_presetPanel.reset();
    m_debugPanel.reset();
    m_statusBar.reset();
    
    m_isInitialized = false;
}

void UISystem::Render(int viewportW, int viewportH) {
    if (!m_isInitialized) {
        return;
    }

    // Update status bar
    if (m_statusBar) {
        m_statusBar->Update(ImGui::GetIO().Framerate);
        m_statusBar->Render();
    }

    // Render all panels
    if (m_controlPanel) {
        m_controlPanel->Render(m_showControlPanel);
    }

    if (m_objectInfoPanel) {
        m_objectInfoPanel->Render(m_showObjectInfoPanel);
    }

    if (m_cameraPanel) {
        m_cameraPanel->Render(m_showCameraPanel);
    }

    if (m_presetPanel) {
        m_presetPanel->Render(m_showPresetPanel);
    }

    if (m_debugPanel) {
        m_debugPanel->Render(m_showDebugPanel);
    }

    // Sync selection state
    if (m_sim && m_selectedBodyIndex != m_sim->GetSelectedBodyIndex()) {
        m_selectedBodyIndex = m_sim->GetSelectedBodyIndex();
    }
}

void UISystem::SetSelectedBodyIndex(int idx) {
    m_selectedBodyIndex = idx;
    if (m_sim) {
        m_sim->SetSelectedBodyIndex(idx);
    }
}

void UISystem::ApplyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.90f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}

void UISystem::ApplySpaceTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Dark space theme with subtle blue/purple accents
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.95f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.15f, 0.90f);
    
    style.Colors[ImGuiCol_Header] = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.45f, 1.00f);
    
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.35f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.45f, 0.55f, 1.00f);
    
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.22f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.38f, 1.00f);
    
    style.Colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.30f, 0.45f, 1.00f);
    
    style.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.28f, 0.35f, 0.50f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
    
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.90f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.28f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.38f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.48f, 0.55f, 1.00f);
    
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.65f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.60f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.70f, 0.90f, 1.00f);
    
    style.Colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.28f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.50f, 0.60f, 1.00f);
    
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.28f, 0.35f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.50f, 0.60f, 1.00f);
    
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.50f, 0.65f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.70f, 0.80f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.50f, 0.65f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.80f, 0.95f, 1.00f);
    
    // Styling adjustments for better appearance
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 20.0f;
    style.ColumnsMinSpacing = 6.0f;
    
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;
}

} // namespace agss
