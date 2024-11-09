#include"Application.h"

std::string imagePath(256, '\0'); // ��ʼ��Ϊ256���ȵ��ַ���
GLuint textureID = 0;

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
    ImGui_ImplOpenGL3_Init("#version 130");
    return true;
}


void Application::update() {
    while (!glfwWindowShouldClose(appWindow)) {
        // ��ʼ ImGui ֡
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ���� ImGui ����
        ImGui::Begin("Control Panel"); // �������
        ImGui::InputText("Image Path", &imagePath[0], imagePath.size()); // �����
        if (ImGui::Button("Load Image")) { // ���ذ�ť
            std::cout << "Loading image from: " << imagePath << std::endl; // ���������Ϣ
            textureID = loadTexture(imagePath.c_str());
        }
        ImGui::End();

        // ��Ⱦ
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ��ʾͼƬ
        if (textureID > 0) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            ImGui::Begin("Image Display"); // ͼƬչʾ��
            ImGui::Text("Image Loaded:");
            ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(400, 300)); // չʾͼƬ
            ImGui::End();
        }

        // ��Ⱦ ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(appWindow);
        glfwPollEvents();
    }
}

void Application::destory() {
    // ����
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