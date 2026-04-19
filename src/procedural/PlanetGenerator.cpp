#include "procedural/PlanetGenerator.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>
#include <set>
#include <map>

namespace agss {
namespace procedural {

// ============================================================================
// Helper: Edge key for icosphere generation (ensures seamless mesh)
// ============================================================================

static uint64_t makeEdgeKey(uint32_t a, uint32_t b) {
    return (a < b) 
        ? (static_cast<uint64_t>(a) << 32 | static_cast<uint64_t>(b))
        : (static_cast<uint64_t>(b) << 32 | static_cast<uint64_t>(a));
}

// ============================================================================
// Planet Generator Implementation
// ============================================================================

PlanetGenerator::PlanetGenerator() = default;
PlanetGenerator::~PlanetGenerator() = default;

float PlanetGenerator::evaluateFBM(float x, float y, float z, 
                                   const NoiseLayerConfig& config, 
                                   uint32_t seed) const {
    float value = 0.0f;
    float amplitude = config.amplitude;
    float frequency = config.scale;
    float maxAmplitude = 0.0f;
    
    uint32_t currentSeed = seed;
    
    for (int i = 0; i < config.octaves; ++i) {
        SimplexNoise3D noise(currentSeed);
        value += noise.evaluate(x * frequency, y * frequency, z * frequency) * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= config.persistence;
        frequency *= config.lacunarity;
        
        // Deterministic seed variation per octave
        currentSeed = static_cast<uint32_t>(currentSeed * 17 + i * 31);
    }
    
    return (maxAmplitude > 0.0f) ? (value / maxAmplitude) : value;
}

float PlanetGenerator::evaluateRidgedNoise(float x, float y, float z,
                                           const NoiseLayerConfig& config,
                                           uint32_t seed) const {
    float value = 0.0f;
    float amplitude = config.amplitude;
    float frequency = config.scale;
    float maxAmplitude = 0.0f;
    
    uint32_t currentSeed = seed;
    
    for (int i = 0; i < config.octaves; ++i) {
        SimplexNoise3D noise(currentSeed);
        float n = noise.evaluate(x * frequency, y * frequency, z * frequency);
        
        // Ridge transformation: invert absolute value
        n = std::abs(n);
        n = 1.0f - n * n;  // Sharpen ridges
        
        value += n * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= config.persistence;
        frequency *= config.lacunarity;
        currentSeed = static_cast<uint32_t>(currentSeed * 17 + i * 31);
    }
    
    return (maxAmplitude > 0.0f) ? (value / maxAmplitude) : value;
}

float PlanetGenerator::evaluateTerrainNoise(float x, float y, float z,
                                            const TerrainConfig& terrain) const {
    // Normalize input to unit sphere
    glm::vec3 dir = glm::normalize(glm::vec3(x, y, z));
    
    // Evaluate each noise layer
    float baseValue = evaluateFBM(dir.x, dir.y, dir.z, terrain.baseNoise, m_cachedSeed);
    float detailValue = evaluateFBM(dir.x, dir.y, dir.z, terrain.detailNoise, m_cachedSeed + 1000);
    float ridgedValue = evaluateRidgedNoise(dir.x, dir.y, dir.z, terrain.ridgedNoise, m_cachedSeed + 2000);
    
    // Blend layers with configurable weights
    float combined = baseValue * terrain.baseWeight
                   + detailValue * terrain.detailWeight
                   + ridgedValue * terrain.ridgedWeight;
    
    // Normalize combined weight
    float totalWeight = terrain.baseWeight + terrain.detailWeight + terrain.ridgedWeight;
    if (totalWeight > 0.0f) {
        combined /= totalWeight;
    }
    
    return combined;
}

void PlanetGenerator::generateIcosphere(int subdivisions,
                                        std::vector<VertexData>& vertices,
                                        std::vector<uint32_t>& indices) const {
    // Start with icosahedron (12 vertices, 20 faces)
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    
    struct RawVertex {
        float x, y, z;
    };
    
    std::vector<RawVertex> baseVertices = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };
    
