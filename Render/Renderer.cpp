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

void Renderer::SetScene(Scene* scene) {
	activeScene = scene;
}

void Renderer::RenderFrame()
{
	if (activeScene == nullptr)
	{
		std::cout << "[RenderFrame] activeScene is null, returning." << std::endl;
		return;
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

	if (!+ vlk.StartFrame(2, clearValues))
	{
		std::cout << "[RenderFrame] StartFrame returned false." << std::endl;
		return;
	}
	unsigned int frame = 0;
	vlk.GetSwapchainCurrentFrame(frame);

	UpdateCamera();

	UpdateSceneData(frame);

	if (config.enableShadows)
	{
		RenderShadowPass(frame);
		SubmitShadowPass(frame);
	}

	Render(frame);
	vlk.EndFrame(true);
}

//void Renderer::ApplyImportedScene(const ImportedScene& imported) {
//	importedGeometryBytes = imported.geometryBufferBytes;
//
//	primitivesToDraw.clear();
//	primitivesToDraw.reserve(imported.primitives.size());
//
//	for (const ImportedPrimitive& src : imported.primitives) {
//		Primitive dst = {};
//
//		dst.indexCount = src.indexCount;
//		dst.firstIndex = src.firstIndex;
//		dst.instanceIndex = src.instanceIndex;
//		dst.matIndex = src.materialIndex;
//		dst.indexByteOffset = src.indexByteOffset;
//		dst.indexType = src.indexType;
//		dst.vertexOffset = 0;
//
//		primitivesToDraw.push_back(dst);
//		bufferByteOffsets.push_back(src.vertexAttributeByteOffsets);
//	}
//
//	materialData.clear();
//	materialData.resize(imported.materials.size());
//
//	for (size_t i = 0; i < imported.materials.size(); i++) {
//		const ImportedMaterial& src = imported.materials[i];
//
//		materialData[i].baseColorIndex = src.baseColorTextureIndex;
//		materialData[i].metallicRoughnessIndex = src.metallicRoughnessTextureIndex;
//		materialData[i].normalIndex = src.normalTextureIndex;
//
//		materialData[i].baseColorFactor = {
//			src.baseColorFactor.x,
//			src.baseColorFactor.y,
//			src.baseColorFactor.z,
//			src.baseColorFactor.w
//		};
//
//		materialData[i].normalIndex = src.normalTextureIndex < 0 ? UINT32_MAX : static_cast<unsigned int>(src.normalTextureIndex);
//		materialData[i].baseColorIndex = src.baseColorTextureIndex < 0 ? UINT32_MAX : static_cast<unsigned int>(src.baseColorTextureIndex);
//		materialData[i].metallicRoughnessIndex = src.metallicRoughnessTextureIndex < 0 ? UINT32_MAX : static_cast<unsigned int>(src.metallicRoughnessTextureIndex);
//
//		materialData[i].alphaMode = static_cast<unsigned int>(src.alphaMode);
//		materialData[i].alphaCutoff = src.alphaCutoff;
//
//		materialData[i].transmissionFactor = 0.0f;
//		materialData[i].pad0 = 0;
//		materialData[i].pad1 = 0;
//	}
//
//	if (!imported.primitives.empty())
//	{
//		const ImportedPrimitive& first = imported.primitives[0];
//
//		vertexOffset = first.vertexAttributeByteOffsets[0];
//		indexType = first.indexType;
//		indexByteOffset = first.indexByteOffset;
//
//		posBindingDescriptionStride = first.vertexBindingStrides[0];
//		nrmBindingDescriptionStride = first.vertexBindingStrides[1];
//		uvBindingDescriptionStride = first.vertexBindingStrides[2];
//		tangentBindingDescriptionStride = first.vertexBindingStrides[3];
//
//		attributeDescriptionOffset[0] = 0;
//		attributeDescriptionOffset[1] = 0;
//		attributeDescriptionOffset[2] = 0;
//		attributeDescriptionOffset[3] = 0;
//	}
//
//	if (!imported.rootScale.empty())
//	{
//		scale = imported.rootScale;
//	}
//}

static VkIndexType ConvertToVkIndexType(IndexFormat format);
static unsigned int ConvertAlphaMode(AlphaMode alphaMode);

void Renderer::ApplyScene(const Scene& scene) {
	primitivesToDraw.clear();
	bufferByteOffsets.clear();
	materialData.clear();
	objectData.clear();

	if (scene.MeshInstances().empty()) return;

	sceneData = {};

	sceneData.lightDirection = float4{ scene.Sun().direction.x, scene.Sun().direction.y, scene.Sun().direction.z, scene.Sun().direction.w };
	sceneData.lightColor = float4{ scene.Sun().color.x, scene.Sun().color.y, scene.Sun().color.z, scene.Sun().color.w };
	sceneData.ambientLightTerm = float4{ scene.Sun().ambient.x, scene.Sun().ambient.y, scene.Sun().ambient.z, scene.Sun().ambient.w };

	GW::MATH::GMATRIXF view = GW::MATH::GIdentityMatrixF;
	CreateViewMatrix(view);
	sceneData.viewMatrix = view;

	GW::MATH::GMATRIXF projection = GW::MATH::GIdentityMatrixF;
	CreatePerspectiveMatrix(projection);
	sceneData.projectionMatrix = projection;

	const MeshInstance& inst = scene.MeshInstances()[0];

	if (inst.meshId == InvalidSceneIndex || inst.meshId >= scene.Meshes().size()) return;

	const Mesh& mesh = scene.Meshes()[inst.meshId];

	primitivesToDraw.reserve(mesh.primitives.size());
	bufferByteOffsets.reserve(mesh.primitives.size());

	for (uint32_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); primitiveIndex++) {
		const MeshPrimitive& src = mesh.primitives[primitiveIndex];

		Primitive dst = {};

		dst.indexCount = src.indexCount;
		dst.firstIndex = src.firstIndex;
		dst.vertexOffset = 0;
		dst.instanceIndex = primitiveIndex;
		dst.matIndex = src.material != InvalidSceneIndex ? src.material : 0;
		dst.indexByteOffset = src.indexOffset;
		dst.indexType = ConvertToVkIndexType(src.indexFormat);

		primitivesToDraw.push_back(dst);

		bufferByteOffsets.push_back({ static_cast<VkDeviceSize>(src.positionByteOffset),static_cast<VkDeviceSize>(src.normalByteOffset),static_cast<VkDeviceSize>(src.uvByteOffset),static_cast<VkDeviceSize>(src.tangentByteOffset) });
	}

	materialData.resize(scene.Materials().size());
	for (uint32_t matIndex = 0; matIndex < scene.Materials().size(); matIndex++) {
		const MaterialData& src = scene.Materials()[matIndex];

		Material dst = {};
		
		dst.nrmIndex = src.normalTexture != InvalidSceneIndex ? src.normalTexture : UINT32_MAX;
		dst.baseColorIndex = src.baseColorTexture != InvalidSceneIndex ? src.baseColorTexture : UINT32_MAX;
		dst.metallicRoughnessIndex = src.metallicRoughnessTexture != InvalidSceneIndex ? src.metallicRoughnessTexture : UINT32_MAX;
		dst.alphaCutoff = src.alphaCutoff;
		dst.alphaMode = ConvertAlphaMode(src.alphaMode);
		dst.baseColorFactor = src.baseColorFactor;
		dst.transmissionFactor = 0.0f;

		materialData[matIndex] = dst;
	}
	objectData.resize(primitivesToDraw.size());

	for (uint32_t i = 0; i < objectData.size(); i++) {
		objectData[i].worldMatrix = inst.transform.world;
		objectData[i].materialIndex = primitivesToDraw[i].matIndex;
		objectData[i].samplerIndex = 0;
	}

	if (!primitivesToDraw.empty()) {
		const Primitive& first = primitivesToDraw[0];
		const MeshPrimitive& firstMP = mesh.primitives[0];

		vertexOffset = bufferByteOffsets[0][0];
		indexType = first.indexType;
		indexByteOffset = first.indexByteOffset;

		posBindingDescriptionStride = firstMP.positionStride;
		nrmBindingDescriptionStride = firstMP.normalStride;
		uvBindingDescriptionStride = firstMP.uvStride;
		tangentBindingDescriptionStride = firstMP.tangentStride;

		attributeDescriptionOffset[0] = 0;
		attributeDescriptionOffset[1] = 0;
		attributeDescriptionOffset[2] = 0;
		attributeDescriptionOffset[3] = 0;
	}
}

