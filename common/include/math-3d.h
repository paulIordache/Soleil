#ifndef MATH_3D_H
#define MATH_3D_H

#ifdef _WIN64
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#else
#include <math.h>
#endif
#include <cstdio>
#include <cfloat>
#include <numbers>

#include "vulkan-util.h"

#include <assimp/vector3.h>
#include <vulkan/vulkan.h>
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#define powi(base,exp) (int)powf((float)(base), (float)(exp))

#define ToRadian(x) (float)(((x) * std::numbers::pi / 180.0f))
#define ToDegree(x) (float)(((x) * 180.0f / std::numbers::pi))

static float RandomFloat()
{
    float Max = RAND_MAX;
    return (static_cast<float>(RANDOM()) / Max);
}

float RandomFloatRange(float Start, float End);

struct Vector2i
{
    Vector2i() {}

    Vector2i(int xi, int yi)
    {
        x = xi;
        y = yi;
    }

    int x = 0;
    int y = 0;
};

struct Vector3i
{
    union {
        float x;
        float r;
    };

    union {
        float y;
        float g;
    };

    union {
        float z;
        float b;
    };
};

struct Vector2f
{
    union {
        float x = 0.0f;
        float u;
    };

    union {
        float y = 0.0f;
        float v;
    };

    Vector2f()
    {
    }

    Vector2f(float f)
    {
        x = f;
        y = f;
    }

    Vector2f(float _x, float _y)
    {
        x = _x;
        y = _y;
    }

    void Print(bool endl = true) const
    {
        printf("(%f, %f)", x, y);

        if (endl) {
            printf("\n");
        }
    }

    float Length() const
    {
        return glm::length(glm::vec2(x, y));
    }

    void Normalize()
    {
        glm::vec2 v = glm::normalize(glm::vec2(x, y));
        x = v.x;
        y = v.y;
    }
};

inline Vector2f operator*(const Vector2f& l, float f)
{
    glm::vec2 v = glm::vec2(l.x, l.y) * f;
    return Vector2f(v.x, v.y);
}

struct Vector4f;

struct Vector3f
{
    union {
        float x = 0.0f;
        float r;
    };

    union {
        float y = 0.0f;
        float g;
    };

    union {
        float z = 0.0f;
        float b;
    };

    Vector3f() {}

