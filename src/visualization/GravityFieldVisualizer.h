#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include <memory>
#include <functional>

namespace agss {

/**
 * Gravitational Field Visualization Modes
 */
enum class GravityVizMode {
    None = 0,
    VectorField,      // Arrows showing field direction and magnitude
    FieldLines,       // Streamlines following field
    Heatmap           // Color-coded field strength
};

/**
 * Configuration for gravitational field visualization
 */
struct GravityVizConfig {
    GravityVizMode mode = GravityVizMode::None;
    
    // Vector field settings
    float vectorSpacing = 2.0f;        // Spacing between vectors in AU
    float vectorScale = 1.0f;          // Scale factor for arrow size
    float maxVectorLength = 1.0f;      // Maximum arrow length
    
    // Field line settings
    int fieldLineCount = 32;           // Number of field lines per body
    float fieldLineLength = 10.0f;     // Length of field lines in AU
    
    // Heatmap settings
    float heatmapResolution = 0.5f;    // Grid cell size in AU
    float heatmapMaxStrength = 1.0f;   // Strength for full color intensity
    
    // Color gradient (weak to strong field)
    glm::vec3 weakColor{0.0f, 0.0f, 1.0f};    // Blue for weak
    glm::vec3 mediumColor{0.0f, 1.0f, 0.0f};  // Green for medium
    glm::vec3 strongColor{1.0f, 0.0f, 0.0f};  // Red for strong
    
    // Visibility
    bool enabled = false;
    float opacity = 0.6f;
};

/**
 * Gravitational Field Visualizer
 * 
 * Renders gravitational field using multiple visualization modes.
 * Completely separate from physics calculations - only reads state.
 */
class GravityFieldVisualizer {
public:
    GravityFieldVisualizer();
    ~GravityFieldVisualizer();
    
    /**
     * Initialize OpenGL resources
     */
    bool initialize();
    
    /**
     * Shutdown and cleanup resources
     */
    void shutdown();
    
    /**
     * Update visualization data based on current body positions/masses
     * @param bodyPositions World-space positions of bodies
     * @param bodyMasses Masses of bodies (in solar masses)
     */
    void update(const std::vector<glm::dvec3>& bodyPositions,
                const std::vector<double>& bodyMasses);
    
    /**
     * Render the gravitational field visualization
     * @param view View matrix
     * @param projection Projection matrix
     * @param config Visualization configuration
     */
    void render(const glm::mat4& view,
                const glm::mat4& projection,
                const GravityVizConfig& config);
    
    /**
     * Get current configuration (for UI)
     */
    GravityVizConfig& getConfig() { return m_config; }
    const GravityVizConfig& getConfig() const { return m_config; }
    
    /**
     * Toggle visibility
     */
    void setEnabled(bool enabled) { m_config.enabled = enabled; }
    bool isEnabled() const { return m_config.enabled; }
    
private:
    GravityVizConfig m_config;
    
    // OpenGL resources
    GLuint m_vectorVAO = 0;
    GLuint m_vectorVBO = 0;
    GLuint m_lineVAO = 0;
    GLuint m_lineVBO = 0;
    GLuint m_heatmapVAO = 0;
    GLuint m_heatmapVBO = 0;
    GLuint m_heatmapTexture = 0;
    
    // Shader programs
    GLuint m_vectorShader = 0;
    GLuint m_lineShader = 0;
    GLuint m_heatmapShader = 0;
    
    // Cached data
    std::vector<glm::vec3> m_vectorPositions;
    std::vector<glm::vec3> m_vectorDirections;
    std::vector<float> m_vectorMagnitudes;
    
    std::vector<glm::vec3> m_lineVertices;
    std::vector<float> m_lineStrengths;
    
    // Grid dimensions for heatmap
    int m_gridWidth = 0;
    int m_gridHeight = 0;
    
    /**
     * Generate vector field mesh
     */
    void generateVectorField(const std::vector<glm::dvec3>& positions,
                             const std::vector<double>& masses);
    
    /**
     * Generate field lines
     */
    void generateFieldLines(const std::vector<glm::dvec3>& positions,
                            const std::vector<double>& masses);
    
    /**
     * Generate heatmap texture
     */
    void generateHeatmap(const std::vector<glm::dvec3>& positions,
                         const std::vector<double>& masses);
    
    /**
     * Calculate gravitational field at a point
     */
    glm::dvec3 calculateField(const glm::dvec3& point,
                              const std::vector<glm::dvec3>& positions,
                              const std::vector<double>& masses,
                              double G = 0.000295912208286);
    
    /**
     * Create shader program
     */
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
    
    /**
     * Interpolate color based on field strength
     */
    glm::vec3 interpolateColor(float t) const;
};

} // namespace agss
