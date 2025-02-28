#include "Application.h"
#include <vector>
#include <algorithm>
#include <sstream>
#include <utility>
#include <thread>
#include <atomic>
#include <future>
#include <filesystem>

int model_num = 0;
std::atomic<bool> isProcessing(false);
std::atomic<bool> processCompleted(false);
std::promise<std::vector<std::string>> modelPromise; // ����������
std::future<std::vector<std::string>> modelFuture; // ���� future

// ����һ���߳�������Python��ز���
//std::thread pythonThread;
std::atomic<bool> pythonInitialized(false); // ���Python�Ƿ��ѳ�ʼ��

void runImageProcessing(const std::string& imagePath, glm::vec2 targetPoints[4]) {
    isProcessing = true;
    processCompleted = false;

    std::string lcnnPathStr = convertPath("LCNN");

    // ȷ��Python�ѳ�ʼ��
    if (!pythonInitialized) {
        initializePython(lcnnPathStr);
        pythonInitialized = true;
    }

    // ����ͼ��
    processImageWithRangePy(imagePath, targetPoints);
    processImageWithCutPy();

    // ���ؽ��ͼƬ
    std::vector<std::string> imageFiles;
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator("temp_output")) {
        if (entry.is_regular_file()) {
            imageFiles.push_back(entry.path().string());
        }
    }

    // ���� promise ��ֵ�Դ��ݽ��
    try {
        modelPromise.set_value(imageFiles);
    }
    catch (const std::future_error& e) {
        std::cerr << "Future error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    isProcessing = false;
    processCompleted = true;
}


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
    io.Fonts->AddFontFromFileTTF(convertPath("IMGUI/simsun.ttc").c_str(), 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    ImGui::StyleColorsDark();

    // ��ImGui
    ImGui_ImplGlfw_InitForOpenGL(appWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ���� Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (!loadShaders(shaderProgram)) {
        return false;
    }

    // ����Python���߳�
    //pythonThread = std::thread(pythonWorker);

    return true;
}


void Application::update() {
    while (!glfwWindowShouldClose(appWindow)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput();
        workspaceUI();
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

void Application::destroy() {
    std::cout << "Destroying application..." << std::endl;

    // ��������Դ
    scene.clear();
    std::cout << "Scene cleared." << std::endl;
    // ���� ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    std::cout << "ImGui destroyed." << std::endl;

    // ���ٴ��ڲ���ֹ GLFW
    glfwDestroyWindow(appWindow);
    std::cout << "Window destroyed." << std::endl;
    glfwTerminate();
    std::cout << "GLFW terminated." << std::endl;
    std::cout << "Application destroyed successfully." << std::endl;
}

void Application::workspaceUI() {
    IGFD::FileDialogConfig config;
    config.countSelectionMax = 1;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ���� ImGui ���ڵ�λ�úʹ�С����� 30% ����
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(appWidth * 0.3f, appHeight));
    ImGui::Begin(u8"������(Workspace)", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // ------ �ϲ�����python��ɵ�ģ���и����ģ���и������ʼ ------ //
    ImGui::Begin(u8"ͼƬԤ����(Image Preprocessing)", nullptr);
    ImGui::TextWrapped(u8"�˹���������ѡ��Ŀ��ͼƬ����Ԥ��������·���Load������ͼƬ������ѡ��һ�š�");
    ImGui::Spacing();
    ImGui::Spacing();
    if (ImGui::Button(u8"����(Load)")) {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseImageForProcessing",
            "Choose Image",
            "Images{.png,.jpg,.jpeg}",
            config
        );
    }

    // ��ʾѡ�е�ͼƬ·��
    if (!selecImgPath.empty()) {
        ImGui::Text(u8"��ǰͼƬ·��: %s", selecImgPath.c_str());

        // ���ز���������
        if (selecImgTexID == 0) {
            selecImgTexID = loadTexture(selecImgPath.c_str(), targetWidth, targetHeight, false);
        }

        // �ڵ�ǰ������ʾѡ�е�ͼƬ
        if (selecImgTexID != 0) {
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 imagePos = ImGui::GetCursorScreenPos(); // ��ǰ��꣬Ҳ����ͼ�����Ͻ�����
            int width = (int)(windowSize.x * 0.6);
            int height = (int)((width * targetHeight) / targetWidth);
            ImGui::Image((void*)(intptr_t)selecImgTexID, ImVec2(width, height)); // ������ʾ�ĳߴ�
            ImVec2 paintPoints[4] = {
                ImVec2(imagePos.x + targetPoints[0].x * width, imagePos.y + targetPoints[0].y * height),
                ImVec2(imagePos.x + targetPoints[1].x * width, imagePos.y + targetPoints[1].y * height),
                ImVec2(imagePos.x + targetPoints[2].x * width, imagePos.y + targetPoints[2].y * height),
                ImVec2(imagePos.x + targetPoints[3].x * width, imagePos.y + targetPoints[3].y * height)
            };
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddPolyline(paintPoints, 4, IM_COL32(255, 0, 0, 255), true, 1.0f); // ��ɫ�߿��������Ϊ2
        }

        selectTargetUI();

        // ���� demo.py �İ�ť
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextWrapped(u8"����·���Cut����������LCNN����¥��ͼƬ���С�");
        if (ImGui::Button(u8"���У�Cut��")) {
        if (!selecImgPath.empty()) {
            if (!isProcessing) {
                // �����µ� promise �� future ����
                modelPromise = std::promise<std::vector<std::string>>(); // �����µ� promise
                modelFuture = modelPromise.get_future(); // ��ȡ��Ӧ�� future

                // �����������߳�
                std::thread processingThread(runImageProcessing, selecImgPath, targetPoints);
                processingThread.detach(); // ʹ�̷߳���
            } else {
                std::cout << "Processing already in progress." << std::endl;
            }
        } else {
            ImGui::Text("Please select an image first!");
        }
    }

    // ��ʾ����״̬�ͽ��
    if (isProcessing) {
        ImGui::Text(u8"���ڴ����С���");
        ImGui::Text(u8"ģ��Ԥ�������ҪһЩʱ�䣬�����ĵȴ�����");
    }
    if (processCompleted) {
        ImGui::Text(u8"��������ɣ�");
        try {
            std::vector<std::string> imageFiles = modelFuture.get(); // ��ȡ������
            for (const auto& filePath : imageFiles) {
                std::cout << "Loaded file: " << filePath << std::endl;
            }
            createModels(imageFiles);
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving results: " << e.what() << std::endl;
        }
        processCompleted = false; // ����״̬
    }
    }

    // �����ļ�ѡ��Ի���
    if (ImGuiFileDialog::Instance()->Display("ChooseImageForProcessing")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selecImgPath = ImGuiFileDialog::Instance()->GetFilePathName();
            selecImgTexID = 0;
            selectTarget = false;
            targetPoints[0] = glm::vec2(0, 0);
            targetPoints[1] = glm::vec2(1, 0);
            targetPoints[2] = glm::vec2(1, 1);
            targetPoints[3] = glm::vec2(0, 1);
        }
        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::End();
    // ------ �ϲ�����python��ɵ�ģ���и����ģ���и�������� ------ //

    // ------ �²�����OpenGL��ɵ�ģ�Ϳ��ƹ�����ģ��ת����������ʼ ------ //
    ImGui::Begin(u8"3Dģ�Ϳ��� (Model Control)", nullptr);
    config.countSelectionMax = 50;

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

    // ���ȫѡ��ť
    if (model_num > 0) {
        if (ImGui::Button(u8"ȫѡ (Select All)")) {
            selectedModelIndices.clear();
            for (size_t i = 0; i < model_num; ++i) {
                selectedModelIndices.push_back(i); // �������ģ�͵�����
            }
        }
    }

    // �����ѡ�е�ģ�ͣ���ʾ�����������
    if (!selectedModelIndices.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped(u8"����Ĺ���֧����������ģ�͵�ת�����������ã���סctrl���ɽ����Ϸ�Models��ѡ��");
        // ������ָ�Ĭ�ϰ�ť
        if (ImGui::CollapsingHeader("Camera Controls")) {
            if (ImGui::Button(u8"�����ӽ� (Reset)")) {
                camera.ResetToDefault();
            }
        }
        // ---- 1. �����޸� Transform ----
        static glm::vec3 batchPosition(0.0f);
        static glm::vec3 batchRotation(0.0f);
        static glm::vec2 batchSize(1.0f);
        if (ImGui::CollapsingHeader("Transform")) {
            ImGui::InputFloat3("Position", &batchPosition[0]);
            ImGui::InputFloat3("Rotation", &batchRotation[0]);
            ImGui::InputFloat2("Size", &batchSize[0]);
            if (ImGui::Button(u8"Ӧ��ת��(Apply)")) {
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
            ImGui::Text(u8"����ģʽ(Texture Mode):");
            ImGui::RadioButton("UV Mapping", &currentTextureMode, 0);
            ImGui::RadioButton("Projection Mapping", &currentTextureMode, 1);
            //std::cout << "size: " << selectedModelIndices.size() << std::endl;

            if (ImGui::Button(u8"Ӧ������ģʽ(Apply)")) {
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
            if (ImGui::Button(u8"��������(Texture Reset)")) {
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseBatchTexture",
                    "Choose Texture",
                    "Images{.png,.jpg,.jpeg}",
                    config
                );
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseBatchTexture")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string texturePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    unsigned int newTexture = loadTexture(texturePath.c_str(), true);
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
    if (ImGui::Button(u8"ͼƬ��ѡ(Select)")) {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseImages",
            "Choose Images",
            "Images{.png,.jpg,.jpeg}",
            config
        );
    }
    // ����ѡ�е�ͼƬ������ģ��
    if (ImGuiFileDialog::Instance()->Display("ChooseImages")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selectedFiles = ImGuiFileDialog::Instance()->GetSelection();
            std::vector<std::string> filePaths;
            for (const auto& [key, filePath] : selectedFiles) {
                filePaths.push_back(filePath);
            }
            createModels(filePaths); // �����º���
        }
        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::End();
    // ------ �²�����OpenGL��ɵ�ģ�Ϳ��ƹ�����ģ��ת������������ ------ //

    ImGui::End();
}

// ��ѡ��ͼƬ����target rangeѡ��
void Application::selectTargetUI() {
    if (ImGui::Button(u8"����Ŀ�귶Χ(Range)")) {
        selectTarget = true;
        nextTargetPoints[0] = targetPoints[0];
        nextTargetPoints[1] = targetPoints[1];
        nextTargetPoints[2] = targetPoints[2];
        nextTargetPoints[3] = targetPoints[3];
    }
    if (selectTarget) {
        ImGui::SetNextWindowPos(ImVec2(appWidth * 0.3f, 0));
        ImGui::SetNextWindowSize(ImVec2(appWidth * 0.7f, appHeight));
        //ImGui::SetNextWindowSizeConstraints(ImVec2(500, 400), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin(u8"����Ŀ�귶Χ(Select Target Range)", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        // ��ʾѡ�е�ͼƬ
        if (selecImgTexID != 0) {
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 windowPos = ImGui::GetWindowPos();
            float width = windowSize.x * 0.8f;
            float height = windowSize.y * 0.8f;

            float aspectRatio = float(targetWidth) / float(targetHeight); // ����ͼƬ�Ŀ�߱�
            // ���ݿ�߱ȵ���Ŀ��ߴ�
            if (width / height > aspectRatio) {
                width = height * aspectRatio; // �Ը߶�Ϊ��׼
            }
            else {
                height = width / aspectRatio; // �Կ��Ϊ��׼
            }

            // ����ͼƬ�ڴ��ڵ�����λ��
            ImVec2 imagePos = ImVec2((windowSize.x - width) * 0.5f, (windowSize.y - height) * 0.5f);
            ImGui::SetCursorPos(imagePos); // ���ù��λ��
            ImGui::Image((void*)(intptr_t)selecImgTexID, ImVec2(width, height));
            glm::vec2 paintPoints[4]; // ���ջ����ڴ����ϵĵ㣨ԭ��Ϊ������Ļ�����Ͻǣ�

            // ����targetPoints����
            paintPoints[0] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[0].x * width, windowPos.y + imagePos.y + nextTargetPoints[0].y * height);        // ����
            paintPoints[1] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[1].x * width, windowPos.y + imagePos.y + nextTargetPoints[1].y * height);        // ����
            paintPoints[2] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[2].x * width, windowPos.y + imagePos.y + nextTargetPoints[2].y * height);        // ����
            paintPoints[3] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[3].x * width, windowPos.y + imagePos.y + nextTargetPoints[3].y * height);        // ����

            // �϶��߼�
            static int selectedPoint = -1; // ��ǰѡ��ĵ�
            ImVec2 mousePos = ImGui::GetMousePos(); // ��ȡ���λ��

            // �������Ƿ�����ĳ����ķ�Χ��
            if (ImGui::IsMouseClicked(0)) { // ������
                for (int i = 0; i < 4; ++i) {
                    float dist = glm::distance(glm::vec2(mousePos.x, mousePos.y), paintPoints[i]);
                    if (dist < 15.0f) { // �������ڵ�ķ�Χ��
                        selectedPoint = i; // ѡ�иõ�
                        break;
                    }
                }
            }

            // ����ɿ�������selectedPoint�������϶��Ի������
            if (ImGui::IsMouseReleased(0)) {
                selectedPoint = -1;
            }

            // �϶���
            if (ImGui::IsMouseDragging(0) && selectedPoint != -1) {
                // ���µ��λ��Ϊ���λ��
                paintPoints[selectedPoint].x = mousePos.x;
                paintPoints[selectedPoint].y = mousePos.y;

                // ���Ƶ��λ����ͼƬ��Χ��
                paintPoints[selectedPoint].x = glm::clamp(paintPoints[selectedPoint].x,
                    windowPos.x + imagePos.x,
                    windowPos.x + imagePos.x + width);
                paintPoints[selectedPoint].y = glm::clamp(paintPoints[selectedPoint].y,
                    windowPos.y + imagePos.y,
                    windowPos.y + imagePos.y + height);

                // �������ԭͼ��λ��
                nextTargetPoints[selectedPoint] = glm::vec2((paintPoints[selectedPoint].x - windowPos.x - imagePos.x) / width, (paintPoints[selectedPoint].y - windowPos.y - imagePos.y) / height);
            }

            // ��ͼƬ�ϻ����ĸ��㣬�Լ�������
            for (int i = 0; i < 4; ++i) {
                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(paintPoints[i].x, paintPoints[i].y), 7.0f, IM_COL32(255, 0, 0, 255)); // ���ƺ�ɫԲ��
            }

            // ����������
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[0].x, paintPoints[0].y), ImVec2(paintPoints[1].x, paintPoints[1].y), IM_COL32(255, 0, 0, 255), 2.0f); // 0 -> 1
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[1].x, paintPoints[1].y), ImVec2(paintPoints[2].x, paintPoints[2].y), IM_COL32(255, 0, 0, 255), 2.0f); // 1 -> 2
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[2].x, paintPoints[2].y), ImVec2(paintPoints[3].x, paintPoints[3].y), IM_COL32(255, 0, 0, 255), 2.0f); // 2 -> 3
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[3].x, paintPoints[3].y), ImVec2(paintPoints[0].x, paintPoints[0].y), IM_COL32(255, 0, 0, 255), 2.0f); // 3 -> 0
        }

        ImGui::Spacing();
        ImGui::Spacing();
        if (ImGui::Button(u8"Ӧ��(Apply)")) {
            targetPoints[0] = nextTargetPoints[0];
            targetPoints[1] = nextTargetPoints[1];
            targetPoints[2] = nextTargetPoints[2];
            targetPoints[3] = nextTargetPoints[3];
            selectTarget = false; // Ӧ�����ã��رմ���
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"ȡ��(Cancel)")) {
            selectTarget = false; // �رմ���
        }
        ImGui::End();
    }
}

