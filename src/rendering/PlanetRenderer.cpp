#include "rendering/PlanetRenderer.h"

// OpenGL includes (would be via GLAD in actual implementation)
#ifdef _WIN32
#include <GL/glew.h>
#else
#include <GL/gl.h>
#endif

#include <glm/gtc/type_ptr.hpp>
#include <cstring>

namespace agss {
namespace rendering {

PlanetRenderer::PlanetRenderer() = default;
PlanetRenderer::~PlanetRenderer() {
    shutdown();
}

bool PlanetRenderer::initialize() {
    if (m_initialized) {
        return true;
    }
    
    if (!createShaderPrograms()) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void PlanetRenderer::shutdown() {
    // Clean up all cached meshes
    for (auto& mesh : m_cachedData.lodMeshes) {
        deleteMesh(mesh);
    }
    m_cachedData.lodMeshes.clear();
    
    if (m_cachedData.hasClouds) {
        deleteMesh(m_cachedData.cloudMesh);
        m_cachedData.hasClouds = false;
    }
    
    if (m_cachedData.hasRings) {
        deleteMesh(m_cachedData.ringMesh);
        m_cachedData.hasRings = false;
    }
    
    destroyShaderPrograms();
    m_initialized = false;
}

void PlanetRenderer::setConfig(const PlanetRenderConfig& config) {
    m_config = config;
}

bool PlanetRenderer::createShaderPrograms() {
    // In a real implementation, this would:
    // 1. Load shader source from files
    // 2. Compile vertex and fragment shaders
    // 3. Link into programs
    // 4. Cache uniform locations
    
    // Placeholder - actual implementation depends on shader loading system
    // For now, assume shaders are created externally or via a ShaderManager
    
    // m_planetShader = ShaderLoader::load("assets/shaders/planet.vert.glsl", 
    //                                     "assets/shaders/planet.frag.glsl");
    // m_cloudShader = ShaderLoader::load("assets/shaders/cloud.vert.glsl",
    //                                    "assets/shaders/cloud.frag.glsl");
    // m_ringShader = ShaderLoader::load("assets/shaders/ring.vert.glsl",
    //                                   "assets/shaders/ring.frag.glsl");
    
    return true;
}

void PlanetRenderer::destroyShaderPrograms() {
    // In real implementation: glDeleteProgram calls
    m_planetShader = 0;
    m_cloudShader = 0;
    m_ringShader = 0;
}

GLMesh PlanetRenderer::uploadMeshToGPU(const procedural::LODMeshData& meshData) {
    GLMesh mesh;
    
    // Generate VAO
    glGenVertexArrays(1, &mesh.VAO);
    glBindVertexArray(mesh.VAO);
    
    // Generate and fill VBO
    glGenBuffers(1, &mesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 meshData.vertices.size() * sizeof(procedural::VertexData),
                 meshData.vertices.data(),
                 GL_STATIC_DRAW);
    
    // Generate and fill EBO
    glGenBuffers(1, &mesh.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 meshData.indices.size() * sizeof(uint32_t),
                 meshData.indices.data(),
                 GL_STATIC_DRAW);
    
    // Vertex attribute layout:
    // Location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(procedural::VertexData),
                          reinterpret_cast<void*>(offsetof(procedural::VertexData, position)));
    
    // Location 1: normal (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(procedural::VertexData),
                          reinterpret_cast<void*>(offsetof(procedural::VertexData, normal)));
    
    // Location 2: uv (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(procedural::VertexData),
                          reinterpret_cast<void*>(offsetof(procedural::VertexData, uv)));
    
    // Location 3: elevation (float)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(procedural::VertexData),
                          reinterpret_cast<void*>(offsetof(procedural::VertexData, elevation)));
    
    // Location 4: biomeColor (vec3)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(procedural::VertexData),
                          reinterpret_cast<void*>(offsetof(procedural::VertexData, biomeColor)));
    
    glBindVertexArray(0);
    
    mesh.indexCount = meshData.indices.size();
    
    return mesh;
}

void PlanetRenderer::deleteMesh(GLMesh& mesh) {
    if (mesh.VAO != 0) {
        glDeleteVertexArrays(1, &mesh.VAO);
        mesh.VAO = 0;
    }
    if (mesh.VBO != 0) {
        glDeleteBuffers(1, &mesh.VBO);
        mesh.VBO = 0;
    }
    if (mesh.EBO != 0) {
        glDeleteBuffers(1, &mesh.EBO);
        mesh.EBO = 0;
    }
    mesh.indexCount = 0;
}

