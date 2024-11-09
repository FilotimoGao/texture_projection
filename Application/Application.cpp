#include"Application.h"

std::string imagePath(256, '\0'); // 初始化为256长度的字符串
GLuint textureID = 0;

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
    ImGui_ImplOpenGL3_Init("#version 130");
    return true;
}


void Application::update() {
    while (!glfwWindowShouldClose(appWindow)) {
        // 开始 ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 创建 ImGui 界面
        ImGui::Begin("Control Panel"); // 操作面板
        ImGui::InputText("Image Path", &imagePath[0], imagePath.size()); // 输入框
        if (ImGui::Button("Load Image")) { // 加载按钮
            std::cout << "Loading image from: " << imagePath << std::endl; // 输出加载信息
            textureID = loadTexture(imagePath.c_str());
        }
        ImGui::End();

        // 渲染
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 显示图片
        if (textureID > 0) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            ImGui::Begin("Image Display"); // 图片展示区
            ImGui::Text("Image Loaded:");
            ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(400, 300)); // 展示图片
            ImGui::End();
        }

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(appWindow);
        glfwPollEvents();
    }
}

void Application::destory() {
    // 清理
    glDeleteTextures(1, &textureID);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(appWindow);
    glfwTerminate();
}

Application::Application() {
    appWidth = 0;
    appHeight = 0;
    appWindow = nullptr;
}

Application::~Application() {
	if (instance != nullptr) {
		delete instance;
	}
}