#version 330 core

// Planet ring fragment shader with transparency and procedural patterns
in VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 color;
    vec3 worldPos;
} fs_in;

out vec4 outColor;

uniform vec3 uViewPosition;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uOpacity;
uniform float uTime;

// Hash for procedural ring pattern
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Create ring banding pattern
float ringBands(vec2 uv, float bands) {
    float radialPos = uv.s;  // 0 = inner, 1 = outer
    float anglePos = uv.t;
    
    // Multiple concentric bands
    float bandPattern = sin(radialPos * bands * 6.28318) * 0.5 + 0.5;
    
    // Add variation along the ring
    float variation = hash(vec2(floor(anglePos * 20.0), floor(radialPos * 5.0)));
    
    return bandPattern * (0.7 + 0.3 * variation);
}

void main() {
    // Generate ring pattern
    float pattern = ringBands(fs_in.uv, 3.0);
    
    // Vary opacity based on pattern
    float alpha = uOpacity * pattern;
    
    // Fade at inner and outer edges
    float edgeFade = smoothstep(0.0, 0.1, fs_in.uv.s) * (1.0 - smoothstep(0.9, 1.0, fs_in.uv.s));
    alpha *= edgeFade;
    
    // Discard very transparent fragments
    if (alpha < 0.01) {
        discard;
    }
    
    // Simple lighting
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(uLightDirection);
    float diff = max(dot(N, L), 0.0);
    
    // Rings are lit from both sides
    diff = abs(diff);
    
    // Apply lighting to color
    vec3 litColor = fs_in.color * (diff + 0.3);
    
    // Subtle animation shimmer
    float shimmer = sin(uTime * 2.0 + fs_in.uv.t * 10.0) * 0.05;
    litColor += shimmer;
    
    outColor = vec4(litColor, alpha);
}
