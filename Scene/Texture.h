#pragma once

#include <vulkan/vulkan.h>
#include "../RHI/Vulkan/VulkanImage.h"

struct TextureAsset {
	std::string sourcePath;
	bool isCubeMap = false;
	bool isSrgb = true;
};

struct TextureData
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

	VulkanImage image;

	unsigned int sampler = 0;

	bool ownsImage = true;
	bool ownsBuffer = false;

	TextureData() = default;

	TextureData(const TextureData&) = delete;
	TextureData& operator=(const TextureData&) = delete;

	TextureData(TextureData&&) noexcept = default;
	TextureData& operator=(TextureData&&) noexcept = default;
};