void Application::createModels(const std::vector<std::string>& filePaths) {
    // ����ǰ������ѡ���ģ������
    scene.clear();
    selectedModelIndices.clear();
    model_num = 0;

    // ����һ�� vector ���洢�ļ�����·����֮������������
    std::vector<std::pair<int, std::string>> filesWithNumbers;

    for (const auto& filePath : filePaths) {
        int number = extractNumberFromFilename(filePath);
        if (number != -1) {
            filesWithNumbers.emplace_back(number, filePath);
        }
    }

    // ���ļ�����������
    std::sort(filesWithNumbers.begin(), filesWithNumbers.end());

    // �����������ļ�������ģ��
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

        // ��������
        unsigned int texture = loadTexture(filePath.c_str(), true);
        unsigned int projectionTexture = loadTexture(filePath.c_str(), true);

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

Application::Application()
    : appWidth(0), appHeight(0), appWindow(nullptr), shaderProgram(0),
    camera(glm::vec3(0.0f, 0.0f, 3.0f)), // ��ʼ�������Ĭ��λ��Ϊ (0, 0, 3)
    lastX(400.0f), lastY(300.0f), firstMouse(true),
    deltaTime(0.0f), lastFrame(0.0f), isMousePressed(false),
    selecImgTexID(0), selectTarget(false),
    targetPoints{ {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} },
    nextTargetPoints{ {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} },
    targetWidth(0), targetHeight(0) {}

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
    if (ImGui::GetIO().WantCaptureMouse) {
        // ��� ImGui �����������������Ե���¼�
        return;
    }
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
