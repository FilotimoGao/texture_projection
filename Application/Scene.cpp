#include"Scene.h"

// ��� Mesh
void Scene::addMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Mesh* mesh = new Mesh(vertices, indices);
    meshes.push_back(mesh);
}

// ɾ�� Mesh
void Scene::removeMesh(int index) {
    if (index >= 0 && index < meshes.size()) {
        delete meshes[index];
        meshes.erase(meshes.begin() + index);
    }
}

// ���� Mesh ����
void Scene::updateMeshData(int index, const std::vector<float>& newVertices) {
    if (index >= 0 && index < meshes.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, meshes[index]->getVBO());
        glBufferSubData(GL_ARRAY_BUFFER, 0, newVertices.size() * sizeof(float), newVertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// ��Ⱦ����
void Scene::render() {
    for (const auto& mesh : meshes) {
        mesh->Draw();
    }
}

// ����������Դ
void Scene::clear() {
    for (auto& mesh : meshes) {
        delete mesh; // ɾ��ÿ�� Mesh
    }
    meshes.clear(); // �������
}