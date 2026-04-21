#pragma once

#include <string>
#include <glm/glm.hpp>

/**
 * @brief Simple OpenGL shader wrapper
 * 
 * Handles shader compilation, linking, and uniform setting.
 */
class Shader {
public:
    Shader();
    ~Shader();
    
    /**
     * @brief Load and compile shaders from files
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @return true if successful
     */
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    
    /**
     * @brief Load and compile shaders from source strings
     */
    bool loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
    
    /**
     * @brief Activate the shader program
     */
    void use() const;
    
    /**
     * @brief Get the program ID
     */
    unsigned int getID() const { return programID; }
    
    // Uniform setters
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    unsigned int programID = 0;
    
    /**
     * @brief Compile a single shader
     */
    unsigned int compileShader(unsigned int type, const std::string& source);
    
    /**
     * @brief Check for compilation/linking errors
     */
    bool checkCompileErrors(unsigned int object, const std::string& type);
};
