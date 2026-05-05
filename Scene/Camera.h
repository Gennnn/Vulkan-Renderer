#pragma once

#include "GatewareConfig.h"

struct SceneCamera {
	GW::MATH::GMATRIXF view = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF projection = GW::MATH::GIdentityMatrixF;

	GW::MATH::GVECTORF worldPosition = { 0,0,0,1 };

	float fovDegrees = 65.0f;
	float nearPlane = 0.1f;
	float farPlane = 10000.0f;
	float aspectRatio = 16.0f / 9.0f;
};

