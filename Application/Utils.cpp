#include"Utils.h"
#include <STBIMAGE/stb_image.h> // ʹ�� stb_image �����ͼƬ
#include <Python.h>
#include <filesystem>

//namespace py = pybind11;
using namespace std;

// ȫ�ֱ������洢��������
PyObject* pCutFunction = nullptr;
PyObject* pTargetRangeFunction = nullptr;
PyObject* pSegmentFunction = nullptr;


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
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // ����ͼƬ
    stbi_set_flip_vertically_on_load(if_flip);
    int channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
    // ���ͼƬ���� 4 ͨ�����ֶ�ת��Ϊ RGBA
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
    // ������������
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    // �ͷ�����
    if (rgbaData) {
        delete[] rgbaData;
    }
    else {
        stbi_image_free(data);
    }
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
    in vec2 TexCoords;                          // �Ӷ�����ɫ���������������
    in vec4 ProjectedCoords;                    // �Ӷ�����ɫ�������ͶӰ����

    uniform sampler2D texture1;                 // Ĭ������
    uniform sampler2D projectionTexture;         // ͶӰ����
    uniform int textureMode;                     // ����ģʽ (0: UV ӳ��, 1: ͶӰӳ��)
    uniform float projectionAspectRatio;         // ͶӰ�������

    out vec4 FragColor;                          // �����Ƭ����ɫ

    void main() {
        vec4 texColor;

        if (textureMode == 0) { // UV ӳ��
            texColor = texture(texture1, TexCoords); // ʹ��Ĭ���������������
        } else { // ͶӰӳ��
            vec3 projCoords = ProjectedCoords.xyz / ProjectedCoords.w; // ͸�ӳ���
            projCoords.x *= projectionAspectRatio; // ���ݳ���ȵ��� x ����
            projCoords = projCoords * 0.5 + 0.5; // [-1, 1] ת���� [0, 1]

            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
                texColor = vec4(0.0, 0.0, 0.0, 0.0); // ������Χ������ȫ͸��
            } else {
                texColor = texture(projectionTexture, projCoords.xy); // ʹ��ͶӰ����
            }
        }

        // ����͸��Ƭ��
        if (texColor.a < 0.1) {
            discard; // ͸�����ֶ���������Ⱦ
        }

        FragColor = texColor; // ����Ƭ����ɫ
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

std::string convertPath(const std::string& path) {
    std::filesystem::path execPath = std::filesystem::current_path(); // ��ȡ��ִ���ļ�·��
    std::filesystem::path projectRoot = execPath.parent_path().parent_path().parent_path(); // �����������Ƶ���Ŀ¼
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

    // ���ģ������·��
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(("sys.path.append('" + lcnnPathStr + "')").c_str());

    // �� demo ģ�鵼�뺯��
    PyObject* pDemoModule = PyImport_ImportModule("demo");
    if (pDemoModule) {
        // ��ȡ cut ����
        pCutFunction = PyObject_GetAttrString(pDemoModule, "cut");
        if (!pCutFunction || !PyCallable_Check(pCutFunction)) {
            std::cout << "Cut function not found or not callable" << std::endl;
        }

        // ��ȡ target_range ����
        pTargetRangeFunction = PyObject_GetAttrString(pDemoModule, "target_range");
        if (!pTargetRangeFunction || !PyCallable_Check(pTargetRangeFunction)) {
            std::cout << "Target range function not found or not callable" << std::endl;
        }

        Py_DECREF(pDemoModule); // �ͷ� demo ģ������
    }
    else {
        PyErr_Print();
        std::cout << "Demo module not found" << std::endl;
    }

    // �� cutline ģ�鵼�뺯��
    PyObject* pCutlineModule = PyImport_ImportModule("cutline");
    if (pCutlineModule) {
        // ��ȡ segment ����
        pSegmentFunction = PyObject_GetAttrString(pCutlineModule, "segment");
        if (!pSegmentFunction || !PyCallable_Check(pSegmentFunction)) {
            std::cout << "Segment function not found or not callable" << std::endl;
        }

        Py_DECREF(pCutlineModule); // �ͷ� cutline ģ������
    }
    else {
        PyErr_Print();
        std::cout << "Cutline module not found" << std::endl;
    }
}

