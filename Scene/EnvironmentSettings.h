#pragma once

#include "SceneTypes.h"

struct EnvironmentSettings {
	TextureID brdfLut = InvalidSceneIndex;
	TextureID diffuseIrradiance = InvalidSceneIndex;
	TextureID specularPrefilter = InvalidSceneIndex;
	TextureID skybox = InvalidSceneIndex;

	float skyboxIntensity = 1.0f;
	float environmentRotation = 0.0f;
};