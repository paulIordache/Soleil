#include <cstdio>
#include <cstdlib>

#include <GL/glew.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan-app.h"
#include "vulkan-core.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080


void GLFW_KeyCallback(GLFWwindow *window, const int key, int scancode, const int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

#define APP_NAME "Vulkan Engine"

int main(int argc, char *argv[]) {
    VK::VulkanApp App(WINDOW_WIDTH, WINDOW_HEIGHT);

    App.init(APP_NAME);

    App.Execute();

    return 0;
}
