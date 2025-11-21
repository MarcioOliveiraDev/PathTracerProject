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
const int SAMPLES_PER_PIXEL = 800;
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
    // 1. Limite de recursão (profundidade máxima)
    if (depth >= MAX_DEPTH) {
        return Color(0, 0, 0);
    }
    
    // 2. Interseção com a cena
    HitRecord rec;
    if (!scene.hit(r, 0.001f, 1e30f, rec)) {
        // Cor de fundo (céu escuro para Cornell Box)
        return Color(0.05f, 0.05f, 0.05f);
    }
    
    // 3. Se acertou uma luz (material emissivo), retorna a cor da luz
    if (rec.emission.length() > 0.0f) {
        return rec.emission;
    }

    // 4. Otimização: Roleta Russa (Russian Roulette)
    // Encerra caminhos aleatoriamente para economizar tempo em profundidades altas
    if (depth >= RR_DEPTH) {
        float p = std::max({rec.albedo.x, rec.albedo.y, rec.albedo.z});
        p = std::clamp(p, 0.1f, 0.99f); // Probabilidade de continuar
        
        if (random_float() > p) {
            return Color(0, 0, 0); // Caminho "morreu"
        }
        rec.albedo = rec.albedo / p; // Compensa a energia dos que sobreviveram
    }
    
    // 5. VARIANTE 9: Aplicação de Textura Sólida
    // Se o objeto foi marcado como TEXTURED (ex: caixas do OBJ), aplicamos a textura.
    if (rec.mat_type == TEXTURED) {
        // Você pode alternar entre wood, marble, etc.
        rec.albedo = scene.solid_tex.wood(rec.p);
    }

    // 6. Cálculo do Espalhamento (Scattering) baseado no Material
    Ray scattered;
    Vec3 scatter_direction;

    if (rec.mat_type == METAL) {
        // --- MATERIAL METÁLICO (Especular) ---
        Vec3 reflected = Vec3::reflect(r.direction.normalized(), rec.normal);
        
        // Adiciona rugosidade usando o parâmetro 'fuzz' do objeto
        scatter_direction = (reflected + rec.fuzz * cosine_sample_hemisphere(rec.normal)).normalized();
        
        // Se o raio refletido for para dentro da superfície, ele é absorvido
        if (Vec3::dot(scatter_direction, rec.normal) <= 0.0f) {
            return Color(0, 0, 0);
        }
    } else {
        // --- MATERIAL DIFUSO / TEXTURIZADO (Lambertiano) ---
        // Amostragem cosseno para iluminação global suave
        scatter_direction = cosine_sample_hemisphere(rec.normal);
    }

    // Gera o novo raio e continua o rastreamento recursivo
    scattered = Ray(rec.p, scatter_direction);
    Color incoming = trace(scattered, scene, depth + 1);

    // Equação de Renderização simplificada: Cor = Albedo * Luz Recebida
    return rec.albedo * incoming;
}

// Configurar cena Cornell Box
Scene setup_scene() {
    Scene scene;
    
    // 1. Carrega O ARQUIVO DO PROFESSOR completo (paredes + caixas)
    auto mesh = OBJLoader::load("scenes/cornell_box.obj");
    
    if (mesh.empty()) {
        std::cerr << "ERRO: Malha vazia! Verifique o caminho do arquivo." << std::endl;
        return scene;
    }

    // 2. Lógica de Normalização (Auto-Scale)
    // A Cornell Box original vai de 0 a 555. Queremos converter para -1 a 1 (tamanho 2).
    
    // Encontra os limites (Bounding Box)
    float min_x = 1e9, max_x = -1e9;
    float min_y = 1e9, max_y = -1e9;
    float min_z = 1e9, max_z = -1e9;

    for (const auto& tri : mesh) {
        for (const auto& v : {tri.v0, tri.v1, tri.v2}) {
            if (v.x < min_x) min_x = v.x; if (v.x > max_x) max_x = v.x;
            if (v.y < min_y) min_y = v.y; if (v.y > max_y) max_y = v.y;
            if (v.z < min_z) min_z = v.z; if (v.z > max_z) max_z = v.z;
        }
    }

    // Calcula o centro e a escala
    float center_x = (min_x + max_x) / 2.0f;
    float center_y = min_y; // Base no 0
    float center_z = (min_z + max_z) / 2.0f;
    
    // A sala tem ~555 de altura. Queremos altura ~2.0 na cena.
    float max_dim = max_y - min_y; 
    float scale = 2.0f / max_dim; 

    std::cout << "Escalando cena... Fator: " << scale << std::endl;

    // Aplica a transformação em todos os triângulos carregados
    for (auto& tri : mesh) {
        auto transform = [&](Point3& p) {
            p.x = (p.x - center_x) * scale;
            p.y = (p.y - center_y) * scale;
            p.z = (p.z - center_z) * scale;
            
            // Opcional: Girar 180 graus se a sala estiver de costas
            // (A Cornell box original olha para +Z, nossa câmera olha para -Z ou vice versa)
            // Experimente descomentar se vir tudo preto:
            p.x = -p.x; 
            p.z = -p.z; 
        };
        
        transform(tri.v0);
        transform(tri.v1);
        transform(tri.v2);
        
        // Recalcula normal
        Vec3 e1 = tri.v1 - tri.v0;
        Vec3 e2 = tri.v2 - tri.v0;
        tri.normal = Vec3::cross(e1, e2).normalized();
        
        // Adiciona à cena
        scene.triangles.push_back(tri);
    }

    // 3. Adiciona a Luz e Objetos Extras
    // Como escalamos tudo para tamanho 2.0, a luz deve ficar perto de y=1.98
    scene.spheres.push_back(Sphere(Point3(0, 1.98f, 0), 0.25f, Color(0,0,0), DIFFUSE, 0.0f, Color(15,15,15)));
    
    // Esfera Metálica (Exemplo extra, já que o OBJ já tem as caixas)
    // Posicionada levemente à frente
    scene.spheres.push_back(Sphere(Point3(0.4f, 0.4f, -0.4f), 0.4f, Color(0.8f, 0.8f, 0.8f), METAL, 0.05f));

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
