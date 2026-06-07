#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Geometry.h"

#include "stb_image.h"
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <string>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(char const * path);

unsigned int setupVAO(const std::vector<Vertex>& vertices) {
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    glBindVertexArray(0);
    return VAO;
}

unsigned int createTextTexture(const std::string& text, const std::string& fontPath, int fontSize) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return 0;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font at: " << fontPath << std::endl;
        FT_Done_FreeType(ft);
        return 0;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    int textWidth = 0;
    int maxAscent = 0;
    int maxDescent = 0;

    for (char c : text) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYPE: Failed to load Glyph for: " << c << std::endl;
            continue;
        }
        FT_GlyphSlot glyph = face->glyph;
        textWidth += (glyph->advance.x >> 6);
        int ascent = glyph->bitmap_top;
        int descent = glyph->bitmap.rows - glyph->bitmap_top;
        if (ascent > maxAscent) maxAscent = ascent;
        if (descent > maxDescent) maxDescent = descent;
    }

    int textHeight = maxAscent + maxDescent;
    
    // Tabela kenar boşlukları (Metnin ortalanması ve sınır taşmalarını önlemek için)
    int paddingX = 24;
    int paddingY = 16;
    int totalWidth = textWidth + paddingX;
    int totalHeight = textHeight + paddingY;

    if (totalWidth <= 0 || totalHeight <= 0) {
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return 0;
    }

    // Canlı sıcak krem rengi arka plan (RGB = 248, 245, 236)
    std::vector<unsigned char> pixelBuffer(totalWidth * totalHeight * 4, 0);
    for (int i = 0; i < totalWidth * totalHeight * 4; i += 4) {
        pixelBuffer[i + 0] = 248; // R
        pixelBuffer[i + 1] = 245; // G
        pixelBuffer[i + 2] = 236; // B
        pixelBuffer[i + 3] = 255; // A
    }

    // Altın/Bronz rengi şık dekoratif çerçeve kenarlıkları çiz (Çift çizgi)
    int borderWidth1 = 4; // Kalın dış kenarlık
    int innerGap = 2;     // Kenarlıklar arası boşluk
    int borderWidth2 = 1; // İnce iç çizgi
    
    for (int y = 0; y < totalHeight; ++y) {
        for (int x = 0; x < totalWidth; ++x) {
            bool outerBorder = (x < borderWidth1) || (x >= totalWidth - borderWidth1) ||
                               (y < borderWidth1) || (y >= totalHeight - borderWidth1);
            
            int bx = x - borderWidth1 - innerGap;
            int by = y - borderWidth1 - innerGap;
            int bMaxX = totalWidth - borderWidth1 - innerGap;
            int bMaxY = totalHeight - borderWidth1 - innerGap;
            bool innerBorder = !outerBorder && (
                               (x >= borderWidth1 + innerGap && x < borderWidth1 + innerGap + borderWidth2) ||
                               (x >= bMaxX - borderWidth2 && x < bMaxX) ||
                               (y >= borderWidth1 + innerGap && y < borderWidth1 + innerGap + borderWidth2) ||
                               (y >= bMaxY - borderWidth2 && y < bMaxY)
                               );

            if (outerBorder || innerBorder) {
                int pixelIndex = (y * totalWidth + x) * 4;
                pixelBuffer[pixelIndex + 0] = 184; // R (Altın/Bronz)
                pixelBuffer[pixelIndex + 1] = 142; // G
                pixelBuffer[pixelIndex + 2] = 66;  // B
            }
        }
    }

    // Metni tabelanın ortasına hizalama
    int xOffset = paddingX / 2;
    for (char c : text) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot glyph = face->glyph;
        int glyphWidth = glyph->bitmap.width;
        int glyphHeight = glyph->bitmap.rows;
        int glyphTop = glyph->bitmap_top;
        int yOffset = (maxAscent - glyphTop) + (paddingY / 2);

        for (int y = 0; y < glyphHeight; ++y) {
            for (int x = 0; x < glyphWidth; ++x) {
                int destX = xOffset + x;
                int destY = yOffset + y;

                if (destX >= 0 && destX < totalWidth && destY >= 0 && destY < totalHeight) {
                    unsigned char val = glyph->bitmap.buffer[y * glyphWidth + x];
                    int pixelIndex = (destY * totalWidth + destX) * 4;

                    // Koyu kömür siyahı/antrasit yazı rengi (RGB = 44, 44, 44)
                    float alpha = val / 255.0f;
                    pixelBuffer[pixelIndex + 0] = static_cast<unsigned char>((1.0f - alpha) * pixelBuffer[pixelIndex + 0] + alpha * 44);
                    pixelBuffer[pixelIndex + 1] = static_cast<unsigned char>((1.0f - alpha) * pixelBuffer[pixelIndex + 1] + alpha * 44);
                    pixelBuffer[pixelIndex + 2] = static_cast<unsigned char>((1.0f - alpha) * pixelBuffer[pixelIndex + 2] + alpha * 44);
                }
            }
        }
        xOffset += (glyph->advance.x >> 6);
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, totalWidth, totalHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return textureID;
}

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 1.0f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// Kapı ve Lamba Kontrolleri
bool doorOpen = false;
float doorAngle = 0.0f;
bool lampOn = true;
bool flyMode = false;
bool nightMode = false;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Bilgisayar Grafikleri - Mini 3D Ev Sahnesi", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile our shader program
    Shader ourShader("../shader.vert", "../shader.frag");
    Shader skyboxShader("../skybox.vert", "../skybox.frag");

    // GEOMETRİ HAZIRLIĞI
    // ------------------------------------------------------------------
    // 1. Ev Duvarları (Küp) - Her yüzey 1x1 Texture tekrarlasın
    float cubeVertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // 2. Zemin (Plane) - 40x40 boyutlarında ve 40x40 oranında kaplama tekrarı (tiling)
    std::vector<Vertex> planeData = Geometry::createPlane(40.0f, 40.0f, 40.0f, 40.0f);
    unsigned int planeVAO = setupVAO(planeData);

    // 3. Ev Çatı (Triangular Prism) - Genişletilmiş ev boyutuna uyumlu saçaklı tasarım (5.4 x 1.2 x 4.4)
    std::vector<Vertex> roofData = Geometry::createRoofPrism(5.4f, 1.2f, 4.4f);
    unsigned int roofVAO = setupVAO(roofData);

    // 4. Ağaç Gövde (Cylinder) - Yükseklik 1.0, Yarıçap 0.1
    std::vector<Vertex> trunkData = Geometry::createCylinder(0.1f, 1.0f, 20);
    unsigned int trunkVAO = setupVAO(trunkData);

    // 5. Ağaç Yapraklar (Cone) - Yükseklik 1.0, Yarıçap 0.5
    std::vector<Vertex> leavesData = Geometry::createCone(0.5f, 1.0f, 20);
    unsigned int leavesVAO = setupVAO(leavesData);

    // Kuyu Suyu Geometrisi (Disk/Circle) - Yarıçap 0.49
    std::vector<Vertex> waterData = Geometry::createCircle(0.49f, 32);
    unsigned int waterVAO = setupVAO(waterData);

    // Ağaç Modellerinin Listesi (Konum, Gövde Yarıçapı, Gövde Yüksekliği, Yaprak Yarıçapı, Yaprak Yüksekliği)
    struct Tree {
        glm::vec3 position;
        float trunkRadius;
        float trunkHeight;
        float leavesRadius;
        float leavesHeight;
    };
    std::vector<Tree> trees = {
        { glm::vec3(4.5f, -0.5f, -3.5f), 0.15f, 1.0f, 0.8f, 1.6f },
        { glm::vec3(-4.5f, -0.5f, 3.0f), 0.12f, 0.8f, 0.6f, 1.2f },
        { glm::vec3(-4.8f, -0.5f, -4.0f), 0.16f, 1.2f, 0.8f, 1.7f },
        { glm::vec3(5.0f, -0.5f, 4.0f), 0.1f, 0.7f, 0.5f, 1.1f }
    };

    // DOKULARI YÜKLE (TEXTURES)
    // ------------------------------------------------------------------
    stbi_set_flip_vertically_on_load(true);
    unsigned int grassTexture = loadTexture("../assets/grass.png");
    unsigned int brickTexture = loadTexture("../assets/brick.png");
    unsigned int roofTexture  = loadTexture("../assets/roof.png");
    unsigned int woodTexture  = loadTexture("../assets/wood.png");
    unsigned int fireTexture  = loadTexture("../assets/fire.png");
    unsigned int flowerTexture = loadTexture("../assets/flower.png");
    unsigned int waterTexture  = loadTexture("../assets/water.png");
    unsigned int welcomeTexture = createTextTexture("WELCOME", "../assets/georgia.ttf", 64);
    unsigned int catTexture  = loadTexture("../assets/cat.png");

    ourShader.use();
    ourShader.setInt("diffuseMap", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // Gece modu veya Gündüz moduna göre cycleValue (0.0f = Gece, 1.0f = Gündüz)
        float cycleValue = nightMode ? 0.0f : 1.0f; 

        // Gökyüzü Zenith ve Horizon Renkleri (Gündüz ve Gece Karışımı)
        glm::vec3 dayZenith(0.12f, 0.36f, 0.76f);   // Derin mavi zenith
        glm::vec3 dayHorizon(0.55f, 0.78f, 0.98f);  // Açık mavi-turkuaz horizon
        glm::vec3 nightZenith(0.01f, 0.01f, 0.03f); // Çok koyu lacivert zenith
        glm::vec3 nightHorizon(0.03f, 0.04f, 0.08f); // Ufukta hafif lacivert aydınlık
        
        glm::vec3 zenithColor = glm::mix(nightZenith, dayZenith, cycleValue);
        glm::vec3 horizonColor = glm::mix(nightHorizon, dayHorizon, cycleValue);
        glm::vec3 sunColor = glm::vec3(1.0f, 0.9f, 0.6f);  // Güneş parlaması rengi
        glm::vec3 moonColor = glm::vec3(0.65f, 0.8f, 1.0f); // Ay parlaması rengi
        
        // Işık Rengi ve Şiddeti (Güneş Işığı vs Loş Mavi Ay Işığı)
        glm::vec3 dayLight(1.0f, 1.0f, 0.9f);   // Güneş Işığı (Hafif sarımsı)
        glm::vec3 nightLight(0.08f, 0.08f, 0.2f); // Ay Işığı (Soluk mavi, loş)
        glm::vec3 lightColor = glm::mix(nightLight, dayLight, cycleValue);
        
        // Güneş ve Ay Konumları
        glm::vec3 sunPos = glm::vec3(15.0f, 38.0f, -25.0f);
        glm::vec3 moonPos = glm::vec3(-15.0f, 35.0f, 25.0f);
        glm::vec3 lightPos = glm::mix(moonPos, sunPos, cycleValue); // Yönlü ışık kaynağı gök cismine göre değişir

        // render
        glClearColor(horizonColor.r, horizonColor.g, horizonColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- SKYBOX ÇİZİMİ ---
        glDisable(GL_DEPTH_TEST);
        skyboxShader.use();
        skyboxShader.setMat4("projection", glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f));
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(camera.GetViewMatrix())));
        skyboxShader.setVec3("sunDir", glm::normalize(sunPos));
        skyboxShader.setVec3("moonDir", glm::normalize(moonPos));
        skyboxShader.setVec3("zenithColor", zenithColor);
        skyboxShader.setVec3("horizonColor", horizonColor);
        skyboxShader.setVec3("sunColor", sunColor);
        skyboxShader.setVec3("moonColor", moonColor);
        skyboxShader.setFloat("cycleValue", cycleValue);
        
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glEnable(GL_DEPTH_TEST);

        // activate shader
        ourShader.use();
        ourShader.setBool("isFlame", false);
        ourShader.setBool("isWater", false);
        ourShader.setBool("isWall", false);
        ourShader.setVec3("lightColor", lightColor);
        ourShader.setVec3("lightPos", lightPos);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("skyColor", horizonColor);
        ourShader.setFloat("time", currentFrame);

        // create transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        glActiveTexture(GL_TEXTURE0);

        // --- ÇİZİM İŞLEMLERİ ---

        // 1. Zemin (Ground)
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f)); // Çim Tiling (Geometride zaten 40 tekrar var, çarpan 1.0 olmalı)
        glm::mat4 modelPlane = glm::mat4(1.0f);
        modelPlane = glm::translate(modelPlane, glm::vec3(0.0f, -0.5f, 0.0f));
        ourShader.setMat4("model", modelPlane);
        glDrawArrays(GL_TRIANGLES, 0, planeData.size());

        // 1b. Ev İçi Zemin (Düz Uzun Açık Renk Ahşap Tahtalar - Parke Görünümü)
        glBindVertexArray(cubeVAO);
        ourShader.setBool("useTexture", false);
        ourShader.setFloat("specularStrength", 0.25f);
        ourShader.setFloat("shininess", 32.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        float floorWidth = 5.0f;
        float floorDepth = 4.0f;
        float plankWidth = 0.16f; // Tahta genişliği
        float gap = 0.005f;       // Derz aralığı (gölgeler ve derinlik hissi için)
        float startX = -floorWidth / 2.0f + plankWidth / 2.0f;
        float endX = floorWidth / 2.0f;

        for (float x = startX; x < endX; x += plankWidth) {
            // Doğal ahşap tonu çeşitliliği için hafif renk dalgalanması
            float tone = 0.025f * sin(x * 15.0f) + 0.01f * cos(x * 30.0f);
            glm::vec3 plankColor = glm::vec3(0.85f, 0.75f, 0.62f) + tone; // Açık meşe rengi
            ourShader.setVec3("objectColor", plankColor);

            glm::mat4 modelPlank = glm::mat4(1.0f);
            modelPlank = glm::translate(modelPlank, glm::vec3(x, -0.49f, 0.0f));
            modelPlank = glm::scale(modelPlank, glm::vec3(plankWidth - gap, 0.02f, floorDepth));
            ourShader.setMat4("model", modelPlank);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Noktasal Işıkların Uniform Değerlerinin Tanımlanması
        float flicker = 0.8f + 0.2f * sin(currentFrame * 15.0f) * cos(currentFrame * 7.0f);
        glm::vec3 fireColor = glm::vec3(1.0f, 0.45f, 0.05f) * flicker;
        
        ourShader.setBool("lampOn", lampOn);
        ourShader.setVec3("lampPos", glm::vec3(1.0f, 0.7f, 2.08f));
        ourShader.setVec3("lampColor", glm::vec3(1.0f, 0.85f, 0.55f));
        ourShader.setVec3("firePos", glm::vec3(2.3f, -0.45f, 0.0f));
        ourShader.setVec3("fireColor", fireColor);

        // İç Oda Lambaları Uniform Tanımları
        ourShader.setVec3("roomLightLeftPos", glm::vec3(-1.5f, 0.95f, 0.0f));
        ourShader.setVec3("roomLightLeftColor", glm::vec3(0.9f, 0.8f, 0.6f)); // Sıcak sarı ışık
        ourShader.setVec3("roomLightRightPos", glm::vec3(1.0f, 0.95f, 0.0f));
        ourShader.setVec3("roomLightRightColor", glm::vec3(0.9f, 0.8f, 0.6f));

        // Kapı Açılma Animasyonu
        float targetAngle = doorOpen ? -90.0f : 0.0f;
        doorAngle += (targetAngle - doorAngle) * deltaTime * 5.0f;

        // 2. Ev Duvarları (Separate walls - Smooth painted plaster surface)
        glBindVertexArray(cubeVAO);
        ourShader.setBool("isWall", true); // Enable window hole cutouts in shader
        ourShader.setBool("useTexture", false); // Disable texture to get a clean plaster finish
        ourShader.setVec3("objectColor", 0.22f, 0.48f, 0.72f); // Vibrant warm steel blue
        ourShader.setFloat("specularStrength", 0.05f); // Matte plaster surface
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        // A) Arka Duvar (5.0 x 1.8 x 0.05)
        ourShader.setVec2("uvScale", glm::vec2(5.0f, 2.0f));
        glm::mat4 modelBackWall = glm::mat4(1.0f);
        modelBackWall = glm::translate(modelBackWall, glm::vec3(0.0f, 0.4f, -2.0f));
        modelBackWall = glm::scale(modelBackWall, glm::vec3(5.0f, 1.8f, 0.05f));
        ourShader.setMat4("model", modelBackWall);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // B) Sol Duvar (0.05 x 1.8 x 4.0)
        ourShader.setVec2("uvScale", glm::vec2(4.0f, 2.0f));
        glm::mat4 modelLeftWall = glm::mat4(1.0f);
        modelLeftWall = glm::translate(modelLeftWall, glm::vec3(-2.5f, 0.4f, 0.0f));
        modelLeftWall = glm::scale(modelLeftWall, glm::vec3(0.05f, 1.8f, 4.0f));
        ourShader.setMat4("model", modelLeftWall);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // C) Sağ Duvar (0.05 x 1.8 x 4.0)
        ourShader.setVec2("uvScale", glm::vec2(4.0f, 2.0f));
        glm::mat4 modelRightWall = glm::mat4(1.0f);
        modelRightWall = glm::translate(modelRightWall, glm::vec3(2.5f, 0.4f, 0.0f));
        modelRightWall = glm::scale(modelRightWall, glm::vec3(0.05f, 1.8f, 4.0f));
        ourShader.setMat4("model", modelRightWall);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // D) Ön Duvar Sol Parça (3.2 x 1.8 x 0.05) - Kapı soluna kadar
        ourShader.setVec2("uvScale", glm::vec2(3.2f, 2.0f));
        glm::mat4 modelFrontWallLeft = glm::mat4(1.0f);
        modelFrontWallLeft = glm::translate(modelFrontWallLeft, glm::vec3(-0.9f, 0.4f, 2.0f));
        modelFrontWallLeft = glm::scale(modelFrontWallLeft, glm::vec3(3.2f, 1.8f, 0.05f));
        ourShader.setMat4("model", modelFrontWallLeft);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // E) Ön Duvar Sağ Parça (1.2 x 1.8 x 0.05) - Kapı sağından duvara kadar
        ourShader.setVec2("uvScale", glm::vec2(1.2f, 2.0f));
        glm::mat4 modelFrontWallRight = glm::mat4(1.0f);
        modelFrontWallRight = glm::translate(modelFrontWallRight, glm::vec3(1.9f, 0.4f, 2.0f));
        modelFrontWallRight = glm::scale(modelFrontWallRight, glm::vec3(1.2f, 1.8f, 0.05f));
        ourShader.setMat4("model", modelFrontWallRight);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // F) Ön Duvar Kapı Üstü Parça (0.6 x 0.6 x 0.05) - Kapının üst kısmı
        ourShader.setVec2("uvScale", glm::vec2(0.6f, 0.6f));
        glm::mat4 modelFrontWallTop = glm::mat4(1.0f);
        modelFrontWallTop = glm::translate(modelFrontWallTop, glm::vec3(1.0f, 1.0f, 2.0f));
        modelFrontWallTop = glm::scale(modelFrontWallTop, glm::vec3(0.6f, 0.6f, 0.05f));
        ourShader.setMat4("model", modelFrontWallTop);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3. İç Oda Bölme Duvarı (Bölme x = -0.5f hizasına çekildi, kapı z = -1.2f tarafında - Tuğla Kaplama)
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, brickTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setBool("isWall", false); // Bölme duvarda pencere boşluğu açılmasın
        ourShader.setVec3("objectColor", 0.9f, 0.85f, 0.8f); // Sıcak tuğla tonu
        ourShader.setFloat("specularStrength", 0.1f);         // Mat tuğla yansıması
        ourShader.setFloat("shininess", 16.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        // Arka Kısım Bölmesi (kapı boşluğunun arkası: z = -2.0f ile -1.6f arası)
        ourShader.setVec2("uvScale", glm::vec2(0.4f, 1.8f)); // Sıkışmayı önlemek için gerçekçi tiling
        glm::mat4 modelWallBack = glm::mat4(1.0f);
        modelWallBack = glm::translate(modelWallBack, glm::vec3(-0.5f, 0.4f, -1.8f));
        modelWallBack = glm::scale(modelWallBack, glm::vec3(0.05f, 1.8f, 0.4f));
        ourShader.setMat4("model", modelWallBack);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ön Kısım Bölmesi (kapı boşluğunun önü: z = -0.8f ile 2.0f arası)
        ourShader.setVec2("uvScale", glm::vec2(2.8f, 1.8f));
        glm::mat4 modelWallFront = glm::mat4(1.0f);
        modelWallFront = glm::translate(modelWallFront, glm::vec3(-0.5f, 0.4f, 0.6f));
        modelWallFront = glm::scale(modelWallFront, glm::vec3(0.05f, 1.8f, 2.8f));
        ourShader.setMat4("model", modelWallFront);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Üst Kısım Bölmesi (İç kapı üstü kemeri: z = -1.6f ile -0.8f arası)
        ourShader.setVec2("uvScale", glm::vec2(0.8f, 0.6f));
        glm::mat4 modelWallTop = glm::mat4(1.0f);
        modelWallTop = glm::translate(modelWallTop, glm::vec3(-0.5f, 1.0f, -1.2f));
        modelWallTop = glm::scale(modelWallTop, glm::vec3(0.05f, 0.6f, 0.8f));
        ourShader.setMat4("model", modelWallTop);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3c. Bölme Duvarı Ahşap Direkleri (Uçlardaki bozuk doku görünümünü kapatmak için destek kolonları)
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.5f, 0.35f, 0.2f); // Koyu ahşap rengi
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setVec2("uvScale", glm::vec2(0.5f, 2.0f));

        // Direk 1 (z = -0.8f kapı eşiği)
        glm::mat4 modelCol1 = glm::mat4(1.0f);
        modelCol1 = glm::translate(modelCol1, glm::vec3(-0.5f, 0.4f, -0.8f));
        modelCol1 = glm::scale(modelCol1, glm::vec3(0.06f, 1.8f, 0.06f));
        ourShader.setMat4("model", modelCol1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Direk 2 (z = -1.6f kapı eşiği)
        glm::mat4 modelCol2 = glm::mat4(1.0f);
        modelCol2 = glm::translate(modelCol2, glm::vec3(-0.5f, 0.4f, -1.6f));
        modelCol2 = glm::scale(modelCol2, glm::vec3(0.06f, 1.8f, 0.06f));
        ourShader.setMat4("model", modelCol2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3b. Dış Duvarların İç Tuğla Panelleri (Loft tarzı şömine odası görünümü)
        glBindTexture(GL_TEXTURE_2D, brickTexture);
        ourShader.setBool("isWall", true); // İç panellerde pencere boşluğu açılmalı
        ourShader.setVec3("objectColor", 0.9f, 0.85f, 0.8f);

        // Arka Duvar İç Paneli
        ourShader.setVec2("uvScale", glm::vec2(5.0f, 2.0f));
        glm::mat4 modelInnerBack = glm::mat4(1.0f);
        modelInnerBack = glm::translate(modelInnerBack, glm::vec3(0.0f, 0.4f, -1.97f));
        modelInnerBack = glm::scale(modelInnerBack, glm::vec3(4.96f, 1.78f, 0.01f));
        ourShader.setMat4("model", modelInnerBack);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sol Duvar İç Paneli
        ourShader.setVec2("uvScale", glm::vec2(4.0f, 2.0f));
        glm::mat4 modelInnerLeft = glm::mat4(1.0f);
        modelInnerLeft = glm::translate(modelInnerLeft, glm::vec3(-2.47f, 0.4f, 0.0f));
        modelInnerLeft = glm::scale(modelInnerLeft, glm::vec3(0.01f, 1.78f, 3.96f));
        ourShader.setMat4("model", modelInnerLeft);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sağ Duvar İç Paneli
        ourShader.setVec2("uvScale", glm::vec2(4.0f, 2.0f));
        glm::mat4 modelInnerRight = glm::mat4(1.0f);
        modelInnerRight = glm::translate(modelInnerRight, glm::vec3(2.47f, 0.4f, 0.0f));
        modelInnerRight = glm::scale(modelInnerRight, glm::vec3(0.01f, 1.78f, 3.96f));
        ourShader.setMat4("model", modelInnerRight);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ön Duvar Sol İç Paneli
        ourShader.setVec2("uvScale", glm::vec2(3.2f, 2.0f));
        glm::mat4 modelInnerFrontL = glm::mat4(1.0f);
        modelInnerFrontL = glm::translate(modelInnerFrontL, glm::vec3(-0.9f, 0.4f, 1.97f));
        modelInnerFrontL = glm::scale(modelInnerFrontL, glm::vec3(3.18f, 1.78f, 0.01f));
        ourShader.setMat4("model", modelInnerFrontL);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ön Duvar Sağ İç Paneli
        ourShader.setVec2("uvScale", glm::vec2(1.2f, 2.0f));
        glm::mat4 modelInnerFrontR = glm::mat4(1.0f);
        modelInnerFrontR = glm::translate(modelInnerFrontR, glm::vec3(1.9f, 0.4f, 1.97f));
        modelInnerFrontR = glm::scale(modelInnerFrontR, glm::vec3(1.18f, 1.78f, 0.01f));
        ourShader.setMat4("model", modelInnerFrontR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ön Duvar Kapı Üstü İç Paneli
        ourShader.setVec2("uvScale", glm::vec2(0.6f, 0.6f));
        glm::mat4 modelInnerFrontT = glm::mat4(1.0f);
        modelInnerFrontT = glm::translate(modelInnerFrontT, glm::vec3(1.0f, 1.0f, 1.97f));
        modelInnerFrontT = glm::scale(modelInnerFrontT, glm::vec3(0.58f, 0.58f, 0.01f));
        ourShader.setMat4("model", modelInnerFrontT);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        ourShader.setBool("isWall", false); // Tavan vb. çizimler için isWall sıfırlanmalı

        // 3b. Ev Tavanı (Ceiling - to cover hollow roof from below)
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.85f, 0.8f, 0.75f); // Soft warm ceiling color
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(5.4f, 4.4f));

        glm::mat4 modelCeiling = glm::mat4(1.0f);
        modelCeiling = glm::translate(modelCeiling, glm::vec3(0.0f, 1.29f, 0.0f)); // Just below roof y=1.3
        modelCeiling = glm::scale(modelCeiling, glm::vec3(5.4f, 0.02f, 4.4f));     // Covers the entire roof area (including eaves)
        ourShader.setMat4("model", modelCeiling);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 4. Ev Çatısı (Prism) - Büyütülmüş Saçaklı Çatı (5.4 x 1.2 x 4.4)
        glBindVertexArray(roofVAO);
        glBindTexture(GL_TEXTURE_2D, roofTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("specularStrength", 0.2f);
        ourShader.setFloat("shininess", 32.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(6.0f, 4.0f)); // Çatı Tiling
        glm::mat4 modelRoof = glm::mat4(1.0f);
        modelRoof = glm::translate(modelRoof, glm::vec3(0.0f, 1.3f, 0.0f)); 
        ourShader.setMat4("model", modelRoof);
        glDrawArrays(GL_TRIANGLES, 0, roofData.size());

        // 5. Kapı (3D Menteşeli Kapı - Modern Çelik Kapı)
        glBindVertexArray(cubeVAO);
        ourShader.setBool("useTexture", false); // Ahşap dokusunu devre dışı bırakıyoruz
        ourShader.setVec3("objectColor", 0.22f, 0.25f, 0.28f); // Modern çelik mavisi-gri
        ourShader.setFloat("specularStrength", 0.6f);
        ourShader.setFloat("shininess", 64.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        glm::mat4 doorBase = glm::mat4(1.0f);
        // Sol menteşe x=0.7f hizasındadır
        doorBase = glm::translate(doorBase, glm::vec3(0.7f, 0.1f, 2.0f));
        doorBase = glm::rotate(doorBase, glm::radians(doorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

        // Ana Kapı Gövdesi
        glm::mat4 modelDoor = glm::translate(doorBase, glm::vec3(0.3f, 0.0f, 0.0f));
        modelDoor = glm::scale(modelDoor, glm::vec3(0.6f, 1.2f, 0.05f));
        ourShader.setMat4("model", modelDoor);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Kapı Üzerindeki Çelik Paneller (3D derinlik kazandırmak için)
        ourShader.setVec3("objectColor", 0.16f, 0.18f, 0.21f); // Biraz daha koyu ton
        ourShader.setFloat("specularStrength", 0.8f);

        // Üst Panel
        glm::mat4 modelPanel1 = glm::translate(doorBase, glm::vec3(0.3f, 0.25f, 0.026f));
        modelPanel1 = glm::scale(modelPanel1, glm::vec3(0.44f, 0.42f, 0.01f));
        ourShader.setMat4("model", modelPanel1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Alt Panel
        glm::mat4 modelPanel2 = glm::translate(doorBase, glm::vec3(0.3f, -0.35f, 0.026f));
        modelPanel2 = glm::scale(modelPanel2, glm::vec3(0.44f, 0.42f, 0.01f));
        ourShader.setMat4("model", modelPanel2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 5b. Welcome Tabelası (Kapı Üstü Tabela)
        glBindTexture(GL_TEXTURE_2D, welcomeTexture);
        ourShader.setBool("useTexture", true); // Tabela için dokuyu tekrar aktif ediyoruz
        ourShader.setVec2("uvScale", glm::vec2(1.0f, -1.0f)); // Dikey olarak aynala (yazıyı dik hale getirir)
        ourShader.setVec2("uvOffset", glm::vec2(0.0f, 1.0f));  // Dikey döndürme için gerekli ofset
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("specularStrength", 0.2f);
        ourShader.setFloat("shininess", 32.0f);
        
        glm::mat4 modelWelcome = glm::mat4(1.0f);
        modelWelcome = glm::translate(modelWelcome, glm::vec3(1.0f, 1.05f, 2.027f)); // Kapı üstü hizası
        modelWelcome = glm::scale(modelWelcome, glm::vec3(0.42f, 0.12f, 0.01f));
        ourShader.setMat4("model", modelWelcome);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Diğer çizimlerin etkilenmemesi için ofseti sıfırla
        ourShader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));

        // Kapı Kolu (Altın rengi)
        ourShader.setBool("useTexture", false);
        ourShader.setVec3("objectColor", 0.85f, 0.65f, 0.2f);
        ourShader.setFloat("specularStrength", 1.0f);
        ourShader.setFloat("shininess", 64.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        glm::mat4 modelHandle = glm::translate(doorBase, glm::vec3(0.48f, 0.0f, 0.035f)); 
        modelHandle = glm::scale(modelHandle, glm::vec3(0.04f, 0.04f, 0.04f));
        ourShader.setMat4("model", modelHandle);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 6. Kapı Üstü Lambası (3D Model)
        // Lamba Demiri
        glBindVertexArray(cubeVAO);
        ourShader.setBool("useTexture", false);
        ourShader.setVec3("objectColor", 0.1f, 0.1f, 0.1f);
        ourShader.setFloat("specularStrength", 0.1f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        glm::mat4 modelLampRod = glm::mat4(1.0f);
        modelLampRod = glm::translate(modelLampRod, glm::vec3(1.0f, 0.8f, 2.04f));
        modelLampRod = glm::scale(modelLampRod, glm::vec3(0.03f, 0.03f, 0.08f));
        ourShader.setMat4("model", modelLampRod);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ampul / Fener (Lamba açıkken sarımtırak parlar)
        ourShader.setVec3("objectColor", lampOn ? glm::vec3(1.0f, 1.0f, 0.7f) : glm::vec3(0.3f, 0.3f, 0.2f));
        ourShader.setFloat("specularStrength", lampOn ? 0.0f : 0.2f);
        glm::mat4 modelLampBulb = glm::mat4(1.0f);
        modelLampBulb = glm::translate(modelLampBulb, glm::vec3(1.0f, 0.74f, 2.08f));
        modelLampBulb = glm::scale(modelLampBulb, glm::vec3(0.08f, 0.12f, 0.08f));
        ourShader.setMat4("model", modelLampBulb);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 6b. İÇ ODA LAMBALARI (Her iki oda için tavan lambaları)
        // Sol Oda Lambası (Yatak Odası - Tavan ortası sarkıt)
        ourShader.setVec3("objectColor", glm::vec3(0.1f, 0.1f, 0.1f));
        ourShader.setFloat("specularStrength", 0.1f);
        glm::mat4 modelLampRodLeft = glm::mat4(1.0f);
        modelLampRodLeft = glm::translate(modelLampRodLeft, glm::vec3(-1.5f, 1.15f, 0.0f));
        modelLampRodLeft = glm::scale(modelLampRodLeft, glm::vec3(0.02f, 0.3f, 0.02f));
        ourShader.setMat4("model", modelLampRodLeft);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 0.8f)); // Parlak sıcak beyaz
        ourShader.setFloat("specularStrength", 0.0f);
        glm::mat4 modelLampBulbLeft = glm::mat4(1.0f);
        modelLampBulbLeft = glm::translate(modelLampBulbLeft, glm::vec3(-1.5f, 0.95f, 0.0f));
        modelLampBulbLeft = glm::scale(modelLampBulbLeft, glm::vec3(0.06f, 0.06f, 0.06f));
        ourShader.setMat4("model", modelLampBulbLeft);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sağ Oda Lambası (Oturma Odası - Tavan ortası sarkıt)
        ourShader.setVec3("objectColor", glm::vec3(0.1f, 0.1f, 0.1f));
        ourShader.setFloat("specularStrength", 0.1f);
        glm::mat4 modelLampRodRight = glm::mat4(1.0f);
        modelLampRodRight = glm::translate(modelLampRodRight, glm::vec3(1.0f, 1.15f, 0.0f));
        modelLampRodRight = glm::scale(modelLampRodRight, glm::vec3(0.02f, 0.3f, 0.02f));
        ourShader.setMat4("model", modelLampRodRight);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ampul (Sıcak ışık)
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 0.8f));
        ourShader.setFloat("specularStrength", 0.0f);
        glm::mat4 modelLampBulbRight = glm::mat4(1.0f);
        modelLampBulbRight = glm::translate(modelLampBulbRight, glm::vec3(1.0f, 0.95f, 0.0f));
        modelLampBulbRight = glm::scale(modelLampBulbRight, glm::vec3(0.06f, 0.06f, 0.06f));
        ourShader.setMat4("model", modelLampBulbRight);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 7. Pencereler (Saydam oldukları için rendering sırasının sonuna taşındı)

        // 8. İÇ DETAYLAR
        // ------------------------------------------------------------------
        
        // A) YATAK (Sol Oda - Yatak Odası - Left wall yaslanmış, engellemesiz konum)
        // Ahşap Baza (Z yönünde uzanır)
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.45f, 0.25f, 0.15f);
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        glm::mat4 modelBedBase = glm::mat4(1.0f);
        modelBedBase = glm::translate(modelBedBase, glm::vec3(-1.8f, -0.4f, 0.5f));
        modelBedBase = glm::scale(modelBedBase, glm::vec3(1.2f, 0.2f, 1.6f));
        ourShader.setMat4("model", modelBedBase);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Beyaz Çarşaf
        ourShader.setBool("useTexture", false);
        ourShader.setVec3("objectColor", 0.9f, 0.9f, 0.9f);
        glm::mat4 modelBedMattress = glm::mat4(1.0f);
        modelBedMattress = glm::translate(modelBedMattress, glm::vec3(-1.8f, -0.2f, 0.5f));
        modelBedMattress = glm::scale(modelBedMattress, glm::vec3(1.1f, 0.2f, 1.5f));
        ourShader.setMat4("model", modelBedMattress);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Kırmızı Yorgan
        ourShader.setVec3("objectColor", 0.8f, 0.15f, 0.15f);
        glm::mat4 modelBedBlanket = glm::mat4(1.0f);
        modelBedBlanket = glm::translate(modelBedBlanket, glm::vec3(-1.8f, -0.19f, 0.35f));
        modelBedBlanket = glm::scale(modelBedBlanket, glm::vec3(1.11f, 0.21f, 1.1f));
        ourShader.setMat4("model", modelBedBlanket);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Yastık (Baş kısmı z=1.1)
        ourShader.setVec3("objectColor", 0.95f, 0.95f, 0.95f);
        glm::mat4 modelBedPillow = glm::mat4(1.0f);
        modelBedPillow = glm::translate(modelBedPillow, glm::vec3(-1.8f, -0.05f, 1.1f));
        modelBedPillow = glm::scale(modelBedPillow, glm::vec3(0.8f, 0.08f, 0.25f));
        ourShader.setMat4("model", modelBedPillow);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // B) KOLTUK (Sağ Oda - Oturma Odası - Bölme duvarına yaslı, şömineye dönük, z=0.7)
        // Arkalık
        ourShader.setVec3("objectColor", 0.8f, 0.7f, 0.55f); 
        glm::mat4 modelSofaBack = glm::mat4(1.0f);
        modelSofaBack = glm::translate(modelSofaBack, glm::vec3(-0.3f, -0.1f, 0.7f));
        modelSofaBack = glm::scale(modelSofaBack, glm::vec3(0.15f, 0.8f, 1.4f));
        ourShader.setMat4("model", modelSofaBack);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Oturma Minderi
        glm::mat4 modelSofaSeat = glm::mat4(1.0f);
        modelSofaSeat = glm::translate(modelSofaSeat, glm::vec3(0.075f, -0.3f, 0.7f));
        modelSofaSeat = glm::scale(modelSofaSeat, glm::vec3(0.6f, 0.4f, 1.4f));
        ourShader.setMat4("model", modelSofaSeat);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sol Kolçak
        ourShader.setVec3("objectColor", 0.7f, 0.6f, 0.45f);
        glm::mat4 modelSofaArmL = glm::mat4(1.0f);
        modelSofaArmL = glm::translate(modelSofaArmL, glm::vec3(0.075f, -0.2f, 1.425f));
        modelSofaArmL = glm::scale(modelSofaArmL, glm::vec3(0.6f, 0.6f, 0.15f));
        ourShader.setMat4("model", modelSofaArmL);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sağ Kolçak
        glm::mat4 modelSofaArmR = glm::mat4(1.0f);
        modelSofaArmR = glm::translate(modelSofaArmR, glm::vec3(0.075f, -0.2f, -0.025f));
        modelSofaArmR = glm::scale(modelSofaArmR, glm::vec3(0.6f, 0.6f, 0.15f));
        ourShader.setMat4("model", modelSofaArmR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // C) ŞÖMİNE VE ANIMASYONLU DOKULU ATEŞ (Sağ Oda - x=2.5f duvarına yaslı, z=0.0f)
        // Sol Sütun (Brick)
        glBindTexture(GL_TEXTURE_2D, brickTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        glm::mat4 modelFireplaceL = glm::mat4(1.0f);
        modelFireplaceL = glm::translate(modelFireplaceL, glm::vec3(2.3f, -0.1f, -0.4f));
        modelFireplaceL = glm::scale(modelFireplaceL, glm::vec3(0.2f, 0.8f, 0.15f));
        ourShader.setMat4("model", modelFireplaceL);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sağ Sütun (Brick)
        glm::mat4 modelFireplaceR = glm::mat4(1.0f);
        modelFireplaceR = glm::translate(modelFireplaceR, glm::vec3(2.3f, -0.1f, 0.4f));
        modelFireplaceR = glm::scale(modelFireplaceR, glm::vec3(0.2f, 0.8f, 0.15f));
        ourShader.setMat4("model", modelFireplaceR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Üst Ahşap Raf (Wood)
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setVec3("objectColor", 0.6f, 0.4f, 0.2f);
        glm::mat4 modelFireplaceMantel = glm::mat4(1.0f);
        modelFireplaceMantel = glm::translate(modelFireplaceMantel, glm::vec3(2.3f, 0.35f, 0.0f));
        modelFireplaceMantel = glm::scale(modelFireplaceMantel, glm::vec3(0.25f, 0.1f, 1.0f));
        ourShader.setMat4("model", modelFireplaceMantel);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Şömine İç Arka Siyah Hazne
        ourShader.setBool("useTexture", false);
        ourShader.setVec3("objectColor", 0.08f, 0.08f, 0.08f);
        glm::mat4 modelFireplaceBack = glm::mat4(1.0f);
        modelFireplaceBack = glm::translate(modelFireplaceBack, glm::vec3(2.45f, -0.1f, 0.0f));
        modelFireplaceBack = glm::scale(modelFireplaceBack, glm::vec3(0.08f, 0.8f, 0.7f));
        ourShader.setMat4("model", modelFireplaceBack);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Şömine İçindeki Odun Parçaları (Stacked Burnt Logs)
        ourShader.setBool("useTexture", false); // Doku kullanılmıyor, düz renk tonları
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);

        // 1. Sol Alt Odun (Açılı)
        ourShader.setVec3("objectColor", 0.22f, 0.10f, 0.04f); // Koyu kahve
        glm::mat4 modelLog1 = glm::mat4(1.0f);
        modelLog1 = glm::translate(modelLog1, glm::vec3(2.27f, -0.47f, 0.15f));
        modelLog1 = glm::rotate(modelLog1, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelLog1 = glm::scale(modelLog1, glm::vec3(0.08f, 0.06f, 0.35f));
        ourShader.setMat4("model", modelLog1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 2. Sağ Alt Odun (Açılı)
        ourShader.setVec3("objectColor", 0.20f, 0.09f, 0.04f);
        glm::mat4 modelLog2 = glm::mat4(1.0f);
        modelLog2 = glm::translate(modelLog2, glm::vec3(2.33f, -0.47f, -0.15f));
        modelLog2 = glm::rotate(modelLog2, glm::radians(-20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelLog2 = glm::scale(modelLog2, glm::vec3(0.08f, 0.06f, 0.35f));
        ourShader.setMat4("model", modelLog2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3. Üst Orta Odun (Çapraz duran)
        ourShader.setVec3("objectColor", 0.26f, 0.13f, 0.06f);
        glm::mat4 modelLog3 = glm::mat4(1.0f);
        modelLog3 = glm::translate(modelLog3, glm::vec3(2.26f, -0.42f, 0.0f));
        modelLog3 = glm::rotate(modelLog3, glm::radians(85.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelLog3 = glm::scale(modelLog3, glm::vec3(0.07f, 0.05f, 0.38f));
        ourShader.setMat4("model", modelLog3);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 4. Ön Küçük Odun
        ourShader.setVec3("objectColor", 0.18f, 0.08f, 0.03f);
        glm::mat4 modelLog4 = glm::mat4(1.0f);
        modelLog4 = glm::translate(modelLog4, glm::vec3(2.20f, -0.48f, 0.05f));
        modelLog4 = glm::rotate(modelLog4, glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelLog4 = glm::scale(modelLog4, glm::vec3(0.05f, 0.04f, 0.20f));
        ourShader.setMat4("model", modelLog4);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 5. Arka Küçük Odun
        ourShader.setVec3("objectColor", 0.17f, 0.08f, 0.03f);
        glm::mat4 modelLog5 = glm::mat4(1.0f);
        modelLog5 = glm::translate(modelLog5, glm::vec3(2.38f, -0.48f, -0.05f));
        modelLog5 = glm::rotate(modelLog5, glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelLog5 = glm::scale(modelLog5, glm::vec3(0.05f, 0.04f, 0.20f));
        ourShader.setMat4("model", modelLog5);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ŞÖMİNE GERÇEK ATEŞ DOKULU YILDIZ BILLBOARD VE EK ALEVLER (Star-Billboard & Offset Flames)
        glBindTexture(GL_TEXTURE_2D, fireTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("specularStrength", 0.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        ourShader.setBool("isFlame", true);

        // Transparan alevlerin çakışma ve derinlik tamponu (depth writing) kaynaklı kenar çizgisi hatalarını engellemek için depth mask kapatılıyor
        glDepthMask(GL_FALSE);

        // Ana Düzlem 1 (0 derece)
        glm::mat4 modelFireBillboard1 = glm::mat4(1.0f);
        modelFireBillboard1 = glm::translate(modelFireBillboard1, glm::vec3(2.3f, -0.45f + 0.2f * flicker, 0.0f));
        modelFireBillboard1 = glm::scale(modelFireBillboard1, glm::vec3(0.4f, 0.4f * flicker, 0.005f));
        ourShader.setMat4("model", modelFireBillboard1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ana Düzlem 2 (90 derece)
        glm::mat4 modelFireBillboard2 = glm::mat4(1.0f);
        modelFireBillboard2 = glm::translate(modelFireBillboard2, glm::vec3(2.3f, -0.45f + 0.2f * flicker, 0.0f));
        modelFireBillboard2 = glm::rotate(modelFireBillboard2, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelFireBillboard2 = glm::scale(modelFireBillboard2, glm::vec3(0.4f, 0.4f * flicker, 0.005f));
        ourShader.setMat4("model", modelFireBillboard2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ana Düzlem 3 (45 derece)
        glm::mat4 modelFireBillboard3 = glm::mat4(1.0f);
        modelFireBillboard3 = glm::translate(modelFireBillboard3, glm::vec3(2.3f, -0.45f + 0.2f * flicker, 0.0f));
        modelFireBillboard3 = glm::rotate(modelFireBillboard3, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelFireBillboard3 = glm::scale(modelFireBillboard3, glm::vec3(0.4f, 0.4f * flicker, 0.005f));
        ourShader.setMat4("model", modelFireBillboard3);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Ana Düzlem 4 (135 derece)
        glm::mat4 modelFireBillboard4 = glm::mat4(1.0f);
        modelFireBillboard4 = glm::translate(modelFireBillboard4, glm::vec3(2.3f, -0.45f + 0.2f * flicker, 0.0f));
        modelFireBillboard4 = glm::rotate(modelFireBillboard4, glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelFireBillboard4 = glm::scale(modelFireBillboard4, glm::vec3(0.4f, 0.4f * flicker, 0.005f));
        ourShader.setMat4("model", modelFireBillboard4);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Hacim/Doluluk kazandırmak için hafif kaydırılmış ek alevler
        glm::mat4 modelFireBillboard5 = glm::mat4(1.0f);
        modelFireBillboard5 = glm::translate(modelFireBillboard5, glm::vec3(2.25f, -0.45f + 0.18f * flicker, 0.1f));
        modelFireBillboard5 = glm::rotate(modelFireBillboard5, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelFireBillboard5 = glm::scale(modelFireBillboard5, glm::vec3(0.3f, 0.35f * flicker, 0.005f));
        ourShader.setMat4("model", modelFireBillboard5);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 modelFireBillboard6 = glm::mat4(1.0f);
        modelFireBillboard6 = glm::translate(modelFireBillboard6, glm::vec3(2.35f, -0.45f + 0.18f * flicker, -0.1f));
        modelFireBillboard6 = glm::rotate(modelFireBillboard6, glm::radians(120.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelFireBillboard6 = glm::scale(modelFireBillboard6, glm::vec3(0.3f, 0.35f * flicker, 0.005f));
        ourShader.setMat4("model", modelFireBillboard6);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        glDepthMask(GL_TRUE); // Derinlik tamponu yazımı tekrar açılıyor
        ourShader.setBool("isFlame", false);

        // 8d. KÖŞE MASASI VE TELEVİZYON (Corner Table & TV)
        // Masayı ve TV'yi köşeye çapraz yerleştirmek (duvarlarla üçgen yapacak şekilde)
        // ve koltuğa bakacak şekilde -45 derece döndürmek için bir temel matris oluşturuyoruz.
        glm::mat4 baseTvTable = glm::mat4(1.0f);
        baseTvTable = glm::translate(baseTvTable, glm::vec3(1.95f, 0.0f, -1.45f)); // Köşe konumu (duvarlarla üçgen yapar)
        baseTvTable = glm::rotate(baseTvTable, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Koltuğa dönmesi için -45 derece dönüş

        // Masa Tablası (Mavi renkli modern masa üstü - Doku kullanılmıyor)
        glBindVertexArray(cubeVAO);
        ourShader.setBool("useTexture", false);
        ourShader.setVec3("objectColor", 0.12f, 0.38f, 0.72f); // Vibrant steel blue (kullanıcı isteğiyle mavi)
        ourShader.setFloat("specularStrength", 0.3f); // Hafif parlak cilalı cila görünümü
        ourShader.setFloat("shininess", 64.0f);

        glm::mat4 modelTableTop = baseTvTable;
        modelTableTop = glm::translate(modelTableTop, glm::vec3(0.0f, -0.1f, 0.0f));
        modelTableTop = glm::scale(modelTableTop, glm::vec3(1.0f, 0.04f, 0.55f)); // Genişletilmiş masa (genişlik = 1.0, derinlik = 0.55)
        ourShader.setMat4("model", modelTableTop);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Masa Ayakları (4 adet - Koyu Mavi)
        ourShader.setVec3("objectColor", 0.08f, 0.26f, 0.52f); // Bacaklar biraz daha koyu mavi
        float legX[2] = { -0.45f, 0.45f }; // Masa genişliği 1.0 olduğu için offset 0.45
        float legZ[2] = { -0.23f, 0.23f }; // Masa derinliği 0.55 olduğu için offset 0.23
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                glm::mat4 modelLeg = baseTvTable;
                modelLeg = glm::translate(modelLeg, glm::vec3(legX[i], -0.3f, legZ[j]));
                modelLeg = glm::scale(modelLeg, glm::vec3(0.04f, 0.36f, 0.04f));
                ourShader.setMat4("model", modelLeg);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // BÜYÜTÜLMÜŞ TELEVİZYON (Koltuğa Dönük)
        // Televizyon Altlığı (Siyah Metal/Plastik)
        ourShader.setBool("useTexture", false);
        ourShader.setVec3("objectColor", 0.12f, 0.12f, 0.12f);
        ourShader.setFloat("specularStrength", 0.5f);
        ourShader.setFloat("shininess", 32.0f);

        glm::mat4 modelTvBase = baseTvTable;
        modelTvBase = glm::translate(modelTvBase, glm::vec3(0.0f, -0.07f, 0.0f)); // Masa üstü y=-0.08 hizasında durur
        modelTvBase = glm::scale(modelTvBase, glm::vec3(0.4f, 0.02f, 0.25f)); // Büyütüldü (0.3 -> 0.4)
        ourShader.setMat4("model", modelTvBase);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Televizyon Ayağı (Stand Neck)
        glm::mat4 modelTvNeck = baseTvTable;
        modelTvNeck = glm::translate(modelTvNeck, glm::vec3(0.0f, 0.0f, 0.0f));
        modelTvNeck = glm::scale(modelTvNeck, glm::vec3(0.05f, 0.12f, 0.05f)); // Büyütüldü (0.1 -> 0.12)
        ourShader.setMat4("model", modelTvNeck);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Televizyon Çerçevesi (TV Bezel/Body)
        glm::mat4 modelTvFrame = baseTvTable;
        modelTvFrame = glm::translate(modelTvFrame, glm::vec3(0.0f, 0.27f, 0.0f)); // Neck yüksekliği ile orantılı
        modelTvFrame = glm::scale(modelTvFrame, glm::vec3(0.92f, 0.42f, 0.05f)); // Büyütüldü (0.7 -> 0.92, 0.28 -> 0.42)
        ourShader.setMat4("model", modelTvFrame);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Televizyon Ekranı (Kedi Resmi / catTexture - Işıksız Canlı Mod)
        glBindTexture(GL_TEXTURE_2D, catTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setBool("isScreen", true); // Shader'da aydınlatmayı kapatıp ekranı canlı parlatmak için
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("specularStrength", 0.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));

        glm::mat4 modelTvScreen = baseTvTable;
        modelTvScreen = glm::translate(modelTvScreen, glm::vec3(0.0f, 0.27f, 0.027f)); // TV çerçevesinin biraz önünde
        modelTvScreen = glm::scale(modelTvScreen, glm::vec3(0.88f, 0.38f, 0.005f)); // Büyütüldü (0.66 -> 0.88, 0.24 -> 0.38)
        ourShader.setMat4("model", modelTvScreen);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        ourShader.setBool("isScreen", false); // Diğer çizimler için ekran modunu kapatıyoruz

        // 9. BACA VE BACA DUMANI ANIMASYONU
        // ------------------------------------------------------------------
        // Baca (Tuğla kaplamalı sütun, çatı sağ tarafında)
        glBindTexture(GL_TEXTURE_2D, brickTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("specularStrength", 0.1f);
        ourShader.setFloat("shininess", 16.0f);
        glm::mat4 modelChimney = glm::mat4(1.0f);
        modelChimney = glm::translate(modelChimney, glm::vec3(2.0f, 1.8f, 0.0f));
        modelChimney = glm::scale(modelChimney, glm::vec3(0.3f, 1.0f, 0.3f));
        ourShader.setMat4("model", modelChimney);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Bacadan Çıkan Saydam Duman Parçacıkları (3 adet küp döngüsel olarak yükselir)
        for (int i = 0; i < 3; ++i) {
            float t = fmod(currentFrame + i * 1.0f, 3.0f);
            float smokeY = 2.3f + t * 0.6f; 
            float smokeX = 2.0f + t * 0.1f + 0.05f * sin(currentFrame * 3.0f + i); 
            float smokeZ = 0.0f + 0.05f * cos(currentFrame * 3.0f + i);
            float smokeScale = 0.05f + t * 0.06f; 
            float smokeAlpha = 0.5f * (1.0f - t / 3.0f); 

            glBindVertexArray(cubeVAO);
            ourShader.setBool("useTexture", false);
            ourShader.setVec3("objectColor", 0.5f, 0.5f, 0.5f);
            ourShader.setFloat("specularStrength", 0.0f);
            ourShader.setFloat("shininess", 8.0f);
            ourShader.setFloat("objectAlpha", smokeAlpha); 
            
            glm::mat4 modelSmoke = glm::mat4(1.0f);
            modelSmoke = glm::translate(modelSmoke, glm::vec3(smokeX, smokeY, smokeZ));
            modelSmoke = glm::scale(modelSmoke, glm::vec3(smokeScale, smokeScale, smokeScale));
            ourShader.setMat4("model", modelSmoke);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        ourShader.setFloat("objectAlpha", 1.0f); 

        // 10. Çitler (Evin etrafını çevreleyen kapılı ahşap çit sistemi - Genişletilmiş yard: -6.0 ile 6.0 arası)
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);
        ourShader.setFloat("specularStrength", 0.1f);
        ourShader.setFloat("shininess", 16.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));

        float fenceMin = -6.0f;
        float fenceMax = 6.0f;
        float step = 0.8f;
        
        // Çit direklerinin üstüne sokak lambası çizimi
        auto drawFenceLamp = [&](glm::vec3 postPos) {
            // 1. Metal Lamba Sapı (Neck/Mount)
            ourShader.setBool("useTexture", false);
            ourShader.setVec3("objectColor", 0.15f, 0.15f, 0.15f); // Koyu metalik sap
            ourShader.setFloat("specularStrength", 0.5f);
            ourShader.setFloat("shininess", 32.0f);
            
            glm::mat4 modelNeck = glm::mat4(1.0f);
            modelNeck = glm::translate(modelNeck, postPos + glm::vec3(0.0f, 0.32f, 0.0f));
            modelNeck = glm::scale(modelNeck, glm::vec3(0.02f, 0.04f, 0.02f));
            ourShader.setMat4("model", modelNeck);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // 2. Yarı Parlak Ampul / Fener (G tuşu ile kontrol edilir)
            if (lampOn) {
                ourShader.setVec3("objectColor", 1.0f, 0.82f, 0.45f); // Sıcak sokak lambası sarısı
                ourShader.setBool("isFlame", true); // Emissive
            } else {
                ourShader.setVec3("objectColor", 0.25f, 0.25f, 0.2f); // Sönük lamba
                ourShader.setBool("isFlame", false);
            }
            
            glm::mat4 modelBulb = glm::mat4(1.0f);
            modelBulb = glm::translate(modelBulb, postPos + glm::vec3(0.0f, 0.36f, 0.0f));
            modelBulb = glm::scale(modelBulb, glm::vec3(0.04f, 0.04f, 0.04f));
            ourShader.setMat4("model", modelBulb);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            ourShader.setBool("isFlame", false);
        };
        
        // Ön ve Arka çitler (X boyunca dizilim)
        for (float x = fenceMin; x < fenceMax + 0.01f; x += step) {
            bool isGate = (x >= 0.79f && x <= 1.21f); // Ön tarafta kapı hizasında (x = 1.0) geçit boşluğu

            // Arka Çit Direkleri
            glm::mat4 modelPost = glm::mat4(1.0f);
            modelPost = glm::translate(modelPost, glm::vec3(x, -0.2f, fenceMin));
            glm::mat4 modelPostScaled = glm::scale(modelPost, glm::vec3(0.06f, 0.6f, 0.06f));
            ourShader.setMat4("model", modelPostScaled);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Çit direği üstündeki lambayı çiz
            drawFenceLamp(glm::vec3(x, -0.2f, fenceMin));

            // Çit kaplamasını geri yükle
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            ourShader.setBool("useTexture", true);
            ourShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);

            // Arka Yatay Kirişler
            if (x < fenceMax - 0.01f) {
                for (float ry : {-0.3f, -0.1f}) {
                    glm::mat4 modelRail = glm::mat4(1.0f);
                    modelRail = glm::translate(modelRail, glm::vec3(x + step/2.0f, ry, fenceMin));
                    modelRail = glm::scale(modelRail, glm::vec3(step, 0.03f, 0.03f));
                    ourShader.setMat4("model", modelRail);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            // Ön Çit Direkleri (Kapı hariç)
            if (!isGate) {
                glm::mat4 modelPost = glm::mat4(1.0f);
                modelPost = glm::translate(modelPost, glm::vec3(x, -0.2f, fenceMax));
                glm::mat4 modelPostScaled = glm::scale(modelPost, glm::vec3(0.06f, 0.6f, 0.06f));
                ourShader.setMat4("model", modelPostScaled);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // Çit lambasını çiz
                drawFenceLamp(glm::vec3(x, -0.2f, fenceMax));

                // Çit kaplamasını geri yükle
                glBindTexture(GL_TEXTURE_2D, woodTexture);
                ourShader.setBool("useTexture", true);
                ourShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);

                // Ön Çit Yatay Kirişler
                if (x < fenceMax - 0.01f) {
                    float nextX = x + step;
                    bool nextIsGate = (nextX >= 0.79f && nextX <= 1.21f);
                    if (!nextIsGate) {
                        for (float ry : {-0.3f, -0.1f}) {
                            glm::mat4 modelRail = glm::mat4(1.0f);
                            modelRail = glm::translate(modelRail, glm::vec3(x + step/2.0f, ry, fenceMax));
                            modelRail = glm::scale(modelRail, glm::vec3(step, 0.03f, 0.03f));
                            ourShader.setMat4("model", modelRail);
                            glDrawArrays(GL_TRIANGLES, 0, 36);
                        }
                    }
                }
            }
        }

        // Sol ve Sağ çitler (Z boyunca dizilim)
        for (float z = fenceMin; z < fenceMax + 0.01f; z += step) {
            // Sol Çit Direkleri
            glm::mat4 modelPost = glm::mat4(1.0f);
            modelPost = glm::translate(modelPost, glm::vec3(fenceMin, -0.2f, z));
            glm::mat4 modelPostScaled = glm::scale(modelPost, glm::vec3(0.06f, 0.6f, 0.06f));
            ourShader.setMat4("model", modelPostScaled);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Çit lambasını çiz
            drawFenceLamp(glm::vec3(fenceMin, -0.2f, z));

            // Çit kaplamasını geri yükle
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            ourShader.setBool("useTexture", true);
            ourShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);

            // Sol Yatay Kirişler
            if (z < fenceMax - 0.01f) {
                for (float ry : {-0.3f, -0.1f}) {
                    glm::mat4 modelRail = glm::mat4(1.0f);
                    modelRail = glm::translate(modelRail, glm::vec3(fenceMin, ry, z + step/2.0f));
                    modelRail = glm::scale(modelRail, glm::vec3(0.03f, 0.03f, step));
                    ourShader.setMat4("model", modelRail);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            // Sağ Çit Direkleri
            glm::mat4 modelPostR = glm::mat4(1.0f);
            modelPostR = glm::translate(modelPostR, glm::vec3(fenceMax, -0.2f, z));
            glm::mat4 modelPostR_scaled = glm::scale(modelPostR, glm::vec3(0.06f, 0.6f, 0.06f));
            ourShader.setMat4("model", modelPostR_scaled);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Çit lambasını çiz
            drawFenceLamp(glm::vec3(fenceMax, -0.2f, z));

            // Çit kaplamasını geri yükle
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            ourShader.setBool("useTexture", true);
            ourShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);

            // Sağ Yatay Kirişler
            if (z < fenceMax - 0.01f) {
                for (float ry : {-0.3f, -0.1f}) {
                    glm::mat4 modelRail = glm::mat4(1.0f);
                    modelRail = glm::translate(modelRail, glm::vec3(fenceMax, ry, z + step/2.0f));
                    modelRail = glm::scale(modelRail, glm::vec3(0.03f, 0.03f, step));
                    ourShader.setMat4("model", modelRail);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // 11. Ağaçlar (Döngü ile çizim)
        for (const auto& tree : trees) {
            // Gövde (Wood texture)
            glBindVertexArray(trunkVAO);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            ourShader.setBool("useTexture", true);
            ourShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("specularStrength", 0.05f);
            ourShader.setFloat("shininess", 8.0f);
            ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
            glm::mat4 modelTrunk = glm::mat4(1.0f);
            modelTrunk = glm::translate(modelTrunk, tree.position);
            modelTrunk = glm::scale(modelTrunk, glm::vec3(tree.trunkRadius / 0.1f, tree.trunkHeight, tree.trunkRadius / 0.1f));
            ourShader.setMat4("model", modelTrunk);
            glDrawArrays(GL_TRIANGLES, 0, trunkData.size());
            
            // Yapraklar (Çim dokusunu yeşil maskeleyerek çizim)
            glBindVertexArray(leavesVAO);
            glBindTexture(GL_TEXTURE_2D, grassTexture);
            ourShader.setBool("useTexture", true);
            ourShader.setVec3("objectColor", 0.15f, 0.55f, 0.15f);
            ourShader.setFloat("specularStrength", 0.05f);
            ourShader.setFloat("shininess", 8.0f);
            ourShader.setVec2("uvScale", glm::vec2(2.0f, 2.0f));
            glm::mat4 modelLeaves = glm::mat4(1.0f);
            modelLeaves = glm::translate(modelLeaves, glm::vec3(tree.position.x, tree.position.y + tree.trunkHeight, tree.position.z));
            modelLeaves = glm::scale(modelLeaves, glm::vec3(tree.leavesRadius / 0.5f, tree.leavesHeight, tree.leavesRadius / 0.5f));
            ourShader.setMat4("model", modelLeaves);
            glDrawArrays(GL_TRIANGLES, 0, leavesData.size());
        }

        // 12. TAŞ YOL (Stepping Stone Path)
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, brickTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.55f, 0.55f, 0.55f);
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 16.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));

        for (int i = 0; i < 6; ++i) {
            float z = 2.2f + i * 0.7f;
            glm::mat4 modelStone = glm::mat4(1.0f);
            modelStone = glm::translate(modelStone, glm::vec3(1.0f, -0.49f, z));
            modelStone = glm::scale(modelStone, glm::vec3(0.7f, 0.01f, 0.4f));
            ourShader.setMat4("model", modelStone);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 13. KUYU (Well - Evin sağ tarafında x = 4.0f, z = 0.0f)
        // A) Kuyu Silindir Taş Duvarı
        glBindVertexArray(trunkVAO);
        glBindTexture(GL_TEXTURE_2D, brickTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.6f, 0.6f, 0.6f); // Gri taş tonu
        ourShader.setFloat("specularStrength", 0.1f);
        ourShader.setFloat("shininess", 16.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(4.0f, 1.0f));

        glm::mat4 modelWellWall = glm::mat4(1.0f);
        modelWellWall = glm::translate(modelWellWall, glm::vec3(4.0f, -0.5f, 0.0f));
        modelWellWall = glm::scale(modelWellWall, glm::vec3(5.0f, 0.7f, 5.0f)); // radius = 0.5, height = 0.7
        ourShader.setMat4("model", modelWellWall);
        glDrawArrays(GL_TRIANGLES, 0, trunkData.size());

        // B) Kuyu İçindeki Su Yüzeyi (Textured animated circle disk)
        glBindVertexArray(waterVAO);
        glBindTexture(GL_TEXTURE_2D, waterTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setBool("isWater", true);
        ourShader.setVec3("objectColor", 0.7f, 0.9f, 1.0f); // Turkuaz/Mavi su tonu
        ourShader.setFloat("specularStrength", 1.5f);        // Yüksek yansıma
        ourShader.setFloat("shininess", 256.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.5f, 1.5f));
        ourShader.setVec2("uvOffset", glm::vec2(currentFrame * 0.04f, currentFrame * 0.02f)); // Dalga hareketi

        glm::mat4 modelWater = glm::mat4(1.0f);
        modelWater = glm::translate(modelWater, glm::vec3(4.0f, -0.3f, 0.0f)); // su seviyesi y = -0.3
        ourShader.setMat4("model", modelWater);
        glDrawArrays(GL_TRIANGLES, 0, waterData.size());

        // Diğer çizimler için uvOffset ve isWater değerlerini sıfırlayalım
        ourShader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));
        ourShader.setBool("isWater", false);

        // C) Kuyu Ahşap Direkleri
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.5f, 0.35f, 0.2f);
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setVec2("uvScale", glm::vec2(0.5f, 2.0f));

        // Direk 1
        glm::mat4 modelWellPost1 = glm::mat4(1.0f);
        modelWellPost1 = glm::translate(modelWellPost1, glm::vec3(4.0f, 0.1f, -0.45f));
        modelWellPost1 = glm::scale(modelWellPost1, glm::vec3(0.05f, 1.2f, 0.05f));
        ourShader.setMat4("model", modelWellPost1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Direk 2
        glm::mat4 modelWellPost2 = glm::mat4(1.0f);
        modelWellPost2 = glm::translate(modelWellPost2, glm::vec3(4.0f, 0.1f, 0.45f));
        modelWellPost2 = glm::scale(modelWellPost2, glm::vec3(0.05f, 1.2f, 0.05f));
        ourShader.setMat4("model", modelWellPost2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // D) Kuyu Çatısı
        glBindVertexArray(roofVAO);
        glBindTexture(GL_TEXTURE_2D, roofTexture);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        ourShader.setVec3("objectColor", 0.9f, 0.9f, 0.9f);

        glm::mat4 modelWellRoof = glm::mat4(1.0f);
        modelWellRoof = glm::translate(modelWellRoof, glm::vec3(4.0f, 0.7f, 0.0f));
        modelWellRoof = glm::rotate(modelWellRoof, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelWellRoof = glm::scale(modelWellRoof, glm::vec3(1.1f / 5.4f, 0.3f / 1.2f, 0.7f / 4.4f));
        ourShader.setMat4("model", modelWellRoof);
        glDrawArrays(GL_TRIANGLES, 0, roofData.size());

        // 14. GAZEO (Çardak)
        // A) Gazebo Tabanı
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", 0.55f, 0.4f, 0.25f);
        ourShader.setFloat("specularStrength", 0.05f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);
        ourShader.setVec2("uvScale", glm::vec2(1.5f, 1.5f));

        glm::mat4 modelGazBase = glm::mat4(1.0f);
        modelGazBase = glm::translate(modelGazBase, glm::vec3(-4.0f, -0.48f, 0.0f));
        modelGazBase = glm::scale(modelGazBase, glm::vec3(1.6f, 0.04f, 1.6f));
        ourShader.setMat4("model", modelGazBase);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // B) Gazebo Direkleri (Corner Posts)
        ourShader.setVec2("uvScale", glm::vec2(0.5f, 2.0f));
        for (float px : {-4.75f, -3.25f}) {
            for (float pz : {-0.75f, 0.75f}) {
                glm::mat4 modelGazPost = glm::mat4(1.0f);
                modelGazPost = glm::translate(modelGazPost, glm::vec3(px, 0.1f, pz));
                modelGazPost = glm::scale(modelGazPost, glm::vec3(0.08f, 1.2f, 0.08f));
                ourShader.setMat4("model", modelGazPost);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // C) Gazebo Çatısı (Roof) - Ölçekleme Düzeltildi!
        glBindVertexArray(roofVAO);
        glBindTexture(GL_TEXTURE_2D, roofTexture);
        ourShader.setVec2("uvScale", glm::vec2(2.0f, 2.0f));
        ourShader.setVec3("objectColor", 0.9f, 0.9f, 0.9f);
        glm::mat4 modelGazRoof = glm::mat4(1.0f);
        modelGazRoof = glm::translate(modelGazRoof, glm::vec3(-4.0f, 0.7f, 0.0f));
        modelGazRoof = glm::scale(modelGazRoof, glm::vec3(1.8f / 5.4f, 0.5f / 1.2f, 1.8f / 4.4f)); // Genişlik=1.8, Yükseklik=0.5, Derinlik=1.8
        ourShader.setMat4("model", modelGazRoof);
        glDrawArrays(GL_TRIANGLES, 0, roofData.size());

        // D) Gazebo İç Bankları (Benches)
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        ourShader.setVec3("objectColor", 0.5f, 0.35f, 0.2f);
        ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));
        // Sol Bank
        glm::mat4 modelBenchL = glm::mat4(1.0f);
        modelBenchL = glm::translate(modelBenchL, glm::vec3(-4.5f, -0.3f, 0.0f));
        modelBenchL = glm::scale(modelBenchL, glm::vec3(0.3f, 0.2f, 1.2f));
        ourShader.setMat4("model", modelBenchL);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // Sağ Bank
        glm::mat4 modelBenchR = glm::mat4(1.0f);
        modelBenchR = glm::translate(modelBenchR, glm::vec3(-3.5f, -0.3f, 0.0f));
        modelBenchR = glm::scale(modelBenchR, glm::vec3(0.3f, 0.2f, 1.2f));
        ourShader.setMat4("model", modelBenchR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // E) Gazebo İç Masası (Table)
        // Masa Ayağı
        glm::mat4 modelTabLeg = glm::mat4(1.0f);
        modelTabLeg = glm::translate(modelTabLeg, glm::vec3(-4.0f, -0.3f, 0.0f));
        modelTabLeg = glm::scale(modelTabLeg, glm::vec3(0.08f, 0.38f, 0.08f));
        ourShader.setMat4("model", modelTabLeg);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // Masa Üstü
        glm::mat4 modelTabTop = glm::mat4(1.0f);
        modelTabTop = glm::translate(modelTabTop, glm::vec3(-4.0f, -0.1f, 0.0f));
        modelTabTop = glm::scale(modelTabTop, glm::vec3(0.5f, 0.03f, 0.5f));
        ourShader.setMat4("model", modelTabTop);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 15. ARABA (Low-Poly 3D Car parked outside the yard)
        // Araba taban koordinatı: x = -2.8f, y = -0.5f (zemin), z = 7.5f (sol çitlerin önü, girişin yanı)
        glm::mat4 modelCar = glm::mat4(1.0f);
        modelCar = glm::translate(modelCar, glm::vec3(-2.8f, -0.5f, 7.5f));
        modelCar = glm::rotate(modelCar, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Parked angled next to path

        // Tekerlek çizim fonksiyonu (Silindir lastik ve gümüş jant kapakları)
        auto drawCarWheel = [&](glm::vec3 offset, bool isLeft) {
            glm::mat4 modelWheel = modelCar;
            modelWheel = glm::translate(modelWheel, offset);

            // A) Lastik Sırtı (Cylinder)
            glBindVertexArray(trunkVAO);
            ourShader.setBool("useTexture", false);
            ourShader.setVec3("objectColor", 0.08f, 0.08f, 0.08f); // Mat Siyah Lastik
            ourShader.setFloat("specularStrength", 0.1f);
            ourShader.setFloat("shininess", 8.0f);
            ourShader.setFloat("objectAlpha", 1.0f);
            
            glm::mat4 modelTire = modelWheel;
            modelTire = glm::rotate(modelTire, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Align along X axis
            modelTire = glm::scale(modelTire, glm::vec3(2.4f, 0.15f, 2.4f)); // width = 0.15, radius = 0.24 (BUG FIXED: circular scale)
            modelTire = glm::translate(modelTire, glm::vec3(0.0f, -0.5f, 0.0f)); // Center cylinder height along Y before rotation
            ourShader.setMat4("model", modelTire);
            glDrawArrays(GL_TRIANGLES, 0, trunkData.size());

            // B) Lastik Yan Yanakları (Flat Circle Faces on both sides)
            glBindVertexArray(waterVAO);
            ourShader.setVec3("objectColor", 0.08f, 0.08f, 0.08f);
            for (float sideSign : {-1.0f, 1.0f}) {
                glm::mat4 modelSide = modelWheel;
                modelSide = glm::translate(modelSide, glm::vec3(sideSign * 0.075f, 0.0f, 0.0f));
                modelSide = glm::rotate(modelSide, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                modelSide = glm::scale(modelSide, glm::vec3(0.24f / 0.49f, 1.0f, 0.24f / 0.49f));
                ourShader.setMat4("model", modelSide);
                glDrawArrays(GL_TRIANGLES, 0, waterData.size());
            }

            // C) Gümüş Jant Kapakları (Rim Hubcaps on the outer side)
            ourShader.setVec3("objectColor", 0.75f, 0.75f, 0.75f); // Silver/Grey
            ourShader.setFloat("specularStrength", 0.8f);
            ourShader.setFloat("shininess", 32.0f);
            float rimOffset = isLeft ? -0.077f : 0.077f;
            glm::mat4 modelRim = modelWheel;
            modelRim = glm::translate(modelRim, glm::vec3(rimOffset, 0.0f, 0.0f));
            modelRim = glm::rotate(modelRim, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            modelRim = glm::scale(modelRim, glm::vec3(0.15f / 0.49f, 1.0f, 0.15f / 0.49f));
            ourShader.setMat4("model", modelRim);
            glDrawArrays(GL_TRIANGLES, 0, waterData.size());
            
            // Jant merkezindeki siyah nokta (Center cap)
            ourShader.setVec3("objectColor", 0.1f, 0.1f, 0.1f);
            glm::mat4 modelCenter = modelWheel;
            modelCenter = glm::translate(modelCenter, glm::vec3(rimOffset * 1.01f, 0.0f, 0.0f));
            modelCenter = glm::rotate(modelCenter, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            modelCenter = glm::scale(modelCenter, glm::vec3(0.04f / 0.49f, 1.0f, 0.04f / 0.49f));
            ourShader.setMat4("model", modelCenter);
            glDrawArrays(GL_TRIANGLES, 0, waterData.size());
        };

        // 4 Tekerleğin Çizilmesi
        drawCarWheel(glm::vec3(-0.55f, 0.24f, 0.65f), true);   // Ön Sol
        drawCarWheel(glm::vec3(0.55f, 0.24f, 0.65f), false);   // Ön Sağ
        drawCarWheel(glm::vec3(-0.55f, 0.24f, -0.65f), true);  // Arka Sol
        drawCarWheel(glm::vec3(0.55f, 0.24f, -0.65f), false);  // Arka Sağ

        // 3D Araba Gövdesi ve Detaylar
        glBindVertexArray(cubeVAO);
        ourShader.setBool("useTexture", false);
        ourShader.setFloat("specularStrength", 0.5f);
        ourShader.setFloat("shininess", 32.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        // 1. İÇİ BOŞ ALT ŞASİ / GÖVDE (Hollow Chassis Bucket - Sport Blue)
        ourShader.setVec3("objectColor", 0.12f, 0.45f, 0.85f);
        
        // A) Şasi Tabanı (Floor)
        glm::mat4 modelChassisFloor = modelCar;
        modelChassisFloor = glm::translate(modelChassisFloor, glm::vec3(0.0f, 0.23f, 0.0f));
        modelChassisFloor = glm::scale(modelChassisFloor, glm::vec3(0.96f, 0.02f, 1.96f));
        ourShader.setMat4("model", modelChassisFloor);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // B) Sol Yan Duvar (Left Wall)
        glm::mat4 modelChassisL = modelCar;
        modelChassisL = glm::translate(modelChassisL, glm::vec3(-0.48f, 0.38f, 0.0f));
        modelChassisL = glm::scale(modelChassisL, glm::vec3(0.04f, 0.32f, 1.96f));
        ourShader.setMat4("model", modelChassisL);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // C) Sağ Yan Duvar (Right Wall)
        glm::mat4 modelChassisR = modelCar;
        modelChassisR = glm::translate(modelChassisR, glm::vec3(0.48f, 0.38f, 0.0f));
        modelChassisR = glm::scale(modelChassisR, glm::vec3(0.04f, 0.32f, 1.96f));
        ourShader.setMat4("model", modelChassisR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // D) Ön Duvar / Motor Kapağı Paneli (Front Wall)
        glm::mat4 modelChassisF = modelCar;
        modelChassisF = glm::translate(modelChassisF, glm::vec3(0.0f, 0.38f, 0.98f));
        modelChassisF = glm::scale(modelChassisF, glm::vec3(1.0f, 0.32f, 0.04f));
        ourShader.setMat4("model", modelChassisF);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // E) Arka Duvar / Bagaj Paneli (Rear Wall)
        glm::mat4 modelChassisB = modelCar;
        modelChassisB = glm::translate(modelChassisB, glm::vec3(0.0f, 0.38f, -0.98f));
        modelChassisB = glm::scale(modelChassisB, glm::vec3(1.0f, 0.32f, 0.04f));
        ourShader.setMat4("model", modelChassisB);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // F) Ön Motor Kaputu (Front Hood Cover - Sport Blue - Kapatıcı Panel)
        glm::mat4 modelHoodCover = modelCar;
        modelHoodCover = glm::translate(modelHoodCover, glm::vec3(0.0f, 0.53f, 0.80f));
        modelHoodCover = glm::scale(modelHoodCover, glm::vec3(0.96f, 0.02f, 0.40f));
        ourShader.setMat4("model", modelHoodCover);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // G) Arka Bagaj Kapağı (Rear Trunk Cover - Sport Blue - Kapatıcı Panel)
        glm::mat4 modelTrunkCover = modelCar;
        modelTrunkCover = glm::translate(modelTrunkCover, glm::vec3(0.0f, 0.53f, -0.90f));
        modelTrunkCover = glm::scale(modelTrunkCover, glm::vec3(0.96f, 0.02f, 0.20f));
        ourShader.setMat4("model", modelTrunkCover);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // 2. Üst Tavan & 65 Derece Eğimli Direkler (Cabin Frame - Sport Blue)
        // A) Tavan Plakası (Daha Küçük Tavan Plakası)
        glm::mat4 modelCarRoof = modelCar;
        modelCarRoof = glm::translate(modelCarRoof, glm::vec3(0.0f, 0.88f, -0.1f));
        modelCarRoof = glm::scale(modelCarRoof, glm::vec3(0.80f, 0.04f, 1.1f));
        ourShader.setMat4("model", modelCarRoof);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // B) Ön A-Direkleri (Yatayla 65 Derece / Dikeyden 25 Derece Geriye Eğimli)
        float pillarAngleX = 25.0f; // 90° - 65° = 25°
        float pillarHeightL = 0.353f; // L = H / sin(65°)
        
        for (float sideX : {-0.40f, 0.40f}) {
            // Ön Direkler (A-pillars) - Geriye doğru yatık (-25°)
            glm::mat4 modelPillarF = modelCar;
            modelPillarF = glm::translate(modelPillarF, glm::vec3(sideX, 0.70f, 0.525f));
            modelPillarF = glm::rotate(modelPillarF, glm::radians(-pillarAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
            modelPillarF = glm::scale(modelPillarF, glm::vec3(0.03f, pillarHeightL, 0.03f));
            ourShader.setMat4("model", modelPillarF);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Arka Direkler (C-pillars) - Öne doğru yatık (+25°)
            glm::mat4 modelPillarR = modelCar;
            modelPillarR = glm::translate(modelPillarR, glm::vec3(sideX, 0.70f, -0.725f));
            modelPillarR = glm::rotate(modelPillarR, glm::radians(pillarAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
            modelPillarR = glm::scale(modelPillarR, glm::vec3(0.03f, pillarHeightL, 0.03f));
            ourShader.setMat4("model", modelPillarR);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Orta Direkler (B-pillars) - Dikey (Vertical)
            glm::mat4 modelPillarM = modelCar;
            modelPillarM = glm::translate(modelPillarM, glm::vec3(sideX, 0.70f, -0.1f));
            modelPillarM = glm::scale(modelPillarM, glm::vec3(0.03f, 0.32f, 0.03f));
            ourShader.setMat4("model", modelPillarM);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        // 2b. İç Mekandaki Büyütülmüş Koltuklar ve Direksiyon
        // A) 4 Adet Büyütülmüş Spor Koltuk (Sport Seats - Matte Dark Grey)
        ourShader.setVec3("objectColor", 0.18f, 0.18f, 0.18f);
        ourShader.setFloat("specularStrength", 0.2f);
        ourShader.setFloat("shininess", 16.0f);

        struct SeatConfig {
            glm::vec3 cushionPos;
            glm::vec3 backrestPos;
        } seats[] = {
            { glm::vec3(-0.22f, 0.30f, 0.25f), glm::vec3(-0.22f, 0.49f, 0.10f) },   // Sürücü Koltuğu (Ön Sol)
            { glm::vec3(0.22f, 0.30f, 0.25f), glm::vec3(0.22f, 0.49f, 0.10f) },    // Yolcu Koltuğu (Ön Sağ)
            { glm::vec3(-0.22f, 0.30f, -0.35f), glm::vec3(-0.22f, 0.49f, -0.50f) }, // Arka Sol Yolcu
            { glm::vec3(0.22f, 0.30f, -0.35f), glm::vec3(0.22f, 0.49f, -0.50f) }   // Arka Sağ Yolcu
        };

        for (const auto& seat : seats) {
            // Minder (Enlarged Cushion)
            glm::mat4 modelCushion = modelCar;
            modelCushion = glm::translate(modelCushion, seat.cushionPos);
            modelCushion = glm::scale(modelCushion, glm::vec3(0.36f, 0.12f, 0.36f));
            ourShader.setMat4("model", modelCushion);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Arkalık (Enlarged Backrest)
            glm::mat4 modelBack = modelCar;
            modelBack = glm::translate(modelBack, seat.backrestPos);
            modelBack = glm::scale(modelBack, glm::vec3(0.36f, 0.26f, 0.08f));
            ourShader.setMat4("model", modelBack);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // B) Direksiyon Sistemi (Steering System)
        // Direksiyon Mili (Column - Silver)
        ourShader.setVec3("objectColor", 0.4f, 0.4f, 0.4f);
        ourShader.setFloat("specularStrength", 0.6f);
        glm::mat4 modelCol = modelCar;
        modelCol = glm::translate(modelCol, glm::vec3(-0.22f, 0.54f, 0.45f));
        modelCol = glm::rotate(modelCol, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Eğimli mil
        modelCol = glm::scale(modelCol, glm::vec3(0.015f, 0.015f, 0.14f));
        ourShader.setMat4("model", modelCol);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Direksiyon Simidi (Simit - Matte Black Circle)
        glBindVertexArray(waterVAO);
        ourShader.setVec3("objectColor", 0.08f, 0.08f, 0.08f);
        ourShader.setFloat("specularStrength", 0.1f);
        glm::mat4 modelWheelSimit = modelCar;
        modelWheelSimit = glm::translate(modelWheelSimit, glm::vec3(-0.22f, 0.60f, 0.38f));
        modelWheelSimit = glm::rotate(modelWheelSimit, glm::radians(120.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Mil açısına göre hizalı simit
        modelWheelSimit = glm::scale(modelWheelSimit, glm::vec3(0.14f / 0.49f, 1.0f, 0.14f / 0.49f));
        ourShader.setMat4("model", modelWheelSimit);
        glDrawArrays(GL_TRIANGLES, 0, waterData.size());

        // Bind cubeVAO back for other opaque details
        glBindVertexArray(cubeVAO);


        // 3. Ön ve Arka Tamponlar (Bumpers - Matte Black)
        ourShader.setVec3("objectColor", 0.08f, 0.08f, 0.08f);
        // Ön Tampon
        glm::mat4 modelBumperF = modelCar;
        modelBumperF = glm::translate(modelBumperF, glm::vec3(0.0f, 0.26f, 1.01f));
        modelBumperF = glm::scale(modelBumperF, glm::vec3(0.96f, 0.08f, 0.06f));
        ourShader.setMat4("model", modelBumperF);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // Arka Tampon
        glm::mat4 modelBumperR = modelCar;
        modelBumperR = glm::translate(modelBumperR, glm::vec3(0.0f, 0.26f, -1.01f));
        modelBumperR = glm::scale(modelBumperR, glm::vec3(0.96f, 0.08f, 0.06f));
        ourShader.setMat4("model", modelBumperR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 5. Farlar ve Stop Lambaları (Opaque)
        // Ön Farlar (Bright Yellow)
        ourShader.setVec3("objectColor", 1.0f, 1.0f, 0.7f);
        ourShader.setFloat("specularStrength", 0.0f);
        // Sol Far
        glm::mat4 modelLightFL = modelCar;
        modelLightFL = glm::translate(modelLightFL, glm::vec3(-0.36f, 0.38f, 1.005f));
        modelLightFL = glm::scale(modelLightFL, glm::vec3(0.16f, 0.08f, 0.02f));
        ourShader.setMat4("model", modelLightFL);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // Sağ Far
        glm::mat4 modelLightFR = modelCar;
        modelLightFR = glm::translate(modelLightFR, glm::vec3(0.36f, 0.38f, 1.005f));
        modelLightFR = glm::scale(modelLightFR, glm::vec3(0.16f, 0.08f, 0.02f));
        ourShader.setMat4("model", modelLightFR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Arka Stop Lambaları (Sport Red)
        ourShader.setVec3("objectColor", 0.85f, 0.05f, 0.05f);
        // Sol Stop
        glm::mat4 modelLightRL = modelCar;
        modelLightRL = glm::translate(modelLightRL, glm::vec3(-0.36f, 0.38f, -1.005f));
        modelLightRL = glm::scale(modelLightRL, glm::vec3(0.16f, 0.08f, 0.02f));
        ourShader.setMat4("model", modelLightRL);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // Sağ Stop
        glm::mat4 modelLightRR = modelCar;
        modelLightRR = glm::translate(modelLightRR, glm::vec3(0.36f, 0.38f, -1.005f));
        modelLightRR = glm::scale(modelLightRR, glm::vec3(0.16f, 0.08f, 0.02f));
        ourShader.setMat4("model", modelLightRR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 6. Ön Izgara / Panjur (Grille - Matte Black)
        ourShader.setVec3("objectColor", 0.08f, 0.08f, 0.08f);
        glm::mat4 modelGrille = modelCar;
        modelGrille = glm::translate(modelGrille, glm::vec3(0.0f, 0.36f, 1.002f));
        modelGrille = glm::scale(modelGrille, glm::vec3(0.38f, 0.06f, 0.01f));
        ourShader.setMat4("model", modelGrille);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 7. Egzoz Borusu (Exhaust Pipe - Silver)
        ourShader.setVec3("objectColor", 0.75f, 0.75f, 0.75f);
        ourShader.setFloat("specularStrength", 0.8f);
        ourShader.setFloat("shininess", 32.0f);
        glm::mat4 modelExhaust = modelCar;
        modelExhaust = glm::translate(modelExhaust, glm::vec3(0.32f, 0.24f, -1.03f));
        modelExhaust = glm::scale(modelExhaust, glm::vec3(0.06f, 0.06f, 0.12f));
        ourShader.setMat4("model", modelExhaust);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // 8. Saydam Camlar (İç Koltukların ve Direksiyonun Görünmesi İçin En Son Çizilir)
        ourShader.setVec3("objectColor", 0.5f, 0.8f, 1.0f); // Açık cam mavisi
        ourShader.setFloat("specularStrength", 1.5f);
        ourShader.setFloat("shininess", 256.0f);
        
        // A) Ön Cam (Tekerlek Eğimine Paralel Geriye Yatık -25°)
        ourShader.setFloat("objectAlpha", 0.20f); // %20 saydamlık
        glm::mat4 modelWindshield = modelCar;
        modelWindshield = glm::translate(modelWindshield, glm::vec3(0.0f, 0.70f, 0.525f));
        modelWindshield = glm::rotate(modelWindshield, glm::radians(-pillarAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        modelWindshield = glm::scale(modelWindshield, glm::vec3(0.76f, pillarHeightL, 0.005f));
        ourShader.setMat4("model", modelWindshield);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // B) Arka Cam (Tekerlek Eğimine Paralel Öne Yatık +25°)
        ourShader.setFloat("objectAlpha", 0.20f);
        glm::mat4 modelRearWindow = modelCar;
        modelRearWindow = glm::translate(modelRearWindow, glm::vec3(0.0f, 0.70f, -0.725f));
        modelRearWindow = glm::rotate(modelRearWindow, glm::radians(pillarAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        modelRearWindow = glm::scale(modelRearWindow, glm::vec3(0.76f, pillarHeightL, 0.005f));
        ourShader.setMat4("model", modelRearWindow);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // C) Sol Yan Camlar (B-direği ile bölünmüş düz panel)
        ourShader.setFloat("objectAlpha", 0.15f); // %15 saydamlık
        glm::mat4 modelSideWindowL = modelCar;
        modelSideWindowL = glm::translate(modelSideWindowL, glm::vec3(-0.402f, 0.70f, -0.1f));
        modelSideWindowL = glm::scale(modelSideWindowL, glm::vec3(0.005f, 0.32f, 1.18f));
        ourShader.setMat4("model", modelSideWindowL);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // D) Sağ Yan Camlar
        ourShader.setFloat("objectAlpha", 0.15f);
        glm::mat4 modelSideWindowR = modelCar;
        modelSideWindowR = glm::translate(modelSideWindowR, glm::vec3(0.402f, 0.70f, -0.1f));
        modelSideWindowR = glm::scale(modelSideWindowR, glm::vec3(0.005f, 0.32f, 1.18f));
        ourShader.setMat4("model", modelSideWindowR);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Diğer çizimler için saydamlığı sıfırlayalım
        ourShader.setFloat("objectAlpha", 1.0f);

        // 15b. Pencereler (3D çerçeveli cam pencereler - Saydam çizilmesi için en sona alındı)
        auto drawWindow = [&](glm::vec3 pos, float rotationY = 0.0f) {
            // Local base matrix for this window
            glm::mat4 localModel = glm::mat4(1.0f);
            localModel = glm::translate(localModel, pos);
            if (rotationY != 0.0f) {
                localModel = glm::rotate(localModel, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // 1. Ahşap Çerçeve Çıtaları (4 Adet Kenar - İçi Boş Tasarım)
            glBindVertexArray(cubeVAO);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            ourShader.setBool("useTexture", true);
            ourShader.setVec3("objectColor", 0.5f, 0.35f, 0.2f); 
            ourShader.setFloat("specularStrength", 0.1f);
            ourShader.setFloat("shininess", 16.0f);
            ourShader.setFloat("objectAlpha", 1.0f);
            ourShader.setVec2("uvScale", glm::vec2(1.0f, 1.0f));

            // A) Üst Çıta (Top Border)
            glm::mat4 modelTop = glm::translate(localModel, glm::vec3(0.0f, 0.23f, 0.0f));
            modelTop = glm::scale(modelTop, glm::vec3(0.5f, 0.04f, 0.04f));
            ourShader.setMat4("model", modelTop);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // B) Alt Çıta (Bottom Border)
            glm::mat4 modelBottom = glm::translate(localModel, glm::vec3(0.0f, -0.23f, 0.0f));
            modelBottom = glm::scale(modelBottom, glm::vec3(0.5f, 0.04f, 0.04f));
            ourShader.setMat4("model", modelBottom);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // C) Sol Çıta (Left Border)
            glm::mat4 modelLeft = glm::translate(localModel, glm::vec3(-0.23f, 0.0f, 0.0f));
            modelLeft = glm::scale(modelLeft, glm::vec3(0.04f, 0.42f, 0.04f));
            ourShader.setMat4("model", modelLeft);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // D) Sağ Çıta (Right Border)
            glm::mat4 modelRight = glm::translate(localModel, glm::vec3(0.23f, 0.0f, 0.0f));
            modelRight = glm::scale(modelRight, glm::vec3(0.04f, 0.42f, 0.04f));
            ourShader.setMat4("model", modelRight);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // 2. Cam Bölmesi (Saydam Açık Mavi, Yansıtıcı)
            ourShader.setBool("useTexture", false);
            ourShader.setVec3("objectColor", 0.5f, 0.8f, 1.0f);
            ourShader.setFloat("specularStrength", 1.5f);
            ourShader.setFloat("shininess", 256.0f); 
            ourShader.setFloat("objectAlpha", 0.25f); // Saydam cam (içerisi tamamen gözükecek!)
            
            glm::mat4 modelGlass = glm::scale(localModel, glm::vec3(0.42f, 0.42f, 0.01f)); // İnce cam plakası
            ourShader.setMat4("model", modelGlass);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            // Diğer çizimler için saydamlığı sıfırlayalım
            ourShader.setFloat("objectAlpha", 1.0f);
        };

        // Ön Yüz Sol Pencere (Yatak Odası -2.5 ile -0.5 arası)
        drawWindow(glm::vec3(-1.5f, 0.3f, 2.01f));
        // Ön Yüz Sağ Pencere (Oturma Odası)
        drawWindow(glm::vec3(1.7f, 0.3f, 2.01f));
        // Sol Duvar Penceresi (Yatak Odası)
        drawWindow(glm::vec3(-2.51f, 0.3f, 0.0f), 90.0f);
        // Sağ Duvar Penceresi (Oturma Odası)
        drawWindow(glm::vec3(2.51f, 0.3f, 0.8f), 90.0f);
        // Arka Yüz Sol Pencere
        drawWindow(glm::vec3(-1.5f, 0.3f, -2.01f));
        // Arka Yüz Sağ Pencere
        drawWindow(glm::vec3(1.0f, 0.3f, -2.01f));

        // 15c. BULUTLAR & GÜNEŞ (Floating clouds and glowing low-poly sun in the sky)
        {
            struct Cloud {
                glm::vec3 basePosition;
                glm::vec3 scale;
                float speed;
                float phase;
                int type; // 0: Small (2 cubes), 1: Medium (3 cubes), 2: Large (6 cubes)
            };
            static std::vector<Cloud> clouds = {
                // Alt seviye bulutlar (Bulut irtifası: 10.0 - 13.0)
                { glm::vec3(-35.0f, 10.5f, -15.0f), glm::vec3(3.5f, 1.0f, 2.5f), 0.65f, 0.0f, 1 },
                { glm::vec3(-15.0f, 11.0f,  12.0f), glm::vec3(2.2f, 0.7f, 1.8f), 0.75f, 1.2f, 0 },
                { glm::vec3(  5.0f, 10.0f, -28.0f), glm::vec3(4.2f, 1.2f, 3.0f), 0.55f, 2.5f, 1 },
                { glm::vec3( 25.0f, 11.5f,  -5.0f), glm::vec3(2.5f, 0.8f, 2.0f), 0.70f, 3.8f, 0 },
                { glm::vec3(-48.0f, 12.0f,   2.0f), glm::vec3(3.0f, 1.0f, 2.2f), 0.60f, 0.8f, 0 },
                { glm::vec3( 40.0f, 10.8f, -18.0f), glm::vec3(4.0f, 1.1f, 3.2f), 0.50f, 5.1f, 1 },

                // Orta seviye bulutlar (Bulut irtifası: 14.0 - 17.0)
                { glm::vec3(-25.0f, 15.0f, -20.0f), glm::vec3(6.5f, 1.8f, 4.5f), 0.45f, 1.7f, 2 },
                { glm::vec3( -2.0f, 14.0f,  18.0f), glm::vec3(4.8f, 1.3f, 3.5f), 0.50f, 4.2f, 1 },
                { glm::vec3( 18.0f, 16.0f, -12.0f), glm::vec3(7.2f, 2.0f, 5.0f), 0.38f, 2.9f, 2 },
                { glm::vec3(-12.0f, 15.5f,  -8.0f), glm::vec3(3.8f, 1.1f, 2.8f), 0.52f, 6.3f, 1 },
                { glm::vec3( 35.0f, 14.5f,  15.0f), glm::vec3(6.0f, 1.7f, 4.2f), 0.42f, 0.5f, 2 },
                { glm::vec3(-42.0f, 16.5f, -25.0f), glm::vec3(2.8f, 0.9f, 2.0f), 0.62f, 3.1f, 0 },

                // Yüksek bulutlar (Bulut irtifası: 18.0 - 22.0)
                { glm::vec3(-30.0f, 19.0f,  -2.0f), glm::vec3(8.0f, 2.2f, 5.5f), 0.30f, 5.5f, 2 },
                { glm::vec3(  8.0f, 18.0f, -22.0f), glm::vec3(3.2f, 1.0f, 2.4f), 0.48f, 1.1f, 0 },
                { glm::vec3( 28.0f, 20.5f,  22.0f), glm::vec3(5.0f, 1.4f, 3.8f), 0.40f, 2.4f, 1 },
                { glm::vec3(-10.0f, 21.0f, -32.0f), glm::vec3(9.0f, 2.5f, 6.0f), 0.28f, 4.7f, 2 },
                { glm::vec3( 45.0f, 19.5f,  -8.0f), glm::vec3(4.5f, 1.3f, 3.2f), 0.44f, 3.6f, 1 },
                { glm::vec3(-55.0f, 18.5f,  28.0f), glm::vec3(2.5f, 0.8f, 1.8f), 0.58f, 0.2f, 0 }
            };

            glBindVertexArray(cubeVAO);
            ourShader.setBool("useTexture", false);
            ourShader.setFloat("specularStrength", 0.0f);
            ourShader.setBool("isFlame", true); // Parlak beyaz kendinden aydınlatmalı bulut görünümü

            // Geniş yatay kayma döngüsü (Bulutlar bu aralıkta kayıp döner)
            float range = 130.0f;
            float halfRange = range / 2.0f;

            // BULUTLARIN ÇİZİLMESİ
            for (const auto& cloud : clouds) {
                // Zamanla kayma hesaplaması
                float xOffset = fmod(currentFrame * cloud.speed + cloud.phase * 20.0f, range) - halfRange;
                glm::vec3 pos = cloud.basePosition;
                pos.x += xOffset;

                // Kenarlarda yumuşak kaybolma efekti (Fade-in / Fade-out)
                float distFromCenter = abs(pos.x);
                float edgeFadeStart = halfRange - 15.0f;
                float alphaMultiplier = 1.0f;
                if (distFromCenter > edgeFadeStart) {
                    alphaMultiplier = glm::clamp((halfRange - distFromCenter) / 15.0f, 0.0f, 1.0f);
                }
                float finalAlpha = 0.85f * alphaMultiplier;

                // Bulut tipine göre düşük poligon detaylı model çizimleri
                if (cloud.type == 0) {
                    // Küçük Bulut (2 Kesişen Küp)
                    // 1. Ana Küp
                    ourShader.setVec3("objectColor", 0.96f, 0.97f, 0.98f);
                    ourShader.setFloat("objectAlpha", finalAlpha);
                    glm::mat4 modelMid = glm::mat4(1.0f);
                    modelMid = glm::translate(modelMid, pos);
                    modelMid = glm::scale(modelMid, cloud.scale);
                    ourShader.setMat4("model", modelMid);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // 2. Yan Küp
                    glm::mat4 modelSide = glm::mat4(1.0f);
                    modelSide = glm::translate(modelSide, pos + glm::vec3(-cloud.scale.x * 0.4f, -cloud.scale.y * 0.1f, 0.1f * cloud.scale.z));
                    modelSide = glm::scale(modelSide, cloud.scale * glm::vec3(0.6f, 0.7f, 0.7f));
                    ourShader.setMat4("model", modelSide);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                else if (cloud.type == 1) {
                    // Orta Boy Bulut (3 Kesişen Küp)
                    ourShader.setVec3("objectColor", 0.96f, 0.97f, 0.98f);
                    ourShader.setFloat("objectAlpha", finalAlpha);
                    glm::mat4 modelMid = glm::mat4(1.0f);
                    modelMid = glm::translate(modelMid, pos);
                    modelMid = glm::scale(modelMid, cloud.scale);
                    ourShader.setMat4("model", modelMid);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Sol ek küp
                    glm::mat4 modelLeft = glm::mat4(1.0f);
                    modelLeft = glm::translate(modelLeft, pos + glm::vec3(-cloud.scale.x * 0.45f, -cloud.scale.y * 0.15f, 0.0f));
                    modelLeft = glm::scale(modelLeft, cloud.scale * glm::vec3(0.6f, 0.7f, 0.7f));
                    ourShader.setMat4("model", modelLeft);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Sağ ek küp
                    glm::mat4 modelRight = glm::mat4(1.0f);
                    modelRight = glm::translate(modelRight, pos + glm::vec3(cloud.scale.x * 0.45f, -cloud.scale.y * 0.05f, 0.0f));
                    modelRight = glm::scale(modelRight, cloud.scale * glm::vec3(0.7f, 0.8f, 0.8f));
                    ourShader.setMat4("model", modelRight);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                else {
                    // Büyük/Dev Kümülüs Bulutu (6 Kesişen Küp)
                    ourShader.setVec3("objectColor", 0.96f, 0.97f, 0.98f);
                    ourShader.setFloat("objectAlpha", finalAlpha);
                    glm::mat4 modelMid = glm::mat4(1.0f);
                    modelMid = glm::translate(modelMid, pos);
                    modelMid = glm::scale(modelMid, cloud.scale);
                    ourShader.setMat4("model", modelMid);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Sol
                    glm::mat4 modelLeft = glm::mat4(1.0f);
                    modelLeft = glm::translate(modelLeft, pos + glm::vec3(-cloud.scale.x * 0.4f, -cloud.scale.y * 0.1f, 0.1f * cloud.scale.z));
                    modelLeft = glm::scale(modelLeft, cloud.scale * glm::vec3(0.7f, 0.8f, 0.8f));
                    ourShader.setMat4("model", modelLeft);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Sağ
                    glm::mat4 modelRight = glm::mat4(1.0f);
                    modelRight = glm::translate(modelRight, pos + glm::vec3(cloud.scale.x * 0.4f, -cloud.scale.y * 0.15f, -0.1f * cloud.scale.z));
                    modelRight = glm::scale(modelRight, cloud.scale * glm::vec3(0.6f, 0.75f, 0.75f));
                    ourShader.setMat4("model", modelRight);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Üst kubbe
                    glm::mat4 modelTop = glm::mat4(1.0f);
                    modelTop = glm::translate(modelTop, pos + glm::vec3(0.0f, cloud.scale.y * 0.45f, 0.0f));
                    modelTop = glm::scale(modelTop, cloud.scale * glm::vec3(0.5f, 0.7f, 0.6f));
                    ourShader.setMat4("model", modelTop);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Ön çıkıntı
                    glm::mat4 modelFront = glm::mat4(1.0f);
                    modelFront = glm::translate(modelFront, pos + glm::vec3(-cloud.scale.x * 0.1f, 0.0f, 0.3f * cloud.scale.z));
                    modelFront = glm::scale(modelFront, cloud.scale * glm::vec3(0.4f, 0.6f, 0.7f));
                    ourShader.setMat4("model", modelFront);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // Kuyruk
                    glm::mat4 modelTail = glm::mat4(1.0f);
                    modelTail = glm::translate(modelTail, pos + glm::vec3(cloud.scale.x * 0.55f, -cloud.scale.y * 0.2f, 0.2f * cloud.scale.z));
                    modelTail = glm::scale(modelTail, cloud.scale * glm::vec3(0.3f, 0.5f, 0.4f));
                    ourShader.setMat4("model", modelTail);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            // GÜNEŞİN ÇİZİLMESİ (Emissive Low-Poly Sun with Pulsating Halo & Rotational Rays - Gündüz Belirir)
            if (cycleValue > 0.01f) {
                // A) Güneş Çekirdeği (Core - Parlak Beyaz-Sarı)
                ourShader.setVec3("objectColor", 1.0f, 1.0f, 0.85f);
                ourShader.setFloat("objectAlpha", cycleValue);
                glm::mat4 modelSunCore = glm::mat4(1.0f);
                modelSunCore = glm::translate(modelSunCore, sunPos);
                modelSunCore = glm::rotate(modelSunCore, glm::radians(currentFrame * 4.0f), glm::vec3(0.0f, 1.0f, 1.0f));
                modelSunCore = glm::scale(modelSunCore, glm::vec3(2.8f, 2.8f, 2.8f));
                ourShader.setMat4("model", modelSunCore);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // B) Düşük Poligon Işık Işınları (Counter-Rotating Rays - Volumetrik Etki)
                ourShader.setVec3("objectColor", 1.0f, 0.9f, 0.5f);
                ourShader.setFloat("objectAlpha", 0.18f * cycleValue); // Yarı saydam

                // Işın Seti 1 (Saat yönünde dönen)
                for (int r = 0; r < 3; ++r) {
                    glm::mat4 modelRay = glm::mat4(1.0f);
                    modelRay = glm::translate(modelRay, sunPos);
                    modelRay = glm::rotate(modelRay, glm::radians(currentFrame * 12.0f + r * 60.0f), glm::vec3(0.3f, 0.5f, 0.8f));
                    modelRay = glm::scale(modelRay, glm::vec3(6.5f, 0.25f, 0.25f));
                    ourShader.setMat4("model", modelRay);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // Işın Seti 2 (Ters yönde dönen)
                for (int r = 0; r < 3; ++r) {
                    glm::mat4 modelRay = glm::mat4(1.0f);
                    modelRay = glm::translate(modelRay, sunPos);
                    modelRay = glm::rotate(modelRay, glm::radians(-currentFrame * 8.0f + r * 60.0f), glm::vec3(0.8f, -0.3f, 0.5f));
                    modelRay = glm::scale(modelRay, glm::vec3(0.25f, 6.5f, 0.25f));
                    ourShader.setMat4("model", modelRay);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // C) Güneş Halesi (Halo - Yarı Saydam Altın Sarısı, Boyut ve Saydamlık Pulsar)
                float pulse = sin(currentFrame * 2.0f);
                float haloScaleVal = 4.2f + 0.4f * pulse;
                float haloAlphaVal = 0.15f + 0.05f * pulse;

                ourShader.setVec3("objectColor", 1.0f, 0.75f, 0.2f);
                ourShader.setFloat("objectAlpha", haloAlphaVal * cycleValue);
                glm::mat4 modelSunHalo = glm::mat4(1.0f);
                modelSunHalo = glm::translate(modelSunHalo, sunPos);
                modelSunHalo = glm::rotate(modelSunHalo, glm::radians(-currentFrame * 2.0f), glm::vec3(1.0f, 1.0f, 0.0f));
                modelSunHalo = glm::scale(modelSunHalo, glm::vec3(haloScaleVal, haloScaleVal, haloScaleVal));
                ourShader.setMat4("model", modelSunHalo);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // AYIN ÇİZİLMESİ (Emissive Low-Poly Moon with Pulsating Halo & Rotational Rays - Gece Belirir)
            float moonAlpha = 1.0f - cycleValue;
            if (moonAlpha > 0.01f) {
                // A) Ay Çekirdeği (Core - Gümüş Beyaz-Mavi)
                ourShader.setVec3("objectColor", 0.85f, 0.9f, 1.0f);
                ourShader.setFloat("objectAlpha", moonAlpha);
                glm::mat4 modelMoonCore = glm::mat4(1.0f);
                modelMoonCore = glm::translate(modelMoonCore, moonPos);
                modelMoonCore = glm::rotate(modelMoonCore, glm::radians(-currentFrame * 3.0f), glm::vec3(1.0f, 0.0f, 1.0f));
                modelMoonCore = glm::scale(modelMoonCore, glm::vec3(2.4f, 2.4f, 2.4f));
                ourShader.setMat4("model", modelMoonCore);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // B) Düşük Poligon Gümüş Işınlar (Ay Işınları - Counter-Rotating Rays)
                ourShader.setVec3("objectColor", 0.75f, 0.85f, 1.0f);
                ourShader.setFloat("objectAlpha", 0.12f * moonAlpha);

                for (int r = 0; r < 3; ++r) {
                    glm::mat4 modelRay = glm::mat4(1.0f);
                    modelRay = glm::translate(modelRay, moonPos);
                    modelRay = glm::rotate(modelRay, glm::radians(currentFrame * 8.0f + r * 60.0f), glm::vec3(0.5f, 0.3f, 0.8f));
                    modelRay = glm::scale(modelRay, glm::vec3(5.5f, 0.2f, 0.2f));
                    ourShader.setMat4("model", modelRay);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                for (int r = 0; r < 3; ++r) {
                    glm::mat4 modelRay = glm::mat4(1.0f);
                    modelRay = glm::translate(modelRay, moonPos);
                    modelRay = glm::rotate(modelRay, glm::radians(-currentFrame * 6.0f + r * 60.0f), glm::vec3(0.8f, -0.5f, 0.3f));
                    modelRay = glm::scale(modelRay, glm::vec3(0.2f, 5.5f, 0.2f));
                    ourShader.setMat4("model", modelRay);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // C) Ay Halesi (Halo - Açık Mavi/Gümüş, Pulsar)
                float moonPulse = sin(currentFrame * 1.5f);
                float moonHaloScaleVal = 3.6f + 0.3f * moonPulse;
                float moonHaloAlphaVal = 0.12f + 0.04f * moonPulse;

                ourShader.setVec3("objectColor", 0.65f, 0.8f, 1.0f);
                ourShader.setFloat("objectAlpha", moonHaloAlphaVal * moonAlpha);
                glm::mat4 modelMoonHalo = glm::mat4(1.0f);
                modelMoonHalo = glm::translate(modelMoonHalo, moonPos);
                modelMoonHalo = glm::rotate(modelMoonHalo, glm::radians(currentFrame * 1.5f), glm::vec3(0.0f, 1.0f, 1.0f));
                modelMoonHalo = glm::scale(modelMoonHalo, glm::vec3(moonHaloScaleVal, moonHaloScaleVal, moonHaloScaleVal));
                ourShader.setMat4("model", modelMoonHalo);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // Ayarları sıfırlama
            ourShader.setFloat("objectAlpha", 1.0f);
            ourShader.setBool("isFlame", false);
        }

        // 16. OYUNCU KOLLARI VE ELLERİ (First Person Hands & Arms)
        // Duvarlara yaklaşınca ellerin diğer nesnelerin arkasında kalmaması (clipping) için depth buffer temizleniyor
        glClear(GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeVAO);
        ourShader.setBool("useTexture", false);
        ourShader.setFloat("specularStrength", 0.1f);
        ourShader.setFloat("shininess", 8.0f);
        ourShader.setFloat("objectAlpha", 1.0f);

        // Kamera rotasyon matrisi (ellerin kamera yönüyle senkronize dönmesi için)
        glm::mat4 cameraRot = glm::mat4(1.0f);
        cameraRot[0] = glm::vec4(camera.Right, 0.0f);
        cameraRot[1] = glm::vec4(camera.Up, 0.0f);
        cameraRot[2] = glm::vec4(-camera.Front, 0.0f);

        // Sol Kol (Mavi Ceket Kolu)
        ourShader.setVec3("objectColor", 0.1f, 0.2f, 0.4f); // Koyu mavi ceket kolu
        glm::mat4 modelLeftArm = glm::mat4(1.0f);
        modelLeftArm = glm::translate(modelLeftArm, camera.Position + camera.Front * 0.25f - camera.Right * 0.1f - camera.Up * 0.12f);
        modelLeftArm = modelLeftArm * cameraRot;
        modelLeftArm = glm::scale(modelLeftArm, glm::vec3(0.035f, 0.035f, 0.15f));
        ourShader.setMat4("model", modelLeftArm);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sol El (Ten rengi yumruk)
        ourShader.setVec3("objectColor", 0.9f, 0.7f, 0.6f);
        glm::mat4 modelLeftHand = glm::mat4(1.0f);
        modelLeftHand = glm::translate(modelLeftHand, camera.Position + camera.Front * 0.33f - camera.Right * 0.1f - camera.Up * 0.12f);
        modelLeftHand = modelLeftHand * cameraRot;
        modelLeftHand = glm::scale(modelLeftHand, glm::vec3(0.04f, 0.04f, 0.04f));
        ourShader.setMat4("model", modelLeftHand);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sağ Kol (Mavi Ceket Kolu)
        ourShader.setVec3("objectColor", 0.1f, 0.2f, 0.4f);
        glm::mat4 modelRightArm = glm::mat4(1.0f);
        modelRightArm = glm::translate(modelRightArm, camera.Position + camera.Front * 0.25f + camera.Right * 0.1f - camera.Up * 0.12f);
        modelRightArm = modelRightArm * cameraRot;
        modelRightArm = glm::scale(modelRightArm, glm::vec3(0.035f, 0.035f, 0.15f));
        ourShader.setMat4("model", modelRightArm);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sağ El (Ten rengi yumruk)
        ourShader.setVec3("objectColor", 0.9f, 0.7f, 0.6f);
        glm::mat4 modelRightHand = glm::mat4(1.0f);
        modelRightHand = glm::translate(modelRightHand, camera.Position + camera.Front * 0.33f + camera.Right * 0.1f - camera.Up * 0.12f);
        modelRightHand = modelRightHand * cameraRot;
        modelRightHand = glm::scale(modelRightHand, glm::vec3(0.04f, 0.04f, 0.04f));
        ourShader.setMat4("model", modelRightHand);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &roofVAO);
    glDeleteVertexArrays(1, &trunkVAO);
    glDeleteVertexArrays(1, &leavesVAO);

    glfwTerminate();
    return 0;
}

// Evin duvarlarıyla çarpışma tespiti yapan fonksiyon (AABB - 2D Segment bazlı çarpışma kontrolü)
bool checkWallCollision(float x, float z, float r) {
    // 1. Sol Dış Duvar: x = -2.5, z = [-2.0, 2.0]
    if (z >= -2.0f - r && z <= 2.0f + r) {
        if (std::abs(x - (-2.5f)) < r) return true;
    }
    // 2. Sağ Dış Duvar: x = 2.5, z = [-2.0, 2.0]
    if (z >= -2.0f - r && z <= 2.0f + r) {
        if (std::abs(x - 2.5f) < r) return true;
    }
    // 3. Arka Dış Duvar: z = -2.0, x = [-2.5, 2.5]
    if (x >= -2.5f - r && x <= 2.5f + r) {
        if (std::abs(z - (-2.0f)) < r) return true;
    }
    // 4. Ön Dış Duvar Sol Parça: z = 2.0, x = [-2.5, 0.7]
    if (x >= -2.5f - r && x <= 0.7f + r) {
        if (std::abs(z - 2.0f) < r) return true;
    }
    // 5. Ön Dış Duvar Sağ Parça: z = 2.0, x = [1.3, 2.5]
    if (x >= 1.3f - r && x <= 2.5f + r) {
        if (std::abs(z - 2.0f) < r) return true;
    }
    // 6. Dış Kapı (Kapalıysa): z = 2.0, x = [0.7, 1.3]
    if (!doorOpen) {
        if (x >= 0.7f - r && x <= 1.3f + r) {
            if (std::abs(z - 2.0f) < r) return true;
        }
    }
    // 7. İç Bölme Duvarı Arka Parça: x = -0.5, z = [-2.0, -1.6]
    if (z >= -2.0f - r && z <= -1.6f + r) {
        if (std::abs(x - (-0.5f)) < r) return true;
    }
    // 8. İç Bölme Duvarı Ön Parça: x = -0.5, z = [-0.8, 2.0]
    if (z >= -0.8f - r && z <= 2.0f + r) {
        if (std::abs(x - (-0.5f)) < r) return true;
    }
    return false;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float velocity = camera.MovementSpeed * deltaTime;
    if (!flyMode) {
        glm::vec3 prevPos = camera.Position;

        // Walk mode: Project movement onto the horizontal XZ plane
        glm::vec3 horizontalFront = glm::vec3(camera.Front.x, 0.0f, camera.Front.z);
        if (glm::length(horizontalFront) > 0.0001f) {
            horizontalFront = glm::normalize(horizontalFront);
        }
        glm::vec3 horizontalRight = glm::vec3(camera.Right.x, 0.0f, camera.Right.z);
        if (glm::length(horizontalRight) > 0.0001f) {
            horizontalRight = glm::normalize(horizontalRight);
        }

        glm::vec3 nextPos = camera.Position;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            nextPos += horizontalFront * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            nextPos -= horizontalFront * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            nextPos -= horizontalRight * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            nextPos += horizontalRight * velocity;

        // Oyuncu çarpışma yarıçapı
        float r = 0.22f;

        // Kayarak çarpışma çözümleme (Sliding Collision Resolution)
        // 1. Önce sadece X yönünde hareket etmeyi dene
        float candidateX = nextPos.x;
        if (checkWallCollision(candidateX, prevPos.z, r)) {
            candidateX = prevPos.x; // Engel varsa X hareketini sıfırla
        }

        // 2. Sonra Z yönünde hareket etmeyi dene (X kararına bağlı olarak)
        float candidateZ = nextPos.z;
        if (checkWallCollision(candidateX, candidateZ, r)) {
            candidateZ = prevPos.z; // Engel varsa Z hareketini sıfırla
        }

        // Kameranın yeni konumunu belirle ve yüksekliği yer seviyesine sabitle
        camera.Position = glm::vec3(candidateX, 0.5f, candidateZ);
    } else {
        // Fly mode: Free vertical/horizontal navigation
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    // Kapı Açma/Kapama ('F' tuşu debounced)
    static bool fKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyWasPressed) {
            doorOpen = !doorOpen;
            fKeyWasPressed = true;
        }
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fKeyWasPressed = false;
    }

    // Lamba Açma/Kapama ('G' tuşu debounced)
    static bool gKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gKeyWasPressed) {
            lampOn = !lampOn;
            gKeyWasPressed = true;
        }
    } else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
        gKeyWasPressed = false;
    }

    // Uçuş Modu Açma/Kapama ('N' tuşu debounced)
    static bool nKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        if (!nKeyWasPressed) {
            flyMode = !flyMode;
            nKeyWasPressed = true;
        }
    } else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
        nKeyWasPressed = false;
    }

    // Gece Modu Açma/Kapama ('C' tuşu debounced)
    static bool cKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cKeyWasPressed) {
            nightMode = !nightMode;
            cKeyWasPressed = true;
        }
    } else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        cKeyWasPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}
