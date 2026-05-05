#pragma once

#include "../Core/GatewareConfig.h"
#include "../Scene/Camera.h"
#include "../Platform/InputSystem.h"

struct CameraControllerConfig {
	float moveSpeed = 5.0f;
	float lookSensitivity = 160.0f;
};

class CameraController {
public:
	bool Initialize(const CameraControllerConfig& config);

	void InitializeCameraView(SceneCamera& camera);
	void Update(SceneCamera& camera, InputSystem& input, float dt);
private:
	void RebuildProjection(SceneCamera& camera);
	void SetLookAt(SceneCamera& camera, const GW::MATH::GVECTORF& position, const GW::MATH::GVECTORF& target);
private:
	CameraControllerConfig config;
	GW::MATH::GMatrix math;
	float accumulatedPitch = 0.0f;
};