    // Normalize to unit sphere
    for (auto& v : baseVertices) {
        float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        v.x /= len; v.y /= len; v.z /= len;
    }
    
    // Initial icosahedron indices (20 triangles)
    std::vector<uint32_t> baseIndices = {
        0, 11, 5,   0, 5, 1,    0, 1, 7,    0, 7, 10,   0, 10, 11,
        1, 5, 9,    5, 11, 4,   11, 10, 2,  10, 7, 6,   7, 1, 8,
        3, 9, 4,    3, 4, 2,    3, 2, 6,    3, 6, 8,    3, 8, 9,
        4, 9, 5,    2, 4, 11,   6, 2, 10,   8, 6, 7,    9, 8, 1
    };
    
    vertices.clear();
    indices.clear();
    
    // Convert base vertices
    for (const auto& v : baseVertices) {
        VertexData vd;
        vd.position = glm::vec3(v.x, v.y, v.z);
        vd.normal = vd.position;  // Unit sphere normal
        vd.uv = sphereUV(vd.position);
        vd.elevation = 0.0f;
        vd.biomeColor = glm::vec3(1.0f);
        vertices.push_back(vd);
    }
    
    // Subdivision cache
    std::map<uint64_t, uint32_t> edgeCache;
    
    // Midpoint function with caching
    auto getMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t {
        uint64_t key = makeEdgeKey(a, b);
        
        auto it = edgeCache.find(key);
        if (it != edgeCache.end()) {
            return it->second;
        }
        
        // Create new vertex at midpoint, normalized to sphere
        const auto& va = vertices[a].position;
        const auto& vb = vertices[b].position;
        
        glm::vec3 mid = glm::normalize((va + vb) * 0.5f);
        
        VertexData vd;
        vd.position = mid;
        vd.normal = mid;
        vd.uv = sphereUV(mid);
        vd.elevation = 0.0f;
        vd.biomeColor = glm::vec3(1.0f);
        
        uint32_t newIndex = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vd);
        edgeCache[key] = newIndex;
        
        return newIndex;
    };
    
    // Perform subdivisions
    indices = baseIndices;
    
    for (int sub = 0; sub < subdivisions; ++sub) {
        edgeCache.clear();
        std::vector<uint32_t> newIndices;
        newIndices.reserve(indices.size() * 4);
        
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t a = indices[i];
            uint32_t b = indices[i + 1];
            uint32_t c = indices[i + 2];
            
            uint32_t ab = getMidpoint(a, b);
            uint32_t bc = getMidpoint(b, c);
            uint32_t ca = getMidpoint(c, a);
            
            // Create 4 new triangles
            newIndices.push_back(a); newIndices.push_back(ab); newIndices.push_back(ca);
            newIndices.push_back(b); newIndices.push_back(bc); newIndices.push_back(ab);
            newIndices.push_back(c); newIndices.push_back(ca); newIndices.push_back(bc);
            newIndices.push_back(ab); newIndices.push_back(bc); newIndices.push_back(ca);
        }
        
        indices = std::move(newIndices);
    }
}

glm::vec2 PlanetGenerator::sphereUV(const glm::vec3& position) const {
    // Spherical coordinates for UV mapping (seamless at poles)
    glm::vec3 p = glm::normalize(position);
    
    float u = 0.5f + std::atan2(p.z, p.x) / (2.0f * glm::pi<float>());
    float v = 0.5f - std::asin(std::clamp(p.y, -1.0f, 1.0f)) / glm::pi<float>();
    
    return glm::vec2(u, v);
}

