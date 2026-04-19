#include "debug/OrbitTrailsSystem.h"

#include "physics/PhysicsBody.h"

#include <algorithm>
#include <cmath>

namespace agss {

OrbitTrailsSystem::OrbitTrailsSystem() = default;

OrbitTrailsSystem::~OrbitTrailsSystem() = default;

void OrbitTrailsSystem::Configure(size_t maxPointsPerBody, bool enabled) {
    m_config.maxPointsPerBody = (maxPointsPerBody == 0) ? 1 : maxPointsPerBody;
    m_config.enabled = enabled;
}

void OrbitTrailsSystem::Reset(size_t bodyCount) {
    m_history.clear();
    m_timestamps.clear();
    m_history.resize(bodyCount);
    m_timestamps.resize(bodyCount);
    m_currentSampleCount = 0;
}

void OrbitTrailsSystem::PushSamples(const std::vector<PhysicsBody*>& bodies) {
    if (!m_config.enabled) return;
    
    if (m_history.size() != bodies.size()) {
        Reset(bodies.size());
    }
    
    // Respect sample interval for performance
    if (m_config.sampleInterval > 1) {
        m_currentSampleCount++;
        if (m_currentSampleCount % m_config.sampleInterval != 0) {
            return;
        }
    }
    
    for (size_t i = 0; i < bodies.size(); ++i) {
        auto* b = bodies[i];
        if (!b) continue;
        
        auto& h = m_history[i];
        auto& t = m_timestamps[i];
        
        h.push_back(b->position);
        t.push_back(m_currentSampleCount * m_config.sampleInterval);
        
        // Trim to max points
        while (h.size() > m_config.maxPointsPerBody) {
            h.erase(h.begin());
            t.erase(t.begin());
        }
    }
}

std::vector<glm::vec3> OrbitTrailsSystem::interpolateSegment(
    const glm::dvec3& p0,
    const glm::dvec3& p1,
    const glm::dvec3& p2,
    const glm::dvec3& p3,
    int segments) const 
{
    std::vector<glm::vec3> result;
    result.reserve(segments + 1);
    
    if (!m_config.smoothInterpolation || segments <= 0) {
        // Simple linear interpolation
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            result.push_back(glm::mix(glm::vec3(p1), glm::vec3(p2), t));
        }
        return result;
    }
    
    // Catmull-Rom spline interpolation
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float t2 = t * t;
        float t3 = t2 * t;
        
        // Catmull-Rom basis functions
        float b0 = -0.5f * t3 + t2 - 0.5f * t;
        float b1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
        float b2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        float b3 = 0.5f * t3 - 0.5f * t2;
        
        glm::dvec3 point = b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;
        result.push_back(glm::vec3(point));
    }
    
    return result;
}

float OrbitTrailsSystem::calculateAlpha(size_t index, size_t totalPoints) const {
    if (!m_config.fadeEnabled || totalPoints <= 1) {
        return m_config.alphaMax;
    }
    
    float t = static_cast<float>(index) / static_cast<float>(totalPoints - 1);
    
    // Apply visual length multiplier
    t = t * m_visualLength;
    t = glm::clamp(t, 0.0f, 1.0f);
    
    // Smooth step for nicer fade curve
    t = t * t * (3.0f - 2.0f * t);
    
    return m_config.alphaMin + (m_config.alphaMax - m_config.alphaMin) * t;
}

glm::vec3 OrbitTrailsSystem::calculateColor(size_t index, size_t totalPoints) const {
    if (!m_config.useColorGradient || totalPoints <= 1) {
        return m_config.newColor;
    }
    
    float t = static_cast<float>(index) / static_cast<float>(totalPoints - 1);
    t = glm::clamp(t, 0.0f, 1.0f);
    
    return glm::mix(m_config.oldColor, m_config.newColor, t);
}

TrailRenderData OrbitTrailsSystem::BuildRenderDataForBody(size_t bodyIndex, float alphaMin) const {
    TrailRenderData out{};
    out.bodyIndex = static_cast<int>(bodyIndex);
    
    if (bodyIndex >= m_history.size()) return out;
    
    const auto& h = m_history[bodyIndex];
    if (h.empty()) return out;
    
    const size_t n = h.size();
    
    if (m_config.smoothInterpolation && n >= 4 && m_config.interpolationSegments > 0) {
        // Use interpolated points
        size_t totalInterpolated = (n - 3) * (m_config.interpolationSegments + 1) + 1;
        out.vertices.reserve(totalInterpolated);
        
        for (size_t i = 0; i < n - 3; ++i) {
            auto segment = interpolateSegment(h[i], h[i+1], h[i+2], h[i+3], 
                                              m_config.interpolationSegments);
            
            for (size_t j = 0; j < segment.size(); ++j) {
                size_t globalIdx = i * m_config.interpolationSegments + j;
                size_t refIdx = std::min(globalIdx / m_config.interpolationSegments, n - 1);
                
                float alpha = std::max(calculateAlpha(refIdx, n), alphaMin);
                float age = static_cast<float>(refIdx) / n;
                
                out.vertices.push_back({segment[j], alpha, age});
            }
        }
    } else {
        // Use raw history points
        out.vertices.reserve(n);
        
        for (size_t i = 0; i < n; ++i) {
            float alpha = std::max(calculateAlpha(i, n), alphaMin);
            float age = static_cast<float>(i) / n;
            
            out.vertices.push_back({glm::vec3(h[i]), alpha, age});
        }
    }
    
    return out;
}

std::vector<TrailRenderData> OrbitTrailsSystem::BuildAllRenderData(float alphaMin) const {
    std::vector<TrailRenderData> result;
    result.reserve(m_history.size());
    
    for (size_t i = 0; i < m_history.size(); ++i) {
        result.push_back(BuildRenderDataForBody(i, alphaMin));
    }
    
    return result;
}

} // namespace agss

