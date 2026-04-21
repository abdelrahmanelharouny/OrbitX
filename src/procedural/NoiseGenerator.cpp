#include "NoiseGenerator.h"
#include <algorithm>
#include <numeric>

namespace utils {

std::vector<int> NoiseGenerator::p;
bool NoiseGenerator::initialized = false;
uint32_t NoiseGenerator::currentSeed = 0;

void NoiseGenerator::initialize(uint32_t seed) {
    currentSeed = seed;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, 255);
    
    p.resize(512);
    std::iota(p.begin(), p.begin() + 256, 0);
    std::shuffle(p.begin(), p.begin() + 256, gen);
    
    for (int i = 0; i < 256; i++) {
        p[256 + i] = p[i];
    }
    
    initialized = true;
}

float NoiseGenerator::fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float NoiseGenerator::lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float NoiseGenerator::grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float NoiseGenerator::noise(float x, float y, float z) {
    if (!initialized) {
        initialize(12345);
    }
    
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
    
    return lerp(w, 
        lerp(v, 
            lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
            lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))
        ),
        lerp(v, 
            lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))
        )
    );
}

float NoiseGenerator::octaveNoise(float x, float y, float z, int octaves, float persistence) {
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float maxValue = 0;
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2;
    }
    
    return total / maxValue;
}

float NoiseGenerator::normalizedOctaveNoise(float x, float y, float z, int octaves, float persistence) {
    return (octaveNoise(x, y, z, octaves, persistence) + 1.0f) * 0.5f;
}

} // namespace utils
