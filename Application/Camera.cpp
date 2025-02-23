#include"Camera.h"
#include<string>

// ��ȡ��ͼ����
glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}

// �����������
void Camera::ProcessKeyboard(const char* direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (strcmp(direction, "FORWARD") == 0)
        Position += Front * velocity;
    if (strcmp(direction, "BACKWARD") == 0)
        Position -= Front * velocity;
    if (strcmp(direction, "LEFT") == 0)
        Position -= Right * velocity;
    if (strcmp(direction, "RIGHT") == 0)
        Position += Right * velocity;
}

// ��������ƶ�
void Camera::ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch) {
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw += xOffset;
    Pitch += yOffset;

    // ���Ƹ�����
    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // ���������������
    updateCameraVectors();
}

// ����������
void Camera::ProcessMouseScroll(float yOffset) {
    Zoom -= yOffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::ResetToDefault() {
    Position = glm::vec3(0.0f, 0.0f, 3.0f); // Ĭ��λ��
    Yaw = -90.0f;                           // Ĭ��ƫ����
    Pitch = 0.0f;                           // Ĭ�ϸ�����
    Zoom = 45.0f;                           // Ĭ������
    updateCameraVectors();                  // �����������������
}


// ���������������
void Camera::updateCameraVectors() {
    // ����ƫ���Ǻ͸����Ǽ���ǰ����
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // ���¼����ҷ�����Ϸ���
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
