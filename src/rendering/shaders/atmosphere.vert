#version 330 core

// Vertex attributes
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// Outputs to fragment shader
out vec3 FragPos;
out vec2 TexCoord;
out float Visibility;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    
    // Calculate distance from camera for fog effect
    float dist = length(FragPos);
    Visibility = clamp(1.0 - dist / 5.0, 0.0, 1.0);
    
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
