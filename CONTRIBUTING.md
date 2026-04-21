# Contributing to Anti-Gravity Solar System

Thank you for your interest in contributing to this project! This document provides guidelines for contributing.

## How to Contribute

### Reporting Bugs

Before creating bug reports, please check existing issues. When creating a bug report, include:
- A clear and descriptive title
- Steps to reproduce the issue
- Expected vs actual behavior
- System information (OS, compiler, GPU)
- Screenshots or videos if applicable

**Example:**
```markdown
**Bug Summary**: Simulation crashes when loading chaos preset

**Steps to Reproduce**:
1. Launch application
2. Click "3) Chaos" preset button
3. Application crashes after 5 seconds

**Expected Behavior**: Simulation should run with 100+ bodies

**Actual Behavior**: Application crashes with segmentation fault

**System**: Windows 10, GCC 11.2, NVIDIA RTX 3080
```

### Suggesting Enhancements

Enhancement suggestions are welcome! Please provide:
- A clear description of the proposed feature
- Use cases and benefits
- Possible implementation approach (if you have ideas)

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly
5. Commit with clear messages
6. Push to your branch
7. Open a Pull Request

## Code Style Guidelines

### C++ Style

- Use **snake_case** for variables and functions
- Use **PascalCase** for class names
- Use **UPPER_CASE** for constants
- Indent with 2 spaces
- Keep lines under 100 characters

```cpp
// Good example
class CelestialBody {
public:
  double getMass() const;
  void setPosition(double x, double y, double z);
  
private:
  double m_mass;
  glm::dvec3 m_position;
};
```

### Documentation

- Document all public classes and methods
- Include parameter descriptions
- Add usage examples for complex features

### Testing

Before submitting a PR:
- Test on your primary platform
- Verify no memory leaks
- Check for compiler warnings
- Test with different presets and configurations

## Development Setup

### Prerequisites

- CMake 3.20 or higher
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- OpenGL 2.1 support
- Git

### Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Running Tests

```bash
./AntiGravitySolarSystem
```

## Project Structure

```
├── src/
│   ├── core/           # Application core
│   ├── physics/        # Physics engine
│   ├── rendering/      # Graphics rendering
│   ├── ui/             # User interface
│   ├── navigation/     # Navigation system
│   └── procedural/     # Procedural generation
├── assets/             # Data files and presets
├── docs/               # Documentation
└── tests/              # Unit tests (future)
```

## Areas for Contribution

### High Priority
- Performance optimization for large N-body simulations
- Additional celestial body presets
- Enhanced visualization features
- Unit tests for physics engine

### Medium Priority
- Additional camera control options
- Export functionality for simulation data
- VR support exploration
- Multi-language support

### Nice to Have
- Recording/screenshot functionality built-in
- Advanced shader effects
- Network multiplayer mode
- Modding support

## Questions?

Feel free to open an issue for any questions about contributing.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
