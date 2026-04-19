#include "navigation/NavigationUI.h"

#include <cstdio>
#include <cmath>

namespace agss {

NavigationUI::NavigationUI() : m_visible(true) {}

void NavigationUI::Render(const NavigationState& navState, const SpaceshipConfig& config) {
  if (!m_visible) return;
  
  ImGui::Begin("Navigation System");
  
  // === Status Header ===
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "=== NAVIGATION STATUS ===");
  ImGui::Separator();
  
  // Current Mode with color coding
  ImGui::Text("Mode:");
  ImGui::SameLine();
  ImVec4 modeColor = GetModeColor(navState.mode);
  ImGui::TextColored(modeColor, "%s", GetModeString(navState.mode));
  
  // Current Speed
  ImGui::Text("Speed: %s", FormatSpeed(navState.currentSpeed).c_str());
  ImGui::Text("Max Speed: %s", FormatSpeed(config.maxSpeed).c_str());
  
  // Target Information
  ImGui::Separator();
  ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "=== TARGET INFO ===");
  
  if (navState.hasTarget) {
    ImGui::Text("Target: %s", navState.targetName.c_str());
    ImGui::Text("Distance: %s", FormatDistance(navState.distanceToTarget).c_str());
    
    // Proximity warning
    if (navState.isInProximity) {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[PROXIMITY ALERT]");
    }
    if (navState.isLanding) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "[LANDING MODE ACTIVE]");
    }
  } else {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No target selected");
  }
  
  // Warp Status
  ImGui::Separator();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "=== WARP STATUS ===");
  
  ImVec4 warpColor = GetWarpColor(navState.warpState);
  ImGui::TextColored(warpColor, "Warp: %s", GetWarpStateString(navState.warpState));
  
  if (navState.isWarping) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "*** WARP ENGAGED ***");
  }
  
  // Collision Status
  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "=== COLLISION STATUS ===");
  
  if (navState.collisionDetected) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                       "COLLISION with %s!", navState.collisionObjectName.c_str());
  } else {
    ImGui::Text("Status: Clear");
  }
  
  // Position & Orientation Data (for debugging)
  ImGui::Separator();
  ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "=== SHIP DATA ===");
  ImGui::Text("Position: [%.4f, %.4f, %.4f]", 
              navState.position.x, navState.position.y, navState.position.z);
  ImGui::Text("Velocity: [%.4f, %.4f, %.4f]", 
              navState.velocity.x, navState.velocity.y, navState.velocity.z);
  ImGui::Text("Forward: [%.3f, %.3f, %.3f]", 
              navState.forward.x, navState.forward.y, navState.forward.z);
  
  ImGui::End();
}

bool NavigationUI::RenderModeSelector(NavigationMode& currentMode) {
  bool changed = false;
  
  const char* modeNames[] = {"Free Flight", "Orbit Mode", "Focus Mode", "Auto-Pilot"};
  int currentIndex = static_cast<int>(currentMode);
  
  if (ImGui::Combo("Nav Mode", &currentIndex, modeNames, IM_ARRAYSIZE(modeNames))) {
    currentMode = static_cast<NavigationMode>(currentIndex);
    changed = true;
  }
  
  return changed;
}

