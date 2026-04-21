#pragma once

// Simplex Noise implementation for procedural planet generation
// Optimized for real-time performance with seed-based reproducibility

#include <array>
#include <cstdint>
#include <random>

namespace agss {
namespace procedural {

// Fast permutation table for deterministic noise from seed
class PermutationTable {
public:
    explicit PermutationTable(uint32_t seed = 12345) {
        std::mt19937 gen(seed);
        std::uniform_int_distribution<int> dist(0, 255);
        
        // Initialize with 0-255
        for (int i = 0; i < 256; ++i) {
            perm[i] = static_cast<uint8_t>(i);
        }
        
        // Fisher-Yates shuffle with seeded RNG
        for (int i = 255; i > 0; --i) {
            int j = dist(gen) % (i + 1);
            std::swap(perm[i], perm[j]);
        }
        
        // Duplicate for overflow handling
        for (int i = 0; i < 256; ++i) {
            perm[256 + i] = perm[i];
        }
    }
    
    uint8_t get(int index) const {
        return perm[index & 0xFF];
    }
    
private:
    std::array<uint8_t, 512> perm{};
};

// 3D Simplex Noise class
class SimplexNoise3D {
public:
    explicit SimplexNoise3D(uint32_t seed = 12345) 
        : m_perm(seed), m_seed(seed) {}
    
    // Set a new seed for different planet generations
    void setSeed(uint32_t seed) {
        m_seed = seed;
        m_perm = PermutationTable(seed);
    }
    
    uint32_t getSeed() const { return m_seed; }
    
    // Evaluate noise at 3D coordinates [-1, 1] range typically
    float evaluate(float x, float y, float z) const;
    
    // Evaluate with frequency scaling
    float evaluate(float x, float y, float z, float scale) const;
    
private:
    PermutationTable m_perm;
    uint32_t m_seed;
    
    static constexpr float F3 = 1.0f / 3.0f;
    static constexpr float G3 = 1.0f / 6.0f;
    
    int fastfloor(float x) const {
        return x >= 0 ? static_cast<int>(x) : static_cast<int>(x) - 1;
    }
    
    float dot(const std::array<int, 3>& g, float x, float y, float z) const;
};

// Fractal Brownian Motion for multi-octave noise
struct FBMParameters {
    int octaves = 6;
    float lacunarity = 2.0f;      // Frequency multiplier per octave
    float persistence = 0.5f;     // Amplitude multiplier per octave
    float gain = 1.0f;            // Overall amplitude
    float offset = 0.0f;          // Value offset
    
    // Seed for each octave (optional variation)
    uint32_t baseSeed = 12345;
    bool varyOctaveSeeds = false;
};

class FBMMultiLayer {
public:
    explicit FBMMultiLayer(const FBMParameters& params = FBMParameters{})
        : m_params(params) {}
    
    void setParameters(const FBMParameters& params) { m_params = params; }
    const FBMParameters& getParameters() const { return m_params; }
    
    // Evaluate FBM at 3D position
    float evaluate(float x, float y, float z) const;
    
    // Evaluate with custom scale
    float evaluate(float x, float y, float z, float scale) const;
    
private:
    FBMParameters m_params;
};

} // namespace procedural
} // namespace agss
