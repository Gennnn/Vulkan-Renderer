#include "VulkanResourceUtils.h"
#include "GatewareConfig.h"

VulkanBuffer CreateVulkanBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, const void* initialData) {
	VulkanBuffer out;
	out.size = size;
	out.usage = usage;
	out.memoryProperties = memoryProperties;

	GvkHelper::create_buffer(physicalDevice, device, static_cast<unsigned int>(size), usage, memoryProperties, &out.handle, &out.memory);

	if (initialData != nullptr && size > 0) {
		GvkHelper::write_to_buffer(device, out.memory, initialData, static_cast<unsigned int>(size));
	}

	return out;
}