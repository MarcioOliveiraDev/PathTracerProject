#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "ray.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream> // Para debug
#include "vec3.h"

// Atualizando a struct Triangle para suportar MaterialType
struct Triangle {
    Point3 v0, v1, v2;
    Vec3 normal;
    Color albedo;
    Color emission;
    MaterialType mat_type; // Novo campo
    float fuzz;

    Triangle(Point3 v0, Point3 v1, Point3 v2, Color a, MaterialType t = DIFFUSE)
        : v0(v0), v1(v1), v2(v2), albedo(a), emission(Color(0,0,0)), mat_type(t), fuzz(0.0f) {
        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        normal = Vec3::cross(e1, e2).normalized();
    }
    
    bool hit(const Ray& r, float t_min, float t_max, HitRecord& rec) const {
        // Algoritmo de Möller–Trumbore
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
        
        // Passando propriedades do material
        rec.albedo = albedo;
        rec.emission = emission;
        rec.mat_type = mat_type;
        rec.fuzz = fuzz;
        
        return true;
    }
};

class OBJLoader {
public:
    static std::vector<Triangle> load(const std::string& filename) {
        std::vector<Triangle> triangles;
        std::vector<Point3> vertices;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "ERRO CRÍTICO: Não foi possível abrir " << filename << std::endl;
            return triangles;
        }

        // Cores padrão da Cornell Box
        Color red_color(0.63f, 0.065f, 0.05f);
        Color green_color(0.14f, 0.45f, 0.091f);
        Color white_color(0.725f, 0.71f, 0.68f);
        
        // Estado atual do parser
        Color current_color = white_color;
        MaterialType current_type = DIFFUSE; 

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
            else if (type == "usemtl") {
                std::string mat_name;
                iss >> mat_name;
                
                // Lógica simples para detectar materiais pelo nome
                if (mat_name.find("red") != std::string::npos) {
                    current_color = red_color;
                    current_type = DIFFUSE;
                }
                else if (mat_name.find("green") != std::string::npos) {
                    current_color = green_color;
                    current_type = DIFFUSE;
                }
                else if (mat_name.find("short") != std::string::npos || 
                         mat_name.find("tall") != std::string::npos ||
                         mat_name.find("box") != std::string::npos) {
                    current_color = white_color; 
                    // AQUI ESTÁ O SEGREDO DA VARIANTE 9:
                    // Marcamos as caixas como TEXTURED
                    current_type = TEXTURED; 
                }
                else {
                    // Paredes brancas, teto, chão
                    current_color = white_color;
                    current_type = DIFFUSE;
                }
            }
            else if (type == "f") {
                std::vector<int> idx;
                std::string vertex_str;
                while (iss >> vertex_str) {
                    std::istringstream viss(vertex_str);
                    int i;
                    viss >> i;
                    idx.push_back(i - 1);
                }
                
                if (idx.size() >= 3) {
                    // Triângulo 1
                    triangles.push_back(Triangle(vertices[idx[0]], vertices[idx[1]], vertices[idx[2]], current_color, current_type));
                    
                    // Triângulo 2 (se for quadrado/quad)
                    if (idx.size() == 4) {
                        triangles.push_back(Triangle(vertices[idx[0]], vertices[idx[2]], vertices[idx[3]], current_color, current_type));
                    }
                }
            }
        }
        
        std::cout << "OBJ Carregado: " << triangles.size() << " triangulos." << std::endl;
        return triangles;
    }
};

#endif