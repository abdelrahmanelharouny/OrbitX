#pragma once

#include <memory>

struct GLFWwindow;

namespace agss {

class SimulationManager;
class DebugRender2D;

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

  GLFWwindow* m_window = nullptr;
  std::unique_ptr<SimulationManager> m_sim;
  std::unique_ptr<DebugRender2D> m_debugRenderer;
  bool m_running = false;

  // Interaction state for the top-down debug view
  bool m_rightDrag = false;
  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
  glm::dvec2 m_focusWorldXZ{0.0, 0.0};
  bool m_hasFocusTarget = false;
};

} // namespace agss

