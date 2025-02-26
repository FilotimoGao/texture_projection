#pragma once
#include <glad/glad.h>
#include <Python.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#define STB_IMAGE_IMPLEMENTATION

GLuint loadTexture(const char* path, bool if_flip = true);
GLuint loadTexture(const char* path, int& width, int& height, bool if_flip = true);
bool loadShaders(GLuint& shaderProgram);
std::string convertSlashes(const std::string& path);
std::string convertPath(const std::string& path);

void initializePython(const std::string& lcnnPathStr);
void finalizePython();
void processImageWithCutPy();
void processImageWithRangePy(const std::string& imagePath, glm::vec2 targetPoints[4]);
