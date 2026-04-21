# Procedural Planet Generation System

## Overview

This system provides a complete procedural planet generation solution with:
- Multi-layer noise terrain generation
- Biome-based material system
- Dynamic cloud layers
- Planetary rings
- Gas giant shader support
- Level of Detail (LOD) for performance
- Seed-based reproducibility

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐         ┌─────────────────────────┐   │
│  │ PlanetGenerator │────────▶│    PlanetRenderer       │   │
│  │   (Generation)  │  Mesh   │      (Rendering)        │   │
│  │                 │  Data   │                         │   │
│  └─────────────────┘         └─────────────────────────┘   │
│           │                              │                   │
│           ▼                              ▼                   │
│  ┌─────────────────┐         ┌─────────────────────────┐   │
│  │   Noise.cpp     │         │   GLSL Shaders          │   │
│  │   (Simplex/FBM) │         │   - planet.vert/frag    │   │
│  │                 │         │   - cloud.vert/frag     │   │
│  │                 │         │   - ring.vert/frag      │   │
│  └─────────────────┘         └─────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Principles

1. **Separation of Concerns**: Generation is completely separate from rendering
2. **No Per-Frame Regeneration**: Meshes are cached and only regenerated on seed change
3. **Configurable Parameters**: All noise/material settings are exposed via config structs
4. **LOD Support**: Multiple mesh resolutions for performance scaling

## Usage Example

```cpp
#include "procedural/PlanetGenerator.h"
#include "rendering/PlanetRenderer.h"

using namespace agss;

// 1. Create generator and configure planet
procedural::PlanetGenerator generator;
procedural::PlanetGeneratorConfig config;

// Set seed for reproducibility
config.seed = 12345;
config.planetRadius = 6371.0f;  // Earth-like radius

// Configure multi-layer terrain
config.terrain.baseNoise.scale = 1.0f;
config.terrain.baseNoise.amplitude = 1.0f;
config.terrain.baseNoise.octaves = 6;

config.terrain.detailNoise.scale = 4.0f;
config.terrain.detailNoise.amplitude = 0.3f;

config.terrain.ridgedNoise.scale = 2.0f;
config.terrain.ridgedNoise.amplitude = 0.5f;

// Layer blending weights
config.terrain.baseWeight = 1.0f;
config.terrain.detailWeight = 0.3f;
config.terrain.ridgedWeight = 0.5f;

config.terrain.heightScale = 0.1f;  // 10% max displacement

// Configure biomes
config.biome.transitionSmoothness = 0.15f;

// Enable clouds
config.clouds.enabled = true;
config.clouds.cloudCoverage = 0.5f;
config.clouds.cloudSpeed = 0.01f;

// Enable rings (for Saturn-like planet)
config.rings.enabled = true;
config.rings.innerRadius = 1.3f;
config.rings.outerRadius = 2.5f;

// LOD configuration
config.lod.maxSubdivisions = 6;  // ~131k triangles at highest
config.lod.minSubdivisions = 2;  // ~1k triangles at lowest

// 2. Generate the planet (call ONCE or when seed changes)
auto planetData = generator.generate(config);

// 3. Initialize renderer and upload data
rendering::PlanetRenderer renderer;
renderer.initialize();
renderer.uploadPlanetData(planetData);

// 4. In your render loop:
void renderFrame(const glm::mat4& view, const glm::mat4& projection,
                 const glm::vec3& cameraPos, float time) {
    
    glm::mat4 model = glm::translate(glm::mat4(1.0f), planetPosition);
    
    rendering::PlanetUniforms uniforms;
    uniforms.model = model;
    uniforms.view = view;
    uniforms.projection = projection;
    uniforms.viewPosition = cameraPos;
    uniforms.lightDirection = glm::vec3(1.0f, 0.5f, 0.2f);
    uniforms.lightColor = glm::vec3(1.0f);
    uniforms.time = time;
    
    // Renderer automatically selects appropriate LOD
    renderer.render(model, view, projection, cameraPos, uniforms);
}
```

## Creating Different Planet Types

### Earth-like Terrestrial Planet

