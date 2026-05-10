#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

// =============================================================================
//  TextureManager.h — InsideTheFrame Virtual Exhibition
//  Loads PNG/JPG via stb_image, auto-generates mipmaps, caches by path.
//  Falls back to a coloured checkerboard when a file is missing.
// =============================================================================

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <algorithm>

class TextureManager {
public:
    // Load an image from disk (or return cached ID if already loaded).
    GLuint load(const std::string& path,
                glm::vec3 fallbackColor = glm::vec3(0.5f, 0.5f, 0.9f))
    {
        auto it = cache.find(path);
        if (it != cache.end()) return it->second;

        stbi_set_flip_vertically_on_load(true);
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

        GLuint texID;
        if (data) {
            texID = uploadTexture(data, width, height);
            stbi_image_free(data);
            std::cout << "[TextureMgr] Loaded: " << path << " (" << width << "x" << height << ")\n";
        } else {
            std::cout << "[TextureMgr] Missing: " << path << " — checkerboard fallback\n";
            texID = makeCheckerboard(fallbackColor);
        }

        cache[path] = texID;
        allTextures.push_back(texID);
        return texID;
    }

    void cleanup() {
        for (GLuint id : allTextures) glDeleteTextures(1, &id);
        allTextures.clear();
        cache.clear();
    }

    ~TextureManager() { cleanup(); }

private:
    std::unordered_map<std::string, GLuint> cache;
    std::vector<GLuint> allTextures;

    GLuint uploadTexture(unsigned char* data, int width, int height) {
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GLfloat maxAniso = 1.0f;
        glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &maxAniso);
        if (maxAniso > 1.0f)
            glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT*/,
                            std::min(maxAniso, 4.0f));
        glBindTexture(GL_TEXTURE_2D, 0);
        return id;
    }

    GLuint makeCheckerboard(glm::vec3 color) {
        const int SIZE = 64;
        std::vector<unsigned char> pixels(SIZE * SIZE * 4);
        for (int y = 0; y < SIZE; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                bool light = ((x / 8 + y / 8) % 2 == 0);
                int idx = (y * SIZE + x) * 4;
                pixels[idx+0] = light ? (unsigned char)(color.r * 255) : 20;
                pixels[idx+1] = light ? (unsigned char)(color.g * 255) : 20;
                pixels[idx+2] = light ? (unsigned char)(color.b * 255) : 20;
                pixels[idx+3] = 255;
            }
        }
        return uploadTexture(pixels.data(), SIZE, SIZE);
    }
};

#endif // TEXTURE_MANAGER_H
