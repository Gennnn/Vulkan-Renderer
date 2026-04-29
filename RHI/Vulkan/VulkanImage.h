#pragma once
#include <vulkan/vulkan.h>

struct VulkanImage {
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;

	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspectFlags = 0;

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipLevels = 1;

	VulkanImage() = default;

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    VulkanImage(VulkanImage&& other) noexcept
    {
        MoveFrom(other);
    }

    VulkanImage& operator=(VulkanImage&& other) noexcept
    {
        if (this != &other)
            MoveFrom(other);

        return *this;
    }

    bool IsValid() const
    {
        return image != VK_NULL_HANDLE;
    }

    void Destroy(VkDevice device) {

        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image, nullptr);
            image = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }

        format = VK_FORMAT_UNDEFINED;
        aspectFlags = 0;
        width = 0;
        height = 0;
        mipLevels = 1;
    }

private:
    void MoveFrom(VulkanImage& other) {
        image = other.image;
        imageView = other.imageView;
        memory = other.memory;
        format = other.format;
        aspectFlags = other.aspectFlags;
        width = other.width;
        height = other.height;
        mipLevels = other.mipLevels;

        other.image = VK_NULL_HANDLE;
        other.imageView = VK_NULL_HANDLE;
        other.memory = VK_NULL_HANDLE;
        other.format = VK_FORMAT_UNDEFINED;
        other.aspectFlags = 0;
        other.width = 0;
        other.height = 0;
        other.mipLevels = 1;
    }
};