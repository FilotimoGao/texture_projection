#include "Application.h"
#include<vector>
#include <algorithm>
#include <sstream>
#include <utility>

int model_num = 0;

// 添加此函数以提取文件名中的数字
int extractNumberFromFilename(const std::string& filename) {
    std::string numberString;
    for (char c : filename) {
        if (isdigit(c)) {
            numberString += c; // 收集所有数字字符
        }
    }
    return numberString.empty() ? -1 : std::stoi(numberString); // 如果没有数字则返回 -1
}

// 窗口大小调整回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application::getInstance()->setAppWidth(width); // 更新窗口宽度
    Application::getInstance()->setAppHeight(height); // 更新窗口高度
    glViewport(width * 0.3f, 0, width * 0.7f, height); // 设置 OpenGL 渲染区域
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
    // 窗口初始化
    appWidth = width;
    appHeight = height;
    selectedModelIndices.clear();
    glfwInit();
    appWindow = glfwCreateWindow(appWidth, appHeight, "Image Viewer", nullptr, nullptr);
    glfwMakeContextCurrent(appWindow);
    glfwSetFramebufferSizeCallback(appWindow, framebuffer_size_callback);

    // OpenGL loader (GLAD) 初始化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // 设置鼠标模式和回调
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

    // 设置ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // 绑定ImGui
    ImGui_ImplGlfw_InitForOpenGL(appWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 启用 Docking
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
        workspaceUI();
        // 设置 OpenGL 渲染区域（右侧 70% 区域）
        glViewport(appWidth * 0.3f, 0, appWidth * 0.7f, appHeight);
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


void Application::workspaceUI() {
    IGFD::FileDialogConfig config;
    config.countSelectionMax = 1;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 设置 ImGui 窗口的位置和大小（左侧 30% 区域）
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(appWidth * 0.3f, appHeight));
    ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // ------ 上部：由python完成的模型切割工作，模型切割工作区开始 ------ //
    //ImGui::SetNextWindowPos(ImVec2(0, 0));
    //ImGui::SetNextWindowSize(ImVec2(appWidth * 0.3f, appHeight * 0.3f));
    ImGui::Begin("Image Preprocessing", nullptr);

    if (ImGui::Button("Select Image")) {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseImageForProcessing",
            "Choose Image",
            ".png,.jpg,.jpeg",
            config
        );
    }

    // 显示选中的图片路径
    if (!selecImgPath.empty()) {
        ImGui::Text("Selected Image: %s", selecImgPath.c_str());

        // 加载并创建纹理
        if (selecImgTexID == 0) {
            selecImgTexID = loadTexture(selecImgPath.c_str(), targetWidth, targetHeight, false);
        }

        // 在当前窗口显示选中的图片
        if (selecImgTexID != 0) {
            int width = (int)(appWidth * 0.3 * 0.8);
            int height = (int)((width * targetHeight) / targetWidth);
            ImGui::Image((void*)(intptr_t)selecImgTexID, ImVec2(width, height)); // 设置显示的尺寸
        }

        selectTargetUI();

        // 调用 demo.py 的按钮
        if (ImGui::Button("Cut Image")) {
            if (!selecImgPath.empty()) {
                processImageWithDemoPy(selecImgPath);
            }
            else {
                ImGui::Text("Please select an image first!");
            }
        }
    }

    // 处理文件选择对话框
    if (ImGuiFileDialog::Instance()->Display("ChooseImageForProcessing")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selecImgPath = ImGuiFileDialog::Instance()->GetFilePathName();
            selecImgTexID = 0;
            selectTarget = false;
            targetPoints[0] = glm::vec2(0, 0);
            targetPoints[1] = glm::vec2(1, 0);
            targetPoints[2] = glm::vec2(0, 1);
            targetPoints[3] = glm::vec2(1, 1);
        }
        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::End();
    // ------ 上部：由python完成的模型切割工作，模型切割工作区结束 ------ //

    // ------ 下部：由OpenGL完成的模型控制工作，模型转换工作区开始 ------ //
    //ImGui::SetNextWindowPos(ImVec2(0, appHeight * 0.3f)); // 下部窗口位置
    //ImGui::SetNextWindowSize(ImVec2(appWidth * 0.3f, appHeight * 0.7f)); // 下部窗口大小
    ImGui::Begin("Control Panel", nullptr);
    config.countSelectionMax = 50;

    // 显示所有模型，并支持多选
    for (size_t i = 0; i < model_num; ++i) {
        char label[32];
        sprintf(label, "Model %d", (int)i);
        // 检查当前模型是否已被选中
        bool isSelected = std::find(selectedModelIndices.begin(), selectedModelIndices.end(), i) != selectedModelIndices.end();
        // ImGui 的选择逻辑（按住 Ctrl 支持多选，不按 Ctrl 选择则清空所有其他选择）
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
    // 如果有选中的模型，显示批量控制面板
    if (!selectedModelIndices.empty()) {
        ImGui::Separator();
        ImGui::Text("Batch Transform and Texture Controls:");
        // 摄像机恢复默认按钮
        if (ImGui::CollapsingHeader("Camera Controls")) {
            if (ImGui::Button("Reset Camera to Default")) {
                camera.ResetToDefault();
            }
        }
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
        static int currentTextureMode = 1; // 0: UV Mapping, 1: Projection Mapping
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
                    unsigned int newTexture = loadTexture(texturePath.c_str(), true);
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
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseImages",
            "Choose Images",
            ".png,.jpg,.jpeg",
            config
        );
    }
    // 加载选中的图片并生成模型
    if (ImGuiFileDialog::Instance()->Display("ChooseImages")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            auto selectedFiles = ImGuiFileDialog::Instance()->GetSelection();
            scene.clear();
            model_num = 0;
            // 创建一个 vector 来存储文件名和路径，之后按照数字排序
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
                // 为每个模型加载独立的纹理
                unsigned int texture = loadTexture(filePath.c_str(), true);
                unsigned int projectionTexture = loadTexture(filePath.c_str(), true);
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
    // ------ 下部：由OpenGL完成的模型控制工作，模型转换工作区结束 ------ //

    ImGui::End();
}

