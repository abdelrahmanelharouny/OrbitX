#include "PlanetRenderer.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace rendering {

// ============================================================================
// PlanetVertex Implementation
// ============================================================================

std::vector<PlanetVertex> PlanetVertex::fromMesh(
    const procedural::PlanetMesh& mesh) {
    
    std::vector<PlanetVertex> vertices;
    vertices.reserve(mesh.vertexCount);
    
    for (int i = 0; i < mesh.vertexCount; i++) {
        int baseIndex = i * 8;
        
        PlanetVertex vertex;
        vertex.position = glm::vec3(
            mesh.vertices[baseIndex],
            mesh.vertices[baseIndex + 1],
            mesh.vertices[baseIndex + 2]
        );
        vertex.normal = glm::vec3(
            mesh.vertices[baseIndex + 3],
            mesh.vertices[baseIndex + 4],
            mesh.vertices[baseIndex + 5]
        );
        vertex.texCoord = glm::vec2(
            mesh.vertices[baseIndex + 6],
            mesh.vertices[baseIndex + 7]
        );
        vertex.height = (i < static_cast<int>(mesh.heights.size())) 
                       ? mesh.heights[i] : 0.5f;
        
        vertices.push_back(vertex);
    }
    
    return vertices;
}

// ============================================================================
// PlanetRenderer Implementation
// ============================================================================

PlanetRenderer::PlanetRenderer() = default;

PlanetRenderer::~PlanetRenderer() {
    cleanup();
}

bool PlanetRenderer::initialize() {
    // Load shaders
    if (!planetShader.loadFromFile(
            "src/rendering/shaders/planet.vert",
            "src/rendering/shaders/planet.frag")) {
        std::cerr << "Failed to load planet shader\n";
        return false;
    }
    
    if (!atmosphereShader.loadFromFile(
            "src/rendering/shaders/atmosphere.vert",
            "src/rendering/shaders/atmosphere.frag")) {
        std::cerr << "Failed to load atmosphere shader\n";
        return false;
    }
    
    if (!cloudsShader.loadFromFile(
            "src/rendering/shaders/clouds.vert",
            "src/rendering/shaders/clouds.frag")) {
        std::cerr << "Failed to load clouds shader\n";
        return false;
    }
    
    return true;
}

PlanetRenderer::PlanetVAO PlanetRenderer::createVAO(
    const procedural::PlanetInstance& instance) {
    
    PlanetVAO vao;
    
    // Create main planet VAO
    {
        auto vertices = PlanetVertex::fromMesh(instance.getMesh());
        
        glGenVertexArrays(1, &vao.vao);
        glGenBuffers(1, &vao.vbo);
        glGenBuffers(1, &vao.ebo);
        
        glBindVertexArray(vao.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vao.vbo);
        glBufferData(GL_ARRAY_BUFFER, 
                    vertices.size() * sizeof(PlanetVertex),
                    vertices.data(),
                    GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    instance.getMesh().indices.size() * sizeof(unsigned int),
                    instance.getMesh().indices.data(),
                    GL_STATIC_DRAW);
        
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PlanetVertex),
                             (void*)offsetof(PlanetVertex, position));
        
        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PlanetVertex),
                             (void*)offsetof(PlanetVertex, normal));
        
        // TexCoord attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PlanetVertex),
                             (void*)offsetof(PlanetVertex, texCoord));
        
        // Height attribute (for biome coloring)
        glEnableVertexAttribArray(3);
        glVertexAttrib1f(3, 0.5f);  // Will be set per-vertex in improved version
        
        glBindVertexArray(0);
        
        vao.indexCount = static_cast<int>(instance.getMesh().indices.size());
    }
    
    // Create atmosphere VAO if enabled
    if (instance.hasAtmosphere()) {
        const auto& atmMesh = instance.getConfig().hasAtmosphere 
                            ? procedural::PlanetGenerator::generateAtmosphere(
                                1.0f, instance.getConfig().atmosphereThickness)
                            : procedural::PlanetMesh{};
        
        if (atmMesh.vertexCount > 0) {
            glGenVertexArrays(1, &vao.atmVAO);
            glGenBuffers(1, &vao.atmVBO);
            glGenBuffers(1, &vao.atmEBO);
            
            glBindVertexArray(vao.atmVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, vao.atmVBO);
            glBufferData(GL_ARRAY_BUFFER,
                        atmMesh.vertices.size() * sizeof(float),
                        atmMesh.vertices.data(),
                        GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.atmEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                        atmMesh.indices.size() * sizeof(unsigned int),
                        atmMesh.indices.data(),
                        GL_STATIC_DRAW);
            
            // Position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                                 (void*)0);
            
            // Normal
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                                 (void*)(3 * sizeof(float)));
            
            // TexCoord
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                                 (void*)(6 * sizeof(float)));
            
            glBindVertexArray(0);
            
            vao.atmIndexCount = static_cast<int>(atmMesh.indices.size());
        } else {
            vao.atmVAO = 0;
            vao.atmIndexCount = 0;
        }
    } else {
        vao.atmVAO = 0;
        vao.atmIndexCount = 0;
    }
    
    // Create cloud VAO if enabled
    if (instance.hasClouds()) {
        const auto& cloudMesh = instance.getConfig().hasClouds
                              ? procedural::PlanetGenerator::generateCloudLayer(
                                  1.0f, instance.getConfig())
                              : procedural::PlanetMesh{};
        
        if (cloudMesh.vertexCount > 0) {
            glGenVertexArrays(1, &vao.cloudVAO);
            glGenBuffers(1, &vao.cloudVBO);
            glGenBuffers(1, &vao.cloudEBO);
            
            glBindVertexArray(vao.cloudVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, vao.cloudVBO);
            glBufferData(GL_ARRAY_BUFFER,
                        cloudMesh.vertices.size() * sizeof(float),
                        cloudMesh.vertices.data(),
                        GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.cloudEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                        cloudMesh.indices.size() * sizeof(unsigned int),
                        cloudMesh.indices.data(),
                        GL_STATIC_DRAW);
            
            // Position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                                 (void*)0);
            
            // Normal
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                                 (void*)(3 * sizeof(float)));
            
            // TexCoord
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                                 (void*)(6 * sizeof(float)));
            
            glBindVertexArray(0);
            
            vao.cloudIndexCount = static_cast<int>(cloudMesh.indices.size());
        } else {
            vao.cloudVAO = 0;
            vao.cloudIndexCount = 0;
        }
    } else {
        vao.cloudVAO = 0;
        vao.cloudIndexCount = 0;
    }
    
    return vao;
}

