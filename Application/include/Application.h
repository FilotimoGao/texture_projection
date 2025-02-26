#pragma once
#include "Utils.h"
#include "Scene.h"
#include "Camera.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <filesystem>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_internal.h>
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
    void setAppWidth(int width) { appWidth = width; }
    void setAppHeight(int height) { appHeight = height; }
    void createModels(const std::vector<std::string>& filePaths);

private:
    Application();
    static Application* instance;

    uint32_t appWidth;
    uint32_t appHeight;
    GLFWwindow* appWindow;

    Scene scene;
    GLuint shaderProgram; // 着色器程序

    std::string selecImgPath; // 存储选中的图片路径
    unsigned int selecImgTexID;  // 存储选中的图片纹理ID

    void workspaceUI();
    //void runImageProcessing(const std::string& imagePath, glm::vec2 targetPoints[4]);

    // 关于选定图片目标范围的变量
    bool selectTarget;
    int targetWidth, targetHeight;
    glm::vec2 targetPoints[4]; // 用于记录最终的切分范围（上左，上右，下右，下左）
    glm::vec2 nextTargetPoints[4]; // 用于记录切分过程中的切分范围（上左，上右，下右，下左）
    void selectTargetUI();

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
