#pragma once

#include <Vulkan/vulkan.h>
#include <utility>

struct VulkanBuffer {
	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize size = 0;
	VkBufferUsageFlags usage = 0;
	VkMemoryPropertyFlags memoryProperties = 0;

	VulkanBuffer() = default;

	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer(VulkanBuffer&& other) noexcept {
		MoveFrom(other);
	}

	VulkanBuffer& operator=(VulkanBuffer&& other) noexcept {
		if (this != &other) {
			MoveFrom(other);
		}
		return *this;
	}

	bool IsValid() const {
		return handle != VK_NULL_HANDLE;
	}

	void Destroy(VkDevice device) {
		if (handle != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, handle, nullptr);
			handle = VK_NULL_HANDLE;
		}

		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}

		size = 0;
		usage = 0;
		memoryProperties = 0;
	}

private:
	void MoveFrom(VulkanBuffer& other) {
		handle = other.handle;
		memory = other.memory;
		size = other.size;
		usage = other.usage;
		memoryProperties = other.memoryProperties;

		other.handle = VK_NULL_HANDLE;
		other.memory = VK_NULL_HANDLE;
		other.size = 0;
		other.usage = 0;
		other.memoryProperties = 0;
	}
};