    Vector3f(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    Vector3f(const float* pFloat)
    {
        x = pFloat[0];
        y = pFloat[1];
        z = pFloat[2];
    }

    Vector3f(const glm::vec3& v)
    {
        x = v[0];
        y = v[1];
        z = v[2];
    }

    void InitRandom(const Vector3f& MinVal, const Vector3f& MaxVal);

    void InitBySphericalCoords(float Radius, float Pitch, float Heading)
    {
        glm::vec3 v(
            Radius * cosf(glm::radians(Pitch)) * sinf(glm::radians(Heading)),
            -Radius * sinf(glm::radians(Pitch)),
            Radius * cosf(glm::radians(Pitch)) * cosf(glm::radians(Heading))
        );
        x = v.x;
        y = v.y;
        z = v.z;
    }

    Vector3f(float f)
    {
        x = y = z = f;
    }

    Vector3f(const Vector4f& v);

    Vector3f& operator+=(const Vector3f& r)
    {
        glm::vec3 v = glm::vec3(x, y, z) + glm::vec3(r.x, r.y, r.z);
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    Vector3f& operator-=(const Vector3f& r)
    {
        glm::vec3 v = glm::vec3(x, y, z) - glm::vec3(r.x, r.y, r.z);
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    Vector3f& operator*=(float f)
    {
        glm::vec3 v = glm::vec3(x, y, z) * f;
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    bool operator==(const Vector3f& r)
    {
        return glm::all(glm::equal(glm::vec3(x, y, z), glm::vec3(r.x, r.y, r.z)));
    }

    bool operator!=(const Vector3f& r)
    {
        return glm::any(glm::notEqual(glm::vec3(x, y, z), glm::vec3(r.x, r.y, r.z)));
    }

    operator const float*() const
    {
        return &(x);
    }

    Vector3f Cross(const Vector3f& v) const;

    float Dot(const Vector3f& v) const
    {
        return glm::dot(glm::vec3(x, y, z), glm::vec3(v.x, v.y, v.z));
    }

    float Distance(const Vector3f& v) const
    {
        return glm::distance(glm::vec3(x, y, z), glm::vec3(v.x, v.y, v.z));
    }

    float Length() const
    {
        return glm::length(glm::vec3(x, y, z));
    }

    bool IsZero() const
    {
        return glm::all(glm::equal(glm::vec3(x, y, z), glm::vec3(0.0f)));
    }

    Vector3f& Normalize();

    void Rotate(float Angle, const Vector3f& Axis);

    Vector3f Negate() const;

    void Print(bool endl = true) const
    {
        printf("(%f, %f, %f)", x, y, z);

        if (endl) {
            printf("\n");
        }
    }

    float* data()
    {
        return &x;
    }

    void SetAll(float f)
    {
        glm::vec3 v(f);
        x = v.x;
        y = v.y;
        z = v.z;
    }

    void SetZero()
    {
        SetAll(0.0f);
    }

    glm::vec3 ToGLM() const
    {
        return glm::vec3(x, y, z);
    }
};

struct Vector4f
{
    union {
        float x = 0.0f;
        float r;
    };

    union {
        float y = 0.0f;
        float g;
    };

    union {
        float z = 0.0f;
        float b;
    };

    union {
        float w = 0.0f;
        float a;
    };

    Vector4f()
    {
    }

    Vector4f(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

    Vector4f(const Vector3f& v, float _w)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        w = _w;
    }

    Vector4f(float f)
    {
        x = y = z = w = f;
    }

    void Print(bool endl = true) const
    {
        printf("(%f, %f, %f, %f)", x, y, z, w);

        if (endl) {
            printf("\n");
        }
    }

    Vector3f to3D() const
    {
        return Vector3f(x, y, z);
    }

    float Length() const
    {
        return glm::length(glm::vec4(x, y, z, w));
    }

    Vector4f& Normalize();

    float Dot(const Vector4f& v) const
    {
        return glm::dot(glm::vec4(x, y, z, w), glm::vec4(v.x, v.y, v.z, v.w));
    }

    bool operator==(const Vector4f& r)
    {
        return glm::all(glm::equal(glm::vec4(x, y, z, w), glm::vec4(r.x, r.y, r.z, r.w)));
    }

    bool operator!=(const Vector4f& r)
    {
        return glm::any(glm::notEqual(glm::vec4(x, y, z, w), glm::vec4(r.x, r.y, r.z, r.w)));
    }

    const float* data() const
    {
        const float* p = &x;
        return p;
    }
};

inline Vector3f operator+(const Vector3f& l, const Vector3f& r)
{
    glm::vec3 v = glm::vec3(l.x, l.y, l.z) + glm::vec3(r.x, r.y, r.z);
    return Vector3f(v.x, v.y, v.z);
}

inline Vector3f operator-(const Vector3f& l, const Vector3f& r)
{
    glm::vec3 v = glm::vec3(l.x, l.y, l.z) - glm::vec3(r.x, r.y, r.z);
    return Vector3f(v.x, v.y, v.z);
}

inline Vector3f operator*(const Vector3f& l, float f)
{
    glm::vec3 v = glm::vec3(l.x, l.y, l.z) * f;
    return Vector3f(v.x, v.y, v.z);
}

inline Vector3f operator/(const Vector3f& l, float f)
{
    glm::vec3 v = glm::vec3(l.x, l.y, l.z) / f;
    return Vector3f(v.x, v.y, v.z);
}

inline Vector3f::Vector3f(const Vector4f& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
}

inline Vector4f operator+(const Vector4f& l, const Vector4f& r)
{
    glm::vec4 v = glm::vec4(l.x, l.y, l.z, l.w) + glm::vec4(r.x, r.y, r.z, r.w);
    return Vector4f(v.x, v.y, v.z, v.w);
}

inline Vector4f operator-(const Vector4f& l, const Vector4f& r)
{
    glm::vec4 v = glm::vec4(l.x, l.y, l.z, l.w) - glm::vec4(r.x, r.y, r.z, r.w);
    return Vector4f(v.x, v.y, v.z, v.w);
}

inline Vector4f operator/(const Vector4f& l, float f)
{
    glm::vec4 v = glm::vec4(l.x, l.y, l.z, l.w) / f;
    return Vector4f(v.x, v.y, v.z, v.w);
}

inline Vector4f operator*(const Vector4f& l, float f)
{
    glm::vec4 v = glm::vec4(l.x, l.y, l.z, l.w) * f;
    return Vector4f(v.x, v.y, v.z, v.w);
}

inline Vector4f operator*(float f, const Vector4f& l)
{
    glm::vec4 v = f * glm::vec4(l.x, l.y, l.z, l.w);
    return Vector4f(v.x, v.y, v.z, v.w);
}

inline Vector4f operator*(const Vector4f& l, const Vector4f& r)
{
    glm::vec4 v = glm::vec4(l.x, l.y, l.z, l.w) * glm::vec4(r.x, r.y, r.z, r.w);
    return Vector4f(v.x, v.y, v.z, v.w);
}

struct PersProjInfo
{
    float FOV = 0.0f;
    float Width = 0.0f;
    float Height = 0.0f;
    float zNear = 0.0f;
    float zFar = 0.0f;
};

struct OrthoProjInfo
{
    float r;
    float l;
    float b;
    float t;
    float n;
    float f;

    float Width;
    float Height;

    void Print()
    {
        printf("Left %f   Right %f\n", l, r);
        printf("Bottom %f Top %f\n", b, t);
        printf("Near %f   Far %f\n", n, f);
    }
};

struct Quaternion
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    Quaternion() {}

    Quaternion(float Angle, const Vector3f& V);

    Quaternion(float _x, float _y, float _z, float _w);

    void Normalize();

    Quaternion Conjugate() const;

    Vector3f ToDegrees();

    bool IsZero() const;
};

Quaternion operator*(const Quaternion& l, const Quaternion& r);

Quaternion operator*(const Quaternion& q, const Vector3f& v);

class Matrix3f;

class Matrix4f
{
public:
    float m[4][4] = { 0.0f };

    Matrix4f()  {}

    Matrix4f(float a00, float a01, float a02, float a03,
             float a10, float a11, float a12, float a13,
             float a20, float a21, float a22, float a23,
             float a30, float a31, float a32, float a33)
    {
        m[0][0] = a00; m[0][1] = a01; m[0][2] = a02; m[0][3] = a03;
        m[1][0] = a10; m[1][1] = a11; m[1][2] = a12; m[1][3] = a13;
        m[2][0] = a20; m[2][1] = a21; m[2][2] = a22; m[2][3] = a23;
        m[3][0] = a30; m[3][1] = a31; m[3][2] = a32; m[3][3] = a33;
    }

    Matrix4f(const aiMatrix4x4& AssimpMatrix)
    {
        m[0][0] = AssimpMatrix.a1; m[0][1] = AssimpMatrix.a2; m[0][2] = AssimpMatrix.a3; m[0][3] = AssimpMatrix.a4;
        m[1][0] = AssimpMatrix.b1; m[1][1] = AssimpMatrix.b2; m[1][2] = AssimpMatrix.b3; m[1][3] = AssimpMatrix.b4;
        m[2][0] = AssimpMatrix.c1; m[2][1] = AssimpMatrix.c2; m[2][2] = AssimpMatrix.c3; m[2][3] = AssimpMatrix.c4;
        m[3][0] = AssimpMatrix.d1; m[3][1] = AssimpMatrix.d2; m[3][2] = AssimpMatrix.d3; m[3][3] = AssimpMatrix.d4;
    }

    Matrix4f(const aiMatrix3x3& AssimpMatrix)
    {
        m[0][0] = AssimpMatrix.a1; m[0][1] = AssimpMatrix.a2; m[0][2] = AssimpMatrix.a3; m[0][3] = 0.0f;
        m[1][0] = AssimpMatrix.b1; m[1][1] = AssimpMatrix.b2; m[1][2] = AssimpMatrix.b3; m[1][3] = 0.0f;
        m[2][0] = AssimpMatrix.c1; m[2][1] = AssimpMatrix.c2; m[2][2] = AssimpMatrix.c3; m[2][3] = 0.0f;
        m[3][0] = 0.0f           ; m[3][1] = 0.0f           ; m[3][2] = 0.0f           ; m[3][3] = 1.0f;
    }

    Matrix4f(const glm::mat4& a)
    {
        m[0][0] = a[0][0]; m[0][1] = a[1][0]; m[0][2] = a[2][0]; m[0][3] = a[3][0];
        m[1][0] = a[0][1]; m[1][1] = a[1][1]; m[1][2] = a[2][1]; m[1][3] = a[3][1];
        m[2][0] = a[0][2]; m[2][1] = a[1][2]; m[2][2] = a[2][2]; m[2][3] = a[3][2];
        m[3][0] = a[0][3]; m[3][1] = a[1][3]; m[3][2] = a[2][3]; m[3][3] = a[3][3];
    }

    Matrix4f(const Matrix3f& a);

    void SetZero()
    {
        ZERO_MEM(m);
    }

    Matrix4f Transpose() const
    {
        glm::mat4 gm;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                gm[j][i] = m[i][j];
            }
        }
        gm = glm::transpose(gm);
        Matrix4f n;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                n.m[i][j] = gm[j][i];
            }
        }
        return n;
    }

    inline void InitIdentity()
    {
        glm::mat4 gm(1.0f);
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                m[i][j] = gm[j][i];
            }
        }
    }

    inline Matrix4f operator*(const Matrix4f& Right) const
    {
        glm::mat4 l;
        glm::mat4 r;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                l[j][i] = m[i][j];
                r[j][i] = Right.m[i][j];
            }
        }
        glm::mat4 res = l * r;
        Matrix4f Ret;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                Ret.m[i][j] = res[j][i];
            }
        }
        return Ret;
    }

    Vector4f operator*(const Vector4f& v) const
    {
        glm::mat4 gm;
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                gm[j][i] = m[i][j];
            }
        }
        glm::vec4 gv(v.x, v.y, v.z, v.w);
        glm::vec4 res = gm * gv;
        return Vector4f(res.x, res.y, res.z, res.w);
    }

    operator const float*() const
    {
        return &(m[0][0]);
    }

    const float* data() const
    {
        return &(m[0][0]);
    }

    void Print() const
    {
        for (int i = 0 ; i < 4 ; i++) {
            printf("%f %f %f %f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
        }
    }

    float Determinant() const;

    Matrix4f Inverse() const;

    void InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ);
    void InitScaleTransform(float Scale);
    void InitScaleTransform(const Vector3f& Scale);

    void InitRotateTransform(float RotateX, float RotateY, float RotateZ);
    void InitRotateTransformZYX(float RotateX, float RotateY, float RotateZ);
    void InitRotateTransform(const Vector3f& Rotate);
    void InitRotateTransform(const Quaternion& quat);
    void InitRotateTransform(const glm::quat& quat);
    void InitRotationFromDir(const Vector3f& Dir);

    void InitTranslationTransform(float x, float y, float z);
    void InitTranslationTransform(const Vector3f& Pos);

    void InitCameraTransform(const Vector3f& Target, const Vector3f& Up);

    void InitCameraTransform(const Vector3f& Pos, const Vector3f& Target, const Vector3f& Up);

    void InitPersProjTransform(const PersProjInfo& p);

    void InitOrthoProjTransform(const OrthoProjInfo& p);

    void CalcClipPlanes(Vector4f& l, Vector4f& r, Vector4f& b, Vector4f& t, Vector4f& n, Vector4f& f) const;

