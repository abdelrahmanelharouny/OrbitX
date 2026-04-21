#include "PlanetInstance.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace procedural {

// ============================================================================
// PlanetInstance Implementation
// ============================================================================

PlanetInstance::PlanetInstance(const PlanetConfig& config) 
    : config(config) {
    generateMeshes();
}

void PlanetInstance::generateMeshes() {
    // Generate main planet mesh
    mesh = PlanetGenerator::generate(config);
    
    // Generate atmosphere if enabled
    if (config.hasAtmosphere) {
        atmosphereMesh = PlanetGenerator::generateAtmosphere(1.0f, config.atmosphereThickness);
    }
    
    // Generate cloud layer if enabled
    if (config.hasClouds) {
        cloudMesh = PlanetGenerator::generateCloudLayer(1.0f, config);
    }
}

void PlanetInstance::update(float deltaTime) {
    currentRotation += config.rotationSpeed * deltaTime;
    
    // Keep rotation in [0, 2π] range
    if (currentRotation > glm::two_pi<float>()) {
        currentRotation -= glm::two_pi<float>();
    }
}

glm::mat4 PlanetInstance::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, currentRotation, glm::vec3(0.0f, 1.0f, 0.0f));
    return model;
}

glm::mat4 PlanetInstance::getAtmosphereModelMatrix() const {
    glm::mat4 model = getModelMatrix();
    float scale = 1.0f + config.atmosphereThickness;
    model = glm::scale(model, glm::vec3(scale));
    return model;
}

glm::mat4 PlanetInstance::getCloudModelMatrix() const {
    glm::mat4 model = getModelMatrix();
    model = glm::scale(model, glm::vec3(config.cloudHeight));
    return model;
}

void PlanetInstance::regenerate(uint32_t newSeed) {
    config.seed = newSeed;
    generateMeshes();
    currentRotation = 0.0f;
}

// ============================================================================
// PlanetManager Implementation
// ============================================================================

PlanetManager& PlanetManager::getInstance() {
    static PlanetManager instance;
    return instance;
}

std::shared_ptr<PlanetInstance> PlanetManager::getOrCreatePlanet(const PlanetConfig& config) {
    // Check if we have a cached planet with same configuration
    for (auto& cached : cache) {
        if (cached.config.seed == config.seed && 
            cached.config.type == config.type &&
            cached.config.resolution == config.resolution) {
            return cached.instance;
        }
    }
    
    // Create new planet
    auto instance = std::make_shared<PlanetInstance>(config);
    
    CachedPlanet cached;
    cached.id = nextId++;
    cached.instance = instance;
    cached.config = config;
    
    cache.push_back(cached);
    
    return instance;
}

void PlanetManager::removePlanet(uint32_t planetId) {
    cache.erase(
        std::remove_if(cache.begin(), cache.end(),
            [planetId](const CachedPlanet& c) { return c.id == planetId; }),
        cache.end()
    );
}

void PlanetManager::clearCache() {
    cache.clear();
    nextId = 0;
}

void PlanetManager::updateAll(float deltaTime) {
    for (auto& cached : cache) {
        cached.instance->update(deltaTime);
    }
}

} // namespace procedural
