#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <cstdlib>

#include "vulkan-util.h"
#include "math-3d.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>

Vector4f& Vector4f::Normalize()
{
    glm::vec4 v(x, y, z, w);
    v = glm::normalize(v);
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;

    return *this;
}

void Vector3f::InitRandom(const Vector3f& MinVal, const Vector3f& MaxVal)
{
    x = RandomFloatRange(MinVal.x, MaxVal.x);
    y = RandomFloatRange(MinVal.y, MaxVal.y);
    z = RandomFloatRange(MinVal.z, MaxVal.z);
}

Vector3f Vector3f::Cross(const Vector3f& v) const
{
    glm::vec3 gv1(x, y, z);
    glm::vec3 gv2(v.x, v.y, v.z);
    glm::vec3 res = glm::cross(gv1, gv2);

    return Vector3f(res.x, res.y, res.z);
}

Vector3f& Vector3f::Normalize()
{
    glm::vec3 v(x, y, z);
    v = glm::normalize(v);
    x = v.x;
    y = v.y;
    z = v.z;

    return *this;
}

void Vector3f::Rotate(float Angle, const Vector3f& V)
{
    glm::vec3 axis(V.x, V.y, V.z);
    glm::quat q = glm::angleAxis(glm::radians(Angle), glm::normalize(axis));
    glm::vec3 res = q * glm::vec3(x, y, z);

    x = res.x;
    y = res.y;
    z = res.z;
}

Vector3f Vector3f::Negate() const
{
    return Vector3f(-x, -y, -z);
}

