#ifndef VULKAN_GLFW_H
#define VULKAN_GLFW_H
#pragma once

#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VK {
    class GLFWCallbacks {
    public:
        virtual ~GLFWCallbacks() = default;

        virtual void Key(GLFWwindow *pWindow, int Key, int Scancode, int Action, int Mods) = 0;

        virtual void MouseMove(GLFWwindow *pWindow, double xpos, double ypos) = 0;

        virtual void MouseButton(GLFWwindow *pWindow, int Button, int Action, int Mods) = 0;
    };

    // Step #1: initialize GLFW and create a window
    GLFWwindow *glfw_vulkan_init(int Width, int Height, const char *pTitle);

    // Step #2: initialize the GLFW callback mechanism
    void glfw_vulkan_set_callbacks(GLFWwindow *pWindow, GLFWCallbacks *pCallbacks);
}

#endif //VULKAN_GLFW_H
