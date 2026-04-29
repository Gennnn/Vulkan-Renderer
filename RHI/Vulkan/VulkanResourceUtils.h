#pragma once
#include "VulkanBuffer.h"
#include <vulkan/vulkan.h>

VulkanBuffer CreateVulkanBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, const void* initialData = nullptr);