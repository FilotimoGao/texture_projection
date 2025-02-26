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
    GLuint shaderProgram; // ��ɫ������

    std::string selecImgPath; // �洢ѡ�е�ͼƬ·��
    unsigned int selecImgTexID;  // �洢ѡ�е�ͼƬ����ID

    void workspaceUI();
    //void runImageProcessing(const std::string& imagePath, glm::vec2 targetPoints[4]);

    // ����ѡ��ͼƬĿ�귶Χ�ı���
    bool selectTarget;
    int targetWidth, targetHeight;
    glm::vec2 targetPoints[4]; // ���ڼ�¼���յ��зַ�Χ���������ң����ң�����
    glm::vec2 nextTargetPoints[4]; // ���ڼ�¼�зֹ����е��зַ�Χ���������ң����ң�����
    void selectTargetUI();

    // === Camera ===
    Camera camera; // ����������
    float lastX, lastY; // ���λ��
    bool firstMouse;    // �������Ƿ��ǵ�һ���ƶ�
    float deltaTime;    // ÿ֡ʱ����
    float lastFrame;    // ��һ֡��ʱ��
    bool isMousePressed; // ����Ƿ���
    
    std::vector<int> selectedModelIndices; // �洢ѡ�е�ģ������

    // ���������봦����
    void processInput();
    void onMousePress(bool pressed); // ��갴�´���
    void mouse_callback(double xpos, double ypos);
    void scroll_callback(double yoffset);
};
