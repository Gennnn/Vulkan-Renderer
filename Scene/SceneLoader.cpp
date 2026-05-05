#include "SceneLoader.h"
#include "Scene.h"

static IndexFormat ConvertFromIndexFormat(VkIndexType type) {
	switch (type) {
	case VK_INDEX_TYPE_UINT16:
		return IndexFormat::UINT16;
	case VK_INDEX_TYPE_UINT32:
		return IndexFormat::UINT32;
	default:
		throw std::runtime_error("Unsupported imported index type.");
	}
}

Scene SceneLoader::BuildSceneFromImported(const ImportedScene& imported, const SceneLoaderInfo& info) {
	Scene scene;

	scene.MainCamera().fovDegrees = info.defaultFOV;
	scene.MainCamera().nearPlane = info.defaultNear;
	scene.MainCamera().farPlane = info.defaultFar;
	scene.MainCamera().aspectRatio = info.defaultAspect;
	scene.MainCamera().worldPosition = info.defaultCameraPosition;

	scene.Sun().direction = info.defaultLightDirection;

	scene.Textures().reserve(imported.textures.size());

	for (const auto& importedTexture : imported.textures) {
		TextureAsset texture;
		texture.sourcePath = importedTexture.resolvedPath;
		texture.isSrgb = true;
		texture.isCubeMap = false;

		scene.Textures().push_back(texture);
	}

	scene.Materials().reserve(imported.materials.size());

	for (const auto& importedMaterial : imported.materials) {
		MaterialData material;

		material.baseColorFactor = importedMaterial.baseColorFactor;

		material.baseColorTexture = importedMaterial.baseColorTextureIndex >= 0 ? static_cast<TextureID>(importedMaterial.baseColorTextureIndex) : InvalidSceneIndex;
		material.normalTexture = importedMaterial.normalTextureIndex >= 0 ? static_cast<TextureID>(importedMaterial.normalTextureIndex) : InvalidSceneIndex;
		material.metallicRoughnessTexture = importedMaterial.metallicRoughnessTextureIndex >= 0 ? static_cast<TextureID>(importedMaterial.metallicRoughnessTextureIndex) : InvalidSceneIndex;

		material.alphaCutoff = importedMaterial.alphaCutoff;

		switch (importedMaterial.alphaMode) {
		default:
		case ImportedAlphaMode::Opaque:
			material.alphaMode = AlphaMode::opaque;
			break;
		case ImportedAlphaMode::Blend:
			material.alphaMode = AlphaMode::blend;
			break;
		case ImportedAlphaMode::Mask:
			material.alphaMode = AlphaMode::mask;
			break;
		}

		scene.Materials().push_back(material);
	}

	Mesh mesh;
	mesh.primitives.reserve(imported.primitives.size());

	for (const auto& importedPrimitive : imported.primitives) {
		MeshPrimitive primitive;

		primitive.firstIndex = importedPrimitive.firstIndex;
		primitive.indexCount = importedPrimitive.indexCount;
		primitive.vertexOffset = importedPrimitive.vertexOffset;
		primitive.indexOffset = importedPrimitive.indexByteOffset;

		primitive.indexFormat = ConvertFromIndexFormat(importedPrimitive.indexType);

		primitive.material = importedPrimitive.materialIndex >= 0 && importedPrimitive.materialIndex < scene.Materials().size() ? importedPrimitive.materialIndex : InvalidSceneIndex;

		primitive.positionByteOffset = importedPrimitive.vertexAttributeByteOffsets[0];
		primitive.normalByteOffset = importedPrimitive.vertexAttributeByteOffsets[1];
		primitive.uvByteOffset = importedPrimitive.vertexAttributeByteOffsets[2];
		primitive.tangentByteOffset = importedPrimitive.vertexAttributeByteOffsets[3];

		primitive.positionStride = importedPrimitive.vertexBindingStrides[0];
		primitive.normalStride = importedPrimitive.vertexBindingStrides[1];
		primitive.uvStride = importedPrimitive.vertexBindingStrides[2];
		primitive.tangentStride = importedPrimitive.vertexBindingStrides[3];

		mesh.primitives.push_back(primitive);
	}

	MeshID meshId = static_cast<MeshID>(scene.Meshes().size());

	scene.Meshes().push_back(mesh);

	MeshInstance inst;
	inst.meshId = meshId;

	double sx = imported.rootScale.size() > 0 ? imported.rootScale[0] : 1.0;
	double sy = imported.rootScale.size() > 1 ? imported.rootScale[1] : 1.0;
	double sz = imported.rootScale.size() > 2 ? imported.rootScale[2] : 1.0;

	inst.transform.world = { static_cast<float>(sx), 0, 0, 0, 0, static_cast<float>(sy), 0, 0, 0, 0, static_cast<float>(sz), 0, 0, 0, 0, 1 };

	scene.MeshInstances().push_back(inst);

	scene.GeometryBufferBytes() = imported.geometryBufferBytes;

	return scene;

}