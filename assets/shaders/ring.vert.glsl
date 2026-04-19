#version 330 core

// Planet ring vertex shader
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inColor;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 color;
    vec3 worldPos;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vec4 worldPosition = uModel * vec4(inPosition, 1.0);
    
    vs_out.worldPos = worldPosition.xyz;
    vs_out.position = inPosition;
    vs_out.normal = mat3(transpose(inverse(uModel))) * inNormal;
    vs_out.uv = inUV;
    vs_out.color = inColor;
    
    gl_Position = uProjection * uView * worldPosition;
}
