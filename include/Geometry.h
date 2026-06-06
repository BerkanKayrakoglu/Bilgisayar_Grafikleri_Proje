#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <cmath>

// Basit bir Vertex yapısı
struct Vertex {
    float Position[3];
    float Normal[3];
    float TexCoords[2];
};

class Geometry {
public:
    // Zemin için düzlem (Plane)
    static std::vector<Vertex> createPlane(float width, float depth, float tileX = 1.0f, float tileY = 1.0f) {
        std::vector<Vertex> vertices;
        float w = width / 2.0f;
        float d = depth / 2.0f;

        // İki üçgen ile kare zemin
        vertices.push_back({{-w, 0.0f, -d}, {0.0f, 1.0f, 0.0f}, {0.0f, tileY}});
        vertices.push_back({{ w, 0.0f, -d}, {0.0f, 1.0f, 0.0f}, {tileX, tileY}});
        vertices.push_back({{ w, 0.0f,  d}, {0.0f, 1.0f, 0.0f}, {tileX, 0.0f}});

        vertices.push_back({{ w, 0.0f,  d}, {0.0f, 1.0f, 0.0f}, {tileX, 0.0f}});
        vertices.push_back({{-w, 0.0f,  d}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{-w, 0.0f, -d}, {0.0f, 1.0f, 0.0f}, {0.0f, tileY}});

        return vertices;
    }

