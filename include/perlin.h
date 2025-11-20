#ifndef PERLIN_H
#define PERLIN_H

#include "vec3.h"
#include <cmath>
#include <random>

class PerlinNoise {
private:
    static const int PERM_SIZE = 256;
    int perm[PERM_SIZE * 2];
    
    float fade(float t) const {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
    
    float lerp(float t, float a, float b) const {
        return a + t * (b - a);
    }
    
    float grad(int hash, float x, float y, float z) const {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
    
public:
    PerlinNoise(unsigned int seed = 0) {
        std::mt19937 gen(seed);
        for (int i = 0; i < PERM_SIZE; i++) {
            perm[i] = i;
        }
        
        // Fisher-Yates shuffle
        for (int i = PERM_SIZE - 1; i > 0; i--) {
            int j = gen() % (i + 1);
            std::swap(perm[i], perm[j]);
        }
        
        for (int i = 0; i < PERM_SIZE; i++) {
            perm[PERM_SIZE + i] = perm[i];
        }
    }
    
    float noise(float x, float y, float z) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);
        
        float u = fade(x);
        float v = fade(y);
        float w = fade(z);
        
        int A  = perm[X] + Y;
        int AA = perm[A] + Z;
        int AB = perm[A + 1] + Z;
        int B  = perm[X + 1] + Y;
        int BA = perm[B] + Z;
        int BB = perm[B + 1] + Z;
        
        return lerp(w,
            lerp(v,
                lerp(u, grad(perm[AA], x, y, z), grad(perm[BA], x-1, y, z)),
                lerp(u, grad(perm[AB], x, y-1, z), grad(perm[BB], x-1, y-1, z))
            ),
            lerp(v,
                lerp(u, grad(perm[AA+1], x, y, z-1), grad(perm[BA+1], x-1, y, z-1)),
                lerp(u, grad(perm[AB+1], x, y-1, z-1), grad(perm[BB+1], x-1, y-1, z-1))
            )
        );
    }
    
    float octave_noise(float x, float y, float z, int octaves, float persistence = 0.5f) const {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float max_value = 0.0f;
        
        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
            max_value += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }
        
        return total / max_value;
    }
};

#endif
