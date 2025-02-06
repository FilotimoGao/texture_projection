#include "Application.h"
#include<vector>

int model_num = 0;

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

    // 加载着色器
    if (!loadShaders()) {
        return false;
    }

    return true;
}


void Application::update() {
    IGFD::FileDialogConfig config;
    config.countSelectionMax = 20; // 设置多选

    while (!glfwWindowShouldClose(appWindow)) {
        // 开始 ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 控制面板
        ImGui::Begin("Control Panel");

        // 显示所有模型，并支持选择
        for (size_t i = 0; i < model_num; ++i) {
            char label[32];
            sprintf(label, "Model %d", (int)i);
            if (ImGui::Selectable(label, scene.getSelectedModelIndex() == (int)i)) {
                scene.setSelectedModelIndex((int)i);
            }
        }

        // 如果有选中的模型，显示控制面板
        int selectedIndex = scene.getSelectedModelIndex();
        if (selectedIndex >= 0) {
            Model* selectedModel = scene.getModel(selectedIndex);
            Transform& transform = selectedModel->getTransform();

            // ---- 1. 变换控制 ----
            ImGui::Text("Transform Controls:");
            ImGui::InputFloat3("Position", &transform.position[0]);
            ImGui::InputFloat3("Rotation", &transform.rotation[0]);
            ImGui::InputFloat2("Size", &transform.size[0]);

            // 更新模型矩阵
            selectedModel->setTransform(transform);

            // ---- 2. 纹理模式控制 ----
            ImGui::Separator(); // 添加分隔线，区分变换控制和纹理模式控制
            ImGui::Text("Texture Mode Controls:");

            // 当前模型的纹理模式
            static int currentMode = 0;
            if (selectedModel->getTextureMode() == TextureMode::UV_MAPPING) {
                currentMode = 0;
            }
            else {
                currentMode = 1;
            }

            // 纹理模式单选按钮
            ImGui::RadioButton("UV Mapping", &currentMode, 0);
            ImGui::RadioButton("Projection Mapping", &currentMode, 1);

            // 更新模型的纹理模式
            if (currentMode == 0) {
                selectedModel->setTextureMode(TextureMode::UV_MAPPING);
            }
            else {
                selectedModel->setTextureMode(TextureMode::PROJECTION_MAPPING);
            }

            // ---- 3. 更改模型纹理 ----
            ImGui::Separator(); // 添加分隔线，区分纹理模式和纹理选择
            ImGui::Text("Texture Controls:");

            // 设置默认纹理
            if (ImGui::Button("Set Default Texture")) {
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseDefaultTexture",  // 对话框唯一标识符
                    "Choose Texture", // 对话框标题
                    ".png,.jpg,.jpeg",         // 可以忽略 vFilters（使用 config.filters）
                    config           // 使用配置
                );
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseDefaultTexture")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string texturePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    unsigned int newTexture = loadTexture(texturePath.c_str());
                    selectedModel->setTexture(newTexture); // 设置默认纹理
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // 如果当前模式是投影纹理，则允许设置投影纹理
            if (currentMode == 1) { // 仅在投影模式下显示设置投影纹理按钮
                if (ImGui::Button("Set Projection Texture")) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseProjectionTexture",  // 对话框唯一标识符
                        "Choose Texture", // 对话框标题
                        ".png,.jpg,.jpeg",         // 可以忽略 vFilters（使用 config.filters）
                        config           // 使用配置
                    );
                }
                if (ImGuiFileDialog::Instance()->Display("ChooseProjectionTexture")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        std::string projectionTexturePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        unsigned int newProjectionTexture = loadTexture(projectionTexturePath.c_str());
                        selectedModel->setProjectionTexture(newProjectionTexture); // 设置投影纹理
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }
        }

        // 文件选择和加载
        if (ImGui::Button("Select Images")) {
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 20; // 设置多选
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseImages",  // 对话框唯一标识符
                "Choose Images", // 对话框标题
                ".png,.jpg,.jpeg",         // 可以忽略 vFilters（使用 config.filters）
                config           // 使用配置
            );
        }

        // 加载选中的图片并生成模型
        if (ImGuiFileDialog::Instance()->Display("ChooseImages")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                auto selectedFiles = ImGuiFileDialog::Instance()->GetSelection();
                scene.clear();
                model_num = 0;

                for (auto& [key, filePath] : selectedFiles) {
                    std::vector<float> vertices = {
                        -0.5f, -0.5f, -0.5f * (model_num + 1), 0.0f, 0.0f,
                         0.5f, -0.5f, -0.5f * (model_num + 1), 1.0f, 0.0f,
                         0.5f,  0.5f, -0.5f * (model_num + 1), 1.0f, 1.0f,
                        -0.5f,  0.5f, -0.5f * (model_num + 1), 0.0f, 1.0f
                    };
                    std::vector<unsigned int> indices = {
                        0, 1, 2,
                        2, 3, 0
                    };

                    // 加载默认纹理
                    unsigned int texture = loadTexture("path/to/default_texture.png");

                    // 加载投影纹理
                    unsigned int projectionTexture = loadTexture(filePath.c_str());

                    // 创建模型并设置属性
                    scene.addModel(vertices, indices);
                    scene.setModelTexture(model_num, texture);

                    Model* model = scene.getModel(model_num);
                    model->setProjectionTexture(projectionTexture);
                    model->setProjectionMatrix(glm::mat4(1.0f)); // 默认投影矩阵
                    model->setTextureMode(TextureMode::UV_MAPPING); // 默认使用 UV 映射

                    model_num++;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::End();

        // 渲染场景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // 设置视图和投影矩阵
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)appWidth / (float)appHeight, 0.1f, 100.0f);

        // 投影矩阵，用于模拟投影仪的位置和方向
        glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
        glm::mat4 lightView = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),  // 投影仪的位置（光源位置）
            glm::vec3(0.0f, 0.0f, 0.0f),  // 投影仪的目标点
            glm::vec3(0.0f, 1.0f, 0.0f)   // 投影仪的上方向
        );
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // 上传矩阵到着色器
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


        // ---- 渲染每个模型 ----
        for (int i = 0; i < model_num; ++i) {
            Model* model = scene.getModel(i);

            // 获取模型的变换矩阵
            glm::mat4 modelMatrix = model->getModelMatrix();

            // 上传模型矩阵到着色器
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

            // 上传投影仪的视图矩阵到着色器
            GLuint lightViewLoc = glGetUniformLocation(shaderProgram, "lightView");
            glUniformMatrix4fv(lightViewLoc, 1, GL_FALSE, glm::value_ptr(lightView));

            // 上传投影仪的投影矩阵到着色器
            GLuint lightProjectionLoc = glGetUniformLocation(shaderProgram, "lightProjection");
            glUniformMatrix4fv(lightProjectionLoc, 1, GL_FALSE, glm::value_ptr(lightProjection));

            // 渲染模型
            model->Draw(shaderProgram);
        }

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(appWindow);
        glfwPollEvents();
    }
}


void Application::destory() {
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
    out vec4 ProjectedCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 lightProjection;
    uniform mat4 lightView;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoords = aTexCoords;
        ProjectedCoords = lightProjection * lightView * model * vec4(aPos, 1.0);
    }
)";

    // 片段着色器代码
    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    in vec4 ProjectedCoords;

    uniform sampler2D texture1;         // 默认纹理
    uniform sampler2D projectionTexture; // 投影纹理
    uniform int textureMode;            // 纹理模式 (0: UV 映射, 1: 投影映射)

    out vec4 FragColor;

    void main() {
        if (textureMode == 0) { // UV 映射
            FragColor = texture(texture1, TexCoords);
        } else { // 投影映射
            // 将投影坐标从裁剪空间转换到纹理坐标
            vec3 projCoords = ProjectedCoords.xyz / ProjectedCoords.w;
            projCoords = projCoords * 0.5 + 0.5; // [-1, 1] 转换到 [0, 1]

            // 检查是否在投影纹理范围内
            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0); // 超出范围返回黑色
            } else {
                FragColor = texture(projectionTexture, projCoords.xy);
            }
        }
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
    : appWidth(0), appHeight(0), appWindow(nullptr), shaderProgram(0) {}

Application::~Application() {
    releaseInstance();
}
