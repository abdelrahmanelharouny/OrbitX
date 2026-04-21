#include "visualization/GravityFieldVisualizer.h"

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include <cmath>
#include <algorithm>

namespace agss {

GravityFieldVisualizer::GravityFieldVisualizer() = default;

GravityFieldVisualizer::~GravityFieldVisualizer() {
    shutdown();
}

bool GravityFieldVisualizer::initialize() {
    // Create vector field shader
    const char* vectorVert = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aDirection;
        layout(location = 2) in float aMagnitude;
        
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        uniform float uVectorScale;
        uniform vec3 uWeakColor;
        uniform vec3 uStrongColor;
        uniform float uOpacity;
        
        out vec3 vColor;
        out float vAlpha;
        
        void main() {
            vec3 dir = normalize(aDirection);
            vec3 tail = aPosition;
            vec3 tip = aPosition + dir * uVectorScale * clamp(aMagnitude, 0.0, 1.0);
            
            // Simple arrow: just render as line from tail to tip
            // Geometry shader would be better for actual arrows
            gl_Position = uProjection * uView * uModel * vec4(tail, 1.0);
            vColor = mix(uWeakColor, uStrongColor, clamp(aMagnitude, 0.0, 1.0));
            vAlpha = uOpacity;
        }
    )";
    
    const char* vectorFrag = R"(
        #version 330 core
        in vec3 vColor;
        in float vAlpha;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(vColor, vAlpha);
        }
    )";
    
    m_vectorShader = createShaderProgram(vectorVert, vectorFrag);
    
    // Create field line shader
    const char* lineVert = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in float aStrength;
        
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        uniform vec3 uWeakColor;
        uniform vec3 uMediumColor;
        uniform vec3 uStrongColor;
        uniform float uOpacity;
        
        out vec3 vColor;
        
        vec3 interpolateColor(float t) {
            if (t < 0.5) {
                return mix(uWeakColor, uMediumColor, t * 2.0);
            } else {
                return mix(uMediumColor, uStrongColor, (t - 0.5) * 2.0);
            }
        }
        
        void main() {
            gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
            vColor = interpolateColor(clamp(aStrength, 0.0, 1.0));
        }
    )";
    
    const char* lineFrag = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        
        uniform float uOpacity;
        
        void main() {
            FragColor = vec4(vColor, uOpacity);
        }
    )";
    
    m_lineShader = createShaderProgram(lineVert, lineFrag);
    
    // Create heatmap shader
    const char* heatmapVert = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aTexCoord;
        
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        
        out vec2 vTexCoord;
        
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
        }
    )";
    
    const char* heatmapFrag = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        
        uniform sampler2D uHeatmapTexture;
        uniform vec3 uWeakColor;
        uniform vec3 uMediumColor;
        uniform vec3 uStrongColor;
        uniform float uOpacity;
        
        vec3 interpolateColor(float t) {
            if (t < 0.5) {
                return mix(uWeakColor, uMediumColor, t * 2.0);
            } else {
                return mix(uMediumColor, uStrongColor, (t - 0.5) * 2.0);
            }
        }
        
        void main() {
            float strength = texture(heatmapTexture, vTexCoord).r;
            vec3 color = interpolateColor(strength);
            FragColor = vec4(color, uOpacity);
        }
    )";
    
    m_heatmapShader = createShaderProgram(heatmapVert, heatmapFrag);
    
    // Generate VAOs
    glGenVertexArrays(1, &m_vectorVAO);
    glGenBuffers(1, &m_vectorVBO);
    
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    
    glGenVertexArrays(1, &m_heatmapVAO);
    glGenBuffers(1, &m_heatmapVBO);
    glGenTextures(1, &m_heatmapTexture);
    
    return true;
}

void GravityFieldVisualizer::shutdown() {
    if (m_vectorVAO) {
        glDeleteVertexArrays(1, &m_vectorVAO);
        glDeleteBuffers(1, &m_vectorVBO);
    }
    if (m_lineVAO) {
        glDeleteVertexArrays(1, &m_lineVAO);
        glDeleteBuffers(1, &m_lineVBO);
    }
    if (m_heatmapVAO) {
        glDeleteVertexArrays(1, &m_heatmapVAO);
        glDeleteBuffers(1, &m_heatmapVBO);
        glDeleteTextures(1, &m_heatmapTexture);
    }
    if (m_vectorShader) glDeleteProgram(m_vectorShader);
    if (m_lineShader) glDeleteProgram(m_lineShader);
    if (m_heatmapShader) glDeleteProgram(m_heatmapShader);
    
    m_vectorVAO = m_lineVAO = m_heatmapVAO = 0;
    m_vectorVBO = m_lineVBO = m_heatmapVBO = 0;
    m_heatmapTexture = 0;
    m_vectorShader = m_lineShader = m_heatmapShader = 0;
}

