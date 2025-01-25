#include "Application.h"
#include<vector>

// 窗口大小调整回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

Application* Application::instance = nullptr;

Application* Application::getInstance() {
    if (instance == nullptr) {
        instance = new Application();
    }
    return instance;
}

void Application::releaseInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

bool Application::init(const int& width, const int& height) {
    // 初始化窗口大小并记录
    appWidth = width;
    appHeight = height;

    // 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // 创建窗口并 OpenGL 上下文
    appWindow = glfwCreateWindow(appWidth, appHeight, "Image Viewer", nullptr, nullptr);
    if (!appWindow) {
        glfwTerminate();
        std::cout << "Failed to create GLFW window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(appWindow);
    glfwSetFramebufferSizeCallback(appWindow, framebuffer_size_callback);

    // 初始化 OpenGL loader (GLAD)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // 设置 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // 设置 ImGui 绑定
    ImGui_ImplGlfw_InitForOpenGL(appWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 初始化平面顶点数据
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices = {
        // 位置          // 纹理坐标
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f
    };
    indices = {
        0, 1, 2,
        2, 3, 0
    };

    scene.addMesh(vertices, indices);

    // 加载着色器
    if (!loadShaders()) {
        return false;
    }

    return true;
}


void Application::update() {
    while (!glfwWindowShouldClose(appWindow)) {
        // 开始 ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 控制面板
        ImGui::Begin("Control Panel");
        ImGui::InputText("Image Path", &imagePath[0], imagePath.size());
        if (ImGui::Button("Load Image")) {
            textureID = loadTexture(imagePath.c_str());
        }
        ImGui::InputFloat3("Position", &plane.position[0]);
        ImGui::InputFloat3("Rotation", &plane.rotation[0]);
        ImGui::InputFloat2("Size", &plane.size[0]);
        ImGui::End();

        // 计算模型矩阵
        planeModelMatrix = glm::mat4(1.0f);
        planeModelMatrix = glm::translate(planeModelMatrix, plane.position);
        planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(plane.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(plane.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(plane.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(plane.size, 1.0f));

        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 渲染平面
        if (textureID > 0) {
            ImGui::Begin("Image Display"); // 图片展示区
            ImGui::Text("Image Loaded:");
            ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(400, 300)); // 展示图片
            ImGui::End();

            glUseProgram(shaderProgram);

            glm::mat4 view = glm::lookAt(
                glm::vec3(0.0f, 0.0f, 3.0f),  // 摄像机位置
                glm::vec3(0.0f, 0.0f, 0.0f),  // 目标位置
                glm::vec3(0.0f, 1.0f, 0.0f)   // 上方向
            );
            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),          // FOV
                (float)appWidth / (float)appHeight, // 屏幕宽高比
                0.1f, 100.0f                  // 近远平面
            );

            // 上传到着色器
            GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
            GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(planeModelMatrix));

            glActiveTexture(GL_TEXTURE0); // 激活纹理单元 0
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0); // 绑定到纹理单元 0
            scene.render();
        }

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(appWindow);
        glfwPollEvents();
    }
}


void Application::destory() {
    // 释放纹理资源
    if (textureID > 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    // 清理场景资源
    scene.clear();

    // 清理 ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // 销毁窗口并终止 GLFW
    glfwDestroyWindow(appWindow);
    glfwTerminate();
}


bool Application::loadShaders() {
    // 顶点着色器代码
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoords = aTexCoords;
    }
)";

    // 片段着色器代码
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoords;

        uniform sampler2D texture1;

        void main() {
            FragColor = texture(texture1, TexCoords);
        }
    )";

    // 编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINK_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // 删除着色器
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}


Application::Application()
    : appWidth(0), appHeight(0), appWindow(nullptr), plane({}) 
{
    imagePath = std::string(256, '\0');

    plane.position = glm::vec3(0.0f);
    plane.rotation = glm::vec3(0.0f);
    plane.size = glm::vec2(1.0f);
}

Application::~Application() {
    releaseInstance();
}
