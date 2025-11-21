#ifndef PLANE_H
#define PLANE_H

#include "ray.h"

class Plane {
public:
    Point3 point;
    Vec3 normal;
    Color albedo;
    Color emission;
    
    Plane(Point3 p, Vec3 n, Color a = Color(0.7,0.7,0.7), Color e = Color(0,0,0))
        : point(p), normal(n.normalized()), albedo(a), emission(e) {}
    
    bool hit(const Ray& r, float t_min, float t_max, HitRecord& rec) const {
        float denom = Vec3::dot(normal, r.direction);
        if (std::abs(denom) < 1e-6f) return false;
        
        float t = Vec3::dot(point - r.origin, normal) / denom;
        if (t < t_min || t > t_max) return false;
        
        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.albedo = albedo;
        rec.emission = emission;
        rec.mat_type = DIFFUSE;
        rec.fuzz = 0.0f;
        
        return true;
    }
};

#endif
