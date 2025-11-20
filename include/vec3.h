#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3 operator*(float t) const { return Vec3(x*t, y*t, z*t); }
    Vec3 operator*(const Vec3& v) const { return Vec3(x*v.x, y*v.y, z*v.z); }
    Vec3 operator/(float t) const { return (*this) * (1.0f/t); }
    
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    float length_squared() const { return x*x + y*y + z*z; }
    
    Vec3 normalized() const {
        float len = length();
        return (len > 1e-8f) ? (*this / len) : Vec3(0,0,0);
    }
    
    static float dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }
    
    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x
        );
    }
    
    static Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - n * (2.0f * dot(v, n));
    }
};

using Color = Vec3;
using Point3 = Vec3;

inline Vec3 operator*(float t, const Vec3& v) {
    return v * t;
}

#endif
