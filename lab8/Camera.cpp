#include "Camera.hpp"

namespace gps {

    // Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        //Vectorii de directie initiali
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += cameraFrontDirection * speed;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= cameraFrontDirection * speed;
            break;
        case MOVE_RIGHT:
            cameraPosition += cameraRightDirection * speed;
            break;
        case MOVE_LEFT:
            cameraPosition -= cameraRightDirection * speed;
            break;
        }

        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    void Camera::setCameraPosition(glm::vec3 position) {
        this->cameraPosition = position;
    }

    glm::vec3 Camera::getCameraPosition() {
        return this->cameraPosition;

    }
    void Camera::setCameraTarget(glm::vec3 target) {
        this->cameraTarget = target;
        this->cameraFrontDirection = glm::normalize(target - this->cameraPosition);

        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, worldUp));
        this->cameraUpDirection = glm::normalize(glm::cross(this->cameraRightDirection, this->cameraFrontDirection));
    }

    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        this->cameraFrontDirection = glm::normalize(front);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }
}