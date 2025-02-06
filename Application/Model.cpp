#include "Model.h"

Model::Model(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
    : textureID(0), projectionTexture(0), indices(indices), indexCount(indices.size()) {
    // ��ʼ���任
    transform.position = glm::vec3(0.0f);
    transform.rotation = glm::vec3(0.0f);
    transform.size = glm::vec2(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::mat4(1.0f);

    // ���� VAO��VBO �� EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // �� VBO ���ϴ���������
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // �� EBO ���ϴ���������
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // ���ö������ԣ�λ�ú��������꣩
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // λ������
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // ������������
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // ��� VAO
}

Model::~Model() {
    // ɾ����������������Դ
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    if (textureID > 0) {
        glDeleteTextures(1, &textureID);
    }

    if (projectionTexture > 0) {
        glDeleteTextures(1, &projectionTexture);
    }
}

void Model::setTransform(const Transform& newTransform) {
    transform = newTransform;
    updateModelMatrix();
}

Transform& Model::getTransform() {
    return transform;
}

void Model::updateModelMatrix() {
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, transform.position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(transform.size, 1.0f));
}

void Model::Draw(GLuint shaderProgram) {
    // �������Ĭ��������󶨵�����Ԫ 0
    if (textureID > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    // ���������ͶӰ����
    if (textureMode == TextureMode::PROJECTION_MAPPING && projectionTexture > 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, projectionTexture);

        // �ϴ�ͶӰ������ɫ��
        GLuint lightProjectionLoc = glGetUniformLocation(shaderProgram, "lightProjection");
        glUniformMatrix4fv(lightProjectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    }

    glBindVertexArray(VAO);

    // �ϴ�ģ�;�����ɫ��
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // �ϴ�����ģʽ����ɫ��
    GLuint textureModeLoc = glGetUniformLocation(shaderProgram, "textureMode");
    glUniform1i(textureModeLoc, static_cast<int>(textureMode)); // �ϴ�ö��ֵ��0 �� 1��

    // ��̬����ģ��
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    // �������
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Model::setTexture(unsigned int textureID) {
    this->textureID = textureID;
}

unsigned int Model::getTexture() const {
    return textureID;
}

void Model::setProjectionTexture(unsigned int texture) {
    projectionTexture = texture;
}

void Model::setProjectionMatrix(const glm::mat4& matrix) {
    projectionMatrix = matrix;
}

unsigned int Model::getProjectionTexture() const {
    return projectionTexture;
}

const glm::mat4& Model::getProjectionMatrix() const {
    return projectionMatrix;
}

const glm::mat4& Model::getModelMatrix() const {
    return modelMatrix;
}

void Model::setTextureMode(TextureMode mode) {
    textureMode = mode;
}

TextureMode Model::getTextureMode() const {
    return textureMode;
}

unsigned int Model::getVAO() const {
    return VAO;
}

unsigned int Model::getVBO() const {
    return VBO;
}

unsigned int Model::getEBO() const {
    return EBO;
}
