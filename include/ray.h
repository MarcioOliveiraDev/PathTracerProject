#ifndef RAY_H
#define RAY_H

#include "vec3.h"

struct Ray {
    Point3 origin;
    Vec3 direction;
    
    Ray() {}
    Ray(const Point3& origin, const Vec3& direction) 
        : origin(origin), direction(direction) {}
    
    Point3 at(float t) const {
        return origin + direction * t;
    }
};

enum MaterialType {
    DIFFUSE,
    METAL,
    TEXTURED // <--- NOVO: Para objetos que recebem a textura procedural
};

struct HitRecord {
    Point3 p;           // Ponto de interseção
    Vec3 normal;        // Normal da superfície
    float t;            // Distância ao longo do raio
    Color albedo;       // Cor difusa do material
    Color emission;     // Emissão (luz)
    // ADICIONE ESTES DOIS CAMPOS:
    MaterialType mat_type; 
    float fuzz;
    bool front_face;    // Se acertou face frontal
    
    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = Vec3::dot(r.direction, outward_normal) < 0;
        normal = front_face ? outward_normal : outward_normal * -1.0f;
    }
};

#endif
