#pragma once

#include <memory>

struct GLFWwindow;

namespace agss {

class SimulationManager;
class DebugRender2D;
class NavigationSystem;
class NavigationUI;
class SimulationNavigationAdapter;

class Application {
public:
  bool Init();
  void Run();
  void Shutdown();

private:
  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  void OnKey(int key, int action);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  void OnMouseButton(int button, int action, int mods);
  void OnScroll(double yoffset);
  void UpdateCameraAndPicking();
  void UpdateNavigation(double deltaTime);
  void HandleNavigationInput();

  GLFWwindow* m_window = nullptr;
  std::unique_ptr<SimulationManager> m_sim;
  std::unique_ptr<DebugRender2D> m_debugRenderer;
  std::unique_ptr<NavigationSystem> m_nav;
  std::unique_ptr<NavigationUI> m_navUI;
  std::unique_ptr<SimulationNavigationAdapter> m_navAdapter;
  bool m_running = false;
  
  bool m_navigationEnabled = true;  // Toggle between nav mode and top-down view

  // Interaction state for the top-down debug view
  bool m_rightDrag = false;
  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
  glm::dvec2 m_focusWorldXZ{0.0, 0.0};
  bool m_hasFocusTarget = false;
  
  // Navigation input state
  bool m_keyForward = false;
  bool m_keyBackward = false;
  bool m_keyLeft = false;
  bool m_keyRight = false;
  bool m_keyUp = false;
  bool m_keyDown = false;
  bool m_keyBoost = false;
  bool m_keySlow = false;
};

} // namespace agss

