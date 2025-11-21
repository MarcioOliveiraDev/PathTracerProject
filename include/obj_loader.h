#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "ray.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "vec3.h"

struct Triangle {
    Point3 v0, v1, v2;
    Vec3 normal;
    Color albedo;
    Color emission;
    
    Triangle(Point3 v0, Point3 v1, Point3 v2, Color a = Color(0.7,0.7,0.7), Color e = Color(0,0,0))
        : v0(v0), v1(v1), v2(v2), albedo(a), emission(e) {
        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        normal = Vec3::cross(e1, e2).normalized();
    }
    
    // Möller–Trumbore ray-triangle intersection
    bool hit(const Ray& r, float t_min, float t_max, HitRecord& rec) const {
        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        Vec3 pvec = Vec3::cross(r.direction, e2);
        float det = Vec3::dot(e1, pvec);
        
        if (std::abs(det) < 1e-8f) return false;
        
        float inv_det = 1.0f / det;
        Vec3 tvec = r.origin - v0;
        float u = Vec3::dot(tvec, pvec) * inv_det;
        if (u < 0.0f || u > 1.0f) return false;
        
        Vec3 qvec = Vec3::cross(tvec, e1);
        float v = Vec3::dot(r.direction, qvec) * inv_det;
        if (v < 0.0f || u + v > 1.0f) return false;
        
        float t = Vec3::dot(e2, qvec) * inv_det;
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

class OBJLoader {
public:
    static std::vector<Triangle> load(const std::string& filename, Color default_albedo = Color(0.73,0.73,0.73)) {
        std::vector<Triangle> triangles;
        std::vector<Point3> vertices;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir arquivo OBJ: " << filename << std::endl;
            return triangles;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            
            if (type == "v") {
                float x, y, z;
                iss >> x >> y >> z;
                vertices.push_back(Point3(x, y, z));
            }
            else if (type == "f") {
                std::vector<int> indices;
                std::string vertex;
                while (iss >> vertex) {
                    // Parsear f v1/vt1/vn1 ou f v1//vn1 ou f v1
                    int v_idx;
                    std::istringstream viss(vertex);
                    viss >> v_idx;
                    indices.push_back(v_idx - 1); // OBJ usa índice 1-based
                }
                
                // Triangular face (se for quad, divide em 2 triângulos)
                if (indices.size() >= 3) {
                    triangles.push_back(Triangle(
                        vertices[indices[0]],
                        vertices[indices[1]],
                        vertices[indices[2]],
                        default_albedo
                    ));
                }
                if (indices.size() == 4) {
                    triangles.push_back(Triangle(
                        vertices[indices[0]],
                        vertices[indices[2]],
                        vertices[indices[3]],
                        default_albedo
                    ));
                }
            }
        }
        
        std::cout << "Carregados " << triangles.size() << " triângulos de " << filename << std::endl;
        return triangles;
    }
};

#endif
