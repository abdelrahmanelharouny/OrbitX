#include "core/Application.h"

#include "core/Timer.h"
#include "simulation/SimulationManager.h"
#include "rendering/DebugRender2D.h"
#include "navigation/NavigationSystem.h"
#include "navigation/NavigationUI.h"
#include "navigation/SimulationNavigationAdapter.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/geometric.hpp>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>

namespace agss {

static void GlfwErrorCallback(int error, const char* description) {
  std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool Application::Init() {
  glfwSetErrorCallback(GlfwErrorCallback);
  if (!glfwInit()) {
    std::fprintf(stderr, "Failed to init GLFW\n");
    return false;
  }

  // Bootstrap: keep compatibility profile so we don't require a loader yet.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  m_window = glfwCreateWindow(1280, 720, "Anti-Gravity Solar System - Navigation", nullptr, nullptr);
  if (!m_window) {
    std::fprintf(stderr, "Failed to create window\n");
    return false;
  }

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(1);

  glfwSetWindowUserPointer(m_window, this);
  glfwSetKeyCallback(m_window, &Application::KeyCallback);
  glfwSetMouseButtonCallback(m_window, &Application::MouseButtonCallback);
  glfwSetScrollCallback(m_window, &Application::ScrollCallback);

  m_sim = std::make_unique<SimulationManager>();
  if (!m_sim->Init("assets/data/solar_system.json")) {
    std::fprintf(stderr, "Failed to init simulation\n");
    return false;
  }

  m_debugRenderer = std::make_unique<DebugRender2D>();
  m_debugRenderer->SetCenterWorldXZ(glm::dvec2(0.0, 0.0));

  // Initialize navigation system
  m_nav = std::make_unique<NavigationSystem>();
  m_navAdapter = std::make_unique<SimulationNavigationAdapter>(m_sim.get());
  
  NavigationConfig navConfig;
  if (!m_nav->Init(navConfig, m_navAdapter.get())) {
    std::fprintf(stderr, "Failed to init navigation system\n");
    return false;
  }
  
  m_navUI = std::make_unique<NavigationUI>();

  // ImGui control panel (OpenGL2 backend to match current context)
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL2_Init();

  m_running = true;
  return true;
}

void Application::Run() {
  double nextEnergyPrint = 0.0;
  double prevTime = Timer::NowSeconds();
  
  while (m_running && m_window && !glfwWindowShouldClose(m_window)) {
    glfwPollEvents();
    
    // Calculate delta time in days for navigation consistency
    double currentTime = Timer::NowSeconds();
    double deltaTimeSec = currentTime - prevTime;
    prevTime = currentTime;
    
    // Convert to days (simulation uses AU/day)
    double deltaTimeDays = deltaTimeSec / 86400.0 * m_sim->GetTimeScale();
    
    m_sim->TickFrame();
    
    // Handle navigation input and update
    if (m_navigationEnabled && m_nav) {
      HandleNavigationInput();
      UpdateNavigation(deltaTimeDays);
    }
    
    UpdateCameraAndPicking();

    // Minimal render: clear color changes if anti-gravity enabled.
    if (m_sim->IsAntiGravityEnabled()) {
      glClearColor(0.15f, 0.1f, 0.2f, 1.0f);
    } else {
      glClearColor(0.05f, 0.07f, 0.10f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // UI frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Control panel
    ImGui::Begin("Simulation Controls");
    {
      if (ImGui::Button(m_sim->IsPaused() ? "Play" : "Pause")) {
        m_sim->TogglePaused();
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
        m_sim->ResetToPreset("assets/data/solar_system.json");
      }

      // Speed presets
      const char* speedLabels[] = {"x0.1", "x1", "x10", "x100", "x1000"};
      int preset = static_cast<int>(m_sim->GetSpeedPresetIndex());
      if (preset < 0) preset = 0;
      if (preset > 4) preset = 4;
      if (ImGui::Combo("Time scale", &preset, speedLabels, IM_ARRAYSIZE(speedLabels))) {
        m_sim->SetSpeedPresetIndex(static_cast<size_t>(preset));
      }

      ImGui::Text("Current scale: %.3fx", m_sim->GetTimeScale());

      bool anti = m_sim->IsAntiGravityEnabled();
      if (ImGui::Checkbox("Anti-gravity (repulsive)", &anti)) {
        m_sim->ToggleAntiGravity();
      }

      double g = m_sim->GetGravityConstant();
      float gf = static_cast<float>(g);
      if (ImGui::SliderFloat("G (scaled)", &gf, 0.0f, 0.0015f, "%.7f")) {
        m_sim->SetGravityConstant(static_cast<double>(gf));
      }
      
      // Navigation toggle
      ImGui::Separator();
      bool navEnabled = m_navigationEnabled;
      if (ImGui::Checkbox("Enable Navigation Mode", &navEnabled)) {
        m_navigationEnabled = navEnabled;
      }
      ImGui::Text("Press 'M' to toggle navigation mode");

      // Presets
      if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("1) Solar System")) {
          m_sim->ResetToPreset("assets/data/solar_system.json");
        }
        if (ImGui::Button("2) Binary Star")) {
          m_sim->ResetToPreset("assets/data/presets/binary_star.json");
        }
        if (ImGui::Button("3) Chaos")) {
          m_sim->ResetToPreset("assets/data/presets/chaos.json");
        }
        if (ImGui::Button("4) Anti-grav Demo")) {
          m_sim->ResetToPreset("assets/data/presets/anti_grav.json");
        }
      }

      // Debug toggles
      if (ImGui::CollapsingHeader("Debug Draw", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool dbg = m_sim->IsDebugEnabled();
        if (ImGui::Checkbox("Enable debug", &dbg)) {
          m_sim->ToggleDebugEnabled();
        }
        bool trails = m_sim->DebugDrawTrails();
        if (ImGui::Checkbox("Orbit trails", &trails)) {
          m_sim->ToggleDebugTrails();
        }
        bool vel = m_sim->DebugDrawVelocity();
        if (ImGui::Checkbox("Velocity vectors", &vel)) {
          m_sim->ToggleDebugVelocity();
        }
        bool frc = m_sim->DebugDrawForces();
        if (ImGui::Checkbox("Force vectors", &frc)) {
          m_sim->ToggleDebugForces();
        }
      }

      // Energy report
      const auto& e = m_sim->GetEnergyReport();
      ImGui::Separator();
      ImGui::Text("Energy");
      ImGui::Text("KE:    %.6e", e.kinetic);
      ImGui::Text("PE:    %.6e", e.potential);
      ImGui::Text("Total: %.6e", e.total);

      // Selected body inspector
      ImGui::Separator();
      ImGui::Text("Selection");
      if (m_sim->HasSelection()) {
        const auto info = m_sim->GetSelectedBodyInfo();
        ImGui::Text("Name: %s", info.name.c_str());
        ImGui::Text("Mass: %.6e", info.mass);
        ImGui::Text("Speed: %.6e AU/day", info.speed);
        ImGui::Text("Dist to primary: %.6f AU", info.distanceFromPrimary);
        if (ImGui::Button("Focus")) {
          // same behavior as 'F'
          const auto snap = m_sim->GetPhysicsSnapshot();
          const int idx = m_sim->GetSelectedBodyIndex();
          if (idx >= 0 && static_cast<size_t>(idx) < snap.positions.size()) {
            m_focusWorldXZ = glm::dvec2(snap.positions[static_cast<size_t>(idx)].x,
                                        snap.positions[static_cast<size_t>(idx)].z);
            m_hasFocusTarget = true;
          }
        }
      } else {
        ImGui::Text("Click a body to select.");
      }
    }
    ImGui::End();
    
    // Navigation UI (only when navigation is enabled)
    if (m_navigationEnabled && m_navUI && m_nav) {
      m_navUI->Render(*m_nav);
    }

    // Debug overlay rendering (top-down XZ)
    if (m_debugRenderer && m_sim->IsDebugEnabled() && !m_navigationEnabled) {
      int w = 0, h = 0;
      glfwGetFramebufferSize(m_window, &w, &h);

      const auto snap = m_sim->GetPhysicsSnapshot();
      const auto trails = m_sim->BuildTrailRenderData(0.05f);

      m_debugRenderer->SetDrawTrails(m_sim->DebugDrawTrails());
      m_debugRenderer->SetDrawVelocity(m_sim->DebugDrawVelocity());
      m_debugRenderer->SetDrawForce(m_sim->DebugDrawForces());
      m_debugRenderer->SetSelectedIndex(m_sim->GetSelectedBodyIndex());
      m_debugRenderer->Render(w, h, snap.positions, snap.velocities, snap.forces, trails);
    }

    // Render UI
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    // Periodic energy print (useful even before UI exists).
    const double now = Timer::NowSeconds();
    if (now >= nextEnergyPrint) {
      const auto& e = m_sim->GetEnergyReport();
      std::fprintf(stdout, "Energy: KE=%.6e  PE=%.6e  Total=%.6e\n", e.kinetic, e.potential, e.total);
      nextEnergyPrint = now + 1.0;
    }

    glfwSwapBuffers(m_window);
  }
}

void Application::Shutdown() {
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  m_nav.reset();
  m_navUI.reset();
  m_navAdapter.reset();
  m_sim.reset();
  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
  glfwTerminate();
}

void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)scancode;
  (void)mods;
  if (auto* self = static_cast<Application*>(glfwGetWindowUserPointer(window))) {
    self->OnKey(key, action);
  }
}

void Application::OnKey(int key, int action) {
  // Handle key release for navigation input tracking
  bool keyReleased = (action == GLFW_RELEASE);
  
  // Track navigation key states for continuous input
  if (m_navigationEnabled) {
    switch (key) {
      case GLFW_KEY_W:
        m_keyForward = !keyReleased;
        break;
      case GLFW_KEY_S:
        m_keyBackward = !keyReleased;
        break;
      case GLFW_KEY_A:
        m_keyLeft = !keyReleased;
        break;
      case GLFW_KEY_D:
        m_keyRight = !keyReleased;
        break;
      case GLFW_KEY_Q:
        m_keyUp = !keyReleased;
        break;
      case GLFW_KEY_E:
        m_keyDown = !keyReleased;
        break;
      case GLFW_KEY_LEFT_SHIFT:
      case GLFW_KEY_RIGHT_SHIFT:
        m_keyBoost = !keyReleased;
        break;
      case GLFW_KEY_LEFT_CONTROL:
      case GLFW_KEY_RIGHT_CONTROL:
        m_keySlow = !keyReleased;
        break;
    }
  }
  
  // Only process press events for toggle actions
  if (action != GLFW_PRESS) {
    return;
  }

  if (ImGui::GetIO().WantCaptureKeyboard) {
    return;
  }

  if (key == GLFW_KEY_ESCAPE) {
    m_running = false;
    return;
  }

  if (!m_sim) {
    return;
  }

  // Navigation mode toggle
  if (key == GLFW_KEY_M) {
    m_navigationEnabled = !m_navigationEnabled;
    // Reset key states when toggling
    m_keyForward = m_keyBackward = m_keyLeft = m_keyRight = false;
    m_keyUp = m_keyDown = m_keyBoost = m_keySlow = false;
    return;
  }

  // Navigation-specific controls (only when navigation is enabled)
  if (m_navigationEnabled && m_nav) {
    // Mode cycling
    if (key == GLFW_KEY_N) {
      auto currentMode = m_nav->GetNavigationMode();
      switch (currentMode) {
        case NavigationMode::FreeFlight:
          m_nav->SetNavigationMode(NavigationMode::Orbit);
          break;
        case NavigationMode::Orbit:
          m_nav->SetNavigationMode(NavigationMode::Focus);
          break;
        case NavigationMode::Focus:
          m_nav->SetNavigationMode(NavigationMode::FreeFlight);
          break;
      }
    }
    
    // Lock nearest target
    if (key == GLFW_KEY_T) {
      int nearestIdx = m_nav->FindNearestTarget(m_nav->GetConfig().targetLockDistance);
      if (nearestIdx >= 0) {
        m_nav->LockTarget(nearestIdx);
      }
    }
    
    // Toggle orbit mode
    if (key == GLFW_KEY_O) {
      if (m_nav->GetNavigationMode() == NavigationMode::Orbit) {
        m_nav->SetNavigationMode(NavigationMode::FreeFlight);
      } else {
        // Lock to selected body if available
        if (m_sim->HasSelection()) {
          m_navAdapter->LockToSelectedBody(*m_nav);
        }
        m_nav->SetNavigationMode(NavigationMode::Orbit);
      }
    }
    
    // Toggle focus mode - use different key to avoid conflict with debug forces
    if (key == GLFW_KEY_K) {
      if (m_nav->GetNavigationMode() == NavigationMode::Focus) {
        m_nav->SetNavigationMode(NavigationMode::FreeFlight);
      } else {
        if (m_sim->HasSelection()) {
          m_navAdapter->LockToSelectedBody(*m_nav);
        }
        m_nav->SetNavigationMode(NavigationMode::Focus);
      }
    }
    
    // Toggle camera mode
    if (key == GLFW_KEY_C) {
      m_nav->ToggleCameraMode();
    }
  }

  // Original simulation controls
  if (key == GLFW_KEY_SPACE) {
    m_sim->TogglePaused();
  } else if (key == GLFW_KEY_B) {
    m_sim->ToggleAntiGravity();
  } else if (key == GLFW_KEY_EQUAL) { // speed up (next preset)
    m_sim->IncreaseSpeedPreset();
  } else if (key == GLFW_KEY_MINUS) { // slow down (previous preset)
    m_sim->DecreaseSpeedPreset();
  } else if (key == GLFW_KEY_G) { // increase G
    m_sim->SetGravityConstant(m_sim->GetGravityConstant() * 1.1);
  } else if (key == GLFW_KEY_H) { // decrease G
    m_sim->SetGravityConstant(m_sim->GetGravityConstant() / 1.1);
  } else if (key == GLFW_KEY_D) {
    m_sim->ToggleDebugEnabled();
  } else if (key == GLFW_KEY_L) {
    m_sim->ToggleDebugTrails();
  } else if (key == GLFW_KEY_V) {
    m_sim->ToggleDebugVelocity();
  } else if (key == GLFW_KEY_J) {
    m_sim->ToggleDebugForces();
  } else if (key == GLFW_KEY_1) {
    m_sim->ResetToPreset("assets/data/solar_system.json");
  } else if (key == GLFW_KEY_2) {
    m_sim->ResetToPreset("assets/data/presets/binary_star.json");
  } else if (key == GLFW_KEY_3) {
    m_sim->ResetToPreset("assets/data/presets/chaos.json");
  } else if (key == GLFW_KEY_4) {
    m_sim->ResetToPreset("assets/data/presets/anti_grav.json");
  }
}

void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  if (auto* self = static_cast<Application*>(glfwGetWindowUserPointer(window))) {
    self->OnMouseButton(button, action, mods);
  }
}