private:
    void InitRotationX(float RotateX);
    void InitRotationY(float RotateY);
    void InitRotationZ(float RotateZ);
};

class Matrix3f
{
public:
    float m[3][3] = { 0.0f };

    Matrix3f()  {}

    Matrix3f(const Matrix4f& a)
    {
        m[0][0] = a.m[0][0]; m[0][1] = a.m[0][1]; m[0][2] = a.m[0][2];
        m[1][0] = a.m[1][0]; m[1][1] = a.m[1][1]; m[1][2] = a.m[1][2];
        m[2][0] = a.m[2][0]; m[2][1] = a.m[2][1]; m[2][2] = a.m[2][2];
    }

    Vector3f operator*(const Vector3f& v) const
    {
        glm::mat3 gm;
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                gm[j][i] = m[i][j];
            }
        }
        glm::vec3 gv(v.x, v.y, v.z);
        glm::vec3 res = gm * gv;
        return Vector3f(res.x, res.y, res.z);
    }

    inline Matrix3f operator*(const Matrix3f& Right) const
    {
        glm::mat3 l;
        glm::mat3 r;
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                l[j][i] = m[i][j];
                r[j][i] = Right.m[i][j];
            }
        }
        glm::mat3 res = l * r;
        Matrix3f Ret;
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                Ret.m[i][j] = res[j][i];
            }
        }
        return Ret;
    }

    Matrix3f Transpose() const
    {
        glm::mat3 gm;
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                gm[j][i] = m[i][j];
            }
        }
        gm = glm::transpose(gm);
        Matrix3f n;
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                n.m[i][j] = gm[j][i];
            }
        }
        return n;
    }

    void InitRotateTransform(float RotateX, float RotateY, float RotateZ);

    void Print() const
    {
        for (int i = 0 ; i < 3 ; i++) {
            printf("%f %f %f\n", m[i][0], m[i][1], m[i][2]);
        }
    }

