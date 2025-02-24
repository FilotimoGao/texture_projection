#include "Application.h"
#include<vector>
#include <algorithm>
#include <sstream>
#include <utility>

int model_num = 0;

// ��Ӵ˺�������ȡ�ļ����е�����
int extractNumberFromFilename(const std::string& filename) {
    std::string numberString;
    for (char c : filename) {
        if (isdigit(c)) {
            numberString += c; // �ռ����������ַ�
        }
    }
    return numberString.empty() ? -1 : std::stoi(numberString); // ���û�������򷵻� -1
}

// ���ڴ�С�����ص�
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application::getInstance()->setAppWidth(width); // ���´��ڿ��
    Application::getInstance()->setAppHeight(height); // ���´��ڸ߶�
    glViewport(width * 0.3f, 0, width * 0.7f, height); // ���� OpenGL ��Ⱦ����
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
    // ���ڳ�ʼ��
    appWidth = width;
    appHeight = height;
    selectedModelIndices.clear();
    glfwInit();
    appWindow = glfwCreateWindow(appWidth, appHeight, "Image Viewer", nullptr, nullptr);
    glfwMakeContextCurrent(appWindow);
    glfwSetFramebufferSizeCallback(appWindow, framebuffer_size_callback);

    // OpenGL loader (GLAD) ��ʼ��
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // �������ģʽ�ͻص�
    glfwSetInputMode(appWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(appWindow, [](GLFWwindow* window, double xpos, double ypos) {
        Application::getInstance()->mouse_callback(xpos, ypos);
        });
    glfwSetScrollCallback(appWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        Application::getInstance()->scroll_callback(yoffset);
        });
    glfwSetMouseButtonCallback(appWindow, [](GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                Application::getInstance()->onMousePress(true);
            }
            else if (action == GLFW_RELEASE) {
                Application::getInstance()->onMousePress(false);
            }
        }
        });

    // ����ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // ��ImGui
    ImGui_ImplGlfw_InitForOpenGL(appWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ���� Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (!loadShaders(shaderProgram)) {
        return false;
    }
    return true;
}


void Application::update() {
    while (!glfwWindowShouldClose(appWindow)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput();
        renderUI();
        // ���� OpenGL ��Ⱦ�����Ҳ� 70% ����
        glViewport(appWidth * 0.3f, 0, appWidth * 0.7f, appHeight);
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


void Application::renderUI() {
    IGFD::FileDialogConfig config;
    config.countSelectionMax = 50;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // ���� ImGui ���ڵ�λ�úʹ�С����� 30% ����
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(appWidth * 0.3f, appHeight));
    ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    // ��ʾ����ģ�ͣ���֧�ֶ�ѡ
    for (size_t i = 0; i < model_num; ++i) {
        char label[32];
        sprintf(label, "Model %d", (int)i);
        // ��鵱ǰģ���Ƿ��ѱ�ѡ��
        bool isSelected = std::find(selectedModelIndices.begin(), selectedModelIndices.end(), i) != selectedModelIndices.end();
        // ImGui ��ѡ���߼�����ס Ctrl ֧�ֶ�ѡ������ Ctrl ѡ���������������ѡ��
        if (ImGui::Selectable(label, isSelected)) {
            if (ImGui::GetIO().KeyCtrl) {
                if (isSelected)
                    selectedModelIndices.erase(std::remove(selectedModelIndices.begin(), selectedModelIndices.end(), i), selectedModelIndices.end());
                else
                    selectedModelIndices.push_back(i);
            }
            else {
                selectedModelIndices.clear();
                selectedModelIndices.push_back(i);
            }
        }
    }
    // �����ѡ�е�ģ�ͣ���ʾ�����������
    if (!selectedModelIndices.empty()) {
        ImGui::Separator();
        ImGui::Text("Batch Transform and Texture Controls:");
        // ������ָ�Ĭ�ϰ�ť
        if (ImGui::CollapsingHeader("Camera Controls")) {
            if (ImGui::Button("Reset Camera to Default")) {
                camera.ResetToDefault();
            }
        }
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
            ImGui::Separator();
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
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseImages",
            "Choose Images",
            ".png,.jpg,.jpeg",
            config
        );
    }
    // ����ѡ�е�ͼƬ������ģ��
    if (ImGuiFileDialog::Instance()->Display("ChooseImages")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selectedFiles = ImGuiFileDialog::Instance()->GetSelection();
            scene.clear();
            model_num = 0;
            // ����һ�� vector ���洢�ļ�����·����֮������������
            std::vector<std::pair<int, std::string>> filesWithNumbers;
            for (auto& [key, filePath] : selectedFiles) {
                int number = extractNumberFromFilename(filePath);
                if (number != -1) {
                    filesWithNumbers.emplace_back(number, filePath);
                }
            }
            std::sort(filesWithNumbers.begin(), filesWithNumbers.end());
            for (auto& [key, filePath] : filesWithNumbers) {
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

    // ��python��ɵ�ģ���и��
    // �����µ� ImGui ����λ�úʹ�С
    ImGui::SetNextWindowPos(ImVec2(appWidth * 0.3f + 10, 10));
    ImGui::SetNextWindowSize(ImVec2(appWidth * 0.3f - 20, 200));
    ImGui::Begin("Process Image with demo.py", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    
    if (ImGui::Button("Select Image for Processing")) {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseImageForProcessing",
            "Choose Image",
            ".png,.jpg,.jpeg",
            config
        );
    }
    // ��ʾѡ�е�ͼƬ·��
    if (!selectedImagePath.empty()) {
        ImGui::Text("Selected Image: %s", selectedImagePath.c_str());
    }
    // ���� demo.py �İ�ť
    if (ImGui::Button("Process Image with demo.py")) {
        if (!selectedImagePath.empty()) {
            processImageWithDemoPy(selectedImagePath);
        }
        else {
            ImGui::Text("Please select an image first!");
        }
    }
    ImGui::End();
    // �����ļ�ѡ��Ի���
    if (ImGuiFileDialog::Instance()->Display("ChooseImageForProcessing")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selectedImagePath = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }
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
    double xpos, ypos;
    glfwGetCursorPos(appWindow, &xpos, &ypos);
    // �������Ƿ��� OpenGL ��Ⱦ�����ڣ��Ҳ� 70% ����
    if (xpos > appWidth * 0.3f) {
        isMousePressed = pressed;
        if (pressed) {
            lastX = xpos;
            lastY = ypos;
        }
    }
}
void Application::mouse_callback(double xpos, double ypos) {
    // �������Ƿ��� OpenGL ��Ⱦ�����ڣ��Ҳ� 70% ����
    if (xpos > appWidth * 0.3f) {
        if (!isMousePressed) {
            return;
        }
        float xOffset = xpos - lastX;
        float yOffset = lastY - ypos; // ע�� y �Ƿ���ģ���Ϊ��������ϵ�� OpenGL ����ϵ��ͬ
        lastX = xpos;
        lastY = ypos;
        camera.ProcessMouseMovement(xOffset, yOffset);
    }
}

// ����������
void Application::scroll_callback(double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        // ��� ImGui �����˹������룬����Թ����¼�
        return;
    }
    camera.ProcessMouseScroll(yoffset);
}