PlanetRenderer::PlanetVAO& PlanetRenderer::getOrCreateVAO(
    const procedural::PlanetInstance& instance) {
    
    // For simplicity, create new VAO each time
    // In production, cache by instance pointer or config hash
    planetVAOs.push_back(createVAO(instance));
    return planetVAOs.back();
}

void PlanetRenderer::setTime(float time) {
    currentTime = time;
}

void PlanetRenderer::render(const procedural::PlanetInstance& instance,
                           const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::vec3& lightDir,
                           const glm::vec3& cameraPos) {
    
    PlanetVAO& vao = getOrCreateVAO(instance);
    
    // Render main planet
    planetShader.use();
    setupPlanetShader(
        instance.getModelMatrix(),
        view,
        projection,
        lightDir,
        glm::vec3(1.0f),      // Light color
        glm::vec3(0.1f),      // Ambient
        glm::vec3(1.0f)       // Planet color multiplier
    );
    
    glBindVertexArray(vao.vao);
    glDrawElements(GL_TRIANGLES, vao.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Render atmosphere with additive blending
    if (instance.hasAtmosphere() && vao.atmVAO != 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        
        atmosphereShader.use();
        setupAtmosphereShader(
            instance.getAtmosphereModelMatrix(),
            view,
            projection,
            instance.getConfig().atmosphereColor,
            lightDir,
            cameraPos
        );
        
        glBindVertexArray(vao.atmVAO);
        glDrawElements(GL_TRIANGLES, vao.atmIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glDisable(GL_BLEND);
    }
    
    // Render clouds with alpha blending
    if (instance.hasClouds() && vao.cloudVAO != 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        cloudsShader.use();
        cloudsShader.setFloat("uTime", currentTime);
        setupCloudsShader(
            instance.getCloudModelMatrix(),
            view,
            projection,
            glm::vec3(1.0f)  // Cloud color
        );
        
        glBindVertexArray(vao.cloudVAO);
        glDrawElements(GL_TRIANGLES, vao.cloudIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glDisable(GL_BLEND);
    }
}

void PlanetRenderer::setupPlanetShader(const glm::mat4& model,
                                       const glm::mat4& view,
                                       const glm::mat4& projection,
                                       const glm::vec3& lightDir,
                                       const glm::vec3& lightColor,
                                       const glm::vec3& ambientColor,
                                       const glm::vec3& planetColor) {
    planetShader.setMat4("uModel", model);
    planetShader.setMat4("uView", view);
    planetShader.setMat4("uProjection", projection);
    planetShader.setVec3("uLightDir", lightDir);
    planetShader.setVec3("uLightColor", lightColor);
    planetShader.setVec3("uAmbientColor", ambientColor);
    planetShader.setVec3("uPlanetColor", planetColor);
}

void PlanetRenderer::setupAtmosphereShader(const glm::mat4& model,
                                           const glm::mat4& view,
                                           const glm::mat4& projection,
                                           const glm::vec3& atmosphereColor,
                                           const glm::vec3& lightDir,
                                           const glm::vec3& viewPos) {
    atmosphereShader.setMat4("uModel", model);
    atmosphereShader.setMat4("uView", view);
    atmosphereShader.setMat4("uProjection", projection);
    atmosphereShader.setVec3("uAtmosphereColor", atmosphereColor);
    atmosphereShader.setVec3("uLightDir", lightDir);
    atmosphereShader.setVec3("uViewPos", viewPos);
}

void PlanetRenderer::setupCloudsShader(const glm::mat4& model,
                                       const glm::mat4& view,
                                       const glm::mat4& projection,
                                       const glm::vec3& cloudColor) {
    cloudsShader.setMat4("uModel", model);
    cloudsShader.setMat4("uView", view);
    cloudsShader.setMat4("uProjection", projection);
    cloudsShader.setVec3("uCloudColor", cloudColor);
}

void PlanetRenderer::cleanup() {
    for (auto& vao : planetVAOs) {
        if (vao.vao != 0) {
            glDeleteVertexArrays(1, &vao.vao);
            glDeleteBuffers(1, &vao.vbo);
            glDeleteBuffers(1, &vao.ebo);
        }
        if (vao.atmVAO != 0) {
            glDeleteVertexArrays(1, &vao.atmVAO);
            glDeleteBuffers(1, &vao.atmVBO);
            glDeleteBuffers(1, &vao.atmEBO);
        }
        if (vao.cloudVAO != 0) {
            glDeleteVertexArrays(1, &vao.cloudVAO);
            glDeleteBuffers(1, &vao.cloudVBO);
            glDeleteBuffers(1, &vao.cloudEBO);
        }
    }
    planetVAOs.clear();
}

} // namespace rendering