void PlanetGenerator::applyDisplacement(std::vector<VertexData>& vertices,
                                        float radius,
                                        const TerrainConfig& terrain,
                                        uint32_t seed) const {
    for (auto& vertex : vertices) {
        // Evaluate terrain noise at this point
        float noise = evaluateTerrainNoise(vertex.position.x, vertex.position.y, vertex.position.z, terrain);
        
        // Store elevation for biome calculation
        vertex.elevation = noise;
        
        // Apply displacement along normal
        float displacement = noise * terrain.heightScale * radius;
        vertex.position = vertex.normal * (radius + displacement);
        
        // Recalculate normal (approximate by re-normalizing displaced position)
        vertex.normal = glm::normalize(vertex.position);
        
        // Update UV after displacement
        vertex.uv = sphereUV(vertex.position);
    }
}

glm::vec3 PlanetGenerator::sampleBiomeSmooth(float elevation, const BiomeConfig& biome) const {
    // Find the two nearest biome thresholds for smooth interpolation
    const auto& mats = biome.materials;
    float smoothness = biome.transitionSmoothness;
    
    // Deep water
    if (elevation < biome.deepWaterThreshold + smoothness) {
        float t = std::clamp((elevation - biome.deepWaterThreshold + smoothness) / smoothness, 0.0f, 1.0f);
        return glm::mix(mats[0].color, mats[1].color, t);
    }
    
    // Water
    if (elevation < biome.waterThreshold + smoothness) {
        float t = std::clamp((elevation - biome.waterThreshold + smoothness) / smoothness, 0.0f, 1.0f);
        return glm::mix(mats[1].color, mats[2].color, t);
    }
    
    // Sand
    if (elevation < biome.sandThreshold + smoothness) {
        float t = std::clamp((elevation - biome.sandThreshold + smoothness) / smoothness, 0.0f, 1.0f);
        return glm::mix(mats[2].color, mats[3].color, t);
    }
    
    // Grass
    if (elevation < biome.forestThreshold + smoothness) {
        float t = std::clamp((elevation - biome.forestThreshold + smoothness) / smoothness, 0.0f, 1.0f);
        return glm::mix(mats[3].color, mats[4].color, t);
    }
    
    // Forest
    if (elevation < biome.rockThreshold + smoothness) {
        float t = std::clamp((elevation - biome.rockThreshold + smoothness) / smoothness, 0.0f, 1.0f);
        return glm::mix(mats[4].color, mats[5].color, t);
    }
    
    // Rock to Snow transition
    if (elevation < 1.0f) {
        float rockToSnowThreshold = biome.rockThreshold + smoothness;
        float t = std::clamp((elevation - rockToSnowThreshold) / (1.0f - rockToSnowThreshold), 0.0f, 1.0f);
        return glm::mix(mats[5].color, mats[6].color, t);
    }
    
    return mats[6].color; // Snow
}

glm::vec3 PlanetGenerator::getBiomeColor(float elevation, const BiomeConfig& biome) const {
    return sampleBiomeSmooth(elevation, biome);
}

int PlanetGenerator::calculateLOD(float cameraDistance, const LODConfig& config) const {
    if (!config.enabled) {
        return config.minSubdivisions;
    }
    
    // Find appropriate LOD level based on distance
    for (int i = 0; i < 6; ++i) {
        if (cameraDistance < config.lodDistances[i]) {
            return std::max(config.minSubdivisions, config.maxSubdivisions - i);
        }
    }
    
    return config.minSubdivisions;
}

LODMeshData PlanetGenerator::generateTerrainMesh(const PlanetGeneratorConfig& config, int lodLevel) {
    LODMeshData mesh;
    mesh.lodLevel = lodLevel;
    
    // Generate icosphere with LOD-appropriate subdivision
    int subdivisions = std::clamp(lodLevel, config.lod.minSubdivisions, config.lod.maxSubdivisions);
    generateIcosphere(subdivisions, mesh.vertices, mesh.indices);
    
    // Apply terrain displacement
    applyDisplacement(mesh.vertices, config.planetRadius, config.terrain, config.seed);
    
    // Apply biome colors
    for (auto& vertex : mesh.vertices) {
        if (!config.gasGiant.isGasGiant) {
            vertex.biomeColor = sampleBiomeSmooth(vertex.elevation, config.biome);
        } else {
            // Gas giants use different coloring (handled in shader typically)
            vertex.biomeColor = config.gasGiant.primaryColor;
        }
    }
    
    // Calculate bounding sphere
    mesh.boundingSphereRadius = config.planetRadius * (1.0f + config.terrain.heightScale);
    
    return mesh;
}

