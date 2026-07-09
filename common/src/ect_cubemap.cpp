
#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <gli/gli.hpp>

#include "math-3d.h"
#include "ect_cubemap.h"

#include <corecrt_math_defines.h>

#define CUBE_MAP_INDEX_POS_X 0
#define CUBE_MAP_INDEX_NEG_X 1
#define CUBE_MAP_INDEX_POS_Y 2
#define CUBE_MAP_INDEX_NEG_Y 3
#define CUBE_MAP_INDEX_POS_Z 4
#define CUBE_MAP_INDEX_NEG_Z 5


static int types[6] = {
    CUBE_MAP_INDEX_POS_X,
    CUBE_MAP_INDEX_NEG_X,
    CUBE_MAP_INDEX_POS_Y,
    CUBE_MAP_INDEX_NEG_Y,
    CUBE_MAP_INDEX_POS_Z,
    CUBE_MAP_INDEX_NEG_Z
};

#define CUBEMAP_NUM_FACES 6


static glm::vec3 FaceCoordsToXYZ(int x, int y, int FaceID, int FaceSize) {
    const float A = 2.0f * static_cast<float>(x) / FaceSize;
    const float B = 2.0f * static_cast<float>(y) / FaceSize;

    glm::vec3 Ret;

    switch (FaceID) {
        case CUBE_MAP_INDEX_POS_X:
            Ret = glm::vec3(A - 1.0f, 1.0f, 1.0f - B);
            break;

        case CUBE_MAP_INDEX_NEG_X:
            Ret = glm::vec3(1.0f - A, -1.0f, 1.0f - B);
            break;

        case CUBE_MAP_INDEX_POS_Y:
            Ret = glm::vec3(1.0f - B, A - 1.0f, 1.0f);
            break;

        case CUBE_MAP_INDEX_NEG_Y:
            Ret = glm::vec3(B - 1.0f, A - 1.0f, -1.0f);
            break;

        case CUBE_MAP_INDEX_POS_Z:
            Ret = glm::vec3(-1.0f, A - 1.0f, 1.0f - B);
            break;

        case CUBE_MAP_INDEX_NEG_Z:
            Ret = glm::vec3(1.0f, 1.0f - A, 1.0f - B);
            break;

        default:
            assert(0);
    }

    return Ret;
}


int ConvertEquirectangularImageToCubemap(const Bitmap &b, std::vector<Bitmap> &Cubemap) {
    int FaceSize = b.w_ / 4;

    Cubemap.resize(CUBEMAP_NUM_FACES);

    for (int i = 0; i < CUBEMAP_NUM_FACES; i++) {
        Cubemap[i].Init(FaceSize, FaceSize, b.comp_, b.fmt_);
    }

    int MaxW = b.w_ - 1;
    int MaxH = b.h_ - 1;

    for (int face = 0; face < CUBEMAP_NUM_FACES; face++) {
        for (int y = 0; y < FaceSize; y++) {
            for (int x = 0; x < FaceSize; x++) {
                glm::vec3 P = FaceCoordsToXYZ(x, y, face, FaceSize);
                float R = sqrtf(P.x * P.x + P.y * P.y);
                float phi = atan2f(P.y, P.x);
                float theta = atan2f(P.z, R);

                // Calculate texture coordinates
                float u = static_cast<float>((phi + M_PI) / (2.0f * M_PI));
                float v = static_cast<float>((M_PI / 2.0f - theta) / M_PI);

                // Scale texture coordinates by image size
                float U = u * b.w_;
                float V = v * b.h_;

                // 4-samples for bilinear interpolation
                int U1 = CLAMP(static_cast<int>(floor(U)), 0, MaxW);
                int V1 = CLAMP(static_cast<int>(floor(V)), 0, MaxH);
                int U2 = CLAMP(U1 + 1, 0, MaxW);
                int V2 = CLAMP(V1 + 1, 0, MaxH);

                // Calculate the fractional part
                float s = U - U1;
                float t = V - V1;

                // Fetch 4-samples
                glm::vec4 BottomLeft = b.getPixel(U1, V1);
                glm::vec4 BottomRight = b.getPixel(U2, V1);
                glm::vec4 TopLeft = b.getPixel(U1, V2);
                glm::vec4 TopRight = b.getPixel(U2, V2);

                // Bilinear interpolation
                glm::vec4 color = BottomLeft * (1 - s) * (1 - t) +
                                  BottomRight * (s) * (1 - t) +
                                  TopLeft * (1 - s) * t +
                                  TopRight * (s) * (t);

                Cubemap[face].setPixel(x, y, color);
            } // j loop
        } // i loop
    } // Face loop

    return FaceSize;
}
