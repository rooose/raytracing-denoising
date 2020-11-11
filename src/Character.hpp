#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Character {
public:
    Character();
    ~Character();

    void update(GLFWwindow* fenetre, double deltaTime);

    void setPosition(const glm::vec3& pos);

    void setUp(const glm::vec3& up);

    void setDirection(const glm::vec3& dir);

    void setCharacterSpeed(float speed);

    const glm::vec3& getPosition() const;

    const glm::vec3& getUp() const;

    const glm::vec3& getDirection() const;

    const glm::mat4& getViewMatrix() const;

    float getCharacterSpeed() const;

    void onFocus(GLFWwindow* window, int focused);


private:
    void updateMouse(GLFWwindow* window, double deltaTime);

    void updatekeyboard(GLFWwindow* window, double deltaTime);

    void enableFocus(GLFWwindow* window);

    void disableFocus(GLFWwindow* window);

    void updateVecs();

private:
    glm::vec3 _pos { 2.f, 2.f, 2.f };
    
    float _characterSpeed { 0.005f };
    float _mouseSpeed { 0.2f };
    bool _focused { false };

    float _horizontalAngle { 7.f };
    float _verticalAngle { -2.5f };

    // Generated Data
    glm::vec3 _up { 0.f, 0.f, 0.1f };
    glm::vec3 _dir { 0.f, 1.f, 0.f };
    glm::vec3 _right { 0.f };
    glm::mat4 _viewMatrix { 0.f };
};