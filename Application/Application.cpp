#include "Application.h"
#include<vector>

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

    // ��ʼ��ƽ�涥������
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices = {
        // λ��          // ��������
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

    // ������ɫ��
    if (!loadShaders()) {
        return false;
    }

    return true;
}


void Application::update() {
    while (!glfwWindowShouldClose(appWindow)) {
        // ��ʼ ImGui ֡
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // �������
        ImGui::Begin("Control Panel");
        ImGui::InputText("Image Path", &imagePath[0], imagePath.size());
        if (ImGui::Button("Load Image")) {
            textureID = loadTexture(imagePath.c_str());
        }
        ImGui::InputFloat3("Position", &plane.position[0]);
        ImGui::InputFloat3("Rotation", &plane.rotation[0]);
        ImGui::InputFloat2("Size", &plane.size[0]);
        ImGui::End();

        // ����ģ�;���
        planeModelMatrix = glm::mat4(1.0f);
        planeModelMatrix = glm::translate(planeModelMatrix, plane.position);
        planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(plane.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(plane.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(plane.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(plane.size, 1.0f));

        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ��Ⱦƽ��
        if (textureID > 0) {
            ImGui::Begin("Image Display"); // ͼƬչʾ��
            ImGui::Text("Image Loaded:");
            ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(400, 300)); // չʾͼƬ
            ImGui::End();

            glUseProgram(shaderProgram);

            glm::mat4 view = glm::lookAt(
                glm::vec3(0.0f, 0.0f, 3.0f),  // �����λ��
                glm::vec3(0.0f, 0.0f, 0.0f),  // Ŀ��λ��
                glm::vec3(0.0f, 1.0f, 0.0f)   // �Ϸ���
            );
            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),          // FOV
                (float)appWidth / (float)appHeight, // ��Ļ��߱�
                0.1f, 100.0f                  // ��Զƽ��
            );

            // �ϴ�����ɫ��
            GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
            GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(planeModelMatrix));

            glActiveTexture(GL_TEXTURE0); // ��������Ԫ 0
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0); // �󶨵�����Ԫ 0
            scene.render();
        }

        // ��Ⱦ ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(appWindow);
        glfwPollEvents();
    }
}


void Application::destory() {
    // �ͷ�������Դ
    if (textureID > 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

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

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoords = aTexCoords;
    }
)";

    // Ƭ����ɫ������
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoords;

        uniform sampler2D texture1;

        void main() {
            FragColor = texture(texture1, TexCoords);
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
