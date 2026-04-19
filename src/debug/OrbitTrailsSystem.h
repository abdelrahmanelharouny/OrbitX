#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <memory>

namespace agss {

struct PhysicsBody;

/**
 * Enhanced trail vertex with full color and timing information
 */
struct TrailVertex {
    glm::vec3 position;  // World-space position
    float alpha;         // Fade factor (0..1)
    float age;           // Age of this point (for dynamic effects)
};

/**
 * Render-ready trail data for a single body
 */
struct TrailRenderData {
    std::vector<TrailVertex> vertices;
    int bodyIndex = -1;
};

/**
 * Configuration for orbit trail rendering
 */
struct OrbitTrailConfig {
    size_t maxPointsPerBody = 4096;   // Maximum trail points per body
    bool enabled = true;               // Master enable switch
    
    // Visual properties
    float baseThickness = 2.0f;        // Base line thickness in pixels
    float minThickness = 0.5f;         // Minimum thickness at fade end
    float maxThickness = 4.0f;         // Maximum thickness at newest point
    
    // Fading
    float alphaMin = 0.05f;            // Minimum alpha (oldest points)
    float alphaMax = 1.0f;             // Maximum alpha (newest points)
    bool fadeEnabled = true;           // Enable gradient fading
    
    // Interpolation
    bool smoothInterpolation = true;   // Use Catmull-Rom spline interpolation
    int interpolationSegments = 2;     // Segments between sample points
    
    // Color gradient (optional, overrides alpha-only fading)
    bool useColorGradient = false;
    glm::vec3 oldColor{0.3f, 0.3f, 0.5f};   // Color for older trail segments
    glm::vec3 newColor{1.0f, 1.0f, 1.0f};   // Color for newer trail segments
    
    // Update rate (sample every N physics steps for performance)
    int sampleInterval = 1;            
};

/**
 * Enhanced Orbit Trails System
 * 
 * Provides smooth, visually appealing orbit trails with:
 * - Gradient fading over time
 * - Smooth interpolation between points
 * - Adjustable length and thickness
 * - Optional color gradients
 * 
 * Completely decoupled from physics simulation.
 */
class OrbitTrailsSystem {
public:
    OrbitTrailsSystem();
    ~OrbitTrailsSystem();
    
    /**
     * Configure the trails system
     * @param maxPointsPerBody Maximum points stored per body
     * @param enabled Whether trails are enabled
     */
    void Configure(size_t maxPointsPerBody, bool enabled);
    
    /**
     * Get current configuration for modification
     */
    OrbitTrailConfig& getConfig() { return m_config; }
    const OrbitTrailConfig& getConfig() const { return m_config; }
    
    bool IsEnabled() const { return m_config.enabled; }
    
    /**
     * Reset/clear all trail data
     * @param bodyCount Expected number of bodies
     */
    void Reset(size_t bodyCount);
    
    /**
     * Add new sample points from current physics state
     * @param bodies Current physics bodies
     */
    void PushSamples(const std::vector<PhysicsBody*>& bodies);
    
    /**
     * Build render data for a specific body with fading
     * @param bodyIndex Index of the body
     * @param alphaMin Minimum alpha for fading
     * @return Render-ready trail data
     */
    TrailRenderData BuildRenderDataForBody(size_t bodyIndex, float alphaMin = 0.05f) const;
    
    /**
     * Build render data for all bodies
     * @param alphaMin Minimum alpha for fading
     * @return Vector of trail data for each body
     */
    std::vector<TrailRenderData> BuildAllRenderData(float alphaMin = 0.05f) const;
    
    /**
     * Set trail visibility
     */
    void SetEnabled(bool enabled) { m_config.enabled = enabled; }
    
    /**
     * Set maximum trail length (affects visual appearance, not storage)
     */
    void SetVisualLength(float length) { m_visualLength = length; }
    float GetVisualLength() const { return m_visualLength; }
    
private:
    OrbitTrailConfig m_config;
    std::vector<std::vector<glm::dvec3>> m_history;
    std::vector<std::vector<double>> m_timestamps;  // Simulation time at each point
    float m_visualLength = 1.0f;  // Multiplier for visual trail length
    
    size_t m_currentSampleCount = 0;
    
    /**
     * Generate interpolated points between two history points
     */
    std::vector<glm::vec3> interpolateSegment(
        const glm::dvec3& p0,
        const glm::dvec3& p1,
        const glm::dvec3& p2,
        const glm::dvec3& p3,
        int segments) const;
    
    /**
     * Calculate alpha based on position in trail
     */
    float calculateAlpha(size_t index, size_t totalPoints) const;
    
    /**
     * Calculate color based on position in trail
     */
    glm::vec3 calculateColor(size_t index, size_t totalPoints) const;
};

} // namespace agss

