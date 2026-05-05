#pragma once

#include "Scene.h"
#include "../assets/ImportedScene.h"

struct SceneLoaderInfo {
	float defaultFOV = 65.0f;
	float defaultNear = 0.1f;
	float defaultFar = 10000.0f;
	float defaultAspect = 16.0f / 9.0f;

	GW::MATH::GVECTORF defaultCameraPosition = { 0,3,-8,1 };
	GW::MATH::GVECTORF defaultLightDirection = { -0.577f, -0.577f, -0.577f, 0.0f };
};

class SceneLoader {
public:
	Scene BuildSceneFromImported(const ImportedScene& imported, const SceneLoaderInfo& info);
};