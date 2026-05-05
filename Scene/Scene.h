#pragma once

#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include <vector>
#include "MeshInstance.h"
#include "Material.h"
#include "EnvironmentSettings.h"
#include "Texture.h"

class Scene {
public:
	SceneCamera& MainCamera() { return mainCamera; }
	const SceneCamera& MainCamera() const { return mainCamera; }

	DirectionalLight& Sun() { return sun; }
	const DirectionalLight& Sun() const { return sun; }

	EnvironmentSettings& Environment() { return environment; }
	const EnvironmentSettings& Environment() const { return environment; }

	std::vector<Mesh>& Meshes() { return meshes; }
	std::vector<MeshInstance>& MeshInstances() { return meshInstances; }
	std::vector<MaterialData>& Materials() { return materials; }
	std::vector<TextureAsset>& Textures() { return textures; }
	std::vector<unsigned char>& GeometryBufferBytes() { return geometryBufferBytes; }

	const std::vector<Mesh>& Meshes() const { return meshes; }
	const std::vector<MeshInstance>& MeshInstances() const { return meshInstances; }
	const std::vector<MaterialData>& Materials() const { return materials; }
	const std::vector<TextureAsset>& Textures() const { return textures; }
	const std::vector<unsigned char>& GeometryBufferBytes() const { return geometryBufferBytes; }


private:
	SceneCamera mainCamera;
	DirectionalLight sun;
	EnvironmentSettings environment;

	std::vector<Mesh> meshes;
	std::vector<MeshInstance> meshInstances;
	std::vector<MaterialData> materials;
	std::vector<TextureAsset> textures;
	std::vector<unsigned char> geometryBufferBytes;
};