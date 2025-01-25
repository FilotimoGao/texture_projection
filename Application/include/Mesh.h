#pragma once
#include<glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<vector>

struct Plane {
    glm::vec3 position = glm::vec3(0.0f); // 平面位置
    glm::vec3 rotation = glm::vec3(0.0f); // 平面旋转（欧拉角）
    glm::vec2 size = glm::vec2(1.0f);           // 平面大小（宽度和高度）
};


class Mesh {
public:
    // 顶点和索引数据
    Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    // 绘制函数
    void Draw();

    unsigned int getVAO();
    unsigned int getVBO();
    unsigned int getEBO();

private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};