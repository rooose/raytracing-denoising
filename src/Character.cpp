#include "Character.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

Character::Character()
{
    _viewMatrix = glm::lookAt(_pos, { 0, 0, 0 }, _up);
    _dir = glm::normalize(-_pos);
    _right = glm::normalize(glm::cross(_dir, _up));
}

Character::~Character()
{
}

void Character::update(GLFWwindow* fenetre, double deltaTime)
{
    updateMouse(fenetre, deltaTime);
    updatekeyboard(fenetre, deltaTime);

    _viewMatrix = glm::lookAt(_pos, _pos + _dir, _up);
}

void Character::setPosition(const glm::vec3& pos)
{
    _pos = pos;
}

void Character::setUp(const glm::vec3& up)
{
    _up = up;
}

void Character::setDirection(const glm::vec3& dir)
{
    _dir = dir;
}

void Character::setCharacterSpeed(float speed)
{
    _characterSpeed = speed;
}

const glm::vec3& Character::getPosition() const
{
    return _pos;
}

const glm::vec3& Character::getUp() const
{
    return _up;
}

const glm::vec3& Character::getDirection() const
{
    return _dir;
}

const glm::mat4& Character::getViewMatrix() const
{
    return _viewMatrix;
}

float Character::getCharacterSpeed() const
{
    return 0.0f;
}

void Character::onFocus(GLFWwindow* window, int focused)
{
    if (focused) {
        enableFocus(window);
    } else {
        disableFocus(window);
    }
}

void Character::updateMouse(GLFWwindow* window, double deltaTime)
{
    // Get WindowSize
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (_focused) {
        int mid_width = width / 2, mid_height = height / 2;

        // Reset mouse position for next frame
        glfwSetCursorPos(window, mid_width, mid_height);

        float xDiff = static_cast<float>(xpos - mid_width) / width;
        float yDiff = static_cast<float>(ypos - mid_height) / height;

        // New angles
        _horizontalAngle += _mouseSpeed * float(deltaTime * xDiff);
        _verticalAngle -= _mouseSpeed * float(deltaTime * yDiff);

        _dir = glm::vec3(std::cos(_verticalAngle) * std::sin(_horizontalAngle),
            std::cos(_verticalAngle) * std::cos(_horizontalAngle), std::sin(_verticalAngle));

        _up = glm::vec3(0.f, 0.f, 1.f);

        _right = glm::normalize(glm::cross(_dir, _up));
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && (xpos > 0 && xpos < width && ypos > 0 && ypos < height)) {
        enableFocus(window);
    }
}

void Character::updatekeyboard(GLFWwindow* window, double deltaTime)
{
    // TODO: Add Input manager

    // Move forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        _pos += _dir * (float)deltaTime * _characterSpeed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        _pos -= _dir * (float)deltaTime * _characterSpeed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        _pos += _right * (float)deltaTime * _characterSpeed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        _pos -= _right * (float)deltaTime * _characterSpeed;
    }

    // Quit focus mode
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        disableFocus(window);
    }
}

void Character::enableFocus(GLFWwindow* window)
{
    _focused = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void Character::disableFocus(GLFWwindow* window)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    _focused = false;
}