// 对选中图片进行target range选定
void Application::selectTargetUI() {
    if (ImGui::Button("Select Target Range")) {
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
        ImGui::Begin("Select Target Range", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        // 显示选中的图片
        if (selecImgTexID != 0) {
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 windowPos = ImGui::GetWindowPos();
            float width = windowSize.x * 0.8f;
            float height = windowSize.y * 0.8f;

            float aspectRatio = float(targetWidth) / float(targetHeight); // 计算图片的宽高比
            // 根据宽高比调整目标尺寸
            if (width / height > aspectRatio) {
                width = height * aspectRatio; // 以高度为基准
            }
            else {
                height = width / aspectRatio; // 以宽度为基准
            }

            // 计算图片在窗口的中心位置
            ImVec2 imagePos = ImVec2((windowSize.x - width) * 0.5f, (windowSize.y - height) * 0.5f);
            ImGui::SetCursorPos(imagePos); // 设置光标位置
            ImGui::Image((void*)(intptr_t)selecImgTexID, ImVec2(width, height));
            glm::vec2 paintPoints[4]; // 最终绘制在窗口上的点（原点为整个屏幕在左上角）

            // 更新targetPoints数组
            paintPoints[0] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[0].x * width, windowPos.y + imagePos.y + nextTargetPoints[0].y * height);        // 上左
            paintPoints[1] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[1].x * width, windowPos.y + imagePos.y + nextTargetPoints[1].y * height);        // 上右
            paintPoints[2] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[2].x * width, windowPos.y + imagePos.y + nextTargetPoints[2].y * height);        // 下左
            paintPoints[3] = glm::vec2(windowPos.x + imagePos.x + nextTargetPoints[3].x * width, windowPos.y + imagePos.y + nextTargetPoints[3].y * height);        // 下右

            // 拖动逻辑
            static int selectedPoint = -1; // 当前选择的点
            ImVec2 mousePos = ImGui::GetMousePos(); // 获取鼠标位置

            // 检测鼠标是否点击在某个点的范围内
            if (ImGui::IsMouseClicked(0)) { // 左键点击
                for (int i = 0; i < 4; ++i) {
                    float dist = glm::distance(glm::vec2(mousePos.x, mousePos.y), paintPoints[i]);
                    if (dist < 10.0f) { // 如果点击在点的范围内
                        selectedPoint = i; // 选中该点
                        break;
                    }
                }
            }

            // 鼠标松开须重置selectedPoint，否则拖动仍会带动点
            if (ImGui::IsMouseReleased(0)) {
                selectedPoint = -1;
            }

            // 拖动点
            if (ImGui::IsMouseDragging(0) && selectedPoint != -1) {
                // 更新点的位置为鼠标位置
                paintPoints[selectedPoint].x = mousePos.x;
                paintPoints[selectedPoint].y = mousePos.y;

                // 限制点的位置在图片范围内
                paintPoints[selectedPoint].x = glm::clamp(paintPoints[selectedPoint].x,
                    windowPos.x + imagePos.x,
                    windowPos.x + imagePos.x + width);
                paintPoints[selectedPoint].y = glm::clamp(paintPoints[selectedPoint].y,
                    windowPos.y + imagePos.y,
                    windowPos.y + imagePos.y + height);

                // 更新相对原图的位置
                nextTargetPoints[selectedPoint] = glm::vec2((paintPoints[selectedPoint].x - windowPos.x - imagePos.x) / width, (paintPoints[selectedPoint].y - windowPos.y - imagePos.y) / height);
            }

            // 在图片上绘制四个点，以及四条边
            for (int i = 0; i < 4; ++i) {
                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(paintPoints[i].x, paintPoints[i].y), 5.0f, IM_COL32(255, 0, 0, 255)); // 绘制红色圆点
            }

            // 绘制连接线
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[0].x, paintPoints[0].y), ImVec2(paintPoints[1].x, paintPoints[1].y), IM_COL32(255, 0, 0, 255), 2.0f); // 0 -> 1
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[1].x, paintPoints[1].y), ImVec2(paintPoints[3].x, paintPoints[3].y), IM_COL32(255, 0, 0, 255), 2.0f); // 1 -> 2
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[2].x, paintPoints[2].y), ImVec2(paintPoints[3].x, paintPoints[3].y), IM_COL32(255, 0, 0, 255), 2.0f); // 2 -> 3
            ImGui::GetWindowDrawList()->AddLine(ImVec2(paintPoints[2].x, paintPoints[2].y), ImVec2(paintPoints[0].x, paintPoints[0].y), IM_COL32(255, 0, 0, 255), 2.0f); // 3 -> 0
        }

        ImGui::Spacing();
        ImGui::Spacing();
        if (ImGui::Button("Apply")) {
            targetPoints[0] = nextTargetPoints[0];
            targetPoints[1] = nextTargetPoints[1];
            targetPoints[2] = nextTargetPoints[2];
            targetPoints[3] = nextTargetPoints[3];
            selectTarget = false; // 应用设置，关闭窗口
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            selectTarget = false; // 关闭窗口
        }
        ImGui::End();
    }
}



