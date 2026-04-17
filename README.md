# Anti-Gravity Solar System (bootstrap)

This repo is a **starter implementation** of the architecture described in `implementation_plan.md`.

## Build (Windows)

Prereqs:
- CMake 3.20+
- A C++17 compiler (Visual Studio 2022 recommended)

```bash
cmake -S . -B build
cmake --build build --config Release
```

Run:
- `build/Release/AntiGravitySolarSystem.exe`

## Controls (current bootstrap)

- **Space**: pause / resume
- **A**: toggle anti-gravity force generator
- **Esc**: quit

## Status

Implemented now:
- Fixed-timestep simulation loop + interpolation factor
- `PhysicsEngine` with `IForceGenerator` strategy list
- `GravityForce` and `AntiGravityForce`
- JSON-driven initial bodies (`assets/data/solar_system.json`)

Next increments (per plan):
- Renderer + meshes/shaders (GLAD/OpenGL 4.5 core)
- ImGui UI panels
- Orbit trails + camera system