    // Çatı için Üçgen Prizma (Triangular Prism)
    static std::vector<Vertex> createRoofPrism(float width, float height, float depth) {
        std::vector<Vertex> vertices;
        float w = width / 2.0f;
        float d = depth / 2.0f;

        // Ön Üçgen
        vertices.push_back({{-w, 0.0f,  d}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ w, 0.0f,  d}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}});
        vertices.push_back({{0.0f, height, d}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}});

        // Arka Üçgen
        vertices.push_back({{-w, 0.0f, -d}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}});
        vertices.push_back({{0.0f, height,-d}, {0.0f, 0.0f, -1.0f}, {0.5f, 1.0f}});
        vertices.push_back({{ w, 0.0f, -d}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}});

        // Sağ Eğimli Yüzey (Normal hesaplanmalı)
        float ny = w;
        float nx = height;
        float len = sqrt(nx*nx + ny*ny);
        nx /= len; ny /= len;
        vertices.push_back({{ w, 0.0f,  d}, {nx, ny, 0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ w, 0.0f, -d}, {nx, ny, 0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{0.0f, height,-d}, {nx, ny, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{0.0f, height,-d}, {nx, ny, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{0.0f, height, d}, {nx, ny, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{ w, 0.0f,  d}, {nx, ny, 0.0f}, {0.0f, 0.0f}});

        // Sol Eğimli Yüzey
        vertices.push_back({{-w, 0.0f, -d}, {-nx, ny, 0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{-w, 0.0f,  d}, {-nx, ny, 0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{0.0f, height, d}, {-nx, ny, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{0.0f, height, d}, {-nx, ny, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{0.0f, height,-d}, {-nx, ny, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{-w, 0.0f, -d}, {-nx, ny, 0.0f}, {0.0f, 0.0f}});

        // Alt Yüzey eklenebilir ama ev çatısı olduğu için genelde altı görünmez, es geçiyoruz.
        return vertices;
    }

    // Ağaç Gövdesi için Silindir (Trigonometrik formüllerle)
    static std::vector<Vertex> createCylinder(float radius, float height, int sectorCount) {
        std::vector<Vertex> vertices;
        const float PI = 3.1415926f;
        float sectorStep = 2 * PI / sectorCount;

        for (int i = 0; i < sectorCount; ++i) {
            float sectorAngle1 = i * sectorStep;
            float sectorAngle2 = (i + 1) * sectorStep;

            float x1 = radius * cos(sectorAngle1);
            float z1 = radius * sin(sectorAngle1);
            float x2 = radius * cos(sectorAngle2);
            float z2 = radius * sin(sectorAngle2);

            // Normaller (Silindir yan yüzeyi için normal yatay vektördür)
            float nx1 = cos(sectorAngle1), nz1 = sin(sectorAngle1);
            float nx2 = cos(sectorAngle2), nz2 = sin(sectorAngle2);

            // İlk üçgen
            vertices.push_back({{x1, 0.0f, z1}, {nx1, 0.0f, nz1}, {0.0f, 0.0f}});
            vertices.push_back({{x2, 0.0f, z2}, {nx2, 0.0f, nz2}, {1.0f, 0.0f}});
            vertices.push_back({{x1, height, z1}, {nx1, 0.0f, nz1}, {0.0f, 1.0f}});

            // İkinci üçgen
            vertices.push_back({{x2, 0.0f, z2}, {nx2, 0.0f, nz2}, {1.0f, 0.0f}});
            vertices.push_back({{x2, height, z2}, {nx2, 0.0f, nz2}, {1.0f, 1.0f}});
            vertices.push_back({{x1, height, z1}, {nx1, 0.0f, nz1}, {0.0f, 1.0f}});
        }
        return vertices;
    }

    // Ağaç Yaprakları için Koni (Cone)
    static std::vector<Vertex> createCone(float radius, float height, int sectorCount) {
        std::vector<Vertex> vertices;
        const float PI = 3.1415926f;
        float sectorStep = 2 * PI / sectorCount;

        for (int i = 0; i < sectorCount; ++i) {
            float sectorAngle1 = i * sectorStep;
            float sectorAngle2 = (i + 1) * sectorStep;

            float x1 = radius * cos(sectorAngle1);
            float z1 = radius * sin(sectorAngle1);
            float x2 = radius * cos(sectorAngle2);
            float z2 = radius * sin(sectorAngle2);

            // Yüzey normallerini yaklaşık olarak hesaplayalım
            float nx1 = cos(sectorAngle1), nz1 = sin(sectorAngle1);
            float nx2 = cos(sectorAngle2), nz2 = sin(sectorAngle2);
            float ny = radius / height; // Koni eğimine göre Y normal bileşeni

            vertices.push_back({{x1, 0.0f, z1}, {nx1, ny, nz1}, {0.0f, 0.0f}});
            vertices.push_back({{x2, 0.0f, z2}, {nx2, ny, nz2}, {1.0f, 0.0f}});
            vertices.push_back({{0.0f, height, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}});
        }
        return vertices;
    }

    // Kuyu Suyu için Daire/Disk Geometrisi (Cap)
    static std::vector<Vertex> createCircle(float radius, int sectorCount) {
        std::vector<Vertex> vertices;
        const float PI = 3.1415926f;
        float sectorStep = 2 * PI / sectorCount;

        for (int i = 0; i < sectorCount; ++i) {
            float sectorAngle1 = i * sectorStep;
            float sectorAngle2 = (i + 1) * sectorStep;

            float x1 = radius * cos(sectorAngle1);
            float z1 = radius * sin(sectorAngle1);
            float x2 = radius * cos(sectorAngle2);
            float z2 = radius * sin(sectorAngle2);

            // Dokunun dairesel yayılımı için koordinatlar
            float u1 = 0.5f + 0.5f * cos(sectorAngle1);
            float v1 = 0.5f + 0.5f * sin(sectorAngle1);
            float u2 = 0.5f + 0.5f * cos(sectorAngle2);
            float v2 = 0.5f + 0.5f * sin(sectorAngle2);

            vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}});
            vertices.push_back({{x1, 0.0f, z1}, {0.0f, 1.0f, 0.0f}, {u1, v1}});
            vertices.push_back({{x2, 0.0f, z2}, {0.0f, 1.0f, 0.0f}, {u2, v2}});
        }
        return vertices;
    }
};

#endif
