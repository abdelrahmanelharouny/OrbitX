#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace procedural {

/**
 * @brief Defines the type of planet for generation parameters
 */
enum class PlanetType {
    Rocky,      // Earth-like: water, land, mountains
    GasGiant,   // Jupiter-like: bands, storms
    Icy         // Ice world: frozen surface, cracks
};

/**
 * @brief Color palette for biome definition
 */
struct BiomeColor {
    glm::vec3 color;
    float minHeight;
    float maxHeight;
    
    BiomeColor(const glm::vec3& c, float minH, float maxH) 
        : color(c), minHeight(minH), maxHeight(maxH) {}
};

/**
 * @brief Configuration parameters for planet generation
 */
struct PlanetConfig {
    PlanetType type = PlanetType::Rocky;
    uint32_t seed = 12345;
    int resolution = 64;  // Subdivisions per axis
    
    // Terrain parameters
    float noiseScale = 1.0f;
    int octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float heightMultiplier = 1.0f;
    
    // Rotation speed (radians per second)
    float rotationSpeed = 0.01f;
    
    // Atmosphere settings
    bool hasAtmosphere = true;
    glm::vec3 atmosphereColor = glm::vec3(0.3f, 0.6f, 1.0f);
    float atmosphereThickness = 0.1f;
    
    // Cloud layer settings
    bool hasClouds = false;
    float cloudHeight = 1.05f;
    float cloudCoverage = 0.5f;
    
    // Custom colors (overrides defaults)
    std::vector<BiomeColor> customBiomes;
};

/**
 * @brief Generated mesh data for a planet
 */
struct PlanetMesh {
    std::vector<float> vertices;      // x,y,z, nx,ny,nz, u,v
    std::vector<uint32_t> indices;
    int vertexCount = 0;
    int indexCount = 0;
    
    // Height map for coloring
    std::vector<float> heights;
};

/**
 * @brief Procedural planet generator
 * 
 * Generates spherical meshes with noise-based terrain,
 * biome coloring, and optional atmosphere/clouds.
 */
class PlanetGenerator {
public:
    /**
     * @brief Generate a complete planet mesh based on configuration
     * @param config Planet generation parameters
     * @return Generated mesh data
     */
    static PlanetMesh generate(const PlanetConfig& config);
    
    /**
     * @brief Generate sphere geometry with displaced vertices
     */
    static PlanetMesh generateSphere(int resolution, float radius, 
                                     const PlanetConfig& config);
    
    /**
     * @brief Generate atmosphere mesh (slightly larger sphere)
     */
    static PlanetMesh generateAtmosphere(float radius, float thickness);
    
    /**
     * @brief Generate cloud layer mesh
     */
    static PlanetMesh generateCloudLayer(float radius, const PlanetConfig& config);
    
    /**
     * @brief Get default biome colors for a planet type
     */
    static std::vector<BiomeColor> getDefaultBiomes(PlanetType type);
    
    /**
     * @brief Calculate height at a point on the sphere using noise
     */
    static float calculateHeight(const glm::vec3& position, 
                                 const PlanetConfig& config);
    
    /**
     * @brief Get color for a given height (biome mapping)
     */
    static glm::vec3 getColorForHeight(float height, 
                                       const std::vector<BiomeColor>& biomes);

private:
    /**
     * @brief Convert spherical coordinates to Cartesian
     */
    static glm::vec3 sphereToCartesian(float theta, float phi, float radius);
    
    /**
     * @brief Calculate normal at a vertex position
     */
    static glm::vec3 calculateNormal(const glm::vec3& position, 
                                     const PlanetConfig& config);
};

} // namespace procedural
