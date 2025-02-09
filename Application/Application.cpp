#include "Application.h"
#include<vector>

int model_num = 0;

// ���ڴ�С�����ص�
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
    // ��ʼ�����ڴ�С����¼
    appWidth = width;
    appHeight = height;
    selectedModelIndices.clear();

    // ��ʼ�� GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // �������ڲ� OpenGL ������
    appWindow = glfwCreateWindow(appWidth, appHeight, "Image Viewer", nullptr, nullptr);
    if (!appWindow) {
        glfwTerminate();
        std::cout << "Failed to create GLFW window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(appWindow);
    glfwSetFramebufferSizeCallback(appWindow, framebuffer_size_callback);

    // ��ʼ�� OpenGL loader (GLAD)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); // ȷ����Ȳ��Կ���

    // ��������ƶ��͹��ֻص�
    glfwSetCursorPosCallback(appWindow, [](GLFWwindow* window, double xpos, double ypos) {
        Application::getInstance()->mouse_callback(xpos, ypos);
        });
    glfwSetScrollCallback(appWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        Application::getInstance()->scroll_callback(yoffset);
        });

    // �������
    glfwSetInputMode(appWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // ������갴��/�ͷŵĻص�
    glfwSetMouseButtonCallback(appWindow, [](GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                Application::getInstance()->onMousePress(true); // ��갴��
            }
            else if (action == GLFW_RELEASE) {
                Application::getInstance()->onMousePress(false); // ����ͷ�
            }
        }
        });

    // ���� ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // ���� ImGui ��
    ImGui_ImplGlfw_InitForOpenGL(appWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ������ɫ��
    if (!loadShaders()) {
        return false;
    }

    return true;
}


void Application::update() {
    IGFD::FileDialogConfig config;
    config.countSelectionMax = 20; // ���ö�ѡ

    while (!glfwWindowShouldClose(appWindow)) {
        // ÿ֡ʱ���߼�
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ��������
        processInput();

        // ��ʼ ImGui ֡
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // �������
        ImGui::Begin("Control Panel");

        // ��ʾ����ģ�ͣ���֧�ֶ�ѡ
        for (size_t i = 0; i < model_num; ++i) {
            char label[32];
            sprintf(label, "Model %d", (int)i);

            // ��鵱ǰģ���Ƿ��ѱ�ѡ��
            bool isSelected = std::find(selectedModelIndices.begin(), selectedModelIndices.end(), i) != selectedModelIndices.end();

            // ImGui ��ѡ���߼�
            if (ImGui::Selectable(label, isSelected)) {
                if (ImGui::GetIO().KeyCtrl) { // ��ס Ctrl ֧�ֶ�ѡ
                    if (isSelected) {
                        // �����ѡ����ȡ��ѡ��
                        selectedModelIndices.erase(std::remove(selectedModelIndices.begin(), selectedModelIndices.end(), i), selectedModelIndices.end());
                    }
                    else {
                        // δѡ������ӵ���ѡ�б�
                        selectedModelIndices.push_back(i);
                    }
                }
                else {
                    // δ��ס Ctrl ʱ���������ѡ�񣬽�ѡ��ǰģ��
                    selectedModelIndices.clear();
                    selectedModelIndices.push_back(i);
                }
            }
        }

        // �����ѡ�е�ģ�ͣ���ʾ�����������
        if (!selectedModelIndices.empty()) {
            ImGui::Separator(); // �ָ���
            ImGui::Text("Batch Transform and Texture Controls:");

            // ---- 1. �����޸� Transform ----
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

            // ---- 2. �����޸����� ----
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

                ImGui::Separator(); // �ָ���
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

                        // Ӧ���������б�ѡ�е�ģ��
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

        // �ļ�ѡ��ͼ���
        if (ImGui::Button("Select Images")) {
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 20; // ���ö�ѡ
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseImages",  // �Ի���Ψһ��ʶ��
                "Choose Images", // �Ի������
                ".png,.jpg,.jpeg",         // ���Ժ��� vFilters��ʹ�� config.filters��
                config           // ʹ������
            );
        }

        // ����ѡ�е�ͼƬ������ģ��
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

                    // Ϊÿ��ģ�ͼ��ض���������
                    unsigned int texture = loadTexture(filePath.c_str());
                    unsigned int projectionTexture = loadTexture(filePath.c_str());

                    // ����ģ�Ͳ�Ϊ�����ö����������ͶӰ����
                    scene.addModel(vertices, indices);

                    Model* model = scene.getModel(model_num);
                    model->setTexture(texture);
                    model->setProjectionTexture(projectionTexture);
                    model->setProjectionMatrix(glm::mat4(1.0f)); // Ĭ��ͶӰ����
                    model->setTextureMode(TextureMode::UV_MAPPING); // Ĭ��ʹ�� UV ӳ��

                    model_num++;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::End();

        // ��Ⱦ����
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // ������ͼ��ͶӰ����
        glm::mat4 view = camera.GetViewMatrix(); // ʹ�������������ͼ����
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)appWidth / (float)appHeight,
            0.1f,
            100.0f
        );

        // ͶӰ��������ģ��ͶӰ�ǵ�λ�úͷ���
        glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
        glm::mat4 lightView = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),  // ͶӰ�ǵ�λ�ã���Դλ�ã�
            glm::vec3(0.0f, 0.0f, 0.0f),  // ͶӰ�ǵ�Ŀ���
            glm::vec3(0.0f, 1.0f, 0.0f)   // ͶӰ�ǵ��Ϸ���
        );
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // �ϴ�������ɫ��
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // ---- ��Ⱦÿ��ģ�� ----
        for (int i = 0; i < model_num; ++i) {
            Model* model = scene.getModel(i);

            // ��ȡģ�͵ı任����
            glm::mat4 modelMatrix = model->getModelMatrix();

            // �ϴ�ģ�;�����ɫ��
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

            // �ϴ�ͶӰ�ǵ���ͼ������ɫ��
            GLuint lightViewLoc = glGetUniformLocation(shaderProgram, "lightView");
            glUniformMatrix4fv(lightViewLoc, 1, GL_FALSE, glm::value_ptr(lightView));

            // �ϴ�ͶӰ�ǵ�ͶӰ������ɫ��
            GLuint lightProjectionLoc = glGetUniformLocation(shaderProgram, "lightProjection");
            glUniformMatrix4fv(lightProjectionLoc, 1, GL_FALSE, glm::value_ptr(lightProjection));

            // ��Ⱦģ��
            model->Draw(shaderProgram);
        }

        // ��Ⱦ ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(appWindow);
        glfwPollEvents();
    }
}



