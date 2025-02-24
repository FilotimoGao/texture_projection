#pragma once
#include <glad/glad.h>
#include <Python.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION

GLuint loadTexture(const char* path);
bool loadShaders(GLuint& shaderProgram);
void processImageWithDemoPy(const std::string& imagePath);