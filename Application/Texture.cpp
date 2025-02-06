#include"Texture.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <STBIMAGE/stb_image.h> // 使用 stb_image 库加载图片

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // 设置边界颜色
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // RGBA 白色
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 设置纹理过滤模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载图片
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;  // 灰度图
        else if (channels == 3)
            format = GL_RGB;  // RGB 图
        else if (channels == 4)
            format = GL_RGBA; // RGBA 图
        else {
            std::cout << "Unsupported texture format: " << path << std::endl;
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
            return 0;
        }

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
