
#pragma once

#include "math-3d.h"

struct BasicMeshEntry {
    u32 NumIndices = 0;
    u32 NumVertices = 0;
    u32 BaseVertex = 0;
    u32 BaseIndex = 0;
    u32 ValidFaces = 0;
    int MaterialIndex = -1;
    Matrix4f Transformation;
};