//void Renderer::UploadImportedTextures(const ImportedScene& imported) {
//	texData.clear();
//	texData.resize(imported.textures.size());
//
//	samplers.clear();
//
//	const size_t samplerCount = imported.samplers.empty() ? 1 : imported.samplers.size();
//	samplers.resize(samplerCount);
//
//	for (size_t i = 0; i < samplerCount; i++) {
//		CreateSampler(vlk, samplers[i], VK_SAMPLER_ADDRESS_MODE_REPEAT);
//	}
//
//	for (size_t i = 0; i < imported.textures.size(); i++) {
//		const ImportedTexture& texture = imported.textures[i];
//
//		if (texture.resolvedPath.empty()) {
//			texData[i].sampler = 0;
//			continue;
//		}
//
//		UploadTextureToGPU(vlk, texture.resolvedPath, texData[i].image.memory, texData[i].image.image, texData[i].image.imageView);
//		texData[i].sampler = texture.samplerIndex >= 0 && texture.samplerIndex < static_cast<int>(samplers.size()) ? static_cast<unsigned int>(texture.samplerIndex) : 0;
//	}
//}

bool Renderer::InitializeGraphicsForScene(const Scene& scene) {
	activeScene = const_cast<Scene*>(&scene);

	GetHandlesFromSurface();

	ApplyScene(scene);
	UploadSceneTextures(scene);

	InitializeGeometryBuffer();
	InitializeUniformBuffer();
	InitializeStorageBuffer();
	InitializeMaterialStorageBuffer();

	InitializeShadowPass();
	InitializeShadowCommandBuffers();

	InitializeBloomPass();
	InitializeBloomCommandBuffers();

	CompileShaders();

	InitializeGraphicsPipeline();
	InitializeShadowGraphicsPipeline();
	InitializeSkyboxGraphicsPipeline();
	InitializeOffscreenGraphicsPipeline();
	InitializeBloomGraphicsPipeline();

	CreateDescriptorPool();
	AllocateDescriptorSets();
	UpdateDescriptorSets();

	CreateBloomDescriptorSets();

	return true;
}

