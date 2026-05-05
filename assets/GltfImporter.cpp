#include "GltfImporter.h"

#include "../TinyGLTF/tiny_gltf.h"

#include <stdexcept>
#include <iostream>

ImportedScene GltfImporter::Load(const std::filesystem::path& gltfPath) {
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string error;
	std::string warning;

	const std::string pathString = gltfPath.string();

	bool loaded = false;

	const std::string extension = gltfPath.extension().string();

	if (extension == ".glb") {
		loaded = loader.LoadBinaryFromFile(&model, &error, &warning, pathString);
	}
	else {
		loaded = loader.LoadASCIIFromFile(&model, &error, &warning, pathString);
	}

	if (!warning.empty()) {
		std::cout << "glTF warning: " << warning << std::endl;
	}

	if (!error.empty()) {
		std::cout << "glTF error: " << error << std::endl;
	}

	if (!loaded) {
		throw std::runtime_error("Failed to load glTF file: " + pathString);
	}

	ImportedScene outScene;
	outScene.sourcePath = pathString;

	if (!model.buffers.empty()) {
		outScene.geometryBufferBytes = model.buffers[0].data;
	}

	if (!model.nodes.empty() && !model.nodes[0].scale.empty()) {
		outScene.rootScale = model.nodes[0].scale;
	}

	outScene.samplers.resize(model.samplers.size());
	for (size_t i = 0; i < model.samplers.size(); i++) {
		const tinygltf::Sampler& sampler = model.samplers[i];

		outScene.samplers[i].magFilter = sampler.magFilter;
		outScene.samplers[i].minFilter = sampler.minFilter;
		outScene.samplers[i].wrapS = sampler.wrapS;
		outScene.samplers[i].wrapT = sampler.wrapT;
	}

	outScene.textures.resize(model.textures.size());
	for (size_t i = 0; i < model.textures.size(); ++i)
	{
		const tinygltf::Texture& texture = model.textures[i];

		outScene.textures[i].samplerIndex = texture.sampler;

		if (texture.source >= 0 && texture.source < static_cast<int>(model.images.size()))
		{
			const tinygltf::Image& image = model.images[texture.source];
			outScene.textures[i].resolvedPath = ResolveTexturePath(gltfPath, image.uri);
		}
	}

	outScene.materials.resize(model.materials.size());
	for (size_t i = 0; i < model.materials.size(); ++i)
	{
		const tinygltf::Material& mat = model.materials[i];
		ImportedMaterial& dst = outScene.materials[i];

		dst.baseColorTextureIndex =
			mat.pbrMetallicRoughness.baseColorTexture.index;

		dst.metallicRoughnessTextureIndex =
			mat.pbrMetallicRoughness.metallicRoughnessTexture.index;

		dst.normalTextureIndex =
			mat.normalTexture.index;

		const auto& baseColorFactor =
			mat.pbrMetallicRoughness.baseColorFactor;

		if (baseColorFactor.size() >= 4)
		{
			dst.baseColorFactor.x = static_cast<float>(baseColorFactor[0]);
			dst.baseColorFactor.y = static_cast<float>(baseColorFactor[1]);
			dst.baseColorFactor.z = static_cast<float>(baseColorFactor[2]);
			dst.baseColorFactor.w = static_cast<float>(baseColorFactor[3]);
		}

		dst.alphaMode = ConvertAlphaMode(mat.alphaMode);
		dst.alphaCutoff = static_cast<float>(mat.alphaCutoff);
	}

	uint32_t primitiveIndex = 0;

	for (size_t meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex)
	{
		const tinygltf::Mesh& mesh = model.meshes[meshIndex];

		for (size_t primIndex = 0; primIndex < mesh.primitives.size(); ++primIndex)
		{
			const tinygltf::Primitive& primitive = mesh.primitives[primIndex];

			if (primitive.indices < 0)
			{
				std::cout << "Skipping primitive without indices." << std::endl;
				continue;
			}

			const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

			if (indexAccessor.bufferView < 0)
			{
				std::cout << "Skipping primitive with invalid index buffer view." << std::endl;
				continue;
			}

			const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];

			ImportedPrimitive importedPrimitive = {};

			importedPrimitive.indexCount = static_cast<uint32_t>(indexAccessor.count);

			importedPrimitive.indexByteOffset = static_cast<uint32_t>(indexBufferView.byteOffset + indexAccessor.byteOffset);

			const uint32_t indexElementSize = GetIndexElementSize(indexAccessor.componentType);

			importedPrimitive.firstIndex = importedPrimitive.indexByteOffset / indexElementSize;

			importedPrimitive.indexType = GetVkIndexType(indexAccessor.componentType);

			importedPrimitive.materialIndex = primitive.material;

			importedPrimitive.instanceIndex = primitiveIndex;

			importedPrimitive.vertexOffset = 0;

			auto readAttribute = [&](const char* name, int slot)
				{
					auto found = primitive.attributes.find(name);
					if (found == primitive.attributes.end())
					{
						std::cout << "Warning: primitive missing attribute " << name << std::endl;
						return;
					}

					const int accessorIndex = found->second;

					if (accessorIndex < 0 ||
						accessorIndex >= static_cast<int>(model.accessors.size()))
					{
						std::cout << "Warning: invalid accessor for attribute " << name << std::endl;
						return;
					}

					const tinygltf::Accessor& accessor = model.accessors[accessorIndex];

					if (accessor.bufferView < 0 ||
						accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
					{
						std::cout << "Warning: invalid buffer view for attribute " << name << std::endl;
						return;
					}

					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

					importedPrimitive.vertexAttributeByteOffsets[slot] = static_cast<VkDeviceSize>(bufferView.byteOffset + accessor.byteOffset);

					importedPrimitive.vertexBindingStrides[slot] = static_cast<uint32_t>(accessor.ByteStride(bufferView));
				};

			readAttribute("POSITION", 0);
			readAttribute("NORMAL", 1);
			readAttribute("TEXCOORD_0", 2);
			readAttribute("TANGENT", 3);

			outScene.primitives.push_back(importedPrimitive);
			primitiveIndex++;
		}
	}

	return outScene;
}

ImportedAlphaMode GltfImporter::ConvertAlphaMode(const std::string& alphaMode)
{
	if (alphaMode == "MASK")
		return ImportedAlphaMode::Mask;

	if (alphaMode == "BLEND")
		return ImportedAlphaMode::Blend;

	return ImportedAlphaMode::Opaque;
}

std::string GltfImporter::ResolveTexturePath(const std::filesystem::path& gltfPath, const std::string& imageUri)
{
	if (imageUri.empty()) return {};

	std::filesystem::path parent = gltfPath.parent_path();
	std::filesystem::path resolved = parent / imageUri;

	return resolved.string();
}

uint32_t GltfImporter::GetIndexElementSize(int componentType)
{
	switch (componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return sizeof(uint16_t);

	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return sizeof(uint32_t);

	default:
		throw std::runtime_error("Unsupported glTF index component type.");
	}
}

VkIndexType GltfImporter::GetVkIndexType(int componentType)
{
	switch (componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return VK_INDEX_TYPE_UINT16;

	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return VK_INDEX_TYPE_UINT32;

	default:
		throw std::runtime_error("Unsupported glTF index component type.");
	}
}