#version 330 core

// Cloud layer fragment shader with procedural noise
in VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 worldPos;
} fs_in;

out vec4 outColor;

uniform vec3 uViewPosition;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uCloudColor;
uniform float uCloudDensity;
uniform float uCloudCoverage;

// Hash function for procedural noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// 2D noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Smoothstep interpolation
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal Brownian Motion for cloud detail
float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float maxVal = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p * frequency);
        maxVal += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value / maxVal;
}

void main() {
    // Generate cloud pattern using FBM
    vec2 cloudUV = fs_in.uv * 3.0;  // Scale for appropriate cloud size
    float cloudNoise = fbm(cloudUV, 5);
    
    // Apply coverage threshold
    float cloudThreshold = 1.0 - uCloudCoverage;
    float cloudAlpha = smoothstep(cloudThreshold, cloudThreshold + 0.3, cloudNoise);
    
    // Vary cloud density
    cloudAlpha *= uCloudDensity;
    
    // Soft edges for clouds
    if (cloudAlpha < 0.01) {
        discard;
    }
    
    // Lighting on clouds (simple diffuse)
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(uLightDirection);
    float diff = max(dot(N, L), 0.0);
    
    // Self-shadowing in thicker cloud areas
    float shadow = 1.0 - cloudNoise * 0.3;
    
    vec3 litColor = uCloudColor * (diff * shadow + 0.2);
    
    outColor = vec4(litColor, cloudAlpha);
}
