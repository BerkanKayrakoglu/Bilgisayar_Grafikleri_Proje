cmake_minimum_required(VERSION 3.20)
project(Bilgisayar_Grafikleri_Lab3)

set(CMAKE_CXX_STANDARD 17)

# CMake uyumluluk hatalarını önlemek için:
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# FetchContent modülünü dahil ediyoruz
include(FetchContent)

# 1. FreeType Kütüphanesini İndir ve Derle
message(STATUS "FreeType indiriliyor...")
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

FetchContent_Declare(
        freetype
        URL https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-2.zip
)
FetchContent_MakeAvailable(freetype)

# 2. Assimp Kütüphanesini İndir ve Derle
message(STATUS "Assimp indiriliyor...")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        assimp
        URL https://github.com/assimp/assimp/archive/refs/tags/v5.3.1.zip
)
FetchContent_MakeAvailable(assimp)

# 3. GLFW Kütüphanesini İndir ve Derle (Uyumsuzluk sorununu kökten çözer)
message(STATUS "GLFW indiriliyor...")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/releases/download/3.3.9/glfw-3.3.9.zip
)
FetchContent_MakeAvailable(glfw)

# Kendi include klasörümüzü (glad vb. için) tanıtıyoruz
include_directories(include)

# Projeye dahil edilecek kaynak dosyalar
add_executable(Bilgisayar_Grafikleri_Lab3
        main.cpp
        glad.c
        Shader.cpp
        stb_image.cpp
)

# Kütüphaneleri projeye bağlıyoruz
target_link_libraries(Bilgisayar_Grafikleri_Lab3
        glfw
        opengl32
        assimp
        freetype
)
