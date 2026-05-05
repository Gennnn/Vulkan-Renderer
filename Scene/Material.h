#pragma once

#include "Defines.h"
#include <cstdint>
#include "SceneTypes.h"
#include "GatewareConfig.h"

enum class AlphaMode {
	opaque,
	mask,
	blend
};

struct MaterialData {
	GW::MATH::GVECTORF baseColorFactor = { 1, 1, 1, 1 };
	TextureID baseColorTexture = InvalidSceneIndex;
	TextureID normalTexture = InvalidSceneIndex;
	TextureID metallicRoughnessTexture = InvalidSceneIndex;

	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;

	AlphaMode alphaMode = AlphaMode::opaque;

	float alphaCutoff = 0.0f;
};

struct Material {
	GW::MATH::GVECTORF baseColorFactor;

	uint32_t nrmIndex;
	uint32_t baseColorIndex;
	uint32_t metallicRoughnessIndex;
	uint32_t alphaMode;

	float alphaCutoff;
	float transmissionFactor;
	float pad0;
	float pad1;

};