#version 330 core

// Inputs from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 LightDir;
in vec3 ViewDir;

// Uniforms
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uPlanetColor;
uniform float uHeight;  // Height value for biome coloring

// Output color
out vec4 FragColor;

// Biome colors (rocky planet default)
const vec3 COLOR_DEEP_WATER = vec3(0.0, 0.1, 0.4);
const vec3 COLOR_SHALLOW_WATER = vec3(0.0, 0.3, 0.6);
const vec3 COLOR_SAND = vec3(0.76, 0.7, 0.5);
const vec3 COLOR_GRASS = vec3(0.2, 0.5, 0.2);
const vec3 COLOR_ROCK = vec3(0.4, 0.35, 0.25);
const vec3 COLOR_SNOW = vec3(0.9, 0.9, 0.95);

vec3 getBiomeColor(float height) {
    if (height < 0.3) return COLOR_DEEP_WATER;
    else if (height < 0.4) return COLOR_SHALLOW_WATER;
    else if (height < 0.45) return COLOR_SAND;
    else if (height < 0.6) return COLOR_GRASS;
    else if (height < 0.75) return COLOR_ROCK;
    else return COLOR_SNOW;
}

void main() {
    // Normalize inputs
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(LightDir);
    vec3 viewDir = normalize(ViewDir);
    
    // Ambient lighting
    vec3 ambient = uAmbientColor * uPlanetColor;
    
    // Diffuse lighting (Lambertian)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uPlanetColor;
    
    // Specular lighting (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor;
    
    // Apply biome coloring based on height
    vec3 baseColor = getBiomeColor(uHeight);
    
    // Combine lighting with biome color
    vec3 result = (ambient + diffuse) * baseColor + specular;
    
    // Day/night cycle: darken the night side
    float dayFactor = max(dot(norm, lightDir), 0.0);
    result = mix(ambient * baseColor, result, dayFactor);
    
    FragColor = vec4(result, 1.0);
}