void finalizePython() {
    // ����������
    Py_XDECREF(pCutFunction);
    Py_XDECREF(pTargetRangeFunction);
    Py_Finalize();
}

void processImageWithCutPy() {
    std::string imagePath = "temp_output/range.png";

    // ��ȡ GIL
    //PyGILState_STATE gstate = PyGILState_Ensure();

    // ��麯���Ƿ����
    if (pCutFunction) {
        // ������ת��Ϊ Python ����
        PyObject* pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(imagePath.c_str()));

        // ���ú���
        PyObject_CallObject(pCutFunction, pArgs);

        // ����
        Py_XDECREF(pArgs);
    }
    else {
        std::cout << "Cut function not available" << std::endl;
    }

    //PyGILState_Release(gstate); // �ͷ� GIL
}

void processImageWithRangePy(const std::string& imagePath, glm::vec2 targetPoints[4]) {
    // ��ȡ GIL
    //PyGILState_STATE gstate = PyGILState_Ensure();

    // ��麯���Ƿ����
    if (pTargetRangeFunction) {
        // ������ת��Ϊ Python ����
        std::string path = convertSlashes(imagePath);
        PyObject* pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(path.c_str()));

        // �����洢����б�
        PyObject* pPointsList = PyList_New(4);
        for (int i = 0; i < 4; ++i) {
            PyObject* pPoint = PyTuple_New(2);
            PyTuple_SetItem(pPoint, 0, PyFloat_FromDouble(targetPoints[i].x));
            PyTuple_SetItem(pPoint, 1, PyFloat_FromDouble(targetPoints[i].y));
            PyList_SetItem(pPointsList, i, pPoint); // ע�⣺���������ü����Ĺ���PyList_SetItem �ᵼ�� pPoint �����ü�������
        }

        // ���б���Ϊ�ڶ�����������
        PyTuple_SetItem(pArgs, 1, pPointsList);

        // ���ú���
        PyObject_CallObject(pTargetRangeFunction, pArgs);

        // ����
        Py_XDECREF(pArgs);
    }
    else {
        std::cout << "Target range function not available" << std::endl;
    }

    //PyGILState_Release(gstate); // �ͷ� GIL
}

void processImageWithSegmentPy(const std::string& imagePath, const std::vector<std::pair<glm::vec2, glm::vec2>>& lines) {
    // ��ȡ GIL
    // PyGILState_STATE gstate = PyGILState_Ensure();

    // ��麯���Ƿ����
    if (pSegmentFunction) { // ���� pSegmentFunction ��ȫ�ֱ�����ָ�� Python �� segment ����
        // ������ת��Ϊ Python ����
        std::string path = convertSlashes(imagePath);
        PyObject* pArgs = PyTuple_New(2);

        // ��һ��������ͼ��·��
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(path.c_str()));

        // �ڶ����������߶��б�
        PyObject* pLinesList = PyList_New(lines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            PyObject* pLineTuple = PyTuple_New(2);
            PyTuple_SetItem(pLineTuple, 0, PyTuple_New(2));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 0), 0, PyFloat_FromDouble(lines[i].first.x));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 0), 1, PyFloat_FromDouble(lines[i].first.y));
            PyTuple_SetItem(pLineTuple, 1, PyTuple_New(2));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 1), 0, PyFloat_FromDouble(lines[i].second.x));
            PyTuple_SetItem(PyTuple_GetItem(pLineTuple, 1), 1, PyFloat_FromDouble(lines[i].second.y));

            // ���߶���ӵ��б���
            PyList_SetItem(pLinesList, i, pLineTuple); // ע�⣺���������ü����Ĺ���
        }

        // ���߶��б���Ϊ�ڶ�����������
        PyTuple_SetItem(pArgs, 1, pLinesList);

        // ���ú���
        PyObject_CallObject(pSegmentFunction, pArgs);

        // ����
        Py_XDECREF(pArgs);
    }
    else {
        std::cout << "Segment function not available" << std::endl;
    }

    // PyGILState_Release(gstate); // �ͷ� GIL
}