LODMeshData PlanetGenerator::generateCloudMesh(const CloudConfig& config, float planetRadius) {
    LODMeshData mesh;
    mesh.lodLevel = 0;
    
    // Cloud sphere uses fewer subdivisions (smooth sphere)
    generateIcosphere(4, mesh.vertices, mesh.indices);
    
    float cloudRadius = planetRadius * config.cloudAltitude;
    
    for (auto& vertex : mesh.vertices) {
        vertex.position = vertex.normal * cloudRadius;
        vertex.elevation = 1.0f;  // Mark as cloud layer
    }
    
    mesh.boundingSphereRadius = cloudRadius;
    return mesh;
}

LODMeshData PlanetGenerator::generateRingMesh(const RingConfig& config, float planetRadius) {
    LODMeshData mesh;
    mesh.lodLevel = 0;
    
    float innerR = planetRadius * config.innerRadius;
    float outerR = planetRadius * config.outerRadius;
    
    int radialSegments = config.segmentCount;
    int angularSegments = 256;  // Full circle resolution
    
    // Generate ring as triangle strip
    for (int ang = 0; ang <= angularSegments; ++ang) {
        float angle = (static_cast<float>(ang) / angularSegments) * 2.0f * glm::pi<float>();
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        
        // Inner vertex
        VertexData vInner;
        vInner.position = glm::vec3(innerR * cosA, 0.0f, innerR * sinA);
        vInner.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Up-facing
        vInner.uv = glm::vec2(0.0f, static_cast<float>(ang) / angularSegments);
        vInner.elevation = 0.0f;
        vInner.biomeColor = config.color;
        mesh.vertices.push_back(vInner);
        
        // Outer vertex
        VertexData vOuter;
        vOuter.position = glm::vec3(outerR * cosA, 0.0f, outerR * sinA);
        vOuter.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vOuter.uv = glm::vec2(1.0f, static_cast<float>(ang) / angularSegments);
        vOuter.elevation = 0.0f;
        vOuter.biomeColor = config.color;
        mesh.vertices.push_back(vOuter);
    }
    
    // Generate indices for triangle strip
    for (int ang = 0; ang < angularSegments; ++ang) {
        uint32_t base = static_cast<uint32_t>(ang * 2);
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }
    
    mesh.boundingSphereRadius = outerR;
    return mesh;
}

GeneratedPlanetData PlanetGenerator::generate(const PlanetGeneratorConfig& config) {
    GeneratedPlanetData data;
    data.seed = config.seed;
    data.radius = config.planetRadius;
    data.isGasGiant = config.gasGiant.isGasGiant;
    data.config = config;
    
    m_cachedSeed = config.seed;
    
    // Generate LOD meshes for terrain
    int numLODs = config.lod.maxSubdivisions - config.lod.minSubdivisions + 1;
    data.lodMeshes.resize(static_cast<size_t>(numLODs));
    
    for (int lod = config.lod.minSubdivisions; lod <= config.lod.maxSubdivisions; ++lod) {
        data.lodMeshes[static_cast<size_t>(lod - config.lod.minSubdivisions)] = 
            generateTerrainMesh(config, lod);
    }
    
    // Generate cloud mesh if enabled
    if (config.clouds.enabled && !config.gasGiant.isGasGiant) {
        data.cloudMesh = generateCloudMesh(config.clouds, config.planetRadius);
    }
    
    // Generate ring mesh if enabled
    if (config.rings.enabled) {
        data.ringMesh = generateRingMesh(config.rings, config.planetRadius);
    }
    
    // Cache the result
    m_cachedData = data;
    m_hasCache = true;
    
    return data;
}

} // namespace procedural
} // namespace agss
