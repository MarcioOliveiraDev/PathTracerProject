#ifndef SOLID_TEXTURE_H
#define SOLID_TEXTURE_H

#include "vec3.h"
#include "perlin.h"
#include <cmath>

class SolidTexture {
private:
    PerlinNoise perlin;
    
public:
    SolidTexture(unsigned int seed = 42) : perlin(seed) {}
    
    // Textura de mármore (marble)
    Color marble(const Point3& p, float scale = 5.0f) const {
        float noise_val = perlin.octave_noise(p.x * scale, p.y * scale, p.z * scale, 6, 0.5f);
        float pattern = std::sin(p.x * scale + 3.0f * noise_val);
        float t = (pattern + 1.0f) * 0.5f; // Normaliza [0,1]
        
        // Cores de mármore (branco a cinza)
        Color white(0.9f, 0.85f, 0.8f);
        Color gray(0.5f, 0.5f, 0.52f);
        return white * (1.0f - t) + gray * t;
    }
    
    // Textura de madeira (wood)
    Color wood(const Point3& p, float scale = 10.0f) const {
        float r = std::sqrt(p.x*p.x + p.z*p.z);
        float noise_val = perlin.octave_noise(p.x * 2.0f, p.y * 2.0f, p.z * 2.0f, 4, 0.5f);
        float rings = std::sin(r * scale + noise_val * 3.0f);
        float t = (rings + 1.0f) * 0.5f;
        
        // Cores de madeira (marrom escuro a claro)
        Color dark(0.4f, 0.2f, 0.1f);
        Color light(0.7f, 0.5f, 0.3f);
        return dark * (1.0f - t) + light * t;
    }
    
    // Textura xadrez 3D
    Color checkerboard(const Point3& p, float scale = 2.0f) const {
        int xi = static_cast<int>(std::floor(p.x * scale));
        int yi = static_cast<int>(std::floor(p.y * scale));
        int zi = static_cast<int>(std::floor(p.z * scale));
        
        bool is_even = ((xi + yi + zi) % 2) == 0;
        return is_even ? Color(0.9f, 0.9f, 0.9f) : Color(0.2f, 0.2f, 0.2f);
    }
    
    // Textura de nuvens/fumaça
    Color clouds(const Point3& p, float scale = 3.0f) const {
        float noise_val = perlin.octave_noise(p.x * scale, p.y * scale, p.z * scale, 6, 0.5f);
        noise_val = (noise_val + 1.0f) * 0.5f; // Normaliza [0,1]
        
        Color sky_blue(0.5f, 0.7f, 1.0f);
        Color white(1.0f, 1.0f, 1.0f);
        return sky_blue * (1.0f - noise_val) + white * noise_val;
    }
};

#endif
