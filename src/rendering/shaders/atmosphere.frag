#version 330 core

// Inputs from vertex shader
in vec3 FragPos;
in vec2 TexCoord;
in float Visibility;

// Uniforms
uniform vec3 uAtmosphereColor;
uniform vec3 uLightDir;
uniform vec3 uViewPos;

// Output color
out vec4 FragColor;

void main() {
    vec3 normal = normalize(FragPos);
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 lightDir = normalize(uLightDir);
    
    // Rim lighting for atmospheric glow
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 3.0);
    
    // Light scattering effect
    float scatter = max(dot(normal, lightDir), 0.0);
    scatter = pow(scatter, 2.0);
    
    // Combine effects
    vec3 atmosphere = uAtmosphereColor * (rim * 0.8 + scatter * 0.3);
    
    // Apply visibility fog
    atmosphere *= Visibility;
    
    // Additive blending for glow effect
    FragColor = vec4(atmosphere, 0.6);
}
