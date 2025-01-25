#include"Scene.h"

// 添加 Mesh
void Scene::addMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Mesh* mesh = new Mesh(vertices, indices);
    meshes.push_back(mesh);
}

// 删除 Mesh
void Scene::removeMesh(int index) {
    if (index >= 0 && index < meshes.size()) {
        delete meshes[index];
        meshes.erase(meshes.begin() + index);
    }
}

// 更新 Mesh 数据
void Scene::updateMeshData(int index, const std::vector<float>& newVertices) {
    if (index >= 0 && index < meshes.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, meshes[index]->getVBO());
        glBufferSubData(GL_ARRAY_BUFFER, 0, newVertices.size() * sizeof(float), newVertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// 渲染场景
void Scene::render() {
    for (const auto& mesh : meshes) {
        mesh->Draw();
    }
}

// 清理所有资源
void Scene::clear() {
    for (auto& mesh : meshes) {
        delete mesh; // 删除每个 Mesh
    }
    meshes.clear(); // 清空容器
}