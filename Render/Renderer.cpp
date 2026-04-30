#include "Renderer.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "shaderc/shaderc.h"
#include "../TextureUtils.h"
#include "../ktx/include/ktxvulkan.h"
#include "../Materials/TextureUtilsKTX.h"

#include <dxcapi.h>
#include <wrl/client.h>

#ifdef _WIN32
#pragma comment(lib, "shaderc_combined.lib")
#endif

using Microsoft::WRL::ComPtr;

void Renderer::RenderFrame() {
	UpdateCamera();

	unsigned int frame = 0;
	vlk.GetSwapchainCurrentFrame(frame);

	UpdateSceneData(frame);

	if (config.enableShadows) {
		RenderShadowPass(frame);
		SubmitShadowPass(frame);
	}
	
	VkClearValue clearValues[2]{};

	clearValues[0].color = { {
		config.clearColor[0],
		config.clearColor[1],
		config.clearColor[2],
		config.clearColor[3]
	} };

	clearValues[1].depthStencil = {
		config.clearDepth,
		config.clearStencil
	};

	if (+vlk.StartFrame(2, clearValues)) {
		Render(frame);
		vlk.EndFrame(true);
	}
}

void Renderer::ApplyImportedScene(const ImportedScene& imported) {
	importedGeometryBytes = imported.geometryBufferBytes;

	primitivesToDraw.clear();
	primitivesToDraw.reserve(imported.primitives.size());

	for (const ImportedPrimitive& src : imported.primitives) {
		Primitive dst = {};

		dst.indexCount = src.indexCount;
		dst.firstIndex = src.firstIndex;
		dst.instanceIndex = src.instanceIndex;
		dst.matIndex = src.materialIndex;
		dst.indexByteOffset = src.indexByteOffset;
		dst.indexType = src.indexType;
		dst.vertexOffset = 0;

		primitivesToDraw.push_back(dst);
		bufferByteOffsets.push_back(src.vertexAttributeByteOffsets);
	}

	materialData.clear();
	materialData.resize(imported.materials.size());

	for (size_t i = 0; i < imported.materials.size(); i++) {
		const ImportedMaterial& src = imported.materials[i];

		materialData[i].baseColorIndex = src.baseColorTextureIndex;
		materialData[i].metallicRoughnessIndex = src.metallicRoughnessTextureIndex;
		materialData[i].normalIndex = src.normalTextureIndex;

		materialData[i].baseColorFactor = {
			src.baseColorFactor.x,
			src.baseColorFactor.y,
			src.baseColorFactor.z,
			src.baseColorFactor.w
		};

		materialData[i].normalIndex = src.normalTextureIndex < 0 ? UINT32_MAX : static_cast<unsigned int>(src.normalTextureIndex);
		materialData[i].baseColorIndex = src.baseColorTextureIndex < 0 ? UINT32_MAX : static_cast<unsigned int>(src.baseColorTextureIndex);
		materialData[i].metallicRoughnessIndex = src.metallicRoughnessTextureIndex < 0 ? UINT32_MAX : static_cast<unsigned int>(src.metallicRoughnessTextureIndex);

		materialData[i].alphaMode = static_cast<unsigned int>(src.alphaMode);
		materialData[i].alphaCutoff = src.alphaCutoff;

		materialData[i].transmissionFactor = 0.0f;
		materialData[i].pad0 = 0;
		materialData[i].pad1 = 0;
	}

	if (!imported.primitives.empty())
	{
		const ImportedPrimitive& first = imported.primitives[0];

		vertexOffset = first.vertexAttributeByteOffsets[0];
		indexType = first.indexType;
		indexByteOffset = first.indexByteOffset;

		posBindingDescriptionStride = first.vertexBindingStrides[0];
		nrmBindingDescriptionStride = first.vertexBindingStrides[1];
		uvBindingDescriptionStride = first.vertexBindingStrides[2];
		tangentBindingDescriptionStride = first.vertexBindingStrides[3];

		attributeDescriptionOffset[0] = 0;
		attributeDescriptionOffset[1] = 0;
		attributeDescriptionOffset[2] = 0;
		attributeDescriptionOffset[3] = 0;
	}

	if (!imported.rootScale.empty())
	{
		scale = imported.rootScale;
	}
}

void Renderer::UploadImportedTextures(const ImportedScene& imported) {
	texData.clear();
	texData.resize(imported.textures.size());

	samplers.clear();

	const size_t samplerCount = imported.samplers.empty() ? 1 : imported.samplers.size();
	samplers.resize(samplerCount);

	for (size_t i = 0; i < samplerCount; i++) {
		CreateSampler(vlk, samplers[i], VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}

	for (size_t i = 0; i < imported.textures.size(); i++) {
		const ImportedTexture& texture = imported.textures[i];

		if (texture.resolvedPath.empty()) continue;

		UploadTextureToGPU(vlk, texture.resolvedPath, texData[i].image.memory, texData[i].image.image, texData[i].image.imageView);
		texData[i].sampler = texture.samplerIndex >= 0 && texture.samplerIndex < static_cast<int>(samplers.size()) ? static_cast<unsigned int>(texture.samplerIndex) : 0;
	}

	samplers.clear();
	samplers.resize(imported.samplers.size());

	for (size_t i = 0; i < samplers.size(); i++) {
		CreateSampler(vlk, samplers[i], VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}
}