void Application::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  (void)xoffset;
  if (auto* self = static_cast<Application*>(glfwGetWindowUserPointer(window))) {
    self->OnScroll(yoffset);
  }
}

void Application::OnMouseButton(int button, int action, int mods) {
  (void)mods;
  if (!m_sim || !m_debugRenderer) return;

  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  double x = 0.0, y = 0.0;
  glfwGetCursorPos(m_window, &x, &y);
  m_lastMouseX = x;
  m_lastMouseY = y;

  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    m_rightDrag = (action == GLFW_PRESS);
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    int w = 0, h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    const auto snap = m_sim->GetPhysicsSnapshot();

    // Pick in screen-space against body projected points.
    int bestIdx = -1;
    double bestDist2 = 0.0;
    const double pickRadiusPx = 12.0;
    const double pickRadius2 = pickRadiusPx * pickRadiusPx;
    for (size_t i = 0; i < snap.positions.size(); ++i) {
      const auto s = m_debugRenderer->WorldXZToScreen(snap.positions[i], w, h);
      const double dx = s.x - x;
      const double dy = s.y - y;
      const double d2 = dx * dx + dy * dy;
      if (d2 <= pickRadius2 && (bestIdx < 0 || d2 < bestDist2)) {
        bestIdx = static_cast<int>(i);
        bestDist2 = d2;
      }
    }

    m_sim->SetSelectedBodyIndex(bestIdx);
    if (bestIdx >= 0) {
      // Auto focus target (smooth)
      m_focusWorldXZ = glm::dvec2(snap.positions[static_cast<size_t>(bestIdx)].x,
                                  snap.positions[static_cast<size_t>(bestIdx)].z);
      m_hasFocusTarget = true;
    }
  }
}

