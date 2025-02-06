#pragma once
#include<glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<vector>

struct Transform {
    glm::vec3 position = glm::vec3(0.0f); // 模型位置
    glm::vec3 rotation = glm::vec3(0.0f); // 模型旋转（欧拉角）
    glm::vec2 size = glm::vec2(1.0f);     // 模型缩放大小（宽度和高度）
};

enum class TextureMode {
    UV_MAPPING,        // 使用直接 UV 映射
    PROJECTION_MAPPING // 使用投影纹理
};

class Model {
public:
    // 构造函数和析构函数
    Model(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    ~Model();

    // 绘制函数
    void Draw(GLuint shaderProgram);

    // 设置和获取纹理 ID
    void setTexture(unsigned int textureID);
    unsigned int getTexture() const;

    // 设置和获取变换属性
    void setTransform(const Transform& transform);
    Transform& getTransform();

    // 设置和获取投影纹理和矩阵
    void setProjectionTexture(unsigned int texture);
    void setProjectionMatrix(const glm::mat4& matrix);
    unsigned int getProjectionTexture() const;
    const glm::mat4& getProjectionMatrix() const;
    const glm::mat4& getModelMatrix() const;

    // 设置和获取纹理模式
    void setTextureMode(TextureMode mode);
    TextureMode getTextureMode() const;

    // 更新模型矩阵
    void updateModelMatrix(); 

    // 获取缓冲区
    unsigned int getVAO() const;
    unsigned int getVBO() const;
    unsigned int getEBO() const;

private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int textureID = 0;           // 存储默认的纹理 ID
    unsigned int projectionTexture = 0;  // 存储投影纹理 ID
    TextureMode textureMode = TextureMode::UV_MAPPING; // 默认使用 UV 映射

    Transform transform;         // 模型的变换属性
    glm::mat4 modelMatrix = glm::mat4(1.0f);   // 模型变换矩阵
    glm::mat4 projectionMatrix = glm::mat4(1.0f); // 投影矩阵

    std::vector<unsigned int> indices;   // 存储索引数据
    size_t indexCount = 0;               // 索引数量，用于动态绘制
};
