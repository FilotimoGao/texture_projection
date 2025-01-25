#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include"Mesh.h"

class Scene {
public:
    void addMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void removeMesh(int index);
    void updateMeshData(int index, const std::vector<float>& newVertices);
    void render();
    void clear();

private:
    std::vector<Mesh*> meshes; // 存储所有物体
};