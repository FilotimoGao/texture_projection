#pragma once
#include<glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<vector>

struct Transform {
    glm::vec3 position = glm::vec3(0.0f); // ģ��λ��
    glm::vec3 rotation = glm::vec3(0.0f); // ģ����ת��ŷ���ǣ�
    glm::vec2 size = glm::vec2(1.0f);     // ģ�����Ŵ�С����Ⱥ͸߶ȣ�
};

enum class TextureMode {
    UV_MAPPING,        // ʹ��ֱ�� UV ӳ��
    PROJECTION_MAPPING // ʹ��ͶӰ����
};

class Model {
public:
    // ���캯������������
    Model(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    ~Model();

    // ���ƺ���
    void Draw(GLuint shaderProgram);

    // ���úͻ�ȡ���� ID
    void setTexture(unsigned int textureID);
    unsigned int getTexture() const;

    // ���úͻ�ȡ�任����
    void setTransform(const Transform& transform);
    Transform& getTransform();

    // ���úͻ�ȡͶӰ����;���
    void setProjectionTexture(unsigned int texture);
    void setProjectionMatrix(const glm::mat4& matrix);
    unsigned int getProjectionTexture() const;
    const glm::mat4& getProjectionMatrix() const;
    const glm::mat4& getModelMatrix() const;

    // ���úͻ�ȡ����ģʽ
    void setTextureMode(TextureMode mode);
    TextureMode getTextureMode() const;

    // ����ģ�;���
    void updateModelMatrix(); 

    // ��ȡ������
    unsigned int getVAO() const;
    unsigned int getVBO() const;
    unsigned int getEBO() const;

private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int textureID = 0;           // �洢Ĭ�ϵ����� ID
    unsigned int projectionTexture = 0;  // �洢ͶӰ���� ID
    TextureMode textureMode = TextureMode::UV_MAPPING; // Ĭ��ʹ�� UV ӳ��

    Transform transform;         // ģ�͵ı任����
    glm::mat4 modelMatrix = glm::mat4(1.0f);   // ģ�ͱ任����
    glm::mat4 projectionMatrix = glm::mat4(1.0f); // ͶӰ����

    std::vector<unsigned int> indices;   // �洢��������
    size_t indexCount = 0;               // �������������ڶ�̬����
};
