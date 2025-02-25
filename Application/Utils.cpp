#include"Utils.h"
#include <STBIMAGE/stb_image.h> // 使用 stb_image 库加载图片
#include <Python.h>
#include <filesystem>

//namespace py = pybind11;
using namespace std;

GLuint loadTexture(const char* path, bool if_flip) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // 设置边界颜色
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // RGBA 白色
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 设置纹理过滤模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载图片
    int width, height, channels;
    stbi_set_flip_vertically_on_load(if_flip);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;  // 灰度图
        else if (channels == 3)
            format = GL_RGB;  // RGB 图
        else if (channels == 4)
            format = GL_RGBA; // RGBA 图
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
        glDeleteTextures(1, &textureID); // 失败时清理纹理
        return 0; // 返回 0 表示加载失败
    }
    stbi_image_free(data);
    return textureID;
}

GLuint loadTexture(const char* path, int& width, int& height, bool if_flip) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // 设置边界颜色
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // RGBA 白色
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 设置纹理过滤模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载图片
    int channels;
    stbi_set_flip_vertically_on_load(if_flip);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format;
        if (channels == 1)
            format = GL_RED;  // 灰度图
        else if (channels == 3)
            format = GL_RGB;  // RGB 图
        else if (channels == 4)
            format = GL_RGBA; // RGBA 图
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
        glDeleteTextures(1, &textureID); // 失败时清理纹理
        return 0; // 返回 0 表示加载失败
    }
    stbi_image_free(data);
    return textureID;
}


bool loadShaders(GLuint& shaderProgram) {
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
        cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
        return false;
    }

    // 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
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
        cerr << "ERROR::PROGRAM::LINK_FAILED\n" << infoLog << endl;
        return false;
    }

    // 删除着色器
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}


// 替换反斜杠为正斜杠
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

    // 获取可执行文件的路径
    std::filesystem::path execPath = std::filesystem::current_path();

    // 推导出项目根目录
    std::filesystem::path projectRoot = execPath.parent_path().parent_path().parent_path(); // 假设三层上移到根目录
    std::filesystem::path lcnnPath = projectRoot / "LCNN";
    std::string lcnnPathStr = convertSlashes(lcnnPath.string());

    // 添加模块搜索路径
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(("sys.path.append('" + lcnnPathStr + "')").c_str());  // 动态添加路径

    // 导入模块
    PyObject* pModule = PyImport_ImportModule("demo");
    if (pModule == NULL) {
        PyErr_Print();
        std::cout << "Module not found" << std::endl;
        return;
    }

    // 获取函数
    PyObject* pFunc = PyObject_GetAttrString(pModule, "cut");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        std::cout << "Function not found or not callable" << std::endl;
        Py_DECREF(pModule);
        return;
    }

    // 将参数转换为 Python 对象
    const char* arg0 = imagePath.c_str();
    PyObject* pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(arg0));

    // 调用函数
    PyObject_CallObject(pFunc, pArgs);

    // 清理
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
    Py_Finalize();
}