GLuint GravityFieldVisualizer::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertexSource, nullptr);
    glCompileShader(vertShader);
    
    GLint success;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vertShader, 512, nullptr, log);
        // Log error in production
    }
    
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragShader);
    
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fragShader, 512, nullptr, log);
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    return program;
}

glm::dvec3 GravityFieldVisualizer::calculateField(
    const glm::dvec3& point,
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses,
    double G) 
{
    glm::dvec3 field{0.0};
    
    for (size_t i = 0; i < positions.size(); ++i) {
        glm::dvec3 r = positions[i] - point;
        double distSq = glm::dot(r, r);
        double dist = glm::length(r);
        
        if (dist < 0.01) continue; // Avoid singularity
        
        // g = G * M / r^2 (direction towards mass)
        double strength = G * masses[i] / distSq;
        field += glm::normalize(r) * strength;
    }
    
    return field;
}

void GravityFieldVisualizer::generateVectorField(
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses) 
{
    m_vectorPositions.clear();
    m_vectorDirections.clear();
    m_vectorMagnitudes.clear();
    
    // Determine bounds
    glm::dvec3 minPos{1e10}, maxPos{-1e10};
    for (const auto& p : positions) {
        minPos = glm::min(minPos, p);
        maxPos = glm::max(maxPos, p);
    }
    
    // Expand bounds
    double margin = 5.0;
    minPos -= glm::dvec3(margin);
    maxPos += glm::dvec3(margin);
    
    float spacing = m_config.vectorSpacing;
    double maxMag = 0.0;
    
    // First pass: calculate all vectors and find max magnitude
    struct VectorData {
        glm::vec3 pos;
        glm::vec3 dir;
        float mag;
    };
    std::vector<VectorData> vectors;
    
    for (double x = minPos.x; x <= maxPos.x; x += spacing) {
        for (double z = minPos.z; z <= maxPos.z; z += spacing) {
            glm::dvec3 point{x, 0.0, z};
            glm::dvec3 field = calculateField(point, positions, masses);
            double mag = glm::length(field);
            
            if (mag > 1e-10) {
                vectors.push_back({
                    glm::vec3(point),
                    glm::normalize(glm::vec3(field)),
                    static_cast<float>(mag)
                });
                maxMag = std::max(maxMag, mag);
            }
        }
    }
    
    // Second pass: normalize magnitudes
    if (maxMag > 1e-10) {
        for (const auto& v : vectors) {
            m_vectorPositions.push_back(v.pos);
            m_vectorDirections.push_back(v.dir);
            m_vectorMagnitudes.push_back(v.mag / static_cast<float>(maxMag));
        }
    }
    
    // Upload to GPU
    glBindVertexArray(m_vectorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_vectorVBO);
    
    struct Vertex {
        glm::vec3 position;
        glm::vec3 direction;
        float magnitude;
    };
    
    std::vector<Vertex> vertices;
    for (size_t i = 0; i < m_vectorPositions.size(); ++i) {
        vertices.push_back({
            m_vectorPositions[i],
            m_vectorDirections[i],
            m_vectorMagnitudes[i]
        });
    }
    
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void GravityFieldVisualizer::generateFieldLines(
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses) 
{
    m_lineVertices.clear();
    m_lineStrengths.clear();
    
    int linesPerBody = m_config.fieldLineCount;
    float lineLength = m_config.fieldLineLength;
    float startRadius = 0.5f;
    
    for (size_t bodyIdx = 0; bodyIdx < positions.size(); ++bodyIdx) {
        if (masses[bodyIdx] <= 0) continue;
        
        glm::dvec3 center = positions[bodyIdx];
        
        for (int i = 0; i < linesPerBody; ++i) {
            // Distribute starting points on sphere around body
            float theta = 2.0f * 3.14159f * static_cast<float>(i) / linesPerBody;
            float phi = 3.14159f * static_cast<float>(i) / linesPerBody;
            
            glm::dvec3 point = center + glm::dvec3(
                startRadius * sin(phi) * cos(theta),
                startRadius * cos(phi),
                startRadius * sin(phi) * sin(theta)
            );
            
            // Integrate field line
            float stepSize = 0.1f;
            int maxSteps = static_cast<int>(lineLength / stepSize);
            
            for (int step = 0; step < maxSteps; ++step) {
                glm::dvec3 field = calculateField(point, positions, masses);
                double mag = glm::length(field);
                
                if (mag < 1e-10) break;
                
                m_lineVertices.push_back(glm::vec3(point));
                m_lineStrengths.push_back(static_cast<float>(mag));
                
                // Move along field direction
                point += glm::normalize(field) * stepSize;
                
                // Check if we've gone too far
                if (glm::distance(point, center) > lineLength) break;
            }
            
            // Add separator (degenerate vertex)
            m_lineVertices.push_back(m_lineVertices.back());
            m_lineStrengths.push_back(0.0f);
        }
    }
    
    // Upload to GPU
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    
    struct Vertex {
        glm::vec3 position;
        float strength;
    };
    
    std::vector<Vertex> vertices;
    for (size_t i = 0; i < m_lineVertices.size(); ++i) {
        vertices.push_back({m_lineVertices[i], m_lineStrengths[i]});
    }
    
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void GravityFieldVisualizer::generateHeatmap(
    const std::vector<glm::dvec3>& positions,
    const std::vector<double>& masses) 
{
    // Determine bounds
    glm::dvec3 minPos{1e10}, maxPos{-1e10};
    for (const auto& p : positions) {
        minPos = glm::min(minPos, p);
        maxPos = glm::max(maxPos, p);
    }
    
    double margin = 5.0;
    minPos -= glm::dvec3(margin);
    maxPos += glm::dvec3(margin);
    
    float resolution = m_config.heatmapResolution;
    m_gridWidth = static_cast<int>((maxPos.x - minPos.x) / resolution) + 1;
    m_gridHeight = static_cast<int>((maxPos.z - minPos.z) / resolution) + 1;
    
    // Clamp grid size
    m_gridWidth = std::min(m_gridWidth, 256);
    m_gridHeight = std::min(m_gridHeight, 256);
    
    std::vector<float> textureData(m_gridWidth * m_gridHeight);
    
    double maxStrength = 0.0;
    
    // Calculate field strength at each grid point
    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            double u = static_cast<double>(x) / (m_gridWidth - 1);
            double v = static_cast<double>(y) / (m_gridHeight - 1);
            
            glm::dvec3 point{
                minPos.x + u * (maxPos.x - minPos.x),
                0.0,
                minPos.z + v * (maxPos.z - minPos.z)
            };
            
            glm::dvec3 field = calculateField(point, positions, masses);
            double strength = glm::length(field);
            
            textureData[y * m_gridWidth + x] = static_cast<float>(strength);
            maxStrength = std::max(maxStrength, strength);
        }
    }
    
    // Normalize
    if (maxStrength > 1e-10) {
        for (auto& val : textureData) {
            val /= static_cast<float>(maxStrength);
        }
    }
    
    // Upload texture
    glBindTexture(GL_TEXTURE_2D, m_heatmapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_gridWidth, m_gridHeight, 0,
                 GL_RED, GL_FLOAT, textureData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Generate mesh
    glBindVertexArray(m_heatmapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_heatmapVBO);
    
    struct Vertex {
        glm::vec3 position;
        glm::vec2 texCoord;
    };
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            double u = static_cast<double>(x) / (m_gridWidth - 1);
            double v = static_cast<double>(y) / (m_gridHeight - 1);
            
            glm::vec3 pos{
                static_cast<float>(minPos.x + u * (maxPos.x - minPos.x)),
                0.0f,
                static_cast<float>(minPos.z + v * (maxPos.z - minPos.z))
            };
            
            vertices.push_back({pos, glm::vec2(u, v)});
        }
    }
    
    // Generate indices for triangle strip
    for (int y = 0; y < m_gridHeight - 1; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            indices.push_back(y * m_gridWidth + x);
            indices.push_back((y + 1) * m_gridWidth + x);
        }
        if (y < m_gridHeight - 2) {
            indices.push_back((y + 1) * m_gridWidth);
            indices.push_back((y + 1) * m_gridWidth);
        }
    }
    
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

