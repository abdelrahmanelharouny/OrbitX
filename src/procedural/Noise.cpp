#include "procedural/Noise.h"

#include <algorithm>
#include <cmath>

namespace agss {
namespace procedural {

// Gradient table for simplex noise - 12 gradient vectors for 3D
static constexpr std::array<std::array<int, 3>, 12> GRAD3 = {{
    {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
    {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
    {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
}};

float SimplexNoise3D::dot(const std::array<int, 3>& g, float x, float y, float z) const {
    return static_cast<float>(g[0]) * x + static_cast<float>(g[1]) * y + static_cast<float>(g[2]) * z;
}

float SimplexNoise3D::evaluate(float x, float y, float z) const {
    // Skew input space to determine which simplex cell we're in
    float s = (x + y + z) * F3;
    int i = fastfloor(x + s);
    int j = fastfloor(y + s);
    int k = fastfloor(z + s);
    
    // Unskew back to (x,y,z) space
    float t = static_cast<float>(i + j + k) * G3;
    float X0 = static_cast<float>(i) - t;
    float Y0 = static_cast<float>(j) - t;
    float Z0 = static_cast<float>(k) - t;
    
    // Distances from cell origin
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;
    
    // Determine which simplex we're in (6 possible orderings)
    int i1, j1, k1; // Offsets for second corner
    int i2, j2, k2; // Offsets for third corner
    
    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0;
            i2 = 1; j2 = 1; k2 = 0;
        } else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0;
            i2 = 1; j2 = 0; k2 = 1;
        } else {
            i1 = 0; j1 = 0; k1 = 1;
            i2 = 1; j2 = 0; k2 = 1;
        }
    } else {
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1;
            i2 = 0; j2 = 1; k2 = 1;
        } else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0;
            i2 = 0; j2 = 1; k2 = 1;
        } else {
            i1 = 0; j1 = 1; k1 = 0;
            i2 = 1; j2 = 1; k2 = 0;
        }
    }
    
    // Offsets for remaining corners
    float x1 = x0 - static_cast<float>(i1) + G3;
    float y1 = y0 - static_cast<float>(j1) + G3;
    float z1 = z0 - static_cast<float>(k1) + G3;
    float x2 = x0 - static_cast<float>(i2) + 2.0f * G3;
    float y2 = y0 - static_cast<float>(j2) + 2.0f * G3;
    float z2 = z0 - static_cast<float>(k2) + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;
    
    // Get gradient indices from permutation table
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    
    int gi0 = m_perm.get(ii + m_perm.get(jj + m_perm.get(kk))) % 12;
    int gi1 = m_perm.get(ii + i1 + m_perm.get(jj + j1 + m_perm.get(kk + k1))) % 12;
    int gi2 = m_perm.get(ii + i2 + m_perm.get(jj + j2 + m_perm.get(kk + k2))) % 12;
    int gi3 = m_perm.get(ii + 1 + m_perm.get(jj + 1 + m_perm.get(kk + 1))) % 12;
    
    // Calculate contribution from each corner
    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;
    
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 >= 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * dot(GRAD3[gi0], x0, y0, z0);
    }
    
    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 >= 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * dot(GRAD3[gi1], x1, y1, z1);
    }
    
    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 >= 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * dot(GRAD3[gi2], x2, y2, z2);
    }
    
    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 >= 0.0f) {
        t3 *= t3;
        n3 = t3 * t3 * dot(GRAD3[gi3], x3, y3, z3);
    }
    
    // Scale to [-1, 1] range
    return 32.0f * (n0 + n1 + n2 + n3);
}

float SimplexNoise3D::evaluate(float x, float y, float z, float scale) const {
    return evaluate(x * scale, y * scale, z * scale);
}

float FBMMultiLayer::evaluate(float x, float y, float z) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxAmplitude = 0.0f;
    
    uint32_t currentSeed = m_params.baseSeed;
    
    for (int i = 0; i < m_params.octaves; ++i) {
        SimplexNoise3D noise(currentSeed);
        value += noise.evaluate(x * frequency, y * frequency, z * frequency) * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= m_params.persistence;
        frequency *= m_params.lacunarity;
        
        if (m_params.varyOctaveSeeds) {
            currentSeed = static_cast<uint32_t>(currentSeed * 17 + i * 31);
        }
    }
    
    return (value / maxAmplitude) * m_params.gain + m_params.offset;
}

float FBMMultiLayer::evaluate(float x, float y, float z, float scale) const {
    return evaluate(x * scale, y * scale, z * scale);
}

} // namespace procedural
} // namespace agss
