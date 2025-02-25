#pragma once
#include <glad/glad.h>
#include <Python.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION

GLuint loadTexture(const char* path, bool if_flip = true);
GLuint loadTexture(const char* path, int& width, int& height, bool if_flip = true);
bool loadShaders(GLuint& shaderProgram);
std::string convertSlashes(const std::string& path);
void processImageWithDemoPy(const std::string& imagePath);