#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include"Model.h"

class Scene {
public:
    void setModelTexture(int index, unsigned int textureID);
    void addModel(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void removeModel(int index);
    Model* getModel(int index);
    void updateModelData(int index, const std::vector<float>& newVertices);
    void render(GLuint shaderProgram);
    void clear();

    // ѡ��ģ�͵���ع���
    int getSelectedModelIndex() const;
    void setSelectedModelIndex(int index);

private:
    std::vector<Model*> models; // �洢���� Model
    int selectedModelIndex = -1; // ��ǰѡ�е� Model ����
};
