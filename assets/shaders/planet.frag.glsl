#version 330 core

// Planet fragment shader with biome blending and optional gas giant mode
in VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 uv;
    float elevation;
    vec3 biomeColor;
    vec3 worldPos;
} fs_in;

out vec4 outColor;

uniform vec3 uViewPosition;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

// Biome colors (for shader-based recalculation if needed)
uniform vec3 uDeepWaterColor;
uniform vec3 uWaterColor;
uniform vec3 uSandColor;
uniform vec3 uGrassColor;
uniform vec3 uForestColor;
uniform vec3 uRockColor;
uniform vec3 uSnowColor;

// Gas giant parameters
uniform bool uIsGasGiant;
uniform float uTime;
uniform vec3 uGasPrimaryColor;
uniform vec3 uGasSecondaryColor;
uniform vec3 uGasAccentColor;
uniform float uGasBandCount;
uniform float uGasTurbulence;

// Cloud parameters (for planet with clouds)
uniform bool uHasClouds;
uniform sampler2D uCloudNoise;

// Smoothstep helper for biome transitions
vec3 smoothBiome(float e, float t1, float t2, vec3 c1, vec3 c2) {
    float t = smoothstep(t1, t2, e);
    return mix(c1, c2, t);
}

// Gas giant band pattern using layered noise
float gasBands(vec3 pos, float bandCount) {
    float y = pos.y * 0.5 + 0.5;  // Normalize to 0-1
    return sin(y * bandCount * 3.14159) * 0.5 + 0.5;
}

// Simple hash for procedural noise in gas giants
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise2D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

void main() {
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(uLightDirection);
    vec3 V = normalize(uViewPosition - fs_in.worldPos);
    vec3 H = normalize(L + V);
    
    // Diffuse lighting
    float diff = max(dot(N, L), 0.0);
    
    // Specular (simple Blinn-Phong)
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    
    vec3 baseColor;
    
    if (uIsGasGiant) {
        // Gas giant shader - layered bands with turbulence
        vec2 gasUV = fs_in.uv;
        
        // Animate UVs for atmospheric movement
        gasUV.x += uTime * 0.01;
        
        // Multiple band layers with different speeds
        float band1 = gasBands(fs_in.position, uGasBandCount);
        float band2 = gasBands(fs_in.position, uGasBandCount * 1.3) * 0.5;
        
        // Add turbulence using FBM
        float turbulence = fbm(gasUV * 4.0 + vec2(uTime * 0.02, 0.0), 4) * uGasTurbulence;
        
        // Combine bands
        float bands = band1 + band2 + turbulence;
        bands = bands / 1.5;  // Normalize
        
        // Color interpolation based on band intensity
        baseColor = mix(uGasSecondaryColor, uGasPrimaryColor, bands);
        baseColor = mix(baseColor, uGasAccentColor, smoothstep(0.8, 1.0, bands));
        
        // Softer lighting for gas giants
        diff = diff * 0.7 + 0.3;
        spec = 0.0;  // No sharp specular on gas giants
    } else {
        // Terrestrial planet - use pre-calculated biome color
        baseColor = fs_in.biomeColor;
        
        // Optional: Recalculate biome in shader for more flexibility
        // float e = fs_in.elevation;
        // baseColor = smoothBiome(e, -0.5, -0.1, uDeepWaterColor, uWaterColor);
        // baseColor = mix(baseColor, smoothBiome(e, -0.1, 0.05, uWaterColor, uSandColor), step(-0.1, e));
        // ... etc for all biomes
    }
    
    // Water special handling (specular reflection)
    if (!uIsGasGiant && fs_in.elevation < -0.1) {
        // Water has higher specular
        float waterSpec = pow(max(dot(N, H), 0.0), 64.0);
        spec = waterSpec * 0.8;
        
        // Fresnel effect for water
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        baseColor = mix(baseColor, vec3(0.3), fresnel * 0.5);
    }
    
    // Snow gets extra brightness
    if (!uIsGasGiant && fs_in.elevation > 0.7) {
        baseColor *= 1.2;
    }
    
    // Apply lighting
    vec3 ambient = uAmbientColor * baseColor;
    vec3 diffuse = uLightColor * baseColor * diff;
    vec3 specular = uLightColor * vec3(spec) * 0.5;
    
    vec3 finalColor = ambient + diffuse + specular;
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    
    outColor = vec4(finalColor, 1.0);
}