bool NavigationUI::RenderTargetSelector(const std::vector<std::string>& bodyNames, 
                                         int& selectedIndex) {
  bool changed = false;
  
  if (bodyNames.empty()) {
    ImGui::Text("No bodies available");
    return false;
  }
  
  // Build combo items
  std::string preview = (selectedIndex >= 0 && selectedIndex < static_cast<int>(bodyNames.size()))
                        ? bodyNames[static_cast<size_t>(selectedIndex)] 
                        : "Select target...";
  
  if (ImGui::BeginCombo("Target", preview.c_str())) {
    // None option
    if (ImGui::Selectable("None", selectedIndex < 0)) {
      selectedIndex = -1;
      changed = true;
    }
    
    for (int i = 0; i < static_cast<int>(bodyNames.size()); ++i) {
      bool isSelected = (i == selectedIndex);
      if (ImGui::Selectable(bodyNames[i].c_str(), isSelected)) {
        selectedIndex = i;
        changed = true;
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    
    ImGui::EndCombo();
  }
  
  return changed;
}

bool NavigationUI::RenderWarpControls(bool isWarping, WarpState warpState) {
  bool engaged = isWarping;
  
  ImGui::Text("Warp Drive:");
  ImGui::SameLine();
  
  ImVec4 warpColor = GetWarpColor(warpState);
  ImGui::TextColored(warpColor, "%s", GetWarpStateString(warpState));
  
  if (!isWarping) {
    if (ImGui::Button("Engage Warp")) {
      engaged = true;
    }
  } else {
    if (ImGui::Button("Disengage Warp")) {
      engaged = false;
    }
  }
  
  return engaged != isWarping;
}

bool NavigationUI::RenderCollisionSelector(CollisionResponse& currentResponse) {
  bool changed = false;
  
  const char* responseNames[] = {"None", "Soft Stop", "Bounce", "Slide"};
  int currentIndex = static_cast<int>(currentResponse);
  
  if (ImGui::Combo("Collision Response", &currentIndex, 
                   responseNames, IM_ARRAYSIZE(responseNames))) {
    currentResponse = static_cast<CollisionResponse>(currentIndex);
    changed = true;
  }
  
  return changed;
}

std::string NavigationUI::FormatSpeed(double speedAuPerDay) {
  char buffer[64];
  
  // Show in AU/day and km/s for readability
  double kmPerSecond = speedAuPerDay * 1731.456; // 1 AU/day ≈ 1731.456 km/s
  
  if (speedAuPerDay < 0.001) {
    std::snprintf(buffer, sizeof(buffer), "%.6f AU/day (%.3f km/s)", 
                  speedAuPerDay, kmPerSecond);
  } else if (speedAuPerDay < 1.0) {
    std::snprintf(buffer, sizeof(buffer), "%.4f AU/day (%.2f km/s)", 
                  speedAuPerDay, kmPerSecond);
  } else {
    std::snprintf(buffer, sizeof(buffer), "%.2f AU/day (%.1f km/s)", 
                  speedAuPerDay, kmPerSecond);
  }
  
  return std::string(buffer);
}

std::string NavigationUI::FormatDistance(double distanceAu) {
  char buffer[64];
  
  // Show in AU and km for readability
  double km = distanceAu * 149597870.7; // 1 AU ≈ 149.6 million km
  
  if (distanceAu < 0.0001) {
    std::snprintf(buffer, sizeof(buffer), "%.6f AU (%.0f km)", distanceAu, km);
  } else if (distanceAu < 1.0) {
    std::snprintf(buffer, sizeof(buffer), "%.5f AU (%.0f km)", distanceAu, km);
  } else {
    std::snprintf(buffer, sizeof(buffer), "%.3f AU (%.0f km)", distanceAu, km);
  }
  
  return std::string(buffer);
}

ImVec4 NavigationUI::GetWarpColor(WarpState state) {
  switch (state) {
    case WarpState::Inactive:
      return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
    case WarpState::Accelerating:
      return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    case WarpState::Active:
      return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
    case WarpState::Decelerating:
      return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange
    default:
      return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  }
}

ImVec4 NavigationUI::GetModeColor(NavigationMode mode) {
  switch (mode) {
    case NavigationMode::FreeFlight:
      return ImVec4(0.5f, 0.8f, 1.0f, 1.0f);  // Light blue
    case NavigationMode::OrbitMode:
      return ImVec4(0.5f, 1.0f, 0.5f, 1.0f);  // Light green
    case NavigationMode::FocusMode:
      return ImVec4(1.0f, 0.5f, 1.0f, 1.0f);  // Magenta
    case NavigationMode::AutoPilot:
      return ImVec4(1.0f, 1.0f, 0.5f, 1.0f);  // Light yellow
    default:
      return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  }
}

// Helper functions to convert enums to strings
static const char* GetModeString(NavigationMode mode) {
  switch (mode) {
    case NavigationMode::FreeFlight: return "Free Flight";
    case NavigationMode::OrbitMode: return "Orbit Mode";
    case NavigationMode::FocusMode: return "Focus Mode";
    case NavigationMode::AutoPilot: return "Auto-Pilot";
    default: return "Unknown";
  }
}

static const char* GetWarpStateString(WarpState state) {
  switch (state) {
    case WarpState::Inactive: return "Inactive";
    case WarpState::Accelerating: return "Accelerating";
    case WarpState::Active: return "ACTIVE";
    case WarpState::Decelerating: return "Decelerating";
    default: return "Unknown";
  }
}

} // namespace agss
