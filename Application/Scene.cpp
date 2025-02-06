#include"Scene.h"

void Scene::setModelTexture(int index, unsigned int textureID) {
    if (index >= 0 && index < models.size()) {
        if (textureID > 0) { // ȷ��������Ч
            models[index]->setTexture(textureID);
        }
        else {
            std::cerr << "Invalid texture ID for model index: " << index << std::endl;
        }
    }
}

// ��� Model
void Scene::addModel(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Model* model = new Model(vertices, indices);
    models.push_back(model);
}

// ɾ�� Model
void Scene::removeModel(int index) {
    if (index >= 0 && index < models.size()) {
        delete models[index];
        models.erase(models.begin() + index);
    }
}

// ���� Model ����
void Scene::updateModelData(int index, const std::vector<float>& newVertices) {
    if (index >= 0 && index < models.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, models[index]->getVBO());
        glBufferSubData(GL_ARRAY_BUFFER, 0, newVertices.size() * sizeof(float), newVertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// ��Ⱦ����
void Scene::render(GLuint shaderProgram) {
    for (const auto& model : models) {
        model->Draw(shaderProgram); // ÿ��ģ�ͻ��Զ�ʹ������ͶӰ����;���
    }
}

// ����������Դ
void Scene::clear() {
    for (auto& mesh : models) {
        delete mesh; // ɾ��ÿ�� Model
    }
    models.clear(); // �������
    selectedModelIndex = -1; // ����ѡ����
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