private:
    void InitRotationX(float RotateX);
    void InitRotationY(float RotateY);
    void InitRotationZ(float RotateZ);
};

class AABB
{
public:
    AABB() {}

    void Add(const Vector3f& v)
    {
        // Add parentheses around glm::min and glm::max to bypass Windows macros
        glm::vec3 min_vec = (glm::min)(glm::vec3(MinX, MinY, MinZ), glm::vec3(v.x, v.y, v.z));
        glm::vec3 max_vec = (glm::max)(glm::vec3(MaxX, MaxY, MaxZ), glm::vec3(v.x, v.y, v.z));

        MinX = min_vec.x;
        MinY = min_vec.y;
        MinZ = min_vec.z;

        MaxX = max_vec.x;
        MaxY = max_vec.y;
        MaxZ = max_vec.z;
    }

    float MinX = FLT_MAX;
    float MaxX = FLT_MIN;
    float MinY = FLT_MAX;
    float MaxY = FLT_MIN;
    float MinZ = FLT_MAX;
    float MaxZ = FLT_MIN;

    void Print()
    {
        printf("X: [%f,%f]\n", MinX, MaxX);
        printf("Y: [%f,%f]\n", MinY, MaxY);
        printf("Z: [%f,%f]\n", MinZ, MaxZ);
    }

