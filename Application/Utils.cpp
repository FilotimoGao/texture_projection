#include"Utils.h"
#include <STBIMAGE/stb_image.h> // 使用 stb_image 库加载图片
#include <Python.h>
#include <filesystem>

//namespace py = pybind11;
using namespace std;

// 全局变量来存储函数引用
PyObject* pCutFunction = nullptr;
PyObject* pTargetRangeFunction = nullptr;
PyObject* pSegmentFunction = nullptr;


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
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 加载图片
    stbi_set_flip_vertically_on_load(if_flip);
    int channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
    // 如果图片不是 4 通道，手动转换为 RGBA
    unsigned char* rgbaData = nullptr;
    if (channels != 4) {
        rgbaData = new unsigned char[width * height * 4];
        for (int i = 0; i < width * height; ++i) {
            rgbaData[i * 4 + 0] = data[i * channels + 0]; // R
            rgbaData[i * 4 + 1] = data[i * channels + 1]; // G
            rgbaData[i * 4 + 2] = data[i * channels + 2]; // B
            rgbaData[i * 4 + 3] = 255; // A
        }
        stbi_image_free(data);
        data = rgbaData;
    }
    // 加载纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    // 释放数据
    if (rgbaData) {
        delete[] rgbaData;
    }
    else {
        stbi_image_free(data);
    }
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
    in vec2 TexCoords;                          // 从顶点着色器传入的纹理坐标
    in vec4 ProjectedCoords;                    // 从顶点着色器传入的投影坐标

    uniform sampler2D texture1;                 // 默认纹理
    uniform sampler2D projectionTexture;         // 投影纹理
    uniform int textureMode;                     // 纹理模式 (0: UV 映射, 1: 投影映射)
    uniform float projectionAspectRatio;         // 投影纹理长宽比

    out vec4 FragColor;                          // 输出的片段颜色

    void main() {
        vec4 texColor;

        if (textureMode == 0) { // UV 映射
            texColor = texture(texture1, TexCoords); // 使用默认纹理和纹理坐标
        } else { // 投影映射
            vec3 projCoords = ProjectedCoords.xyz / ProjectedCoords.w; // 透视除法
            projCoords.x *= projectionAspectRatio; // 根据长宽比调整 x 坐标
            projCoords = projCoords * 0.5 + 0.5; // [-1, 1] 转换到 [0, 1]

            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
                texColor = vec4(0.0, 0.0, 0.0, 0.0); // 超出范围返回完全透明
            } else {
                texColor = texture(projectionTexture, projCoords.xy); // 使用投影纹理
            }
        }

        // 丢弃透明片段
        if (texColor.a < 0.1) {
            discard; // 透明部分丢弃，不渲染
        }

        FragColor = texColor; // 设置片段颜色
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

std::string convertPath(const std::string& path) {
    std::filesystem::path execPath = std::filesystem::current_path(); // 获取可执行文件路径
    std::filesystem::path projectRoot = execPath.parent_path().parent_path().parent_path(); // 假设三层上移到根目录
    std::filesystem::path absolutePath = projectRoot / path;
    return convertSlashes(absolutePath.string());
}

void createOutputDirectory(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}

void initializePython(const std::string& lcnnPathStr) {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        std::cout << "Python initialization failed" << std::endl;
        return;
    }

    // 添加模块搜索路径
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(("sys.path.append('" + lcnnPathStr + "')").c_str());

    // 从 demo 模块导入函数
    PyObject* pDemoModule = PyImport_ImportModule("demo");
    if (pDemoModule) {
        // 获取 cut 函数
        pCutFunction = PyObject_GetAttrString(pDemoModule, "cut");
        if (!pCutFunction || !PyCallable_Check(pCutFunction)) {
            std::cout << "Cut function not found or not callable" << std::endl;
        }

        // 获取 target_range 函数
        pTargetRangeFunction = PyObject_GetAttrString(pDemoModule, "target_range");
        if (!pTargetRangeFunction || !PyCallable_Check(pTargetRangeFunction)) {
            std::cout << "Target range function not found or not callable" << std::endl;
        }

        Py_DECREF(pDemoModule); // 释放 demo 模块引用
    }
    else {
        PyErr_Print();
        std::cout << "Demo module not found" << std::endl;
    }

    // 从 cutline 模块导入函数
    PyObject* pCutlineModule = PyImport_ImportModule("cutline");
    if (pCutlineModule) {
        // 获取 segment 函数
        pSegmentFunction = PyObject_GetAttrString(pCutlineModule, "segment");
        if (!pSegmentFunction || !PyCallable_Check(pSegmentFunction)) {
            std::cout << "Segment function not found or not callable" << std::endl;
        }

        Py_DECREF(pCutlineModule); // 释放 cutline 模块引用
    }
    else {
        PyErr_Print();
        std::cout << "Cutline module not found" << std::endl;
    }
}

