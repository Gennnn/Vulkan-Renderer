#pragma once
#include <cstdint>
#include <string>
struct RendererConfig {
	std::string startupScenePath = "../Sponza/glTF/Sponza.gltf";

	std::string brdfLutPath = "../Models/Textures/lut_ggx.png";
	std::string diffuseIrradiancePath = "../Models/Textures/diffuse.ktx2";
	std::string specularPrefilterPath = "../Models/Textures/specular.ktx2";
	std::string skyboxPath = "../Models/Textures/skybox_rgba8.ktx2";

	float fovDegrees = 65.0f;
	float nearPlane = 0.1f;
	float farPlane = 10000.0f;
	float lookSens = 160.0f;
	float cameraSpeed = 5.0f;

	uint32_t shadowCascadeCount = 4;
	uint32_t shadowMapSize = 2048;
	float shadowDistance = 80.0f;

	bool enableBloom = true;
	bool enableShadows = true;

	float clearColor[4] = { 0.2f, 0.0f, 0.45f, 1.0f };
	float clearDepth = 0.0f;
	uint32_t clearStencil = 1;
};