Matrix4f::Matrix4f(const Matrix3f& a)
{
    glm::mat4 gm(1.0f);
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            gm[j][i] = a.m[i][j];
        }
    }

    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ)
{
    glm::mat4 gm = glm::scale(glm::mat4(1.0f), glm::vec3(ScaleX, ScaleY, ScaleZ));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitScaleTransform(float Scale)
{
    InitScaleTransform(Scale, Scale, Scale);
}

void Matrix4f::InitScaleTransform(const Vector3f& Scale)
{
    InitScaleTransform(Scale.x, Scale.y, Scale.z);
}

void Matrix4f::InitRotateTransform(float RotateX, float RotateY, float RotateZ)
{
    glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(RotateX), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(RotateY), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(RotateZ), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 gm = rz * ry * rx;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotateTransformZYX(float RotateX, float RotateY, float RotateZ)
{
    glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(RotateX), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(RotateY), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(RotateZ), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 gm = rx * ry * rz;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotateTransform(const Vector3f& Rotate)
{
    InitRotateTransform(Rotate.x, Rotate.y, Rotate.z);
}

void Matrix4f::InitRotationX(float x)
{
    glm::mat4 gm = glm::rotate(glm::mat4(1.0f), x, glm::vec3(1.0f, 0.0f, 0.0f));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotationY(float y)
{
    glm::mat4 gm = glm::rotate(glm::mat4(1.0f), y, glm::vec3(0.0f, 1.0f, 0.0f));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotationZ(float z)
{
    glm::mat4 gm = glm::rotate(glm::mat4(1.0f), z, glm::vec3(0.0f, 0.0f, 1.0f));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotateTransform(const Quaternion& quat)
{
    glm::quat q(quat.w, quat.x, quat.y, quat.z);
    glm::mat4 gm = glm::mat4_cast(q);
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotateTransform(const glm::quat& q)
{
    glm::mat4 gm = glm::mat4_cast(q);
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitRotationFromDir(const Vector3f& Dir)
{
    Vector3f Up(0.0f, 1.0f, 0.0f);
    InitCameraTransform(Dir, Up);
}

void Matrix4f::InitTranslationTransform(float x, float y, float z)
{
    glm::mat4 gm = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitTranslationTransform(const Vector3f& Pos)
{
    InitTranslationTransform(Pos.x, Pos.y, Pos.z);
}

void Matrix4f::InitCameraTransform(const Vector3f& Target, const Vector3f& Up)
{
    glm::vec3 n = glm::normalize(glm::vec3(Target.x, Target.y, Target.z));
    glm::vec3 u = glm::normalize(glm::cross(glm::normalize(glm::vec3(Up.x, Up.y, Up.z)), n));
    glm::vec3 v = glm::cross(n, u);

    m[0][0] = u.x;   m[0][1] = u.y;   m[0][2] = u.z;   m[0][3] = 0.0f;
    m[1][0] = v.x;   m[1][1] = v.y;   m[1][2] = v.z;   m[1][3] = 0.0f;
    m[2][0] = n.x;   m[2][1] = n.y;   m[2][2] = n.z;   m[2][3] = 0.0f;
    m[3][0] = 0.0f;  m[3][1] = 0.0f;  m[3][2] = 0.0f;  m[3][3] = 1.0f;
}

void Matrix4f::InitCameraTransform(const Vector3f& Pos, const Vector3f& Target, const Vector3f& Up)
{
    glm::mat4 gm = glm::lookAtLH(glm::vec3(Pos.x, Pos.y, Pos.z),
                                 glm::vec3(Pos.x + Target.x, Pos.y + Target.y, Pos.z + Target.z),
                                 glm::vec3(Up.x, Up.y, Up.z));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitPersProjTransform(const PersProjInfo& p)
{
    glm::mat4 gm = glm::perspectiveFovLH(glm::radians(p.FOV), p.Width, p.Height, p.zNear, p.zFar);
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix4f::InitOrthoProjTransform(const OrthoProjInfo& p)
{
    glm::mat4 gm = glm::orthoLH(p.l, p.r, p.b, p.t, p.n, p.f);
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

float Matrix4f::Determinant() const
{
    glm::mat4 gm;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            gm[j][i] = m[i][j];
        }
    }
    return glm::determinant(gm);
}

Matrix4f Matrix4f::Inverse() const
{
    glm::mat4 gm;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            gm[j][i] = m[i][j];
        }
    }

    gm = glm::inverse(gm);

    Matrix4f res;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            res.m[i][j] = gm[j][i];
        }
    }
    return res;
}

void Matrix4f::CalcClipPlanes(Vector4f& l, Vector4f& r, Vector4f& b, Vector4f& t, Vector4f& n, Vector4f& f) const
{
    Vector4f Row1(m[0][0], m[0][1], m[0][2], m[0][3]);
    Vector4f Row2(m[1][0], m[1][1], m[1][2], m[1][3]);
    Vector4f Row3(m[2][0], m[2][1], m[2][2], m[2][3]);
    Vector4f Row4(m[3][0], m[3][1], m[3][2], m[3][3]);

    l = Row1 + Row4;
    r = Row1 - Row4;
    b = Row2 + Row4;
    t = Row2 - Row4;
    n = Row3 + Row4;
    f = Row3 - Row4;
}

void Matrix3f::InitRotateTransform(float RotateX, float RotateY, float RotateZ)
{
    glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(RotateX), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(RotateY), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(RotateZ), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 gm = rz * ry * rx;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix3f::InitRotationX(float x)
{
    glm::mat4 gm = glm::rotate(glm::mat4(1.0f), x, glm::vec3(1.0f, 0.0f, 0.0f));
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix3f::InitRotationY(float y)
{
    glm::mat4 gm = glm::rotate(glm::mat4(1.0f), y, glm::vec3(0.0f, 1.0f, 0.0f));
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

void Matrix3f::InitRotationZ(float z)
{
    glm::mat4 gm = glm::rotate(glm::mat4(1.0f), z, glm::vec3(0.0f, 0.0f, 1.0f));
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            m[i][j] = gm[j][i];
        }
    }
}

Quaternion::Quaternion(float Angle, const Vector3f& V)
{
    glm::quat q = glm::angleAxis(glm::radians(Angle), glm::normalize(glm::vec3(V.x, V.y, V.z)));
    x = q.x;
    y = q.y;
    z = q.z;
    w = q.w;
}

Quaternion::Quaternion(float _x, float _y, float _z, float _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;
}

void Quaternion::Normalize()
{
    glm::quat q(w, x, y, z);
    q = glm::normalize(q);
    x = q.x;
    y = q.y;
    z = q.z;
    w = q.w;
}

Quaternion Quaternion::Conjugate() const
{
    glm::quat q(w, x, y, z);
    q = glm::conjugate(q);
    return Quaternion(q.x, q.y, q.z, q.w);
}

Quaternion operator*(const Quaternion& q, const Vector3f& v)
{
    glm::quat gq(q.w, q.x, q.y, q.z);
    glm::quat gv(0.0f, v.x, v.y, v.z);
    glm::quat res = gq * gv;
    return Quaternion(res.x, res.y, res.z, res.w);
}

Quaternion operator*(const Quaternion& l, const Quaternion& r)
{
    glm::quat gl(l.w, l.x, l.y, l.z);
    glm::quat gr(r.w, r.x, r.y, r.z);
    glm::quat res = gl * gr;
    return Quaternion(res.x, res.y, res.z, res.w);
}

Vector3f Quaternion::ToDegrees()
{
    glm::quat q(w, x, y, z);
    glm::vec3 euler = glm::degrees(glm::eulerAngles(q));
    return Vector3f(euler.x, euler.y, euler.z);
}

bool Quaternion::IsZero() const
{
    return (x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f);
}

float RandomFloatRange(float Start, float End)
{
    if (End == Start) {
        return Start;
    }

    if (End < Start) {
        exit(0);
    }

    float Delta = End - Start;
    float RandomValue = RandomFloat() * Delta + Start;

    return RandomValue;
}

void CalcTightLightProjection(const Matrix4f& CameraView,
                              const Vector3f& LightDir,
                              const PersProjInfo& persProjInfo,
                              Vector3f& LightPosWorld,
                              OrthoProjInfo& orthoProjInfo)
{
    Frustum frustum;
    frustum.CalcCorners(persProjInfo);

    Matrix4f InverseCameraView = CameraView.Inverse();
    frustum.Transform(InverseCameraView);

    Frustum view_frustum_in_world_space = frustum;

    Matrix4f LightView;
    Vector3f Origin(0.0f, 0.0f, 0.0f);
    Vector3f Up(0.0f, 1.0f, 0.0f);
    LightView.InitCameraTransform(Origin, LightDir, Up);
    frustum.Transform(LightView);

    AABB aabb;
    frustum.CalcAABB(aabb);

    Vector3f BottomLeft(aabb.MinX, aabb.MinY, aabb.MinZ);
    Vector3f TopRight(aabb.MaxX, aabb.MaxY, aabb.MinZ);
    Vector4f LightPosWorld4d = Vector4f((BottomLeft + TopRight) / 2.0f, 1.0f);

    Matrix4f LightViewInv = LightView.Inverse();
    glm::mat4 gmLightViewInv;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            gmLightViewInv[j][i] = LightViewInv.m[i][j];
        }
    }

    glm::vec4 gLightPosWorld4d(LightPosWorld4d.x, LightPosWorld4d.y, LightPosWorld4d.z, LightPosWorld4d.w);
    gLightPosWorld4d = gmLightViewInv * gLightPosWorld4d;

    LightPosWorld = Vector3f(gLightPosWorld4d.x, gLightPosWorld4d.y, gLightPosWorld4d.z);

    LightView.InitCameraTransform(LightPosWorld, LightDir, Up);
    view_frustum_in_world_space.Transform(LightView);

    AABB final_aabb;
    view_frustum_in_world_space.CalcAABB(final_aabb);
    final_aabb.UpdateOrthoInfo(orthoProjInfo);
}

int CalcNextPowerOfTwo(int x)
{
    int ret = 1;

    if (x == 1) {
        return 2;
    }

    while (ret < x) {
        ret = ret * 2;
    }

    return ret;
}

bool IsPointInsideViewFrustum(const Vector3f& p, const Matrix4f& VP)
{
    glm::mat4 gmVP;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            gmVP[j][i] = VP.m[i][j];
        }
    }

    glm::vec4 p4D(p.x, p.y, p.z, 1.0f);
    glm::vec4 ClipSpaceP = gmVP * p4D;

    bool InsideViewFrustum = ((ClipSpaceP.x <=  ClipSpaceP.w) &&
                              (ClipSpaceP.x >= -ClipSpaceP.w) &&
                              (ClipSpaceP.y <=  ClipSpaceP.w) &&
                              (ClipSpaceP.y >= -ClipSpaceP.w) &&
                              (ClipSpaceP.z <=  ClipSpaceP.w) &&
                              (ClipSpaceP.z >= -ClipSpaceP.w));

    return InsideViewFrustum;
}

glm::quat RotationBetweenVectors(glm::vec3& start, glm::vec3& dest)
{
    start = glm::normalize(start);
    dest = glm::normalize(dest);

    float cosTheta = glm::dot(start, dest);
    glm::vec3 RotationAxis;

    glm::quat ret;

    if (cosTheta < -1 + 0.001f) {
        RotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);

        if (glm::length2(RotationAxis) < 0.01) {
            RotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
        }

        RotationAxis = glm::normalize(RotationAxis);
        ret = glm::angleAxis(glm::radians(180.0f), RotationAxis);
    } else {
        RotationAxis = glm::cross(start, dest);

        float s = sqrt((1 + cosTheta) * 2);
        float invs = 1 / s;

        ret = glm::quat(s * 0.5f, RotationAxis.x * invs, RotationAxis.y * invs, RotationAxis.z * invs);
    }

    return ret;
}