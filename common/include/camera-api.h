

#pragma once

#include "math-3d.h"

class CameraAPI {
public:
    virtual ~CameraAPI() = default;

    virtual const Vector3f GetPos() const = 0;

    virtual Matrix4f GetViewportMatrix() const = 0;

    virtual Matrix4f GetMatrix() const = 0;

    virtual const Matrix4f GetProjectionMat() const = 0;
};