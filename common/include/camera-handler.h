//
// Created by Paul on 10/6/2025.
//

#ifndef CAMERA_HANDLER_H
#define CAMERA_HANDLER_H

#pragma once

struct CameraMovement {
    bool Forward = false;
    bool Backward = false;
    bool StrafeLeft = false;
    bool StrafeRight = false;
    bool Up = false;
    bool Down = false;
    bool FastSpeed = false;
    bool Plus = false;
    bool Minus = false;
};


bool GLFWCameraHandler(CameraMovement &Movement, int Key, int Action, int Mods);

//#endif

#endif //CAMERA_HANDLER_H
