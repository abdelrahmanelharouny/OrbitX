#version 330 core

// Cloud layer vertex shader
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 worldPos;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;
uniform float uCloudSpeed;

void main() {
    // Rotate clouds independently of planet
    float angle = uTime * uCloudSpeed;
    float cosA = cos(angle);
    float sinA = sin(angle);
    
    // Simple Y-axis rotation for cloud movement
    mat4 rotY = mat4(
        cosA, 0.0, -sinA, 0.0,
        0.0, 1.0, 0.0, 0.0,
        sinA, 0.0, cosA, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    
    vec4 rotatedPos = rotY * vec4(inPosition, 1.0);
    vec4 worldPosition = uModel * rotatedPos;
    
    vs_out.worldPos = worldPosition.xyz;
    vs_out.position = inPosition;
    vs_out.normal = mat3(transpose(inverse(uModel))) * inNormal;
    vs_out.uv = inUV + vec2(uTime * uCloudSpeed * 0.5, 0.0);  // UV scroll
    
    gl_Position = uProjection * uView * worldPosition;
}
