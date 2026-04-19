#pragma once

#include <vector>
#include <cmath>
#include <random>
#include <cstdint>

namespace utils {

/**
 * @brief Perlin Noise generator for procedural terrain generation
 * 
 * Implements 3D Perlin noise with support for octaves and persistence.
 * Thread-safe, seed-based generation for reproducible results.
 */
class NoiseGenerator {
public:
    /**
     * @brief Initialize the noise generator with a seed
     * @param seed Random seed for reproducible generation
     */
    static void initialize(uint32_t seed);
    
    /**
     * @brief Generate 3D Perlin noise value at given coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Noise value in range [-1, 1]
     */
    static float noise(float x, float y, float z);
    
    /**
     * @brief Generate layered noise with multiple octaves
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param octaves Number of noise layers
     * @param persistence Amplitude decrease per octave
     * @return Combined noise value in range [-1, 1]
     */
    static float octaveNoise(float x, float y, float z, int octaves = 6, float persistence = 0.5f);
    
    /**
     * @brief Generate normalized noise value in range [0, 1]
     */
    static float normalizedOctaveNoise(float x, float y, float z, int octaves = 6, float persistence = 0.5f);

private:
    static std::vector<int> p;  // Permutation table
    static bool initialized;
    static uint32_t currentSeed;
    
    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y, float z);
};

} // namespace utils
