#include"Utils.h"
#include <STBIMAGE/stb_image.h> // ʹ�� stb_image �����ͼƬ
#include <Python.h>
#include <filesystem>

//namespace py = pybind11;
using namespace std;

GLuint loadTexture(const char* path, bool if_flip) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // �����������
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // ���ñ߽���ɫ
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // RGBA ��ɫ
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // �����������ģʽ
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ����ͼƬ
    int width, height, channels;
    stbi_set_flip_vertically_on_load(if_flip);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;  // �Ҷ�ͼ
        else if (channels == 3)
            format = GL_RGB;  // RGB ͼ
        else if (channels == 4)
            format = GL_RGBA; // RGBA ͼ
        else {
            cout << "Unsupported texture format: " << path << endl;
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
            return 0;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cout << "Failed to load texture: " << path << endl;
        glDeleteTextures(1, &textureID); // ʧ��ʱ��������
        return 0; // ���� 0 ��ʾ����ʧ��
    }
    stbi_image_free(data);
    return textureID;
}

GLuint loadTexture(const char* path, int& width, int& height, bool if_flip) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // �����������
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // ���ñ߽���ɫ
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // RGBA ��ɫ
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // �����������ģʽ
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ����ͼƬ
    int channels;
    stbi_set_flip_vertically_on_load(if_flip);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;  // �Ҷ�ͼ
        else if (channels == 3)
            format = GL_RGB;  // RGB ͼ
        else if (channels == 4)
            format = GL_RGBA; // RGBA ͼ
        else {
            cout << "Unsupported texture format: " << path << endl;
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
            return 0;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cout << "Failed to load texture: " << path << endl;
        glDeleteTextures(1, &textureID); // ʧ��ʱ��������
        return 0; // ���� 0 ��ʾ����ʧ��
    }
    stbi_image_free(data);
    return textureID;
}


bool loadShaders(GLuint& shaderProgram) {
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
        cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
        return false;
    }

    // ����Ƭ����ɫ��
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
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
        cerr << "ERROR::PROGRAM::LINK_FAILED\n" << infoLog << endl;
        return false;
    }

    // ɾ����ɫ��
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}


// �滻��б��Ϊ��б��
std::string convertSlashes(const std::string& path) {
    std::string modifiedPath = path;
    std::replace(modifiedPath.begin(), modifiedPath.end(), '\\', '/');
    return modifiedPath;
}

void processImageWithDemoPy(const std::string& imagePath) {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        std::cout << "Python initialization failed" << std::endl;
        return;
    }

    // ��ȡ��ִ���ļ���·��
    std::filesystem::path execPath = std::filesystem::current_path();

    // �Ƶ�����Ŀ��Ŀ¼
    std::filesystem::path projectRoot = execPath.parent_path().parent_path().parent_path(); // �����������Ƶ���Ŀ¼
    std::filesystem::path lcnnPath = projectRoot / "LCNN";
    std::string lcnnPathStr = convertSlashes(lcnnPath.string());

    // ���ģ������·��
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(("sys.path.append('" + lcnnPathStr + "')").c_str());  // ��̬���·��

    // ����ģ��
    PyObject* pModule = PyImport_ImportModule("demo");
    if (pModule == NULL) {
        PyErr_Print();
        std::cout << "Module not found" << std::endl;
        return;
    }

    // ��ȡ����
    PyObject* pFunc = PyObject_GetAttrString(pModule, "cut");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        std::cout << "Function not found or not callable" << std::endl;
        Py_DECREF(pModule);
        return;
    }

    // ������ת��Ϊ Python ����
    const char* arg0 = imagePath.c_str();
    PyObject* pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(arg0));

    // ���ú���
    PyObject_CallObject(pFunc, pArgs);

    // ����
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
    Py_Finalize();
}
