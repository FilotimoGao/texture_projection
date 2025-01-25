#pragma once
#include<glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<vector>

struct Plane {
    glm::vec3 position = glm::vec3(0.0f); // ƽ��λ��
    glm::vec3 rotation = glm::vec3(0.0f); // ƽ����ת��ŷ���ǣ�
    glm::vec2 size = glm::vec2(1.0f);           // ƽ���С����Ⱥ͸߶ȣ�
};


class Mesh {
public:
    // �������������
    Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    // ���ƺ���
    void Draw();

    unsigned int getVAO();
    unsigned int getVBO();
    unsigned int getEBO();

private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};