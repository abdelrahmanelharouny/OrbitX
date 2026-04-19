#pragma once

// Planet Renderer - Consumes generated mesh data and renders it
// Separated from generation logic (single responsibility principle)

#include "procedural/PlanetGenerator.h"
#include <glm/mat4x4.hpp>
#include <vector>
#include <memory>

namespace agss {
namespace rendering {

// Forward declarations for OpenGL objects (implementation-dependent)
struct GLMesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    size_t indexCount = 0;
};

// ============================================================================
// Render Configuration
// ============================================================================

struct PlanetRenderConfig {
    bool enableShadows = true;
    bool enableAtmosphere = true;
    float atmosphereStrength = 1.0f;
    bool enableCloudShadows = true;
    
    // LOD transition smoothing
    float lodTransitionDistance = 0.5f;
};

// ============================================================================
// Uniform Data (for shader uploads)
// ============================================================================

struct PlanetUniforms {
    glm::mat4 model{1.0f};
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    
    glm::vec3 viewPosition{0.0f};
    glm::vec3 lightDirection{1.0f, 0.5f, 0.2f};
    glm::vec3 lightColor{1.0f};
    glm::vec3 ambientColor{0.1f};
    
    float time = 0.0f;
    bool isGasGiant = false;
    
    // Gas giant parameters
    glm::vec3 gasPrimaryColor{0.8f, 0.7f, 0.5f};
    glm::vec3 gasSecondaryColor{0.6f, 0.5f, 0.4f};
    glm::vec3 gasAccentColor{0.9f, 0.8f, 0.7f};
    float gasBandCount = 8.0f;
    float gasTurbulence = 0.3f;
    
    // Cloud parameters
    glm::vec3 cloudColor{1.0f};
    float cloudDensity = 0.6f;
    float cloudCoverage = 0.5f;
    float cloudSpeed = 0.01f;
    
    // Ring parameters
    float ringOpacity = 0.6f;
};

// ============================================================================
// Cached Render State (avoids per-frame regeneration)
// ============================================================================

struct CachedPlanetRenderData {
    // Meshes for each LOD level
    std::vector<GLMesh> lodMeshes;
    
    // Cloud mesh (if applicable)
    GLMesh cloudMesh;
    bool hasClouds = false;
    
    // Ring mesh (if applicable)
    GLMesh ringMesh;
    bool hasRings = false;
    
    // Current active LOD
    int currentLOD = 0;
    
    // Metadata
    uint32_t seed = 0;
    float radius = 1.0f;
    bool isDirty = true;  // Needs regeneration
    
    // Last camera distance for LOD tracking
    float lastCameraDistance = 0.0f;
};

// ============================================================================
// Planet Renderer Class
// ============================================================================

class PlanetRenderer {
public:
    PlanetRenderer();
    ~PlanetRenderer();
    
    // Initialize renderer (OpenGL context must be active)
    bool initialize();
    
    // Shutdown and cleanup resources
    void shutdown();
    
    // Set render configuration
    void setConfig(const PlanetRenderConfig& config);
    const PlanetRenderConfig& getConfig() const { return m_config; }
    
    // Upload planet data from generator (call when planet changes)
    void uploadPlanetData(const procedural::GeneratedPlanetData& data);
    
    // Render the planet at given transform
    // Automatically handles LOD selection based on camera distance
    void render(const glm::mat4& model, const glm::mat4& view, 
                const glm::mat4& projection, const glm::vec3& cameraPos,
                const PlanetUniforms& uniforms);
    
    // Force regeneration of cached mesh data
    void invalidateCache();
    
    // Check if cache needs refresh
    bool needsRefresh(uint32_t currentSeed) const;
    
    // Get current LOD statistics (for debugging/UI)
    struct RenderStats {
        int currentLOD = 0;
        size_t vertexCount = 0;
        size_t indexCount = 0;
        float cameraDistance = 0.0f;
        bool cloudsVisible = false;
        bool ringsVisible = false;
    };
    RenderStats getStats() const { return m_stats; }
    
private:
    // Upload a single mesh to GPU
    GLMesh uploadMeshToGPU(const procedural::LODMeshData& meshData);
    
    // Delete GPU mesh resources
    void deleteMesh(GLMesh& mesh);
    
    // Select appropriate LOD based on camera distance
    int selectLOD(float cameraDistance, float radius) const;
    
    // Smooth LOD transitions (optional, for advanced implementation)
    void blendLODTransition(int fromLOD, int toLOD, float alpha);
    
    // Create shader programs
    bool createShaderPrograms();
    void destroyShaderPrograms();
    
    // Shader program handles
    unsigned int m_planetShader = 0;
    unsigned int m_cloudShader = 0;
    unsigned int m_ringShader = 0;
    
    // Cached render data
    CachedPlanetRenderData m_cachedData;
    
    // Configuration
    PlanetRenderConfig m_config;
    
    // Statistics
    RenderStats m_stats;
    
    // Initialization state
    bool m_initialized = false;
};

} // namespace rendering
} // namespace agss
