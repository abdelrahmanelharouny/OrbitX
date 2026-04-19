#pragma once

// Planet Generator - Multi-layer terrain with biomes, LOD, and seed-based generation
// Separated from rendering: generates mesh data that renderer consumes

#include "procedural/Noise.h"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>
#include <cstdint>
#include <string>
#include <array>

namespace agss {
namespace procedural {

// ============================================================================
// Configuration Structures (All parameters configurable, no hardcoding)
// ============================================================================

struct NoiseLayerConfig {
    float scale = 1.0f;           // Frequency scale
    float amplitude = 1.0f;       // Height contribution
    int octaves = 6;              // Detail levels
    float persistence = 0.5f;     // Amplitude decay per octave
    float lacunarity = 2.0f;      // Frequency growth per octave
    float offset = 0.0f;          // Value offset
};

struct TerrainConfig {
    // Base terrain (large-scale features)
    NoiseLayerConfig baseNoise{
        .scale = 1.0f,
        .amplitude = 1.0f,
        .octaves = 6,
        .persistence = 0.5f,
        .lacunarity = 2.0f
    };
    
    // Detail noise (fine surface variation)
    NoiseLayerConfig detailNoise{
        .scale = 4.0f,
        .amplitude = 0.3f,
        .octaves = 4,
        .persistence = 0.4f,
        .lacunarity = 2.0f
    };
    
    // Ridged noise (mountain-like structures)
    NoiseLayerConfig ridgedNoise{
        .scale = 2.0f,
        .amplitude = 0.5f,
        .octaves = 5,
        .persistence = 0.45f,
        .lacunarity = 2.0f
    };
    
    // Layer blending weights
    float baseWeight = 1.0f;
    float detailWeight = 0.3f;
    float ridgedWeight = 0.5f;
    
    // Displacement settings
    float heightScale = 0.1f;     // Maximum displacement as fraction of radius
    float seaLevel = 0.0f;        // Elevation threshold for water
};

enum class BiomeType : uint8_t {
    DeepWater = 0,
    Water,
    Sand,
    Grass,
    Forest,
    Rock,
    Snow
};

struct BiomeMaterial {
    glm::vec3 color{1.0f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    std::string name;
};

struct BiomeConfig {
    // Elevation thresholds (normalized -1 to 1)
    float deepWaterThreshold = -0.5f;
    float waterThreshold = -0.1f;
    float sandThreshold = 0.05f;
    float forestThreshold = 0.4f;
    float rockThreshold = 0.7f;
    
    // Temperature could be added here for climate zones
    
    // Material definitions
    std::array<BiomeMaterial, 7> materials{{
        {{0.0f, 0.0f, 0.3f}, 0.1f, 0.0f, "DeepWater"},   // Deep water
        {{0.0f, 0.2f, 0.6f}, 0.2f, 0.0f, "Water"},        // Shallow water
        {{0.76f, 0.7f, 0.5f}, 0.9f, 0.0f, "Sand"},        // Beach/sand
        {{0.2f, 0.6f, 0.2f}, 0.8f, 0.0f, "Grass"},        // Grassland
        {{0.1f, 0.4f, 0.1f}, 0.7f, 0.0f, "Forest"},       // Forest
        {{0.5f, 0.45f, 0.4f}, 0.6f, 0.1f, "Rock"},        // Rocky terrain
        {{0.95f, 0.95f, 1.0f}, 0.5f, 0.0f, "Snow"}        // Snow caps
    }};
    
    // Transition smoothness (0 = hard edge, 1 = smooth blend)
    float transitionSmoothness = 0.15f;
};

struct CloudConfig {
    bool enabled = true;
    float cloudScale = 3.0f;
    float cloudCoverage = 0.5f;     // 0-1 coverage
    float cloudSpeed = 0.01f;       // Rotation speed
    float cloudAltitude = 1.02f;    // As fraction of planet radius
    float cloudDensity = 0.6f;
    glm::vec3 cloudColor{1.0f};
    uint32_t seed = 54321;          // Different seed than terrain
};

struct RingConfig {
    bool enabled = false;
    float innerRadius = 1.3f;       // As fraction of planet radius
    float outerRadius = 2.5f;
    float thickness = 0.05f;
    glm::vec3 color{0.9f, 0.85f, 0.7f};
    float opacity = 0.6f;
    uint32_t seed = 98765;
    int segmentCount = 128;         // Radial segments
};

struct GasGiantConfig {
    bool isGasGiant = false;
    float bandScale = 0.5f;
    float bandCount = 8.0f;
    float turbulence = 0.3f;
    float animationSpeed = 0.005f;
    glm::vec3 primaryColor{0.8f, 0.7f, 0.5f};
    glm::vec3 secondaryColor{0.6f, 0.5f, 0.4f};
    glm::vec3 accentColor{0.9f, 0.8f, 0.7f};
};

struct LODConfig {
    bool enabled = true;
    int maxSubdivisions = 6;        // Highest detail level
    int minSubdivisions = 2;        // Lowest detail level
    float lodDistances[6];          // Distance thresholds for each LOD
    
