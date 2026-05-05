#pragma once

#include <cstdint>
#include <vector>

#include "SceneTypes.h"

enum class IndexFormat : uint8_t {
	UINT16,
	UINT32
};

struct MeshPrimitive {
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;

	IndexFormat indexFormat = IndexFormat::UINT32;

	MaterialID material = InvalidSceneIndex;

	uint64_t positionByteOffset = 0;
	uint64_t normalByteOffset = 0;
	uint64_t tangentByteOffset = 0;
	uint64_t uvByteOffset = 0;

	uint32_t positionStride = 0;
	uint32_t normalStride = 0;
	uint32_t tangentStride = 0;
	uint32_t uvStride = 0;
};

struct Mesh {
	std::vector<MeshPrimitive> primitives;
};