void PlanetRenderer::uploadPlanetData(const procedural::GeneratedPlanetData& data) {
    // Clean up existing meshes
    for (auto& mesh : m_cachedData.lodMeshes) {
        deleteMesh(mesh);
    }
    m_cachedData.lodMeshes.clear();
    
    if (m_cachedData.hasClouds) {
        deleteMesh(m_cachedData.cloudMesh);
        m_cachedData.hasClouds = false;
    }
    
    if (m_cachedData.hasRings) {
        deleteMesh(m_cachedData.ringMesh);
        m_cachedData.hasRings = false;
    }
    
    // Upload LOD meshes
    m_cachedData.lodMeshes.reserve(data.lodMeshes.size());
    for (const auto& lodMesh : data.lodMeshes) {
        m_cachedData.lodMeshes.push_back(uploadMeshToGPU(lodMesh));
    }
    
    // Upload cloud mesh if present
    if (!data.cloudMesh.vertices.empty()) {
        m_cachedData.cloudMesh = uploadMeshToGPU(data.cloudMesh);
        m_cachedData.hasClouds = true;
    }
    
    // Upload ring mesh if present
    if (!data.ringMesh.vertices.empty()) {
        m_cachedData.ringMesh = uploadMeshToGPU(data.ringMesh);
        m_cachedData.hasRings = true;
    }
    
    // Update metadata
    m_cachedData.seed = data.seed;
    m_cachedData.radius = data.radius;
    m_cachedData.isDirty = false;
    
    m_stats.cloudsVisible = m_cachedData.hasClouds;
    m_stats.ringsVisible = m_cachedData.hasRings;
}

int PlanetRenderer::selectLOD(float cameraDistance, float radius) const {
    if (!m_config.enableShadows) {
        // If shadows disabled, might use simpler LOD selection
    }
    
    // Normalize distance by planet radius
    float normalizedDist = cameraDistance / radius;
    
    // Simple distance-based LOD selection
    // Could be enhanced with screen-space error metrics
    if (normalizedDist < 2.0f) return 5;      // Highest detail
    if (normalizedDist < 5.0f) return 4;
    if (normalizedDist < 10.0f) return 3;
    if (normalizedDist < 20.0f) return 2;
    if (normalizedDist < 50.0f) return 1;
    return 0;                                  // Lowest detail
}

void PlanetRenderer::blendLODTransition(int fromLOD, int toLOD, float alpha) {
    // Advanced: Implement geomorphing or cross-fading between LODs
    // This is a placeholder for future enhancement
    
    // For now, we simply snap to the target LOD
    // Production implementations might:
    // 1. Use vertex displacement for geomorphing
    // 2. Alpha-blend between two LOD meshes
    // 3. Use GPU tessellation for smooth transitions
    (void)fromLOD;
    (void)toLOD;
    (void)alpha;
}

void PlanetRenderer::invalidateCache() {
    m_cachedData.isDirty = true;
}

bool PlanetRenderer::needsRefresh(uint32_t currentSeed) const {
    return m_cachedData.isDirty || m_cachedData.seed != currentSeed;
}

