#pragma once

#include "../procedural/PlanetInstance.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace rendering {

/**
 * @brief Vertex format for procedural planet meshes
 */
struct PlanetVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    float height;  // For biome coloring
    
    static std::vector<PlanetVertex> fromMesh(
        const procedural::PlanetMesh& mesh);
};

/**
 * @brief Renderer for procedurally generated planets
 * 
 * Handles VAO/VBO management, shader binding, and render states
 * for planet meshes with atmosphere and cloud layers.
 */
class PlanetRenderer {
public:
    PlanetRenderer();
    ~PlanetRenderer();
    
    /**
     * @brief Initialize renderer resources
     */
    bool initialize();
    
    /**
     * @brief Render a planet instance
     * @param instance Planet to render
     * @param view View matrix
     * @param projection Projection matrix
     * @param lightDir Direction to sun/light source
     * @param cameraPos Camera position for effects
     */
    void render(const procedural::PlanetInstance& instance,
                const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& lightDir,
                const glm::vec3& cameraPos = glm::vec3(0.0f));
    
    /**
     * @brief Set time uniform for animated effects
     */
    void setTime(float time);
    
    /**
     * @brief Cleanup OpenGL resources
     */
    void cleanup();

private:
    struct PlanetVAO {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        int indexCount;
        
        // Atmosphere
        GLuint atmVAO;
        GLuint atmVBO;
        GLuint atmEBO;
        int atmIndexCount;
        
        // Clouds
        GLuint cloudVAO;
        GLuint cloudVBO;
        GLuint cloudEBO;
        int cloudIndexCount;
    };
    
    std::vector<PlanetVAO> planetVAOs;
    
    Shader planetShader;
    Shader atmosphereShader;
    Shader cloudsShader;
    
    float currentTime = 0.0f;
    
    /**
     * @brief Create VAO from planet mesh
     */
    PlanetVAO createVAO(const procedural::PlanetInstance& instance);
    
    /**
     * @brief Find or create VAO for instance
     */
    PlanetVAO& getOrCreateVAO(const procedural::PlanetInstance& instance);
    
    /**
     * @brief Setup shader uniforms for planet rendering
     */
    void setupPlanetShader(const glm::mat4& model,
                          const glm::mat4& view,
                          const glm::mat4& projection,
                          const glm::vec3& lightDir,
                          const glm::vec3& lightColor,
                          const glm::vec3& ambientColor,
                          const glm::vec3& planetColor);
    
    /**
     * @brief Setup shader uniforms for atmosphere
     */
    void setupAtmosphereShader(const glm::mat4& model,
                               const glm::mat4& view,
                               const glm::mat4& projection,
                               const glm::vec3& atmosphereColor,
                               const glm::vec3& lightDir,
                               const glm::vec3& viewPos);
    
    /**
     * @brief Setup shader uniforms for clouds
     */
    void setupCloudsShader(const glm::mat4& model,
                          const glm::mat4& view,
                          const glm::mat4& projection,
                          const glm::vec3& cloudColor);
};

} // namespace rendering
