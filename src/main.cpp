#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>  // Para std::clamp
#include <omp.h>
#include "../include/vec3.h"
#include "../include/ray.h"
#include "../include/sphere.h"
#include "../include/plane.h"
#include "../include/obj_loader.h"
#include "../include/solid_texture.h"
#include "../include/stb_image_write.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// Configurações de renderização
const int WIDTH = 512;
const int HEIGHT = 512;
const int SAMPLES_PER_PIXEL = 1000;
const int MAX_DEPTH = 8;
const int RR_DEPTH = 3;
const float GAMMA = 2.2f;

// Gerador de números aleatórios thread-safe
thread_local std::mt19937 rng(std::random_device{}());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

float random_float() {
    return dist(rng);
}

// Amostragem cosine-weighted hemisphere
Vec3 cosine_sample_hemisphere(const Vec3& normal) {
    float u1 = random_float();
    float u2 = random_float();
    
    float r = std::sqrt(u1);
    float theta = 2.0f * M_PI * u2;
    
    float x = r * std::cos(theta);
    float z = r * std::sin(theta);
    float y = std::sqrt(std::max(0.0f, 1.0f - u1));
    
    // Criar base ortonormal ao redor da normal
    Vec3 tangent = std::abs(normal.x) > 0.1f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    tangent = Vec3::cross(normal, tangent).normalized();
    Vec3 bitangent = Vec3::cross(normal, tangent);
    
    return (tangent * x + normal * y + bitangent * z).normalized();
}

// Classe de cena
class Scene {
public:
    std::vector<Sphere> spheres;
    std::vector<Plane> planes;
    std::vector<Triangle> triangles;
    SolidTexture solid_tex;
    
