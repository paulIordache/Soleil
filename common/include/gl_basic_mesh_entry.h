/*

Copyright 2025 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

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