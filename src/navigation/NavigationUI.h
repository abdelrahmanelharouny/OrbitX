#pragma once

#include "navigation/NavigationSystem.h"
#include <imgui.h>

namespace agss {

/**
 * Navigation UI Renderer
 * Displays navigation-related information in the UI overlay
 */
class NavigationUI {
public:
    NavigationUI() = default;
    ~NavigationUI() = default;
    
    /**
     * Render navigation UI elements
     * @param nav Reference to navigation system
     * @param isOpen Pointer to bool controlling window visibility
     */
    void Render(const NavigationSystem& nav, bool* isOpen = nullptr);
    
    /**
     * Show/hide navigation UI
     */
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    void ToggleVisibility() { m_visible = !m_visible; }
    
private:
    void RenderStatusPanel(const NavigationSystem& nav);
    void RenderTargetInfo(const NavigationSystem& nav);
    void RenderControlsHelp();
    
    bool m_visible = true;
};

} // namespace agss