void Renderer::UploadSceneTextures(const Scene& scene) {
	texData.clear();
	cubeTexData.clear();
	samplers.clear();

	VkSampler defaultSampler = VK_NULL_HANDLE;
	CreateSampler(vlk, defaultSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	samplers.push_back(defaultSampler);

	for (TextureID i = 0; i < scene.Textures().size(); i++) {
		const TextureAsset& textureAsset = scene.Textures()[i];

		if (textureAsset.sourcePath.empty()) continue;

		if (textureAsset.isCubeMap) {
			TextureData& texture = cubeTexData.emplace_back();

			UploadKTXTextureToGPU(vlk, textureAsset.sourcePath, texture.buffer, texture.image.memory, texture.image.image, texture.image.imageView);

			texture.ownsBuffer = true;
			texture.sampler = 0;

			TextureID index = static_cast<TextureID> (cubeTexData.size() - 1);

			if (scene.Environment().diffuseIrradiance == i) {
				sceneData.diffuseIndex = index;
			}
			if (scene.Environment().specularPrefilter == i) {
				sceneData.specularIndex = index;
			}
			if (scene.Environment().skybox == i) {
				sceneData.skyboxIndex = index;
			}
		}
		else {
			TextureData& texture = texData.emplace_back();

			UploadTextureToGPU(vlk, textureAsset.sourcePath, texture.image.memory, texture.image.image, texture.image.imageView);

			texture.sampler = 0;

			TextureID index = static_cast<TextureID>(texData.size() - 1);
			if (scene.Environment().brdfLut == i) {
				sceneData.brdfIndex = index;
			}
		}
	}
}

static VkIndexType ConvertToVkIndexType(IndexFormat format) {
	switch (format) {
	case IndexFormat::UINT16:
		return VK_INDEX_TYPE_UINT16;
	default:
	case IndexFormat::UINT32:
		return VK_INDEX_TYPE_UINT32;
	}
}

static unsigned int ConvertAlphaMode(AlphaMode alphaMode) {
	switch (alphaMode) {
	default:
	case AlphaMode::opaque:
		return 0;
	case AlphaMode::mask:
		return 1;
	case AlphaMode::blend:
		return 2;
	}
}