#include "PlanetGenerator.h"
#include "NoiseGenerator.h"
#include <glm/gtc/noise.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <algorithm>

namespace procedural {

std::vector<BiomeColor> PlanetGenerator::getDefaultBiomes(PlanetType type) {
    switch (type) {
        case PlanetType::Rocky:
            return {
                BiomeColor(glm::vec3(0.0f, 0.1f, 0.4f), 0.0f, 0.3f),    // Deep water
                BiomeColor(glm::vec3(0.0f, 0.3f, 0.6f), 0.3f, 0.4f),    // Shallow water
                BiomeColor(glm::vec3(0.76f, 0.7f, 0.5f), 0.4f, 0.45f),  // Sand/beach
                BiomeColor(glm::vec3(0.2f, 0.5f, 0.2f), 0.45f, 0.6f),   // Grass/forest
                BiomeColor(glm::vec3(0.4f, 0.35f, 0.25f), 0.6f, 0.75f), // Rock/mountains
                BiomeColor(glm::vec3(0.9f, 0.9f, 0.95f), 0.75f, 1.0f)   // Snow peaks
            };
            
        case PlanetType::GasGiant:
            return {
                BiomeColor(glm::vec3(0.8f, 0.7f, 0.6f), 0.0f, 0.2f),    // Light bands
                BiomeColor(glm::vec3(0.6f, 0.5f, 0.4f), 0.2f, 0.4f),    // Medium bands
                BiomeColor(glm::vec3(0.4f, 0.3f, 0.2f), 0.4f, 0.6f),    // Dark bands
                BiomeColor(glm::vec3(0.7f, 0.6f, 0.5f), 0.6f, 0.8f),    // Storm light
                BiomeColor(glm::vec3(0.5f, 0.4f, 0.3f), 0.8f, 1.0f)     // Storm dark
            };
            
        case PlanetType::Icy:
            return {
                BiomeColor(glm::vec3(0.8f, 0.85f, 0.9f), 0.0f, 0.3f),   // Ice plains
                BiomeColor(glm::vec3(0.7f, 0.8f, 0.85f), 0.3f, 0.5f),   // Ice ridges
                BiomeColor(glm::vec3(0.6f, 0.75f, 0.8f), 0.5f, 0.7f),   // Frozen rock
                BiomeColor(glm::vec3(0.5f, 0.7f, 0.75f), 0.7f, 0.85f),  // Glacier
                BiomeColor(glm::vec3(0.9f, 0.95f, 1.0f), 0.85f, 1.0f)   // Pure ice
            };
            
        default:
            return getDefaultBiomes(PlanetType::Rocky);
    }
}

float PlanetGenerator::calculateHeight(const glm::vec3& position, 
                                       const PlanetConfig& config) {
    utils::NoiseGenerator::initialize(config.seed);
    
    glm::vec3 noisePos = position * config.noiseScale;
    
    float height = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxAmplitude = 0.0f;
    
    for (int i = 0; i < config.octaves; i++) {
        float noiseValue = utils::NoiseGenerator::noise(
            noisePos.x * frequency,
            noisePos.y * frequency,
            noisePos.z * frequency
        );
        
        height += noiseValue * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= config.persistence;
        frequency *= config.lacunarity;
    }
    
    // Normalize to [0, 1] range
    height = (height / maxAmplitude + 1.0f) * 0.5f;
    
    // Apply height multiplier and ensure minimum radius
    return std::max(0.0f, height * config.heightMultiplier);
}

glm::vec3 PlanetGenerator::sphereToCartesian(float theta, float phi, float radius) {
    return glm::vec3(
        radius * std::sin(theta) * std::cos(phi),
        radius * std::cos(theta),
        radius * std::sin(theta) * std::sin(phi)
    );
}

glm::vec3 PlanetGenerator::calculateNormal(const glm::vec3& position, 
                                           const PlanetConfig& config) {
    // Simple approach: normalize the position for sphere
    // For more accurate terrain normals, we'd sample neighboring heights
    return glm::normalize(position);
}

PlanetMesh PlanetGenerator::generateSphere(int resolution, float radius, 
                                           const PlanetConfig& config) {
    PlanetMesh mesh;
    
    std::vector<BiomeColor> biomes = config.customBiomes.empty() 
                                     ? getDefaultBiomes(config.type) 
                                     : config.customBiomes;
    
    int rings = resolution;
    int sectors = resolution * 2;
    
    mesh.vertices.reserve(rings * sectors * 8);  // 8 floats per vertex
    mesh.heights.reserve(rings * sectors);
    
    for (int i = 0; i <= rings; i++) {
        float theta = glm::pi<float>() * static_cast<float>(i) / rings;
        
        for (int j = 0; j <= sectors; j++) {
            float phi = 2.0f * glm::pi<float>() * static_cast<float>(j) / sectors;
            
            glm::vec3 basePos = sphereToCartesian(theta, phi, radius);
            
            // Calculate height displacement
            float height = calculateHeight(basePos, config);
            
            // Displace vertex along normal
            glm::vec3 displacedPos = basePos + glm::normalize(basePos) * height * radius * 0.1f;
            
            // Calculate normal
            glm::vec3 normal = calculateNormal(displacedPos, config);
            
            // Store vertex: position (3) + normal (3) + uv (2)
            mesh.vertices.push_back(displacedPos.x);
            mesh.vertices.push_back(displacedPos.y);
            mesh.vertices.push_back(displacedPos.z);
            mesh.vertices.push_back(normal.x);
            mesh.vertices.push_back(normal.y);
            mesh.vertices.push_back(normal.z);
            mesh.vertices.push_back(static_cast<float>(j) / sectors);
            mesh.vertices.push_back(static_cast<float>(i) / rings);
            
            mesh.heights.push_back(height);
        }
    }
    
    // Generate indices
    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < sectors; j++) {
            int first = i * (sectors + 1) + j;
            int second = first + sectors + 1;
            
            // First triangle
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            
            // Second triangle
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    
    mesh.vertexCount = static_cast<int>(mesh.vertices.size()) / 8;
    mesh.indexCount = static_cast<int>(mesh.indices.size());
    
    return mesh;
}

PlanetMesh PlanetGenerator::generateAtmosphere(float radius, float thickness) {
    PlanetMesh mesh;
    
    int resolution = 32;
    int rings = resolution;
    int sectors = resolution * 2;
    float atmRadius = radius * (1.0f + thickness);
    
    mesh.vertices.reserve(rings * sectors * 8);
    
    for (int i = 0; i <= rings; i++) {
        float theta = glm::pi<float>() * static_cast<float>(i) / rings;
        
        for (int j = 0; j <= sectors; j++) {
            float phi = 2.0f * glm::pi<float>() * static_cast<float>(j) / sectors;
            
            glm::vec3 pos = sphereToCartesian(theta, phi, atmRadius);
            glm::vec3 normal = glm::normalize(pos);
            
            mesh.vertices.push_back(pos.x);
            mesh.vertices.push_back(pos.y);
            mesh.vertices.push_back(pos.z);
            mesh.vertices.push_back(normal.x);
            mesh.vertices.push_back(normal.y);
            mesh.vertices.push_back(normal.z);
            mesh.vertices.push_back(static_cast<float>(j) / sectors);
            mesh.vertices.push_back(static_cast<float>(i) / rings);
        }
    }
    
    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < sectors; j++) {
            int first = i * (sectors + 1) + j;
            int second = first + sectors + 1;
            
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    
    mesh.vertexCount = static_cast<int>(mesh.vertices.size()) / 8;
    mesh.indexCount = static_cast<int>(mesh.indices.size());
    
    return mesh;
}

PlanetMesh PlanetGenerator::generateCloudLayer(float radius, const PlanetConfig& config) {
    PlanetMesh mesh;
    
    int resolution = 48;
    int rings = resolution;
    int sectors = resolution * 2;
    float cloudRadius = radius * config.cloudHeight;
    
    utils::NoiseGenerator::initialize(config.seed + 1000);  // Different seed for clouds
    
    mesh.vertices.reserve(rings * sectors * 8);
    
    for (int i = 0; i <= rings; i++) {
        float theta = glm::pi<float>() * static_cast<float>(i) / rings;
        
        for (int j = 0; j <= sectors; j++) {
            float phi = 2.0f * glm::pi<float>() * static_cast<float>(j) / sectors;
            
            glm::vec3 basePos = sphereToCartesian(theta, phi, cloudRadius);
            
            // Cloud noise
            float cloudNoise = utils::NoiseGenerator::octaveNoise(
                basePos.x * 2.0f,
                basePos.y * 2.0f,
                basePos.z * 2.0f,
                4, 0.5f
            );
            
            // Only create vertices where clouds exist
            if (cloudNoise > (1.0f - config.cloudCoverage)) {
                glm::vec3 pos = basePos;
                glm::vec3 normal = glm::normalize(pos);
                
                mesh.vertices.push_back(pos.x);
                mesh.vertices.push_back(pos.y);
                mesh.vertices.push_back(pos.z);
                mesh.vertices.push_back(normal.x);
                mesh.vertices.push_back(normal.y);
                mesh.vertices.push_back(normal.z);
                mesh.vertices.push_back(static_cast<float>(j) / sectors);
                mesh.vertices.push_back(static_cast<float>(i) / rings);
            } else {
                // Add dummy vertex to maintain indexing
                glm::vec3 pos = basePos;
                glm::vec3 normal = glm::normalize(pos);
                
                mesh.vertices.push_back(pos.x);
                mesh.vertices.push_back(pos.y);
                mesh.vertices.push_back(pos.z);
                mesh.vertices.push_back(normal.x);
                mesh.vertices.push_back(normal.y);
                mesh.vertices.push_back(normal.z);
                mesh.vertices.push_back(-1.0f);  // Mark as no-cloud
                mesh.vertices.push_back(-1.0f);
            }
        }
    }
    
    // Generate indices (skip no-cloud triangles in shader)
    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < sectors; j++) {
            int first = i * (sectors + 1) + j;
            int second = first + sectors + 1;
            
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    
    mesh.vertexCount = static_cast<int>(mesh.vertices.size()) / 8;
    mesh.indexCount = static_cast<int>(mesh.indices.size());
    
    return mesh;
}

glm::vec3 PlanetGenerator::getColorForHeight(float height, 
                                             const std::vector<BiomeColor>& biomes) {
    for (const auto& biome : biomes) {
        if (height >= biome.minHeight && height <= biome.maxHeight) {
            // Smooth interpolation within biome range
            float t = (height - biome.minHeight) / (biome.maxHeight - biome.minHeight);
            
            // Find adjacent biome for smooth transition
            for (const auto& other : biomes) {
                if (other.minHeight > biome.maxHeight) {
                    float blendT = (height - biome.maxHeight) / (other.minHeight - biome.maxHeight);
                    blendT = glm::clamp(blendT, 0.0f, 1.0f);
                    return glm::mix(biome.color, other.color, blendT * 0.5f);
                }
            }
            
            return biome.color;
        }
    }
    
    // Fallback
    return glm::vec3(1.0f, 1.0f, 1.0f);
}

PlanetMesh PlanetGenerator::generate(const PlanetConfig& config) {
    return generateSphere(config.resolution, 1.0f, config);
}

} // namespace procedural
