#pragma once
#include "Texture.h"
#include "Scene.h"
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/ImGuiFileDialog.h>

#define app Application::getInstance()

class Application {
public:
    ~Application();
    static Application* getInstance();
    static void releaseInstance();

    bool init(const int& width, const int& height);
    void update();
    void destory();

private:
    Application();
    static Application* instance;

    uint32_t appWidth;
    uint32_t appHeight;
    GLFWwindow* appWindow;

    Scene scene;
    GLuint shaderProgram; // ×ÅÉ«Æ÷³ÌÐò

    bool loadShaders();
};