glm::vec3 GravityFieldVisualizer::interpolateColor(float t) const {
    t = glm::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f) {
        return glm::mix(m_config.weakColor, m_config.mediumColor, t * 2.0f);
    } else {
        return glm::mix(m_config.mediumColor, m_config.strongColor, (t - 0.5f) * 2.0f);
    }
}

void GravityFieldVisualizer::update(
    const std::vector<glm::dvec3>& bodyPositions,
    const std::vector<double>& bodyMasses) 
{
    if (!m_config.enabled || m_config.mode == GravityVizMode::None) {
        return;
    }
    
    switch (m_config.mode) {
        case GravityVizMode::VectorField:
            generateVectorField(bodyPositions, bodyMasses);
            break;
        case GravityVizMode::FieldLines:
            generateFieldLines(bodyPositions, bodyMasses);
            break;
        case GravityVizMode::Heatmap:
            generateHeatmap(bodyPositions, bodyMasses);
            break;
        default:
            break;
    }
}

void GravityFieldVisualizer::render(
    const glm::mat4& view,
    const glm::mat4& projection,
    const GravityVizConfig& config) 
{
    if (!config.enabled || config.mode == GravityVizMode::None) {
        return;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    glm::mat4 model = glm::mat4(1.0f);
    
    switch (config.mode) {
        case GravityVizMode::VectorField: {
            glUseProgram(m_vectorShader);
            
            GLint loc = glGetUniformLocation(m_vectorShader, "uModel");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &model[0][0]);
            loc = glGetUniformLocation(m_vectorShader, "uView");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &view[0][0]);
            loc = glGetUniformLocation(m_vectorShader, "uProjection");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &projection[0][0]);
            loc = glGetUniformLocation(m_vectorShader, "uVectorScale");
            glUniform1f(loc, config.vectorScale);
            loc = glGetUniformLocation(m_vectorShader, "uWeakColor");
            glUniform3f(loc, config.weakColor.r, config.weakColor.g, config.weakColor.b);
            loc = glGetUniformLocation(m_vectorShader, "uStrongColor");
            glUniform3f(loc, config.strongColor.r, config.strongColor.g, config.strongColor.b);
            loc = glGetUniformLocation(m_vectorShader, "uOpacity");
            glUniform1f(loc, config.opacity);
            
            glBindVertexArray(m_vectorVAO);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_vectorPositions.size()));
            glBindVertexArray(0);
            break;
        }
        
        case GravityVizMode::FieldLines: {
            glUseProgram(m_lineShader);
            
            GLint loc = glGetUniformLocation(m_lineShader, "uModel");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &model[0][0]);
            loc = glGetUniformLocation(m_lineShader, "uView");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &view[0][0]);
            loc = glGetUniformLocation(m_lineShader, "uProjection");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &projection[0][0]);
            loc = glGetUniformLocation(m_lineShader, "uWeakColor");
            glUniform3f(loc, config.weakColor.r, config.weakColor.g, config.weakColor.b);
            loc = glGetUniformLocation(m_lineShader, "uMediumColor");
            glUniform3f(loc, config.mediumColor.r, config.mediumColor.g, config.mediumColor.b);
            loc = glGetUniformLocation(m_lineShader, "uStrongColor");
            glUniform3f(loc, config.strongColor.r, config.strongColor.g, config.strongColor.b);
            loc = glGetUniformLocation(m_lineShader, "uOpacity");
            glUniform1f(loc, config.opacity);
            
            glBindVertexArray(m_lineVAO);
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(m_lineVertices.size()));
            glBindVertexArray(0);
            break;
        }
        
        case GravityVizMode::Heatmap: {
            glUseProgram(m_heatmapShader);
            
            GLint loc = glGetUniformLocation(m_heatmapShader, "uModel");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &model[0][0]);
            loc = glGetUniformLocation(m_heatmapShader, "uView");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &view[0][0]);
            loc = glGetUniformLocation(m_heatmapShader, "uProjection");
            glUniformMatrix4fv(loc, 1, GL_FALSE, &projection[0][0]);
            loc = glGetUniformLocation(m_heatmapShader, "uWeakColor");
            glUniform3f(loc, config.weakColor.r, config.weakColor.g, config.weakColor.b);
            loc = glGetUniformLocation(m_heatmapShader, "uMediumColor");
            glUniform3f(loc, config.mediumColor.r, config.mediumColor.g, config.mediumColor.b);
            loc = glGetUniformLocation(m_heatmapShader, "uStrongColor");
            glUniform3f(loc, config.strongColor.r, config.strongColor.g, config.strongColor.b);
            loc = glGetUniformLocation(m_heatmapShader, "uOpacity");
            glUniform1f(loc, config.opacity);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_heatmapTexture);
            loc = glGetUniformLocation(m_heatmapShader, "uHeatmapTexture");
            glUniform1i(loc, 0);
            
            glBindVertexArray(m_heatmapVAO);
            glDrawElements(GL_TRIANGLE_STRIP, m_gridWidth * m_gridHeight * 2,
                          GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            break;
        }
        
        default:
            break;
    }
    
    glEnable(GL_DEPTH_TEST);
}

} // namespace agss