    void UpdateOrthoInfo(struct OrthoProjInfo& o)
    {
        o.r = MaxX;
        o.l = MinX;
        o.b = MinY;
        o.t = MaxY;
        o.n = MinZ;
        o.f = MaxZ;
    }
};

class Frustum
{
public:
    Vector4f NearTopLeft;
    Vector4f NearBottomLeft;
    Vector4f NearTopRight;
    Vector4f NearBottomRight;

    Vector4f FarTopLeft;
    Vector4f FarBottomLeft;
    Vector4f FarTopRight;
    Vector4f FarBottomRight;

    Frustum() {}

    void CalcCorners(const PersProjInfo& persProjInfo)
    {
        float AR = persProjInfo.Height / persProjInfo.Width;

        float tanHalfFOV = tanf(glm::radians(persProjInfo.FOV / 2.0f));

        float NearZ = persProjInfo.zNear;
        float NearX = NearZ * tanHalfFOV;
        float NearY = NearZ * tanHalfFOV * AR;

        NearTopLeft     = Vector4f(-NearX, NearY, NearZ, 1.0f);
        NearBottomLeft  = Vector4f(-NearX, -NearY, NearZ, 1.0f);
        NearTopRight    = Vector4f(NearX, NearY, NearZ, 1.0f);
        NearBottomRight = Vector4f(NearX, -NearY, NearZ, 1.0f);

        float FarZ = persProjInfo.zFar;
        float FarX = FarZ * tanHalfFOV;
        float FarY = FarZ * tanHalfFOV * AR;

        FarTopLeft     = Vector4f(-FarX, FarY, FarZ, 1.0f);
        FarBottomLeft  = Vector4f(-FarX, -FarY, FarZ, 1.0f);
        FarTopRight    = Vector4f(FarX, FarY, FarZ, 1.0f);
        FarBottomRight = Vector4f(FarX, -FarY, FarZ, 1.0f);
    }

