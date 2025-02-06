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
        // ��ʼ ImGui ֡
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // �������
        ImGui::Begin("Control Panel");

        // ��ʾ����ģ�ͣ���֧��ѡ��
        for (size_t i = 0; i < model_num; ++i) {
            char label[32];
            sprintf(label, "Model %d", (int)i);
            if (ImGui::Selectable(label, scene.getSelectedModelIndex() == (int)i)) {
                scene.setSelectedModelIndex((int)i);
            }
        }

        // �����ѡ�е�ģ�ͣ���ʾ�������
        int selectedIndex = scene.getSelectedModelIndex();
        if (selectedIndex >= 0) {
            Model* selectedModel = scene.getModel(selectedIndex);
            Transform& transform = selectedModel->getTransform();

            // ---- 1. �任���� ----
            ImGui::Text("Transform Controls:");
            ImGui::InputFloat3("Position", &transform.position[0]);
            ImGui::InputFloat3("Rotation", &transform.rotation[0]);
            ImGui::InputFloat2("Size", &transform.size[0]);

            // ����ģ�;���
            selectedModel->setTransform(transform);

            // ---- 2. ����ģʽ���� ----
            ImGui::Separator(); // ��ӷָ��ߣ����ֱ任���ƺ�����ģʽ����
            ImGui::Text("Texture Mode Controls:");

            // ��ǰģ�͵�����ģʽ
            static int currentMode = 0;
            if (selectedModel->getTextureMode() == TextureMode::UV_MAPPING) {
                currentMode = 0;
            }
            else {
                currentMode = 1;
            }

            // ����ģʽ��ѡ��ť
            ImGui::RadioButton("UV Mapping", &currentMode, 0);
            ImGui::RadioButton("Projection Mapping", &currentMode, 1);

            // ����ģ�͵�����ģʽ
            if (currentMode == 0) {
                selectedModel->setTextureMode(TextureMode::UV_MAPPING);
            }
            else {
                selectedModel->setTextureMode(TextureMode::PROJECTION_MAPPING);
            }

            // ---- 3. ����ģ������ ----
            ImGui::Separator(); // ��ӷָ��ߣ���������ģʽ������ѡ��
            ImGui::Text("Texture Controls:");

            // ����Ĭ������
            if (ImGui::Button("Set Default Texture")) {
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseDefaultTexture",  // �Ի���Ψһ��ʶ��
                    "Choose Texture", // �Ի������
                    ".png,.jpg,.jpeg",         // ���Ժ��� vFilters��ʹ�� config.filters��
                    config           // ʹ������
                );
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseDefaultTexture")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string texturePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    unsigned int newTexture = loadTexture(texturePath.c_str());
                    selectedModel->setTexture(newTexture); // ����Ĭ������
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // �����ǰģʽ��ͶӰ��������������ͶӰ����
            if (currentMode == 1) { // ����ͶӰģʽ����ʾ����ͶӰ����ť
                if (ImGui::Button("Set Projection Texture")) {
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ChooseProjectionTexture",  // �Ի���Ψһ��ʶ��
                        "Choose Texture", // �Ի������
                        ".png,.jpg,.jpeg",         // ���Ժ��� vFilters��ʹ�� config.filters��
                        config           // ʹ������
                    );
                }
                if (ImGuiFileDialog::Instance()->Display("ChooseProjectionTexture")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        std::string projectionTexturePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        unsigned int newProjectionTexture = loadTexture(projectionTexturePath.c_str());
                        selectedModel->setProjectionTexture(newProjectionTexture); // ����ͶӰ����
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
                        -0.5f, -0.5f, -0.5f * (model_num + 1), 0.0f, 0.0f,
                         0.5f, -0.5f, -0.5f * (model_num + 1), 1.0f, 0.0f,
                         0.5f,  0.5f, -0.5f * (model_num + 1), 1.0f, 1.0f,
                        -0.5f,  0.5f, -0.5f * (model_num + 1), 0.0f, 1.0f
                    };
                    std::vector<unsigned int> indices = {
                        0, 1, 2,
                        2, 3, 0
                    };

                    // ����Ĭ������
                    unsigned int texture = loadTexture("path/to/default_texture.png");

                    // ����ͶӰ����
                    unsigned int projectionTexture = loadTexture(filePath.c_str());

                    // ����ģ�Ͳ���������
                    scene.addModel(vertices, indices);
                    scene.setModelTexture(model_num, texture);

                    Model* model = scene.getModel(model_num);
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
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)appWidth / (float)appHeight, 0.1f, 100.0f);

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
        if (textureMode == 0) { // UV ӳ��
            FragColor = texture(texture1, TexCoords);
        } else { // ͶӰӳ��
            // ��ͶӰ����Ӳü��ռ�ת������������
            vec3 projCoords = ProjectedCoords.xyz / ProjectedCoords.w;
            projCoords = projCoords * 0.5 + 0.5; // [-1, 1] ת���� [0, 1]

            // ����Ƿ���ͶӰ����Χ��
            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
                FragColor = vec4(0.0, 0.0, 0.0, 1.0); // ������Χ���غ�ɫ
            } else {
                FragColor = texture(projectionTexture, projCoords.xy);
            }
        }
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
    : appWidth(0), appHeight(0), appWindow(nullptr), shaderProgram(0) {}

Application::~Application() {
    releaseInstance();
}