    LODConfig() {
        // Default distance thresholds (in world units)
        lodDistances[0] = 0.5f;     // LOD 0: closest
        lodDistances[1] = 1.0f;
        lodDistances[2] = 2.0f;
        lodDistances[3] = 5.0f;
        lodDistances[4] = 10.0f;
        lodDistances[5] = 20.0f;    // LOD 5: farthest
    }
};

struct PlanetGeneratorConfig {
    uint32_t seed = 42;             // Master seed for reproducibility
    float planetRadius = 1.0f;
    int sphereBaseResolution = 8;   // Base icosphere subdivisions
    
    TerrainConfig terrain;
    BiomeConfig biome;
    CloudConfig clouds;
    RingConfig rings;
    GasGiantConfig gasGiant;
    LODConfig lod;
};

// ============================================================================
// Generated Mesh Data (Renderer consumes this - separation of concerns)
// ============================================================================

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    float elevation;                // For biome calculation in shader
    glm::vec3 biomeColor;           // Pre-computed or shader-calculated
};

struct LODMeshData {
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    int lodLevel;
    float boundingSphereRadius;
};

struct GeneratedPlanetData {
    // Main planet mesh (multiple LODs)
    std::vector<LODMeshData> lodMeshes;
    
    // Cloud mesh (separate sphere)
    LODMeshData cloudMesh;
    
    // Ring mesh (if applicable)
    LODMeshData ringMesh;
    
    // Metadata
    uint32_t seed;
    float radius;
    bool isGasGiant;
    
    // Cached configuration for reference
    PlanetGeneratorConfig config;
};

// ============================================================================
// Planet Generator Class
// ============================================================================

class PlanetGenerator {
public:
    PlanetGenerator();
    ~PlanetGenerator();
    
    // Generate a complete planet with all features
    GeneratedPlanetData generate(const PlanetGeneratorConfig& config);
    
    // Generate only specific components (for partial updates)
    LODMeshData generateTerrainMesh(const PlanetGeneratorConfig& config, int lodLevel);
    LODMeshData generateCloudMesh(const CloudConfig& config, float planetRadius);
    LODMeshData generateRingMesh(const RingConfig& config, float planetRadius);
    
    // Calculate biome color at a point (for shader use or pre-baking)
    glm::vec3 getBiomeColor(float elevation, const BiomeConfig& biome) const;
    
    // Get current LOD based on camera distance
    int calculateLOD(float cameraDistance, const LODConfig& config) const;
    
    // Regenerate only if seed changed (caching optimization)
    void setCachedSeed(uint32_t seed) { m_cachedSeed = seed; }
    uint32_t getCachedSeed() const { return m_cachedSeed; }
    bool needsRegeneration(uint32_t currentSeed) const { return m_cachedSeed != currentSeed; }
    
private:
    // Noise evaluation with multi-layer blending
    float evaluateTerrainNoise(float x, float y, float z, const TerrainConfig& terrain) const;
    
    // Ridged noise implementation (|noise| inverted for mountain ridges)
    float evaluateRidgedNoise(float x, float y, float z, const NoiseLayerConfig& config, uint32_t seed) const;
    
    // FBM helper
    float evaluateFBM(float x, float y, float z, const NoiseLayerConfig& config, uint32_t seed) const;
    
    // Icosphere generation with LOD
    void generateIcosphere(int subdivisions, std::vector<VertexData>& vertices, 
                          std::vector<uint32_t>& indices) const;
    
    // UV mapping for sphere (seamless)
    glm::vec2 sphereUV(const glm::vec3& position) const;
    
    // Apply displacement to vertices based on noise
    void applyDisplacement(std::vector<VertexData>& vertices, float radius, 
                          const TerrainConfig& terrain, uint32_t seed) const;
    
    // Smooth biome transitions
    glm::vec3 sampleBiomeSmooth(float elevation, const BiomeConfig& biome) const;
    
    uint32_t m_cachedSeed = 0;
    mutable GeneratedPlanetData m_cachedData;
    bool m_hasCache = false;
};

} // namespace procedural
} // namespace agss
