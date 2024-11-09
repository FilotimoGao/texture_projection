#include"Texture.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <STBIMAGE/stb_image.h> // 使用 stb_image 库加载图片

// 加载纹理函数
GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载图片
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        glDeleteTextures(1, &textureID); // 失败时清理纹理
        return 0; // 返回 0 表示加载失败
    }
    stbi_image_free(data);
    return textureID;
}