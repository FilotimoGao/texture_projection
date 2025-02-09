#pragma once
#include "Texture.h"
#include "Scene.h"
#include "Camera.h"
#include <iostream>
#include <string>
#include <vector>
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
    GLuint shaderProgram; // 着色器程序

    bool loadShaders();

    // === Camera ===
    Camera camera; // 添加相机对象
    float lastX, lastY; // 鼠标位置
    bool firstMouse;    // 标记鼠标是否是第一次移动
    float deltaTime;    // 每帧时间间隔
    float lastFrame;    // 上一帧的时间
    bool isMousePressed; // 鼠标是否按下
    
    std::vector<int> selectedModelIndices; // 存储选中的模型索引

    // 添加相机输入处理函数
    void processInput();
    void onMousePress(bool pressed); // 鼠标按下处理
    void mouse_callback(double xpos, double ypos);
    void scroll_callback(double yoffset);
};
