#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "camera-handler.h"

bool GLFWCameraHandler(CameraMovement &Movement, int Key, int Action, int Mods) {
    bool Press = Action != GLFW_RELEASE;

    bool Handled = true;

    switch (Key) {
        case GLFW_KEY_W:
            Movement.Forward = Press;
            break;

        case GLFW_KEY_S:
            Movement.Backward = Press;
            break;

        case GLFW_KEY_A:
            Movement.StrafeLeft = Press;
            break;

        case GLFW_KEY_D:
            Movement.StrafeRight = Press;
            break;

        case GLFW_KEY_PAGE_UP:
            Movement.Up = Press;
            break;

        case GLFW_KEY_PAGE_DOWN:
            Movement.Down = Press;
            break;

        case GLFW_KEY_KP_ADD:
            Movement.Plus = Press;
            break;

        case GLFW_KEY_KP_SUBTRACT:
            Movement.Minus = Press;
            break;

        default:
            Handled = false;
    }

    if (Mods & GLFW_MOD_SHIFT) {
        Movement.FastSpeed = Press;
    }

    return Handled;
}