void Application::OnScroll(double yoffset) {
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  
  // In navigation mode, scroll controls camera zoom
  if (m_navigationEnabled && m_nav) {
    m_nav->ProcessScroll(static_cast<float>(yoffset * 5.0f));
    return;
  }
  
  // In top-down mode, scroll controls debug view scale
  if (!m_debugRenderer) return;
  double scale = m_debugRenderer->GetScale();
  const double factor = (yoffset > 0.0) ? 1.1 : 1.0 / 1.1;
  scale *= factor;
  scale = std::clamp(scale, 30.0, 2000.0);
  m_debugRenderer->SetScale(scale);
}

void Application::UpdateCameraAndPicking() {
  if (!m_debugRenderer) return;

  // Right-drag pans the camera in world units.
  if (m_rightDrag) {
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);
    const double dx = x - m_lastMouseX;
    const double dy = y - m_lastMouseY;
    m_lastMouseX = x;
    m_lastMouseY = y;

    const double inv = 1.0 / m_debugRenderer->GetScale();
    auto c = m_debugRenderer->GetCenterWorldXZ();
    c.x -= dx * inv;
    c.y -= dy * inv;
    m_debugRenderer->SetCenterWorldXZ(c);
    m_hasFocusTarget = false; // manual override
  }

  // Smooth focus to target (camera "orbit around selected" in top-down)
  if (m_hasFocusTarget) {
    auto c = m_debugRenderer->GetCenterWorldXZ();
    const glm::dvec2 d = m_focusWorldXZ - c;
    const double k = 0.12; // smoothing factor per frame
    c += d * k;
    m_debugRenderer->SetCenterWorldXZ(c);
    if (glm::length(d) < 1e-4) {
      m_hasFocusTarget = false;
    }
  }
}

void Application::UpdateNavigation(double deltaTime) {
  if (!m_nav) return;
  
  // Update navigation system with delta time
  m_nav->Update(deltaTime);
}

void Application::HandleNavigationInput() {
  if (!m_nav) return;
  
  // Get current mouse position for mouse look
  double x = 0.0, y = 0.0;
  glfwGetCursorPos(m_window, &x, &y);
  
  // Calculate mouse delta for look control
  float deltaX = static_cast<float>(x - m_lastMouseX);
  float deltaY = static_cast<float>(y - m_lastMouseY);
  
  // Only apply mouse look when right mouse button is held (to avoid conflict with UI)
  if (m_rightDrag) {
    m_nav->ProcessMouseMovement(deltaX, deltaY);
  }
  
  // Update last mouse position
  m_lastMouseX = x;
  m_lastMouseY = y;
  
  // Set keyboard input state
  m_nav->SetKeyboardInput(
    m_keyForward, m_keyBackward,
    m_keyLeft, m_keyRight,
    m_keyUp, m_keyDown,
    m_keyBoost, m_keySlow
  );
}

} // namespace agss

