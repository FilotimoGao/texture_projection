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
    GLuint shaderProgram; // ��ɫ������

    bool loadShaders();

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