void PlanetRenderer::render(const glm::mat4& model, const glm::mat4& view,
                            const glm::mat4& projection, const glm::vec3& cameraPos,
                            const PlanetUniforms& uniforms) {
    if (!m_initialized || m_cachedData.isDirty || m_cachedData.lodMeshes.empty()) {
        return;
    }
    
    // Calculate camera distance for LOD selection
    glm::vec3 worldPos = glm::vec3(model[3]);
    float cameraDistance = glm::length(cameraPos - worldPos);
    
    m_stats.cameraDistance = cameraDistance;
    
    // Select appropriate LOD
    int targetLOD = selectLOD(cameraDistance, m_cachedData.radius);
    targetLOD = std::clamp(targetLOD, 0, static_cast<int>(m_cachedData.lodMeshes.size()) - 1);
    
    // Handle LOD transition
    if (targetLOD != m_cachedData.currentLOD) {
        // Could blend here for smooth transitions
        m_cachedData.currentLOD = targetLOD;
    }
    
    // Update stats
    if (targetLOD >= 0 && targetLOD < static_cast<int>(m_cachedData.lodMeshes.size())) {
        const auto& mesh = m_cachedData.lodMeshes[targetLOD];
        m_stats.vertexCount = 0;  // Would need to track this
        m_stats.indexCount = mesh.indexCount;
    }
    
    // Bind appropriate shader
    glUseProgram(m_planetShader);
    
    // Upload uniforms
    GLint loc;
    
    loc = glGetUniformLocation(m_planetShader, "uModel");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.model));
    
    loc = glGetUniformLocation(m_planetShader, "uView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.view));
    
    loc = glGetUniformLocation(m_planetShader, "uProjection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.projection));
    
    loc = glGetUniformLocation(m_planetShader, "uViewPosition");
    glUniform3fv(loc, 1, glm::value_ptr(cameraPos));
    
    loc = glGetUniformLocation(m_planetShader, "uLightDirection");
    glUniform3fv(loc, 1, glm::value_ptr(uniforms.lightDirection));
    
    loc = glGetUniformLocation(m_planetShader, "uLightColor");
    glUniform3fv(loc, 1, glm::value_ptr(uniforms.lightColor));
    
    loc = glGetUniformLocation(m_planetShader, "uAmbientColor");
    glUniform3fv(loc, 1, glm::value_ptr(uniforms.ambientColor));
    
    loc = glGetUniformLocation(m_planetShader, "uTime");
    glUniform1f(loc, uniforms.time);
    
    loc = glGetUniformLocation(m_planetShader, "uIsGasGiant");
    glUniform1i(loc, uniforms.isGasGiant ? 1 : 0);
    
    // Gas giant uniforms
    loc = glGetUniformLocation(m_planetShader, "uGasPrimaryColor");
    glUniform3fv(loc, 1, glm::value_ptr(uniforms.gasPrimaryColor));
    
    loc = glGetUniformLocation(m_planetShader, "uGasSecondaryColor");
    glUniform3fv(loc, 1, glm::value_ptr(uniforms.gasSecondaryColor));
    
    loc = glGetUniformLocation(m_planetShader, "uGasAccentColor");
    glUniform3fv(loc, 1, glm::value_ptr(uniforms.gasAccentColor));
    
    loc = glGetUniformLocation(m_planetShader, "uGasBandCount");
    glUniform1f(loc, uniforms.gasBandCount);
    
    loc = glGetUniformLocation(m_planetShader, "uGasTurbulence");
    glUniform1f(loc, uniforms.gasTurbulence);
    
    // Render main planet mesh at selected LOD
    const auto& mesh = m_cachedData.lodMeshes[targetLOD];
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    // Render clouds if enabled and present
    if (m_cachedData.hasClouds && !uniforms.isGasGiant) {
        glUseProgram(m_cloudShader);
        
        // Upload cloud-specific uniforms
        loc = glGetUniformLocation(m_cloudShader, "uModel");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.model));
        
        loc = glGetUniformLocation(m_cloudShader, "uView");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.view));
        
        loc = glGetUniformLocation(m_cloudShader, "uProjection");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.projection));
        
        loc = glGetUniformLocation(m_cloudShader, "uTime");
        glUniform1f(loc, uniforms.time);
        
        loc = glGetUniformLocation(m_cloudShader, "uCloudSpeed");
        glUniform1f(loc, uniforms.cloudSpeed);
        
        loc = glGetUniformLocation(m_cloudShader, "uCloudColor");
        glUniform3fv(loc, 1, glm::value_ptr(uniforms.cloudColor));
        
        loc = glGetUniformLocation(m_cloudShader, "uCloudDensity");
        glUniform1f(loc, uniforms.cloudDensity);
        
        loc = glGetUniformLocation(m_cloudShader, "uCloudCoverage");
        glUniform1f(loc, uniforms.cloudCoverage);
        
        // Enable blending for transparent clouds
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // Don't write to depth buffer for clouds
        
        glBindVertexArray(m_cachedData.cloudMesh.VAO);
        glDrawElements(GL_TRIANGLES, 
                       static_cast<GLsizei>(m_cachedData.cloudMesh.indexCount),
                       GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    
    // Render rings if enabled and present
    if (m_cachedData.hasRings) {
        glUseProgram(m_ringShader);
        
        // Upload ring-specific uniforms
        loc = glGetUniformLocation(m_ringShader, "uModel");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.model));
        
        loc = glGetUniformLocation(m_ringShader, "uView");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.view));
        
        loc = glGetUniformLocation(m_ringShader, "uProjection");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(uniforms.projection));
        
        loc = glGetUniformLocation(m_ringShader, "uOpacity");
        glUniform1f(loc, uniforms.ringOpacity);
        
        loc = glGetUniformLocation(m_ringShader, "uTime");
        glUniform1f(loc, uniforms.time);
        
        // Enable blending for transparent rings
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        
        glBindVertexArray(m_cachedData.ringMesh.VAO);
        glDrawElements(GL_TRIANGLE_STRIP,
                       static_cast<GLsizei>(m_cachedData.ringMesh.indexCount),
                       GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    
    glUseProgram(0);
}

} // namespace rendering
} // namespace agss
