#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <vulkan/vulkan.h>

struct ImportedFloat4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

enum class ImportedAlphaMode : uint32_t {
	Opaque = 0,
	Mask = 1,
	Blend = 2
};

struct ImportedPrimitive {
	uint32_t indexCount = 0;

	uint32_t firstIndex = 0;

	uint32_t indexByteOffset = 0;

	VkIndexType indexType = VK_INDEX_TYPE_UINT32;

	int materialIndex = -1;

	uint32_t instanceIndex = 0;

	std::array<VkDeviceSize, 4> vertexAttributeByteOffsets{};
	std::array<uint32_t, 4> vertexBindingStrides{};
};

struct ImportedTexture
{
	std::string resolvedPath;
	int samplerIndex = -1;
};

struct ImportedSampler
{
	int magFilter = -1;
	int minFilter = -1;
	int wrapS = -1;
	int wrapT = -1;
};

struct ImportedMaterial {
	int baseColorTextureIndex = -1;
	int metallicRoughnessTextureIndex = -1;
	int normalTextureIndex = -1;

	ImportedFloat4 baseColorFactor = { 1, 1, 1, 1 };

	ImportedAlphaMode alphaMode = ImportedAlphaMode::Opaque;
	float alphaCutoff = 0.5f;
};

struct ImportedScene
{
	std::string sourcePath;

	std::vector<unsigned char> geometryBufferBytes;

	std::vector<ImportedPrimitive> primitives;
	std::vector<ImportedTexture> textures;
	std::vector<ImportedSampler> samplers;
	std::vector<ImportedMaterial> materials;

	std::vector<double> rootScale = { 1.0, 1.0, 1.0 };
};