Application::Application()
    : appWidth(0), appHeight(0), appWindow(nullptr), shaderProgram(0),
    camera(glm::vec3(0.0f, 0.0f, 3.0f)), // 初始化相机，默认位置为 (0, 0, 3)
    lastX(400.0f), lastY(300.0f), firstMouse(true),
    deltaTime(0.0f), lastFrame(0.0f), isMousePressed(false),
    selecImgTexID(0), selectTarget(false),
    targetPoints{ {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f} },
    nextTargetPoints{ {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f} },
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
        // 如果 ImGui 捕获了鼠标点击，则忽略点击事件
        return;
    }
    double xpos, ypos;
    glfwGetCursorPos(appWindow, &xpos, &ypos);
    // 检查鼠标是否在 OpenGL 渲染区域内（右侧 70% 区域）
    if (xpos > appWidth * 0.3f) {
        isMousePressed = pressed;
        if (pressed) {
            lastX = xpos;
            lastY = ypos;
        }
    }
}
void Application::mouse_callback(double xpos, double ypos) {
    // 检查鼠标是否在 OpenGL 渲染区域内（右侧 70% 区域）
    if (xpos > appWidth * 0.3f) {
        if (!isMousePressed) {
            return;
        }
        float xOffset = xpos - lastX;
        float yOffset = lastY - ypos; // 注意 y 是反向的，因为窗口坐标系与 OpenGL 坐标系不同
        lastX = xpos;
        lastY = ypos;
        camera.ProcessMouseMovement(xOffset, yOffset);
    }
}

// 处理鼠标滚动
void Application::scroll_callback(double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        // 如果 ImGui 捕获了滚轮输入，则忽略滚动事件
        return;
    }
    camera.ProcessMouseScroll(yoffset);
}

