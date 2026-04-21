#include "Shader.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader() = default;

Shader::~Shader() {
    if (programID != 0) {
        glDeleteProgram(programID);
    }
}

bool Shader::loadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    std::ifstream vFile(vertexPath);
    std::ifstream fFile(fragmentPath);
    
    if (!vFile.is_open() || !fFile.is_open()) {
        std::cerr << "Failed to open shader files: " << vertexPath 
                  << ", " << fragmentPath << std::endl;
        return false;
    }
    
    std::stringstream vBuffer, fBuffer;
    vBuffer << vFile.rdbuf();
    fBuffer << fFile.rdbuf();
    
    return loadFromSource(vBuffer.str(), fBuffer.str());
}

bool Shader::loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
    // Compile vertex shader
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    if (vertexShader == 0) return false;
    
    // Compile fragment shader
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Link program
    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);
    
    // Check linking errors
    if (!checkCompileErrors(programID, "PROGRAM")) {
        glDeleteProgram(programID);
        programID = 0;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    // Clean up shaders (they're linked into the program now)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

unsigned int Shader::compileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    if (!checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")) {
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::checkCompileErrors(unsigned int object, const std::string& type) {
    int success;
    char infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(object, 1024, nullptr, infoLog);
            std::cerr << "Shader compilation error (" << type << "): " 
                      << infoLog << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(object, 1024, nullptr, infoLog);
            std::cerr << "Shader linking error (" << type << "): " 
                      << infoLog << std::endl;
            return false;
        }
    }
    
    return true;
}

void Shader::use() const {
    if (programID != 0) {
        glUseProgram(programID);
    }
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &value[0][0]);
}
