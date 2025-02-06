#include"Scene.h"

void Scene::setModelTexture(int index, unsigned int textureID) {
    if (index >= 0 && index < models.size()) {
        if (textureID > 0) { // 确保纹理有效
            models[index]->setTexture(textureID);
        }
        else {
            std::cerr << "Invalid texture ID for model index: " << index << std::endl;
        }
    }
}

// 添加 Model
void Scene::addModel(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Model* model = new Model(vertices, indices);
    models.push_back(model);
}

// 删除 Model
void Scene::removeModel(int index) {
    if (index >= 0 && index < models.size()) {
        delete models[index];
        models.erase(models.begin() + index);
    }
}

// 更新 Model 数据
void Scene::updateModelData(int index, const std::vector<float>& newVertices) {
    if (index >= 0 && index < models.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, models[index]->getVBO());
        glBufferSubData(GL_ARRAY_BUFFER, 0, newVertices.size() * sizeof(float), newVertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// 渲染场景
void Scene::render(GLuint shaderProgram) {
    for (const auto& model : models) {
        model->Draw(shaderProgram); // 每个模型会自动使用它的投影纹理和矩阵
    }
}

// 清理所有资源
void Scene::clear() {
    for (auto& mesh : models) {
        delete mesh; // 删除每个 Model
    }
    models.clear(); // 清空容器
    selectedModelIndex = -1; // 重置选中项
}

Model* Scene::getModel(int index) {
    if (index >= 0 && index < models.size()) {
        return models[index];
    }
    return nullptr;
}

int Scene::getSelectedModelIndex() const {
    return selectedModelIndex;
}

void Scene::setSelectedModelIndex(int index) {
    if (index >= 0 && index < models.size()) {
        selectedModelIndex = index;
    }
    else {
        selectedModelIndex = -1;
    }
}
