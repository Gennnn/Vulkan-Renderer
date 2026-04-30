#pragma once

#include "ImportedScene.h"

#include <filesystem>
#include <string>

class GltfImporter {
public:
	ImportedScene Load(const std::filesystem::path& gltfPath);
private:
	static ImportedAlphaMode ConvertAlphaMode(const std::string& alphaMode);
	static std::string ResolveTexturePath(const std::filesystem::path& gltfPath, const std::string& imageUri);

	static uint32_t GetIndexElementSize(int componentType);
	static VkIndexType GetVkIndexType(int componentType);
};