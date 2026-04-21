#version 330 core

// Planet vertex shader with LOD support and biome coloring
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in float inElevation;
layout(location = 4) in vec3 inBiomeColor;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 uv;
    float elevation;
    vec3 biomeColor;
    vec3 worldPos;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform bool uIsGasGiant;

void main() {
    vec4 worldPosition = uModel * vec4(inPosition, 1.0);
    vs_out.worldPos = worldPosition.xyz;
    vs_out.position = inPosition;
    vs_out.normal = mat3(transpose(inverse(uModel))) * inNormal;
    vs_out.uv = inUV;
    vs_out.elevation = inElevation;
    vs_out.biomeColor = inBiomeColor;
    
    gl_Position = uProjection * uView * worldPosition;
}