```cpp
procedural::PlanetGeneratorConfig earthConfig;
earthConfig.seed = 42;
earthConfig.planetRadius = 6371.0f;

// Moderate terrain with oceans
earthConfig.terrain.heightScale = 0.08f;
earthConfig.terrain.seaLevel = 0.0f;

// Biome thresholds tuned for Earth-like distribution
earthConfig.biome.deepWaterThreshold = -0.5f;
earthConfig.biome.waterThreshold = -0.1f;
earthConfig.biome.sandThreshold = 0.05f;
earthConfig.biome.forestThreshold = 0.4f;
earthConfig.biome.rockThreshold = 0.7f;

// Partial cloud coverage
earthConfig.clouds.cloudCoverage = 0.5f;
earthConfig.clouds.cloudDensity = 0.6f;

// No rings
earthConfig.rings.enabled = false;
```

### Gas Giant (Jupiter-like)

```cpp
procedural::PlanetGeneratorConfig gasGiantConfig;
gasGiantConfig.seed = 999;
gasGiantConfig.planetRadius = 69911.0f;  // Jupiter radius

// Mark as gas giant (disables terrain displacement)
gasGiantConfig.gasGiant.isGasGiant = true;

// Banded atmosphere
gasGiantConfig.gasGiant.bandCount = 8.0f;
gasGiantConfig.gasGiant.turbulence = 0.3f;
gasGiantConfig.gasGiant.animationSpeed = 0.005f;

// Jovian colors
gasGiantConfig.gasGiant.primaryColor = {0.8f, 0.7f, 0.5f};
gasGiantConfig.gasGiant.secondaryColor = {0.6f, 0.5f, 0.4f};
gasGiantConfig.gasGiant.accentColor = {0.9f, 0.8f, 0.7f};

// No solid surface features
gasGiantConfig.clouds.enabled = false;
```

### Desert Planet (Mars-like)

```cpp
procedural::PlanetGeneratorConfig marsConfig;
marsConfig.seed = 31415;
marsConfig.planetRadius = 3389.0f;

// Lower height variation
marsConfig.terrain.heightScale = 0.05f;

// Adjusted biome thresholds (no water)
marsConfig.biome.waterThreshold = 0.3f;  // Very high = no water
marsConfig.biome.sandThreshold = 0.35f;

// Reddish colors
marsConfig.biome.materials[2].color = {0.7f, 0.4f, 0.3f};  // Sand
marsConfig.biome.materials[5].color = {0.6f, 0.35f, 0.25f}; // Rock

// Thin atmosphere
marsConfig.clouds.cloudCoverage = 0.1f;
marsConfig.clouds.cloudDensity = 0.3f;
```

### Ringed Planet (Saturn-like)

```cpp
procedural::PlanetGeneratorConfig saturnConfig;
saturnConfig.seed = 271828;
saturnConfig.planetRadius = 58232.0f;

// Prominent rings
saturnConfig.rings.enabled = true;
saturnConfig.rings.innerRadius = 1.3f;
saturnConfig.rings.outerRadius = 2.5f;
saturnConfig.rings.thickness = 0.05f;
saturnConfig.rings.opacity = 0.7f;
saturnConfig.rings.color = {0.9f, 0.85f, 0.75f};

// Gas giant surface
saturnConfig.gasGiant.isGasGiant = true;
saturnConfig.gasGiant.bandCount = 10.0f;
```

## Performance Considerations

### LOD Distance Tuning

```cpp
// Adjust LOD distances based on your scene scale
config.lod.lodDistances[0] = 0.5f;   // LOD 5: < 0.5 units
config.lod.lodDistances[1] = 1.0f;   // LOD 4: < 1.0 units
config.lod.lodDistances[2] = 2.0f;   // LOD 3: < 2.0 units
config.lod.lodDistances[3] = 5.0f;   // LOD 2: < 5.0 units
config.lod.lodDistances[4] = 10.0f;  // LOD 1: < 10.0 units
config.lod.lodDistances[5] = 20.0f;  // LOD 0: >= 20.0 units
```

### Triangle Count by LOD