    bool hit(const Ray& r, float t_min, float t_max, HitRecord& rec) const {
        HitRecord temp_rec;
        bool hit_anything = false;
        float closest_so_far = t_max;
        
        for (const auto& sphere : spheres) {
            if (sphere.hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        
        for (const auto& plane : planes) {
            if (plane.hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        
        for (const auto& tri : triangles) {
            if (tri.hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        
        return hit_anything;
    }
};

// Path tracing integrador
Color trace(const Ray& r, const Scene& scene, int depth) {
    if (depth >= MAX_DEPTH) {
        return Color(0, 0, 0);
    }
    
    HitRecord rec;
    if (!scene.hit(r, 0.001f, 1e30f, rec)) {
        // Cor de fundo (céu)
        return Color(0.5f, 0.7f, 1.0f) * 0.3f;
    }
    
    // Se acertou uma luz, retorna emissão
    if (rec.emission.length() > 0.0f) {
        return rec.emission;
    }

    // Heurística simples: esfera metálica é a grande à direita (x > 0)
    bool is_metal = (rec.p.y > 0.1f && rec.p.y < 1.5f && rec.p.x > 0.0f);
    
    // Russian Roulette a partir de RR_DEPTH
    if (depth >= RR_DEPTH) {
        float p = std::max({rec.albedo.x, rec.albedo.y, rec.albedo.z});
        p = std::clamp(p, 0.1f, 0.99f);
        if (random_float() > p) {
            return Color(0, 0, 0);
        }
        rec.albedo = rec.albedo / p;
    }
    
    // VARIANTE 9: Aplicar textura sólida nos objetos (não nas paredes)
    // Você pode identificar paredes por posição ou flag
    // Aqui, vou aplicar textura em esferas e geometria fora de certos limites
    bool is_wall = (std::abs(rec.p.x + 1.0f) < 0.01f) || // parede esquerda
                   (std::abs(rec.p.x - 1.0f) < 0.01f) || // parede direita
                   (std::abs(rec.p.z + 1.0f) < 0.01f) || // parede traseira
                   (std::abs(rec.p.y) < 0.01f) ||        // chão
                   (std::abs(rec.p.y - 2.0f) < 0.01f);   // teto
    
    if (!is_wall && !is_metal) {
        // Aplicar textura sólida (escolha uma):
        // rec.albedo = scene.solid_tex.marble(rec.p);
        rec.albedo = scene.solid_tex.wood(rec.p);
        // rec.albedo = scene.solid_tex.checkerboard(rec.p);
        // rec.albedo = scene.solid_tex.clouds(rec.p);
    }
    
    // Amostragem cosine-weighted
    /*Vec3 scatter_direction = cosine_sample_hemisphere(rec.normal);
    Ray scattered(rec.p, scatter_direction);
    
    // BRDF lambertiano: albedo / π (cancelado por pdf = cos(θ) / π)
    Color incoming = trace(scattered, scene, depth + 1);
    return rec.albedo * incoming;*/

    Vec3 scatter_direction;
    if (is_metal) {
        // MATERIAL METÁLICO: reflexão especular + "fuzz" (rugosidade)
        float fuzz = 0.05f; // 0 = espelho perfeito; maior = mais borrado
        Vec3 reflected = Vec3::reflect(r.direction.normalized(), rec.normal);
        
        // Adiciona um pequeno ruído hemisférico em torno da reflexão para rugosidade
        Vec3 fuzz_dir = cosine_sample_hemisphere(rec.normal);
        scatter_direction = (reflected + fuzz * fuzz_dir).normalized();
        
        // Se refletir para dentro da superfície, mata o raio
        if (Vec3::dot(scatter_direction, rec.normal) <= 0.0f) {
            return Color(0, 0, 0);
        }
    } else {
        // MATERIAL DIFUSO: o que você já tinha
        scatter_direction = cosine_sample_hemisphere(rec.normal);
    }

    Ray scattered(rec.p, scatter_direction);
    Color incoming = trace(scattered, scene, depth + 1);

    // Para metal ideal, o termo é basicamente albedo * radiância refletida.
    // Para difuso, continua coerente com BRDF lambertiano simplificado.
    return rec.albedo * incoming;


}

// Configurar cena Cornell Box
Scene setup_scene() {
    Scene scene;
    
    // Paredes da Cornell Box (planos infinitos com limites implícitos)
    // Parede esquerda (vermelha)
    scene.planes.push_back(Plane(Point3(-1, 0, 0), Vec3(1, 0, 0), Color(0.63f, 0.065f, 0.05f)));
    
    // Parede direita (verde)
    scene.planes.push_back(Plane(Point3(1, 0, 0), Vec3(-1, 0, 0), Color(0.14f, 0.45f, 0.091f)));
    
    // Parede traseira (branca)
    scene.planes.push_back(Plane(Point3(0, 0, -1), Vec3(0, 0, 1), Color(0.725f, 0.71f, 0.68f)));
    
    // Chão (branco)
    scene.planes.push_back(Plane(Point3(0, 0, 0), Vec3(0, 1, 0), Color(0.725f, 0.71f, 0.68f)));
    
    // Teto (branco, com luz no centro)
    scene.planes.push_back(Plane(Point3(0, 2, 0), Vec3(0, -1, 0), Color(0.725f, 0.71f, 0.68f)));
    
    // Luz de área (esfera emissiva no teto)
    scene.spheres.push_back(Sphere(Point3(0, 1.98f, 0), 0.3f, Color(0,0,0), Color(15,15,15)));
    
    // OPÇÃO 1: Carregar caixas de arquivo OBJ
    /*
    auto box1 = OBJLoader::load("scenes/short-box.obj", Color(0.73f, 0.73f, 0.73f));
    auto box2 = OBJLoader::load("scenes/tall-box.obj", Color(0.73f, 0.73f, 0.73f));
    scene.triangles.insert(scene.triangles.end(), box1.begin(), box1.end());
    scene.triangles.insert(scene.triangles.end(), box2.begin(), box2.end());
    */
    
    // OPÇÃO 2: Esferas de teste (mais simples)
    scene.spheres.push_back(Sphere(Point3(-0.4f, 0.3f, -0.3f), 0.3f, Color(0.7f, 0.7f, 0.7f)));
    //Esfera grande metálica
    scene.spheres.push_back(Sphere(Point3(0.4f, 0.5f, 0.2f), 0.5f, Color(0.95f, 0.64f, 0.54f)));
    
    return scene;
}

int main() {
    std::cout << "Iniciando renderização Path Tracing (Variante 9 - Texturas Sólidas)..." << std::endl;
    std::cout << "Resolução: " << WIDTH << "x" << HEIGHT << std::endl;
    std::cout << "Samples: " << SAMPLES_PER_PIXEL << " | Max Depth: " << MAX_DEPTH << std::endl;
    
    Scene scene = setup_scene();
    
    // Buffer de imagem
    std::vector<Color> framebuffer(WIDTH * HEIGHT);
    
    // Câmera
    Point3 cam_pos(0.0f, 1.0f, 3.0f); //MAIS AFASTADA
    Point3 cam_target(0.0f, 1.0f, 0.0f);
    Vec3 cam_dir = (cam_target - cam_pos).normalized();
    Vec3 cam_up(0, 1, 0);
    Vec3 cam_right = Vec3::cross(cam_dir, cam_up).normalized();
    cam_up = Vec3::cross(cam_right, cam_dir).normalized();
    
    float fov = 40.0f * M_PI / 180.0f;
    float scale = std::tan(fov * 0.5f);
    float aspect = static_cast<float>(WIDTH) / HEIGHT;
    
    // Renderização com OpenMP
    auto start_time = omp_get_wtime();
    
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            Color pixel_color(0, 0, 0);
            
            for (int s = 0; s < SAMPLES_PER_PIXEL; s++) {
                float u = (x + random_float()) / WIDTH;
                float v = (y + random_float()) / HEIGHT;
                
                u = (u - 0.5f) * 2.0f;
                v = (v - 0.5f) * 2.0f;
                
                Vec3 ray_dir = (cam_dir + cam_right * (u * scale * aspect) + cam_up * (v * scale)).normalized();
                Ray r(cam_pos, ray_dir);
                
                pixel_color = pixel_color + trace(r, scene, 0);
            }
            
            pixel_color = pixel_color / static_cast<float>(SAMPLES_PER_PIXEL);
            framebuffer[(HEIGHT - 1 - y) * WIDTH + x] = pixel_color;
        }
        
        if (y % 32 == 0) {
            std::cout << "Progresso: " << (100 * y / HEIGHT) << "%\r" << std::flush;
        }
    }
    
    auto end_time = omp_get_wtime();
    std::cout << "\nTempo de renderização: " << (end_time - start_time) << " segundos" << std::endl;
    
    // Tonemap e salvar PNG
    std::vector<unsigned char> pixels(WIDTH * HEIGHT * 3);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        Color c = framebuffer[i];
        
        // Clamp
        c.x = std::clamp(c.x, 0.0f, 1.0f);
        c.y = std::clamp(c.y, 0.0f, 1.0f);
        c.z = std::clamp(c.z, 0.0f, 1.0f);
        
        // Gamma correction
        c.x = std::pow(c.x, 1.0f / GAMMA);
        c.y = std::pow(c.y, 1.0f / GAMMA);
        c.z = std::pow(c.z, 1.0f / GAMMA);
        
        pixels[i*3 + 0] = static_cast<unsigned char>(c.x * 255.0f);
        pixels[i*3 + 1] = static_cast<unsigned char>(c.y * 255.0f);
        pixels[i*3 + 2] = static_cast<unsigned char>(c.z * 255.0f);
    }
    
    stbi_write_png("output/render.png", WIDTH, HEIGHT, 3, pixels.data(), WIDTH * 3);
    std::cout << "Imagem salva em: output/render.png" << std::endl;
    
    return 0;
}
