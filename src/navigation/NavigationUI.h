#pragma once

#include "navigation/SpaceshipNavigation.h"

#include <imgui.h>
#include <string>

namespace agss {

/**
 * @brief Navigation UI System
 * 
 * Provides ImGui-based UI for navigation feedback and control.
 * Displays speed, target info, distance, active mode, and warp status.
 */
class NavigationUI {
public:
  NavigationUI();
  
  /**
   * @brief Render the navigation UI panel
   * @param navState Current navigation state to display
   * @param config Current spaceship configuration
   */
  void Render(const NavigationState& navState, const SpaceshipConfig& config);
  
  /**
   * @brief Render mode selection buttons
   * @param currentMode Current navigation mode
   * @return true if mode was changed
   */
  bool RenderModeSelector(NavigationMode& currentMode);
  
  /**
   * @brief Render target selection dropdown
   * @param bodyNames List of available body names
   * @param selectedIndex Currently selected index
   * @return true if selection changed
   */
  bool RenderTargetSelector(const std::vector<std::string>& bodyNames, int& selectedIndex);
  
  /**
   * @brief Render warp controls
   * @param isWarping Current warp state
   * @param warpState Warp state enum
   * @return true if warp state changed
   */
  bool RenderWarpControls(bool isWarping, WarpState warpState);
  
  /**
   * @brief Render collision response selector
   * @param currentResponse Current collision response mode
   * @return true if response mode changed
   */
  bool RenderCollisionSelector(CollisionResponse& currentResponse);
  
  // Toggle UI visibility
  void SetVisible(bool visible) { m_visible = visible; }
  bool IsVisible() const { return m_visible; }
  void ToggleVisibility() { m_visible = !m_visible; }

private:
  bool m_visible = true;
  
  // Helper to format speed with units
  static std::string FormatSpeed(double speedAuPerDay);
  
  // Helper to format distance with units
  static std::string FormatDistance(double distanceAu);
  
  // Color helpers for different states
  static ImVec4 GetWarpColor(WarpState state);
  static ImVec4 GetModeColor(NavigationMode mode);
};

} // namespace agss
