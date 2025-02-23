#include"Camera.h"
#include<string>

// 获取视图矩阵
glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}

// 处理键盘输入
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

// 处理鼠标移动
void Camera::ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch) {
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw += xOffset;
    Pitch += yOffset;

    // 限制俯仰角
    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // 更新相机方向向量
    updateCameraVectors();
}

// 处理鼠标滚轮
void Camera::ProcessMouseScroll(float yOffset) {
    Zoom -= yOffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::ResetToDefault() {
    Position = glm::vec3(0.0f, 0.0f, 3.0f); // 默认位置
    Yaw = -90.0f;                           // 默认偏航角
    Pitch = 0.0f;                           // 默认俯仰角
    Zoom = 45.0f;                           // 默认缩放
    updateCameraVectors();                  // 更新摄像机方向向量
}


// 更新相机方向向量
void Camera::updateCameraVectors() {
    // 根据偏航角和俯仰角计算前方向
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // 重新计算右方向和上方向
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