void Application::destory() {
    // ��������Դ
    scene.clear();

    // ���� ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // ���ٴ��ڲ���ֹ GLFW
    glfwDestroyWindow(appWindow);
    glfwTerminate();
}


bool Application::loadShaders() {
    // ������ɫ������
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

    // Ƭ����ɫ������
    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    in vec4 ProjectedCoords;

    uniform sampler2D texture1;         // Ĭ������
    uniform sampler2D projectionTexture; // ͶӰ����
    uniform int textureMode;            // ����ģʽ (0: UV ӳ��, 1: ͶӰӳ��)

    out vec4 FragColor;

    void main() {
        vec4 texColor;

        if (textureMode == 0) { // UV ӳ��
            texColor = texture(texture1, TexCoords);
        } else { // ͶӰӳ��
            vec3 projCoords = ProjectedCoords.xyz / ProjectedCoords.w;
            projCoords = projCoords * 0.5 + 0.5; // [-1, 1] ת���� [0, 1]

            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
                texColor = vec4(0.0, 0.0, 0.0, 0.0); // ������Χ������ȫ͸��
            } else {
                texColor = texture(projectionTexture, projCoords.xy);
            }
        }

        // ����͸��Ƭ��
        if (texColor.a < 0.1) {
            discard; // ͸�����ֶ���������Ⱦ
        }

        FragColor = texColor; // ʹ��������ɫ��͸����
    }
    )";

    // ���붥����ɫ��
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

    // ����Ƭ����ɫ��
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // ������ɫ������
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

    // ɾ����ɫ��
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}


Application::Application()
    : appWidth(0), appHeight(0), appWindow(nullptr), shaderProgram(0),
    camera(glm::vec3(0.0f, 0.0f, 3.0f)), // ��ʼ�������Ĭ��λ��Ϊ (0, 0, 3)
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
        // ��ȡ��굱ǰλ�ã���ʼ�� lastX �� lastY
        double xpos, ypos;
        glfwGetCursorPos(appWindow, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    }
}

// ��������ƶ�
void Application::mouse_callback(double xpos, double ypos) {
    if (!isMousePressed) {
        // ������û�а��£�ֱ�ӷ���
        return;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; // ע�� y �Ƿ���ģ���Ϊ��������ϵ�� OpenGL ����ϵ��ͬ

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

// ����������
void Application::scroll_callback(double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