void finalizePython() {
    // 清理函数引用
    Py_XDECREF(pCutFunction);
    Py_XDECREF(pTargetRangeFunction);
    Py_Finalize();
}

void processImageWithCutPy() {
    std::string imagePath = "temp_output/range.png";

    // 获取 GIL
    //PyGILState_STATE gstate = PyGILState_Ensure();

    // 检查函数是否存在
    if (pCutFunction) {
        // 将参数转换为 Python 对象
        PyObject* pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(imagePath.c_str()));

        // 调用函数
        PyObject_CallObject(pCutFunction, pArgs);

        // 清理
        Py_XDECREF(pArgs);
    }
    else {
        std::cout << "Cut function not available" << std::endl;
    }

    //PyGILState_Release(gstate); // 释放 GIL
}

void processImageWithRangePy(const std::string& imagePath, glm::vec2 targetPoints[4]) {
    // 获取 GIL
    //PyGILState_STATE gstate = PyGILState_Ensure();

    // 检查函数是否存在
    if (pTargetRangeFunction) {
        // 将参数转换为 Python 对象
        std::string path = convertSlashes(imagePath);
        PyObject* pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(path.c_str()));

        // 创建存储点的列表
        PyObject* pPointsList = PyList_New(4);
        for (int i = 0; i < 4; ++i) {
            PyObject* pPoint = PyTuple_New(2);
            PyTuple_SetItem(pPoint, 0, PyFloat_FromDouble(targetPoints[i].x));
            PyTuple_SetItem(pPoint, 1, PyFloat_FromDouble(targetPoints[i].y));
            PyList_SetItem(pPointsList, i, pPoint); // 注意：这里是引用计数的管理，PyList_SetItem 会导致 pPoint 的引用计数增加
        }

        // 将列表作为第二个参数传递
        PyTuple_SetItem(pArgs, 1, pPointsList);

        // 调用函数
        PyObject_CallObject(pTargetRangeFunction, pArgs);

        // 清理
        Py_XDECREF(pArgs);
    }
    else {
        std::cout << "Target range function not available" << std::endl;
    }

    //PyGILState_Release(gstate); // 释放 GIL
}

void processImageWithSegmentPy(const std::string& imagePath, const std::vector<std::pair<glm::vec2, glm::vec2>>& lines) {
    // 获取 GIL
    // PyGILState_STATE gstate = PyGILState_Ensure();

    // 检查函数是否存在
    if (pSegmentFunction) { // 假设 pSegmentFunction 是全局变量，指向 Python 的 segment 函数
        // 将参数转换为 Python 对象
        std::string path = convertSlashes(imagePath);
        PyObject* pArgs = PyTuple_New(2);

        // 第一个参数：图像路径
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(path.c_str()));

        // 第二个参数：线段列表
        PyObject* pLinesList = PyList_New(lines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            PyObject* pLineTuple = PyTuple_New(2);
            PyTuple_SetItem(pLineTuple, 0, PyTuple_New(2));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 0), 0, PyFloat_FromDouble(lines[i].first.x));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 0), 1, PyFloat_FromDouble(lines[i].first.y));
            PyTuple_SetItem(pLineTuple, 1, PyTuple_New(2));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 1), 0, PyFloat_FromDouble(lines[i].second.x));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 1), 1, PyFloat_FromDouble(lines[i].second.y));

            // 将线段添加到列表中
            PyList_SetItem(pLinesList, i, pLineTuple); // 注意：这里是引用计数的管理
        }

        // 将线段列表作为第二个参数传递
        PyTuple_SetItem(pArgs, 1, pLinesList);

        // 调用函数
        PyObject_CallObject(pSegmentFunction, pArgs);

        // 清理
        Py_XDECREF(pArgs);
    }
    else {
        std::cout << "Segment function not available" << std::endl;
    }

    // PyGILState_Release(gstate); // 释放 GIL
}
