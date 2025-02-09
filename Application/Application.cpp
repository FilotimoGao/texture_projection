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
    selectedModelIndices.clear();

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); // 确保深度测试开启

    // 设置鼠标移动和滚轮回调
    glfwSetCursorPosCallback(appWindow, [](GLFWwindow* window, double xpos, double ypos) {
        Application::getInstance()->mouse_callback(xpos, ypos);
        });
    glfwSetScrollCallback(appWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        Application::getInstance()->scroll_callback(yoffset);
        });

    // 捕获鼠标
    glfwSetInputMode(appWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // 设置鼠标按下/释放的回调
    glfwSetMouseButtonCallback(appWindow, [](GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                Application::getInstance()->onMousePress(true); // 鼠标按下
            }
            else if (action == GLFW_RELEASE) {
                Application::getInstance()->onMousePress(false); // 鼠标释放
            }
        }
        });

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
        // 每帧时间逻辑
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput();

        // 开始 ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 控制面板
        ImGui::Begin("Control Panel");

        // 显示所有模型，并支持多选
        for (size_t i = 0; i < model_num; ++i) {
            char label[32];
            sprintf(label, "Model %d", (int)i);

            // 检查当前模型是否已被选中
            bool isSelected = std::find(selectedModelIndices.begin(), selectedModelIndices.end(), i) != selectedModelIndices.end();

            // ImGui 的选择逻辑
            if (ImGui::Selectable(label, isSelected)) {
                if (ImGui::GetIO().KeyCtrl) { // 按住 Ctrl 支持多选
                    if (isSelected) {
                        // 如果已选择，则取消选择
                        selectedModelIndices.erase(std::remove(selectedModelIndices.begin(), selectedModelIndices.end(), i), selectedModelIndices.end());
                    }
                    else {
                        // 未选择则添加到多选列表
                        selectedModelIndices.push_back(i);
                    }
                }
                else {
                    // 未按住 Ctrl 时，清空其他选择，仅选择当前模型
                    selectedModelIndices.clear();
                    selectedModelIndices.push_back(i);
                }
            }
        }

        // 如果有选中的模型，显示批量控制面板
        if (!selectedModelIndices.empty()) {
            ImGui::Separator(); // 分隔线
            ImGui::Text("Batch Transform and Texture Controls:");

            // ---- 1. 批量修改 Transform ----
            static glm::vec3 batchPosition(0.0f);
            static glm::vec3 batchRotation(0.0f);
            static glm::vec2 batchSize(1.0f);

            if (ImGui::CollapsingHeader("Transform")) {
                ImGui::InputFloat3("Batch Position", &batchPosition[0]);
                ImGui::InputFloat3("Batch Rotation", &batchRotation[0]);
                ImGui::InputFloat2("Batch Size", &batchSize[0]);

                if (ImGui::Button("Apply Transform")) {
                    for (int index : selectedModelIndices) {
                        Model* model = scene.getModel(index);
                        if (model) {
                            Transform transform = model->getTransform();
                            transform.position = batchPosition;
                            transform.rotation = batchRotation;
                            transform.size = batchSize;
                            model->setTransform(transform);
                        }
                    }
                }
            }

            // ---- 2. 批量修改纹理 ----
            static unsigned int batchDefaultTexture = 0;
            static unsigned int batchProjectionTexture = 0;
            static int currentTextureMode = 0; // 0: UV Mapping, 1: Projection Mapping

            if (ImGui::CollapsingHeader("Texture")) {
                ImGui::Text("Texture Mode:");
                ImGui::RadioButton("UV Mapping", &currentTextureMode, 0);
                ImGui::RadioButton("Projection Mapping", &currentTextureMode, 1);

                if (currentTextureMode == 0) {
                    ImGui::Text("Set Default Texture for Selected:");
                }
                else {
                    ImGui::Text("Set Projection Texture for Selected:");
                }

                if (ImGui::Button("Apply Texture Mode")) {
                    for (int index : selectedModelIndices) {
                        Model* model = scene.getModel(index);
                        if (model) {
                            if (currentTextureMode == 0) {
                                model->setTextureMode(TextureMode::UV_MAPPING);
                            }
                            else {
                                model->setTextureMode(TextureMode::PROJECTION_MAPPING);
                            }
                        }
                    }
                }

                ImGui::Separator(); // 分隔线
                if (ImGui::Button("Choose Texture")) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseBatchTexture",
                        "Choose Texture",
                        ".png,.jpg,.jpeg",
                        config
                    );
                }

                if (ImGuiFileDialog::Instance()->Display("ChooseBatchTexture")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        std::string texturePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        unsigned int newTexture = loadTexture(texturePath.c_str());
                        batchDefaultTexture = newTexture;
                        batchProjectionTexture = newTexture;

                        // 应用纹理到所有被选中的模型
                        for (int index : selectedModelIndices) {
                            Model* model = scene.getModel(index);
                            if (model) {
                                model->setTexture(batchDefaultTexture);
                                model->setProjectionTexture(batchProjectionTexture);
                            }
                        }
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
                        -0.5f, -0.5f, -0.2f * (model_num + 1), 0.0f, 0.0f,
                         0.5f, -0.5f, -0.2f * (model_num + 1), 1.0f, 0.0f,
                         0.5f,  0.5f, -0.2f * (model_num + 1), 1.0f, 1.0f,
                        -0.5f,  0.5f, -0.2f * (model_num + 1), 0.0f, 1.0f
                    };
                    std::vector<unsigned int> indices = {
                        0, 1, 2,
                        2, 3, 0
                    };

                    // 为每个模型加载独立的纹理
                    unsigned int texture = loadTexture(filePath.c_str());
                    unsigned int projectionTexture = loadTexture(filePath.c_str());

                    // 创建模型并为其设置独立的纹理和投影纹理
                    scene.addModel(vertices, indices);

                    Model* model = scene.getModel(model_num);
                    model->setTexture(texture);
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
        glm::mat4 view = camera.GetViewMatrix(); // 使用相机类生成视图矩阵
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)appWidth / (float)appHeight,
            0.1f,
            100.0f
        );

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
        vec4 texColor;

        if (textureMode == 0) { // UV 映射
            texColor = texture(texture1, TexCoords);
        } else { // 投影映射
            vec3 projCoords = ProjectedCoords.xyz / ProjectedCoords.w;
            projCoords = projCoords * 0.5 + 0.5; // [-1, 1] 转换到 [0, 1]

            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
                texColor = vec4(0.0, 0.0, 0.0, 0.0); // 超出范围返回完全透明
            } else {
                texColor = texture(projectionTexture, projCoords.xy);
            }
        }

        // 丢弃透明片段
        if (texColor.a < 0.1) {
            discard; // 透明部分丢弃，不渲染
        }

        FragColor = texColor; // 使用纹理颜色和透明度
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
    : appWidth(0), appHeight(0), appWindow(nullptr), shaderProgram(0),
    camera(glm::vec3(0.0f, 0.0f, 3.0f)), // 初始化相机，默认位置为 (0, 0, 3)
    lastX(400.0f), lastY(300.0f), firstMouse(true),
    deltaTime(0.0f), lastFrame(0.0f), isMousePressed(false) {}

Application::~Application() {
    releaseInstance();
}

void Application::processInput() {
    if (glfwGetKey(appWindow, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard("FORWARD", deltaTime);
    if (glfwGetKey(appWindow, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard("BACKWARD", deltaTime);
    if (glfwGetKey(appWindow, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard("LEFT", deltaTime);
    if (glfwGetKey(appWindow, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard("RIGHT", deltaTime);
}

void Application::onMousePress(bool pressed) {
    isMousePressed = pressed;

    if (pressed) {
        // 获取鼠标当前位置，初始化 lastX 和 lastY
        double xpos, ypos;
        glfwGetCursorPos(appWindow, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    }
}

// 处理鼠标移动
void Application::mouse_callback(double xpos, double ypos) {
    if (!isMousePressed) {
        // 如果鼠标没有按下，直接返回
        return;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; // 注意 y 是反向的，因为窗口坐标系与 OpenGL 坐标系不同

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

// 处理鼠标滚动
void Application::scroll_callback(double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
