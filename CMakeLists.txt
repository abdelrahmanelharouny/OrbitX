cmake_minimum_required(VERSION 3.20)
project(AntiGravitySolarSystem VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ---- Dependencies (FetchContent for a self-contained bootstrap) ----
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.0
)
FetchContent_MakeAvailable(imgui)

add_executable(AntiGravitySolarSystem
  src/main.cpp
  src/core/Application.cpp
  src/core/Timer.cpp
  src/entities/SceneNode.cpp
  src/entities/CelestialBody.cpp
  src/physics/PhysicsEngine.cpp
  src/physics/Integrators.cpp
  src/physics/GravityForce.cpp
  src/physics/AntiGravityForce.cpp
  src/scene/SceneManager.cpp
  src/entities/SolarSystemFactory.cpp
  src/simulation/SimulationManager.cpp
  src/simulation/CollisionSystem.cpp
  src/debug/EnergyMonitor.cpp
  src/debug/OrbitTrailsSystem.cpp
  src/rendering/DebugRender2D.cpp
  src/time/TimeController.cpp

  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)

target_include_directories(AntiGravitySolarSystem PRIVATE src)
target_include_directories(AntiGravitySolarSystem PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(AntiGravitySolarSystem PRIVATE glfw glm::glm nlohmann_json::nlohmann_json)

if (WIN32)
  target_link_libraries(AntiGravitySolarSystem PRIVATE opengl32)
endif()

target_compile_definitions(AntiGravitySolarSystem PRIVATE _USE_MATH_DEFINES)