    void Transform(const Matrix4f& m)
    {
         NearTopLeft     = m * NearTopLeft;
         NearBottomLeft  = m * NearBottomLeft;
         NearTopRight    = m * NearTopRight;
         NearBottomRight = m * NearBottomRight;

         FarTopLeft     = m * FarTopLeft;
         FarBottomLeft  = m * FarBottomLeft;
         FarTopRight    = m * FarTopRight;
         FarBottomRight = m * FarBottomRight;
    }

    void CalcAABB(AABB& aabb)
    {
        aabb.Add(NearTopLeft);
        aabb.Add(NearBottomLeft);
        aabb.Add(NearTopRight);
        aabb.Add(NearBottomRight);

        aabb.Add(FarTopLeft);
        aabb.Add(FarBottomLeft);
        aabb.Add(FarTopRight);
        aabb.Add(FarBottomRight);
    }

    void Print()
    {
        printf("NearTopLeft "); NearTopLeft.Print();
        printf("NearBottomLeft "); NearBottomLeft.Print();
        printf("NearTopRight "); NearTopRight.Print();
        printf("NearBottomLeft "); NearBottomRight.Print();

        printf("FarTopLeft "); FarTopLeft.Print();
        printf("FarBottomLeft "); FarBottomLeft.Print();
        printf("FarTopRight "); FarTopRight.Print();
        printf("FarBottomLeft "); FarBottomRight.Print();
    }
};

class FrustumCulling
{
public:
    FrustumCulling(const Matrix4f& ViewProj)
    {
        Update(ViewProj);
    }

    void Update(const Matrix4f& ViewProj)
    {
        ViewProj.CalcClipPlanes(m_leftClipPlane,
                                m_rightClipPlane,
                                m_bottomClipPlane,
                                m_topClipPlane,
                                m_nearClipPlane,
                                m_farClipPlane);
    }

    bool IsPointInsideViewFrustum(const Vector3f& p) const
    {
        Vector4f p4D(p, 1.0f);

        bool Inside =
            (m_leftClipPlane.Dot(p4D)   >= 0) &&
            (m_rightClipPlane.Dot(p4D)  <= 0) &&
            (m_nearClipPlane.Dot(p4D)   >= 0) &&
            (m_farClipPlane.Dot(p4D)    <= 0);

        return Inside;
    }

private:
    Vector4f m_leftClipPlane;
    Vector4f m_rightClipPlane;
    Vector4f m_bottomClipPlane;
    Vector4f m_topClipPlane;
    Vector4f m_nearClipPlane;
    Vector4f m_farClipPlane;
};

void CalcTightLightProjection(const Matrix4f& CameraView,
                              const Vector3f& LightDir,
                              const PersProjInfo& persProjInfo,
                              Vector3f& LightPosWorld,
                              OrthoProjInfo& orthoProjInfo);

int CalcNextPowerOfTwo(int x);

bool IsPointInsideViewFrustum(const Vector3f& p, const Matrix4f& VP);

glm::quat RotationBetweenVectors(glm::vec3& start, glm::vec3& dest);

#define GLM_PRINT_VEC3(s, v) printf("%s (%f,%f,%f)\n", s, v.x, v.y, v.z)

#endif