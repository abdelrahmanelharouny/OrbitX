#pragma once

#include "PlanetGenerator.h"
#include "../rendering/Shader.h"
#include <glm/glm.hpp>
#include <memory>

namespace procedural {

/**
 * @brief Runtime planet instance with rendering data
 * 
 * Manages a generated planet's mesh, materials, and rotation state.
 * Separates generation (one-time) from rendering (per-frame).
 */
class PlanetInstance {
public:
    /**
     * @brief Create a planet instance from configuration
     * @param config Generation parameters
     */
    explicit PlanetInstance(const PlanetConfig& config);
    
    /**
     * @brief Update planet rotation based on delta time
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);
    
    /**
     * @brief Get the current model matrix including rotation
     */
    glm::mat4 getModelMatrix() const;
    
    /**
     * @brief Get atmosphere model matrix
     */
    glm::mat4 getAtmosphereModelMatrix() const;
    
    /**
     * @brief Get cloud layer model matrix
     */
    glm::mat4 getCloudModelMatrix() const;
    
    // Getters
    const PlanetMesh& getMesh() const { return mesh; }
    const PlanetConfig& getConfig() const { return config; }
    bool hasAtmosphere() const { return config.hasAtmosphere; }
    bool hasClouds() const { return config.hasClouds; }
    
    // Regenerate with new seed
    void regenerate(uint32_t newSeed);

private:
    PlanetConfig config;
    PlanetMesh mesh;
    PlanetMesh atmosphereMesh;
    PlanetMesh cloudMesh;
    
    float currentRotation = 0.0f;
    
    void generateMeshes();
};

/**
 * @brief Manager for all procedural planets
 * 
 * Handles caching, LOD, and efficient generation.
 */
class PlanetManager {
public:
    static PlanetManager& getInstance();
    
    /**
     * @brief Get or create a planet instance
     * @param config Planet configuration
     * @return Reference to planet instance
     */
    std::shared_ptr<PlanetInstance> getOrCreatePlanet(const PlanetConfig& config);
    
    /**
     * @brief Remove a planet from management
     */
    void removePlanet(uint32_t planetId);
    
    /**
     * @brief Clear all cached planets
     */
    void clearCache();
    
    /**
     * @brief Update all managed planets
     */
    void updateAll(float deltaTime);

private:
    PlanetManager() = default;
    ~PlanetManager() = default;
    
    struct CachedPlanet {
        uint32_t id;
        std::shared_ptr<PlanetInstance> instance;
        PlanetConfig config;
    };
    
    std::vector<CachedPlanet> cache;
    uint32_t nextId = 0;
};

} // namespace procedural