| LOD Level | Subdivisions | Triangles | Vertices | Use Case |
|-----------|--------------|-----------|----------|----------|
| 0 | 2 | 1,280 | 642 | Far distance |
| 1 | 3 | 5,120 | 2,562 | Medium distance |
| 2 | 4 | 20,480 | 10,242 | Close view |
| 3 | 5 | 81,920 | 40,962 | Very close |
| 4 | 6 | 327,680 | 163,842 | Extreme close |
| 5 | 7 | 1,310,720 | 655,362 | Cinematic |

### Caching Strategy

```cpp
// Generator caches internally - check before regenerating
if (generator.needsRegeneration(newSeed)) {
    auto newData = generator.generate(newConfig);
    renderer.uploadPlanetData(newData);
    generator.setCachedSeed(newSeed);
}

// Or force refresh if config changed (not just seed)
renderer.invalidateCache();
```

## Shader Features

### Planet Fragment Shader
- Biome-based coloring with smooth transitions
- Water specular reflection with Fresnel effect
- Snow brightness enhancement
- Gas giant band patterns with turbulence
- Animated atmospheric movement

### Cloud Shader
- Procedural FBM cloud generation
- Independent rotation from planet
- Soft edge transparency
- Self-shadowing in thick areas

### Ring Shader
- Procedural banding patterns
- Radial opacity fade
- Subtle shimmer animation
- Double-sided lighting

## Extending the System

### Adding New Biomes

```cpp
// 1. Add to BiomeType enum
enum class BiomeType : uint8_t {
    // ... existing ...
    Volcano = 7,
    Ice = 8
};

// 2. Add materials to BiomeConfig
config.biome.materials[7] = {{0.3f, 0.1f, 0.0f}, 0.9f, 0.0f, "Volcano"};
config.biome.materials[8] = {{0.8f, 0.9f, 1.0f}, 0.3f, 0.0f, "Ice"};

// 3. Add threshold and update sampleBiomeSmooth()
config.biome.volcanoThreshold = 0.85f;
```

### Custom Noise Layers

```cpp
// Add a new noise layer type
struct CanyonNoiseConfig {
    float scale = 3.0f;
    float amplitude = 0.4f;
    int octaves = 4;
    float canyonDepth = 0.5f;
};

// In PlanetGenerator::evaluateTerrainNoise():
float canyonValue = evaluateCanyonNoise(dir.x, dir.y, dir.z, 
                                         terrain.canyonNoise, 
                                         m_cachedSeed + 3000);
combined += canyonValue * terrain.canyonWeight;
```

### GPU Tessellation (Advanced)

For even smoother LOD transitions, consider implementing GPU tessellation:

```glsl
// Tessellation control shader would interpolate between LOD levels
// This avoids popping artifacts during LOD transitions
```

## Troubleshooting

### Seams at UV Boundaries
- Ensure icosphere generation uses seamless edge caching
- Check that noise evaluation wraps correctly at boundaries

### Performance Issues
- Reduce maxSubdivisions for distant planets
- Disable clouds/rings if not needed
- Use lower octaves for noise layers

### Visual Artifacts
- Verify normal recalculation after displacement
- Check biome threshold ordering (must be ascending)
- Ensure proper depth sorting for transparent clouds/rings

## Files Reference

```
src/procedural/
├── Noise.h              # Simplex noise and FBM declarations
├── Noise.cpp            # Noise implementation
├── PlanetGenerator.h    # Generator API and config structs
└── PlanetGenerator.cpp  # Mesh generation logic

src/rendering/
├── PlanetRenderer.h     # Renderer API and uniform structs
└── PlanetRenderer.cpp   # OpenGL rendering implementation

assets/shaders/
├── planet.vert.glsl     # Planet vertex shader
├── planet.frag.glsl     # Planet fragment shader (biomes + gas giants)
├── cloud.vert.glsl      # Cloud vertex shader
├── cloud.frag.glsl      # Cloud fragment shader
├── ring.vert.glsl       # Ring vertex shader
└── ring.frag.glsl       # Ring fragment shader
```
