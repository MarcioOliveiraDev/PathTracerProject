#ifndef SPHERE_H
#define SPHERE_H

#include "ray.h"

class Sphere {
public:
    Point3 center;
    float radius;
    Color albedo;
    Color emission;
    
    Sphere(Point3 c, float r, Color a = Color(0.7,0.7,0.7), Color e = Color(0,0,0))
        : center(c), radius(r), albedo(a), emission(e) {}
    
    bool hit(const Ray& r, float t_min, float t_max, HitRecord& rec) const {
        Vec3 oc = r.origin - center;
        float a = r.direction.length_squared();
        float half_b = Vec3::dot(oc, r.direction);
        float c = oc.length_squared() - radius*radius;
        float discriminant = half_b*half_b - a*c;
        
        if (discriminant < 0) return false;
        
        float sqrtd = std::sqrt(discriminant);
        float root = (-half_b - sqrtd) / a;
        
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root)
                return false;
        }
        
        rec.t = root;
        rec.p = r.at(rec.t);
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.albedo = albedo;
        rec.emission = emission;
        
        return true;
    }
};

#endif
