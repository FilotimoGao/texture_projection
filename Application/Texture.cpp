#include"Texture.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <STBIMAGE/stb_image.h> // ʹ�� stb_image �����ͼƬ

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // �����������
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // ���ñ߽���ɫ
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // RGBA ��ɫ
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // �����������ģʽ
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ����ͼƬ
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;  // �Ҷ�ͼ
        else if (channels == 3)
            format = GL_RGB;  // RGB ͼ
        else if (channels == 4)
            format = GL_RGBA; // RGBA ͼ
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
        glDeleteTextures(1, &textureID); // ʧ��ʱ��������
        return 0; // ���� 0 ��ʾ����ʧ��
    }
    stbi_image_free(data);
    return textureID;
}
