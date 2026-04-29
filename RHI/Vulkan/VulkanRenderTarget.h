#pragma once

#include "VulkanImage.h"
#include <utility>

struct VulkanRenderTarget {
	VulkanImage image;
	VkFramebuffer framebuffer = VK_NULL_HANDLE;

    VulkanRenderTarget() = default;

    VulkanRenderTarget(const VulkanRenderTarget&) = delete;
    VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

    VulkanRenderTarget(VulkanRenderTarget&& other) noexcept
    {
        MoveFrom(other);
    }

    VulkanRenderTarget& operator=(VulkanRenderTarget&& other) noexcept
    {
        if (this != &other)
            MoveFrom(other);

        return *this;
    }

    void DestroyFramebuffer(VkDevice device)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }

    void Destroy(VkDevice device)
    {
        DestroyFramebuffer(device);
        image.Destroy(device);
    }
private:
    void MoveFrom(VulkanRenderTarget& other)
    {
        image = std::move(other.image);
        framebuffer = other.framebuffer;
        other.framebuffer = VK_NULL_HANDLE;
    }
};