#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vulkan/vulkan.h>

#include "camera-api.h"
#include "math-3d.h"
#include "camera-handler.h"
#include "camera-api.h"

static bool constexpr CAMERA_LEFT_HANDED = true;

struct MouseState {
	glm::vec2 m_pos = glm::vec2(0.0f);
	bool m_buttonPressed = false;
};

class GLMCameraFirstPerson : public CameraAPI {

public:

	CameraMovement m_movement;
	float m_acceleration = 40.0f;
	float m_damping = 5.0f;
	float m_maxSpeed = 10.0f;
	float m_fastCoef = 10.0f;
	float m_mouseSpeed = 4.0f;

	GLMCameraFirstPerson() {}

	GLMCameraFirstPerson(const glm::vec3& Pos, const glm::vec3& Target,
		                 const glm::vec3& Up, PersProjInfo& persProjInfo);

	void Init(const glm::vec3& Pos, const glm::vec3& Target,
			  const glm::vec3& Up, PersProjInfo& persProjInfo);

	void Update(float dt);

	void SetMousePos(float x, float y);

	void HandleMouseButton(int Button, int Action, int Mods);

	const glm::mat4& GetProjMatrixGLM() const { return m_persProjection; }

	glm::vec3 GetPosition() const { return m_cameraPos; }

	glm::vec3 GetTarget() const;

	glm::vec3 GetUp() const { return m_up; }

	glm::mat4 GetViewMatrix() const;

	glm::mat4 GetVPMatrix() const;

	glm::mat4 GetVPMatrixNoTranslate() const;

	const PersProjInfo& GetPersProjInfo() const { return m_persProjInfo; }

	void SetPos(const glm::vec3& Pos) { m_cameraPos = Pos; }

	void SetUp(const glm::vec3& Up) { m_up = Up; }

	void SetTarget(const glm::vec3& Target);

	void SetAbsTarget(const glm::vec3& Target);

	void Print() const;

	// Implementation of CameraAPI

	virtual const Vector3f GetPos() const;

	virtual Matrix4f GetViewportMatrix() const;

	virtual Matrix4f GetMatrix() const;

	// TODO: rename to match the GLM version
	virtual const Matrix4f GetProjectionMat() const;

private:

	glm::vec3 CalcAcceleration();
	void CalcVelocity(float dt);
	void CalcCameraOrientation();
	void SetUpVector();

	glm::mat4 m_persProjection = glm::mat4(0.0);
	glm::vec3 m_cameraPos = glm::vec3(0.0f);
	glm::quat m_cameraOrientation = glm::quat(glm::vec3(0.0f));
	glm::vec3 m_velocity = glm::vec3(0.0f);
	glm::vec2 m_oldMousePos = glm::vec2(0.0f);
	glm::vec3 m_up = glm::vec3(0.0f);
	MouseState m_mouseState;
	PersProjInfo m_persProjInfo;
};