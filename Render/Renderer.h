#pragma once

#include "../Core/GatewareConfig.h"

#include "RenderConfig.h"
#include "../Defines.h"
#include "../FileIntoString.h"
#include "../TextureUtils.h"
#include "../Materials/TextureUtilsKTX.h"
#include "../TinyGLTF/tiny_gltf.h"

#include <dxcapi.h>
#include <wrl/client.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

using namespace GW::GRAPHICS;

#if defined WIN32
#include <Windows.h>
#endif
#include <Vulkan/VulkanBuffer.h>
#include <Vulkan/VulkanResourceUtils.h>
#include <Vulkan/VulkanRenderTarget.h>

#define SHADOW_CASCADE_COUNT 4

inline void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;

//OutputDebugStringA is a windows-only function 
#if defined WIN32 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

using Microsoft::WRL::ComPtr;

class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	RendererConfig config;
	VkRenderPass renderPass;
	GW::CORE::GEventReceiver shutdown;
	
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VulkanBuffer geometryBuffer;
	VkShaderModule vertexShader = nullptr;
	VkShaderModule fragmentShader = nullptr;
	VkShaderModule shadowVertexShader = nullptr;
	VkShaderModule shadowFragmentShader = nullptr;
	VkShaderModule skyboxVertexShader = nullptr;
	VkShaderModule skyboxFragmentShader = nullptr;
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

	unsigned int windowWidth, windowHeight;

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string fileName;
	std::string err;
	std::string warn;
	unsigned int indexCount;
	VkDeviceSize indexOffset;
	VkDeviceSize vertexOffset;
	unsigned int posBindingDescriptionStride;
	unsigned int nrmBindingDescriptionStride;
	unsigned int uvBindingDescriptionStride;
	unsigned int tangentBindingDescriptionStride;
	unsigned int attributeDescriptionOffset[4];
	VkDeviceSize indexByteOffset;

	struct VERTEX_DATA {
		float3 position;
		float3 normal;
		float2 uv;
		float4 tangent;
	};

	GW::MATH::GMatrix mathProxy;
	struct SHADER_SCENE_DATA {
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix, lightViewProjectionMatrices[4];
		float4 lightDirection, lightColor, ambientLightTerm, cameraWorldPosition, cascadeSplits, cascadeDepthRanges, cascadeWorldUnitsPerTexel;
		unsigned int brdfIndex, diffuseIndex, specularIndex, shadowIndex, skyboxIndex, shadowSamplerIndex;
		float skyboxRot;
		float pad;
	};
	SHADER_SCENE_DATA sceneData;
	
	std::vector<VulkanBuffer> uniformBuffers;
	std::vector<VulkanBuffer> storageBuffers;
	std::vector<VulkanBuffer> materialStorageBuffers;

	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorPool descriptorPool = nullptr;
	VkDescriptorSetLayout descriptorSetLayout = nullptr;

	GW::INPUT::GInput inputProxy;
	GW::INPUT::GController controllerProxy;
	std::chrono::steady_clock::time_point lastFrameTime;
	float cumulPitch = 0.0f;
	bool cursorCaptured = false;
	struct DIRECTION_INPUT {
		union {
			struct {
				float up;
				float down;
				float left;
				float right;
				float fwd;
				float back;
			};
			float pressedState[6];
		};
	};

	struct TextureData
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

		VulkanImage image;

		unsigned int sampler = 0;

		bool ownsImage = true;
		bool ownsBuffer = false;
	};

	std::vector<std::string> texturePaths;
	
	unsigned int brdfIndex = 0;
	unsigned int diffuseIndex = 1;
	unsigned int specularIndex = 2;
	unsigned int skyboxIndex = 3;
	std::vector<TextureData> texData;
	std::vector<TextureData> cubeTexData;
 	std::vector<VkSampler> samplers;

	VkDescriptorSetLayout textureDescriptorSetLayout;
	VkDescriptorSet textureDescriptorSet;

	VkDescriptorSetLayout cubeTextureDescriptorSetLayout;
	VkDescriptorSet cubeTextureDescriptorSet;

	struct INSTANCE_DATA {
		GW::MATH::GMATRIXF worldMatrix;
		unsigned int samplerIndex;
		unsigned int materialIndex;
		unsigned int pad0;
		unsigned int pad1;
	};
	std::vector<INSTANCE_DATA> objectData;
	

	std::chrono::steady_clock::time_point startTime;
	float4 originalSunDirection;

	struct Primitive
	{
		VkDeviceSize indexByteOffset;
		VkIndexType indexType;
		unsigned int firstIndex;
		unsigned int indexCount;
		unsigned int vertexOffset = 0;
		unsigned int instanceIndex;
		unsigned int matIndex;
	};
	std::vector<Primitive> primitivesToDraw;
	std::vector<double> scale;
	
	struct Material {
		float4 baseColorFactor = { 1,1,1,1 };
		unsigned int normalIndex;
		unsigned int baseColorIndex;
		unsigned int metallicRoughnessIndex;
		unsigned int alphaMode;
		float alphaCutoff;
		float transmissionFactor;
		unsigned int pad0;
		unsigned int pad1;
	};

	std::vector<Material> materialData;
	

	VkIndexType indexType;
	unsigned int indexElementSize;
	std::vector<std::array<VkDeviceSize, 4>> bufferByteOffsets;

	
	VkSampler shadowSampler;
	VkRenderPass shadowRenderPass;
	VkPipeline shadowPipeline;
	unsigned int shadowMapSize = 0;
	unsigned int graphicsQueueFamilyIndex = UINT32_MAX;
	VkQueue graphicsQueue;
	VkCommandPool shadowCommandPool;
	std::vector<VkCommandBuffer> shadowCommandBuffers;

	struct Cascade {

		VulkanImage depthImage;
		VkFramebuffer frameBuffer;

		float splitDepth = 0.0f;
		GW::MATH::GMATRIXF viewProjectionMatrix = GW::MATH::GIdentityMatrixF;

		void Destroy(VkDevice device) {
			if (frameBuffer != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(device, frameBuffer, nullptr);
				frameBuffer = VK_NULL_HANDLE;
			}

			depthImage.Destroy(device);
		}
	};
	std::array<Cascade, SHADOW_CASCADE_COUNT> shadowCascades;
	
	struct PushConstants {
		unsigned int cascadeIndex;
		float pad[31];
	};

	PushConstants pc;

	VkPipeline skyboxPipeline;

	struct BloomImg {
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory memory;
		VkFramebuffer framebuffer;
	};

	VulkanRenderTarget sceneHDR;
	VulkanRenderTarget sceneDepth;
	VulkanRenderTarget bloomHoriz;
	VulkanRenderTarget bloomVert;

	unsigned int bloomSamplerIndex;

	VkRenderPass offscreenRenderPass;
	VkRenderPass bloomRenderPass;

	VkPipeline sceneOffscreenPipeline;
	VkPipeline skyboxOffscreenPipeline;

	VkCommandPool bloomCommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> bloomCommandBuffers;
	std::vector<VkFence> bloomFences;

	VkShaderModule fullscreenVertShader = nullptr;
	VkShaderModule bloomBlurShader = nullptr;
	VkShaderModule bloomCompositeShader = nullptr;

	VkDescriptorPool bloomDescriptorPool = nullptr;
	VkDescriptorSetLayout bloomBlurDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout bloomCompositeDescriptorSetLayout = nullptr;

	std::vector<VkDescriptorSet> bloomVertSets;
	std::vector<VkDescriptorSet> bloomHorizSets;
	std::vector<VkDescriptorSet> bloomCompositeSets;

	VkPipelineLayout bloomBlurPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout bloomCompositePipelineLayout = VK_NULL_HANDLE;

	VkPipeline bloomVertPipeline = VK_NULL_HANDLE;
	VkPipeline bloomHorizPipeline = VK_NULL_HANDLE;
	VkPipeline bloomCompositePipeline = VK_NULL_HANDLE;

	struct BloomPushConstants {
		unsigned int direction;
		float2 texelSize;
		float pad;
	};

	BloomPushConstants bloomPushConstants{};

public:
	bool alive = true;

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk, const RendererConfig& _config)
	{
		win = _win;
		vlk = _vlk;
		config = _config;

		startTime = std::chrono::steady_clock::now();

		fileName = config.startupScenePath;

		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, fileName);

		if (!warn.empty()) {
			std::cout << "glTF warning: " << warn << std::endl;
		}
		if (!err.empty()) {
			std::cout << "glTF error: " << err << std::endl;
		}
		if (!ret) {
			std::cout << "Failed to load glTF scene: " << fileName << std::endl;
			alive = false;
			return;
		}

		texturePaths = { config.brdfLutPath, config.diffuseIrradiancePath, config.specularPrefilterPath, config.skyboxPath };

		shadowMapSize = config.shadowMapSize;

		GetPrimitiveData();
		GetTextureData();
		UpdateWindowDimensions();
		GetMaterialData();
		if (+mathProxy.Create()) {
			GW::MATH::GMATRIXF matrix = GW::MATH::GIdentityMatrixF;
			sceneData = {};

			CreateViewMatrix(matrix);
			sceneData.viewMatrix = matrix;

			CreatePerspectiveMatrix(matrix);
			sceneData.projectionMatrix = matrix;

			float4 lightDir = { -0.972604,-0.227954,-0.0455908 };
			float magnitude = sqrtf(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
			lightDir.x /= magnitude;
			lightDir.y /= magnitude;
			lightDir.z /= magnitude;

			originalSunDirection = lightDir;

			sceneData.lightDirection = originalSunDirection;

			sceneData.lightColor = { 0.998528f,0.980959f,0.396452f };

			sceneData.ambientLightTerm = { 0.2f, 0.2f, 0.2f };
		}
		if (+controllerProxy.Create()) {

		}
		if (+inputProxy.Create(win)) {

		}
		scale = model.nodes[0].scale;
		
		LoadEnvironmentTextures();

		BuildInitialObjectData();

		InitializeGraphics();
		BindShutdownCallback();
	}



private:
	void LoadEnvironmentTextures() {
		for (int i = 0; i < texturePaths.size(); i++) {
			std::string texPath = texturePaths[i];
			std::string extension = texPath.substr(texPath.find_last_of('.'));

			unsigned int ind;
			if (extension == ".png") {
				texData.push_back(TextureData{});
				ind = texData.size() - 1;

				UploadTextureToGPU(vlk, texturePaths[i], texData[ind].image.memory, texData[ind].image.image, texData[ind].image.imageView);

				if (i == brdfIndex) {
					sceneData.brdfIndex = ind;
				}
			}
			else if (extension == ".ktx2") {
				cubeTexData.push_back(TextureData{});
				ind = cubeTexData.size() - 1;

				UploadKTXTextureToGPU(vlk, texturePaths[i], cubeTexData[ind].buffer, cubeTexData[ind].image.memory, cubeTexData[ind].image.image, cubeTexData[ind].image.imageView);

				if (i == specularIndex) {
					sceneData.specularIndex = ind;
				}
				else if (i == diffuseIndex) {
					sceneData.diffuseIndex = ind;
				}
				else if (i == skyboxIndex) {
					sceneData.skyboxIndex = ind;
					//std::cout << "Skybox ind: " << ind;
				}
			}

			//CreateSampler(vlk, samplers[i]);
		}
	}

	void BuildInitialObjectData() {
		objectData.resize(primitivesToDraw.size());
		GW::MATH::GMATRIXF scaleMatrix = { scale[0], 0, 0, 0, 0, scale[1], 0, 0, 0, 0, scale[2], 0, 0, 0, 0, 1 };
		for (int i = 0; i < objectData.size(); i++) {
			mathProxy.MultiplyMatrixF(GW::MATH::GIdentityMatrixF, scaleMatrix, objectData[i].worldMatrix);
			objectData[i].materialIndex = primitivesToDraw[i].matIndex;
			objectData[i].samplerIndex = 0;
		}
	}

	void UpdateWindowDimensions()
	{
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
	}


	void CreateViewMatrix(GW::MATH::GMATRIXF& matrix) {
		GW::MATH::GVECTORF camera = { 0.0f, 3.5f, 0.0f, 1 };
		sceneData.cameraWorldPosition = float4{ camera.x,camera.y,camera.z };
		GW::MATH::GVECTORF target = { 0.0f,0.0f,0,1 };
		GW::MATH::GVECTORF up = { 0,1,0,0 };

		mathProxy.LookAtLHF(camera, target, up, matrix);

		float x, y, z;
		x = target.x - camera.x;
		y = target.y - camera.y;
		z = target.z - camera.z;

		float mag = sqrtf(x * x + y * y + z * z);
		x /= mag;
		y /= mag;
		z /= mag;

		float flatMag = sqrtf(x * x + z * z);
		if (flatMag < 0.0001f) {
			cumulPitch = 0;
		}
		else {
			cumulPitch = -atan2f(y, flatMag);
		}
		
	}

	void CreatePerspectiveMatrix(GW::MATH::GMATRIXF& matrix) {
		float aspectRatio = 0.0f;
		vlk.GetAspectRatio(aspectRatio);
		float fov = G_DEGREE_TO_RADIAN_F(config.fovDegrees);
		mathProxy.ProjectionVulkanLHF(fov, aspectRatio, config.farPlane, config.nearPlane, matrix);
	}

	/*void CreateLightMatrices() {
		GW::MATH::GVECTORF center = { 0,0,0,1 };
		float distance = 30.0f;
		GW::MATH::GVECTORF lightDirection = GW::MATH::GVECTORF{ sceneData.lightDirection.x, sceneData.lightDirection.y, sceneData.lightDirection.z, sceneData.lightDirection.w };
		GW::MATH::GVECTORF lightPosition = { center.x - lightDirection.x * distance, center.y - lightDirection.y * distance , center.z - lightDirection.z * distance, 1 };
		GW::MATH::GVECTORF up = { 0,1,0,0 };
		mathProxy.LookAtLHF(lightPosition, center, up, sceneData.lightViewMatrix);

		mathProxy.ProjectionVulkanLHF(G_DEGREE_TO_RADIAN(60), 1, config.nearPlane, config.farPlane, sceneData.lightProjectionMatrix);
	}*/

	void GetTextureData() {
		texData.clear();
		cubeTexData.clear();
		texData.resize(model.textures.size());
		cubeTexData = {};
		for (int i = 0; i < texData.size(); i++) {
			tinygltf::Image img = model.images[model.textures[i].source];
			UploadTextureToGPU(vlk, FindTextureLocation(img), texData[i].image.memory, texData[i].image.image, texData[i].image.imageView);
			texData[i].sampler = model.textures[i].sampler;
			
		}

		samplers.resize(model.samplers.size());
		for (int i = 0; i < samplers.size(); i++) {
			CreateSampler(vlk, samplers[i], VK_SAMPLER_ADDRESS_MODE_REPEAT);
		}
	}

	void GetMaterialData() {
		materialData.clear();
		materialData.resize(model.materials.size());
		for (int i = 0; i < materialData.size(); i++) {
			tinygltf::Material mat = model.materials[i];
			materialData[i].baseColorIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
			materialData[i].metallicRoughnessIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			materialData[i].normalIndex = mat.normalTexture.index;
			for (int j = 0; j < mat.pbrMetallicRoughness.baseColorFactor.size(); j++) {
				materialData[i].baseColorFactor.xyzw[j] = mat.pbrMetallicRoughness.baseColorFactor[j];
			}
			materialData[i].alphaMode = GetAlphaMode(mat.alphaMode);
			materialData[i].alphaCutoff = mat.alphaCutoff;
			
		}
	}

	unsigned int GetAlphaMode(std::string in) {
		if (in == "MASK") {
			return 1;
		}
		else if (in == "BLEND") {
			return 2;
		}
		return 0;
	}

	std::string FindTextureLocation(const tinygltf::Image& img) {
		size_t slash = fileName.find_last_of("/\\");
		std::string parentFolder;
		if (slash == std::string::npos) {
			parentFolder = "";
		}
		else {
			parentFolder = fileName.substr(0, slash + 1);
		}
		parentFolder += img.uri;
		return parentFolder;
	}

	void GetPrimitiveData() {

		primitivesToDraw.clear();
		

		tinygltf::Primitive primitive = model.meshes[0].primitives[0];

		tinygltf::Accessor accessor = model.accessors[primitive.indices];
		
		indexCount = accessor.count;

		tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
		indexOffset = bufferView.byteOffset + accessor.byteOffset;
		//indexByteOffset = bufferView.byteOffset;

		accessor = model.accessors[primitive.attributes.at("POSITION")];
		bufferView = model.bufferViews[accessor.bufferView];
		vertexOffset = bufferView.byteOffset + accessor.byteOffset;
		

		posBindingDescriptionStride = accessor.ByteStride(bufferView);
		attributeDescriptionOffset[0] = 0;

		accessor = model.accessors[primitive.attributes.at("NORMAL")];
		bufferView = model.bufferViews[accessor.bufferView];
		nrmBindingDescriptionStride = accessor.ByteStride(bufferView);
		attributeDescriptionOffset[1] = 0;

		accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
		bufferView = model.bufferViews[accessor.bufferView];
		uvBindingDescriptionStride = accessor.ByteStride(bufferView);
		attributeDescriptionOffset[2] = 0;

		accessor = model.accessors[primitive.attributes.at("TANGENT")];
		bufferView = model.bufferViews[accessor.bufferView];
		tangentBindingDescriptionStride = accessor.ByteStride(bufferView);
		attributeDescriptionOffset[3] = 0;

		for (int m = 0; m < model.meshes.size(); m++) {
			tinygltf::Mesh mesh = model.meshes[m];
			for (int p = 0; p < mesh.primitives.size(); p++) {
				tinygltf::Primitive primitive = mesh.primitives[p];
				AddBufferByteOffset(primitive, primitivesToDraw.size());
				accessor = model.accessors[primitive.indices];
				FindIndexElementSize(accessor);
				bufferView = model.bufferViews[accessor.bufferView];

				Primitive primitiveObj = {};
				primitiveObj.indexCount = accessor.count;
				unsigned int byteOffset = (accessor.byteOffset + bufferView.byteOffset);
				primitiveObj.firstIndex = byteOffset / indexElementSize;
				primitiveObj.instanceIndex = primitivesToDraw.size();
				primitiveObj.matIndex = primitive.material;
				primitiveObj.indexByteOffset = byteOffset;
				primitiveObj.indexType = (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
				primitivesToDraw.push_back(primitiveObj);
			}
		}

	}

	void AddBufferByteOffset(tinygltf::Primitive& primitive, int index) {
		tinygltf::Accessor accessor = model.accessors[model.meshes[0].primitives[index].indices];
		tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
		std::array<VkDeviceSize,4> bufferByteOffsetList;

		accessor = model.accessors[primitive.attributes.at("POSITION")];
		bufferView = model.bufferViews[accessor.bufferView];
		bufferByteOffsetList[0] = bufferView.byteOffset + accessor.byteOffset;

		accessor = model.accessors[primitive.attributes.at("NORMAL")];
		bufferView = model.bufferViews[accessor.bufferView];
		bufferByteOffsetList[1] = bufferView.byteOffset + accessor.byteOffset;

		accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
		bufferView = model.bufferViews[accessor.bufferView];
		bufferByteOffsetList[2] = bufferView.byteOffset + accessor.byteOffset;

		accessor = model.accessors[primitive.attributes.at("TANGENT")];
		bufferView = model.bufferViews[accessor.bufferView];
		bufferByteOffsetList[3] = bufferView.byteOffset + accessor.byteOffset;

		bufferByteOffsets.push_back(bufferByteOffsetList);
	}

	void FindIndexElementSize(tinygltf::Accessor accessor) {
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			indexType = VK_INDEX_TYPE_UINT16;
			indexElementSize = sizeof(uint16_t);
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			indexType = VK_INDEX_TYPE_UINT32;
			indexElementSize = sizeof(unsigned int);
		}
	}

	void InitializeGraphics()
	{
		GetHandlesFromSurface();
		InitializeGeometryBuffer();
		InitializeUniformBuffer();
		InitializeStorageBuffer();
		InitializeMaterialStorageBuffer();

		InitializeShadowPass();
		InitializeShadowCommandBuffers();

		InitalizeBloomPass();
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
	}

	void GetHandlesFromSurface()
	{
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		vlk.GetRenderPass((void**)&renderPass);

		graphicsQueueFamilyIndex = FindGraphicsQueueFamilyIndex();
		vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
	}

	unsigned int FindGraphicsQueueFamilyIndex() {
		unsigned int queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		for (unsigned int i = 0; i < queueFamilyCount; ++i) {
			if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				return i;
			}
		}
		return 0;
	}

	void InitializeGeometryBuffer()
	{
		CreateGeometryBuffer(model.buffers[0].data.data(),model.buffers[0].data.size());
	}

	void InitializeUniformBuffer()
	{
		unsigned int maxFrames = 0;
		if (+vlk.GetSwapchainImageCount(maxFrames)) {

			uniformBuffers.resize(maxFrames);
			descriptorSets.resize(maxFrames);

			for (int i = 0; i < maxFrames; i++) {
				CreateUniformBuffer(&sceneData, sizeof(SHADER_SCENE_DATA), i);
			}
		}
	}

	void InitializeStorageBuffer()
	{
		unsigned int maxFrames = 0;
		if (+vlk.GetSwapchainImageCount(maxFrames)) {
			storageBuffers.resize(maxFrames);
			for (int i = 0; i < maxFrames; i++) {
				CreateStorageBuffer(objectData.data(), sizeof(INSTANCE_DATA) * objectData.size(), i);
			}
		}
	}

	void InitializeMaterialStorageBuffer()
	{
		unsigned int maxFrames = 0;
		if (+vlk.GetSwapchainImageCount(maxFrames)) {
			materialStorageBuffers.resize(maxFrames);
			for (int i = 0; i < maxFrames; i++) {
				CreateMaterialStorageBuffer(materialData.data(), sizeof(Material) * materialData.size(), i);
			}
		}
	}

	void CreateGeometryBuffer(const void* data, unsigned int sizeInBytes)
	{
		geometryBuffer = CreateVulkanBuffer(physicalDevice, device, sizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data);
	}

	void CreateUniformBuffer(const void* data, unsigned int sizeInBytes, int index)
	{
		uniformBuffers[index] = CreateVulkanBuffer(physicalDevice, device, sizeInBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data);
	}

	void CreateStorageBuffer(const void* data, unsigned int sizeInBytes, int index)
	{
		storageBuffers[index] = CreateVulkanBuffer(physicalDevice, device, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data);
	}

	void CreateMaterialStorageBuffer(const void* data, unsigned int sizeInBytes, int index)
	{
		materialStorageBuffers[index] = CreateVulkanBuffer(physicalDevice, device, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data);
	}

	void InitializeShadowCommandBuffers() {
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		vkCreateCommandPool(device, &poolInfo, nullptr, &shadowCommandPool);

		unsigned int maxFrames = 0;
		vlk.GetSwapchainImageCount(maxFrames);

		shadowCommandBuffers = {};
		shadowCommandBuffers.resize(maxFrames);

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = shadowCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = maxFrames;

		vkAllocateCommandBuffers(device, &allocateInfo, shadowCommandBuffers.data());
	}

	void InitializeBloomCommandBuffers() {
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		vkCreateCommandPool(device, &poolInfo, nullptr, &bloomCommandPool);

		unsigned int maxFrames = 0;
		vlk.GetSwapchainImageCount(maxFrames);

		bloomCommandBuffers = {};
		bloomCommandBuffers.resize(maxFrames);
		bloomFences.resize(maxFrames);

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = bloomCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = maxFrames;

		vkAllocateCommandBuffers(device, &allocateInfo, bloomCommandBuffers.data());

		for (unsigned int i = 0; i < maxFrames; i++) {
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			vkCreateFence(device, &fenceCreateInfo, nullptr, &bloomFences[i]);
		}
	}

	void CreateBloomImages() {

		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		{
			VkImageCreateInfo sceneHDRCreateInfo = {};
			sceneHDRCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			sceneHDRCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			sceneHDRCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			sceneHDRCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			sceneHDRCreateInfo.extent.width = width;
			sceneHDRCreateInfo.extent.height = height;
			sceneHDRCreateInfo.extent.depth = 1;
			sceneHDRCreateInfo.arrayLayers = 1;
			sceneHDRCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			sceneHDRCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			sceneHDRCreateInfo.mipLevels = 1;
			sceneHDRCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			sceneHDRCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			vkCreateImage(device, &sceneHDRCreateInfo, nullptr, &sceneHDR.image.image);

			VkMemoryRequirements mem;
			vkGetImageMemoryRequirements(device, sceneHDR.image.image, &mem);

			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = mem.size;
			allocateInfo.memoryTypeIndex = FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device, &allocateInfo, nullptr, &sceneHDR.image.memory);
			vkBindImageMemory(device, sceneHDR.image.image, sceneHDR.image.memory, 0);

			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			viewCreateInfo.subresourceRange = {};
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.image = sceneHDR.image.image;
			vkCreateImageView(device, &viewCreateInfo, nullptr, &sceneHDR.image.imageView);
		}
		{
			VkImageCreateInfo sceneDepthCreateInfo = {};
			sceneDepthCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			sceneDepthCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			sceneDepthCreateInfo.format = VK_FORMAT_D32_SFLOAT;
			sceneDepthCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			sceneDepthCreateInfo.extent.width = width;
			sceneDepthCreateInfo.extent.height = height;
			sceneDepthCreateInfo.extent.depth = 1;
			sceneDepthCreateInfo.arrayLayers = 1;
			sceneDepthCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			sceneDepthCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			sceneDepthCreateInfo.mipLevels = 1;
			sceneDepthCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			sceneDepthCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			vkCreateImage(device, &sceneDepthCreateInfo, nullptr, &sceneDepth.image.image);

			VkMemoryRequirements mem;
			vkGetImageMemoryRequirements(device, sceneDepth.image.image, &mem);

			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = mem.size;
			allocateInfo.memoryTypeIndex = FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device, &allocateInfo, nullptr, &sceneDepth.image.memory);
			vkBindImageMemory(device, sceneDepth.image.image, sceneDepth.image.memory, 0);

			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
			viewCreateInfo.subresourceRange = {};
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.image = sceneDepth.image.image;
			vkCreateImageView(device, &viewCreateInfo, nullptr, &sceneDepth.image.imageView);
		}
		
		{
			VkImageCreateInfo bloomHorizCreateInfo = {};
			bloomHorizCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			bloomHorizCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			bloomHorizCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			bloomHorizCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			bloomHorizCreateInfo.extent.width = width * 0.5f;
			bloomHorizCreateInfo.extent.height = height * 0.5f;
			bloomHorizCreateInfo.extent.depth = 1;
			bloomHorizCreateInfo.arrayLayers = 1;
			bloomHorizCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			bloomHorizCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			bloomHorizCreateInfo.mipLevels = 1;
			bloomHorizCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bloomHorizCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			vkCreateImage(device, &bloomHorizCreateInfo, nullptr, &bloomHoriz.image.image);

			VkMemoryRequirements mem;
			vkGetImageMemoryRequirements(device, bloomHoriz.image.image, &mem);

			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = mem.size;
			allocateInfo.memoryTypeIndex = FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device, &allocateInfo, nullptr, &bloomHoriz.image.memory);
			vkBindImageMemory(device, bloomHoriz.image.image, bloomHoriz.image.memory, 0);

			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			viewCreateInfo.subresourceRange = {};
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.image = bloomHoriz.image.image;
			vkCreateImageView(device, &viewCreateInfo, nullptr, &bloomHoriz.image.imageView);
		}
		
		{
			VkImageCreateInfo bloomVertCreateInfo = {};
			bloomVertCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			bloomVertCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			bloomVertCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			bloomVertCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			bloomVertCreateInfo.extent.width = width * 0.5f;
			bloomVertCreateInfo.extent.height = height * 0.5f;
			bloomVertCreateInfo.extent.depth = 1;
			bloomVertCreateInfo.arrayLayers = 1;
			bloomVertCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			bloomVertCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			bloomVertCreateInfo.mipLevels = 1;
			bloomVertCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bloomVertCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			vkCreateImage(device, &bloomVertCreateInfo, nullptr, &bloomVert.image.image);

			VkMemoryRequirements mem;
			vkGetImageMemoryRequirements(device, bloomVert.image.image, &mem);

			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = mem.size;
			allocateInfo.memoryTypeIndex = FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device, &allocateInfo, nullptr, &bloomVert.image.memory);
			vkBindImageMemory(device, bloomVert.image.image, bloomVert.image.memory, 0);

			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			viewCreateInfo.subresourceRange = {};
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.image = bloomVert.image.image;
			vkCreateImageView(device, &viewCreateInfo, nullptr, &bloomVert.image.imageView);
		}
		
	}

	void CreateBloomRenderPass() {
		VkAttachmentDescription attachment{};
		attachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;

		VkSubpassDependency subpassDependencies[2]{};
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = 2;
		renderPassCreateInfo.pDependencies = subpassDependencies;

		vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &bloomRenderPass);
	}

	void CreateOffscreenRenderPass() {
		VkAttachmentDescription attachments[2]{};
		attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		attachments[1].format = VK_FORMAT_D32_SFLOAT;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference{};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		VkSubpassDependency subpassDependencies[2]{};
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 2;
		renderPassCreateInfo.pAttachments = attachments;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = 2;
		renderPassCreateInfo.pDependencies = subpassDependencies;

		vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &offscreenRenderPass);
	}

	void CreateOffscreenFramebuffers() {
		unsigned int width = windowWidth;
		unsigned int height = windowHeight;

		{
			VkImageView attachments[2] = { sceneHDR.image.imageView, sceneDepth.image.imageView };

			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = offscreenRenderPass;
			framebufferCreateInfo.attachmentCount = 2;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = width;
			framebufferCreateInfo.height = height;
			framebufferCreateInfo.layers = 1;

			vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &sceneHDR.framebuffer);
		}

		{
			VkImageView attachment = bloomVert.image.imageView;
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = bloomRenderPass;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.pAttachments = &attachment;
			framebufferCreateInfo.width = (unsigned int)width * 0.5f;
			framebufferCreateInfo.height = (unsigned int)height * 0.5f;
			framebufferCreateInfo.layers = 1;

			vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &bloomVert.framebuffer);
		}

		{
			VkImageView attachment = bloomHoriz.image.imageView;
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = bloomRenderPass;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.pAttachments = &attachment;
			framebufferCreateInfo.width = width * 0.5f;
			framebufferCreateInfo.height = height * 0.5f;
			framebufferCreateInfo.layers = 1;

			vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &bloomHoriz.framebuffer);
		}
	}

	void CreatePostSampler() {
		VkSampler postSampler;
		CreateSampler(vlk, postSampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR);
		samplers.push_back(postSampler);
		bloomSamplerIndex = (unsigned int)(samplers.size() - 1);
	}

	void InitalizeBloomPass() {
		CreateBloomImages();
		CreatePostSampler();
		CreateBloomRenderPass();
		CreateOffscreenRenderPass();
		CreateOffscreenFramebuffers();
		TransitionBloomImagesToReadOnly_Once();
	}

	void InitializeShadowPass() {
		CreateShadowImageAndViews();
		TransitionShadowImagesToReadOnly_Once();
		CreateShadowRenderPass();
		CreateShadowFramebuffer();

		sceneData.shadowIndex = texData.size();
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
			TextureData shadowTexture = {};
			shadowTexture.image.image = shadowCascades[i].depthImage.image;
			shadowTexture.image.imageView = shadowCascades[i].depthImage.imageView;
			shadowTexture.image.memory = VK_NULL_HANDLE;
			shadowTexture.sampler = samplers.size();
			shadowTexture.ownsImage = false;
			texData.push_back(shadowTexture);
		}
		VkSampler sampler = {};
		CreateSampler(vlk, sampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_FILTER_NEAREST, 0.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, VK_FALSE, VK_COMPARE_OP_GREATER, VK_FALSE);
		samplers.push_back(sampler);
		sceneData.shadowSamplerIndex = (unsigned int)(samplers.size() - 1);
	}

	void CreateShadowImageAndViews() {
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.width = shadowMapSize;
		imageCreateInfo.extent.height = shadowMapSize;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
			vkCreateImage(device, &imageCreateInfo, nullptr, &shadowCascades[i].depthImage.image);

			VkMemoryRequirements mem;
			vkGetImageMemoryRequirements(device, shadowCascades[i].depthImage.image, &mem);

			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = mem.size;
			allocateInfo.memoryTypeIndex = FindMemoryType(mem.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device, &allocateInfo, nullptr, &shadowCascades[i].depthImage.memory);
			vkBindImageMemory(device, shadowCascades[i].depthImage.image, shadowCascades[i].depthImage.memory, 0);

			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
			viewCreateInfo.subresourceRange = {};
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.image = shadowCascades[i].depthImage.image;
			vkCreateImageView(device, &viewCreateInfo, nullptr, &shadowCascades[i].depthImage.imageView);

		}
	}

	unsigned int FindMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (int i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		return 0;
	}

	void CreateShadowRenderPass() {
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = VK_FORMAT_D32_SFLOAT;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentDescription.flags = 0;

		VkAttachmentReference attachmentReference = {};
		attachmentReference.attachment = 0;
		attachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 0;
		subpassDescription.pDepthStencilAttachment = &attachmentReference;

		VkSubpassDependency subpassDependencies[2] = {};
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo passCreateInfo = {};
		passCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		passCreateInfo.attachmentCount = 1;
		passCreateInfo.pAttachments = &attachmentDescription;
		passCreateInfo.subpassCount = 1;
		passCreateInfo.pSubpasses = &subpassDescription;
		passCreateInfo.dependencyCount = 2;
		passCreateInfo.pDependencies = subpassDependencies;

		vkCreateRenderPass(device, &passCreateInfo, nullptr, &shadowRenderPass);
	}

	void TransitionShadowImagesToReadOnly_Once()
	{
		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		vkCreateCommandPool(device, &poolInfo, nullptr, &pool);

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo alloc{};
		alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc.commandPool = pool;
		alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc.commandBufferCount = 1;

		vkAllocateCommandBuffers(device, &alloc, &cmd);

		VkCommandBufferBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &begin);

		for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			barrier.image = shadowCascades[i].depthImage.image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(
				cmd,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		vkEndCommandBuffer(cmd);

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;

		vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, pool, 1, &cmd);
		vkDestroyCommandPool(device, pool, nullptr);
	}

	void TransitionBloomImagesToReadOnly_Once()
	{
		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		vkCreateCommandPool(device, &poolInfo, nullptr, &pool);

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo alloc{};
		alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc.commandPool = pool;
		alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc.commandBufferCount = 1;

		vkAllocateCommandBuffers(device, &alloc, &cmd);

		VkCommandBufferBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &begin);

		{
			VkImageMemoryBarrier barrier = CreateImageMemoryBarrier(sceneHDR.image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
		{
			VkImageMemoryBarrier barrier = CreateImageMemoryBarrier(bloomVert.image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
		{
			VkImageMemoryBarrier barrier = CreateImageMemoryBarrier(bloomHoriz.image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
		{
			VkImageMemoryBarrier barrier = CreateImageMemoryBarrier(sceneDepth.image.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		vkEndCommandBuffer(cmd);

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;

		vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, pool, 1, &cmd);
		vkDestroyCommandPool(device, pool, nullptr);
	}

	VkImageMemoryBarrier CreateImageMemoryBarrier(VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags) {
		VkImageMemoryBarrier ret{};
		ret.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ret.oldLayout = oldLayout;
		ret.newLayout = newLayout;
		ret.srcAccessMask = srcAccessFlags;
		ret.dstAccessMask = dstAccessFlags;
		ret.image = image;
		ret.subresourceRange.aspectMask = aspectFlags;
		ret.subresourceRange.baseMipLevel = 0;
		ret.subresourceRange.levelCount = 1;
		ret.subresourceRange.baseArrayLayer = 0;
		ret.subresourceRange.layerCount = 1;

		return ret;
	}

	void CreateShadowFramebuffer() {
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferCreateInfo.renderPass = shadowRenderPass;
			frameBufferCreateInfo.attachmentCount = 1;
			frameBufferCreateInfo.pAttachments = &shadowCascades[i].depthImage.imageView;
			frameBufferCreateInfo.width = shadowMapSize;
			frameBufferCreateInfo.height = shadowMapSize;
			frameBufferCreateInfo.layers = 1;

			vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &shadowCascades[i].frameBuffer);
		}
	}

	void CompileShaders()
	{
		// Intialize runtime shader compiler HLSL -> SPIRV
		//shaderc_compiler_t compiler = shaderc_compiler_initialize();
		//shaderc_compile_options_t options = CreateCompileOptions();
		//CompileVertexShader(compiler, options);
		CompileVertexShader_DXC();
		CompileFragmentShader_DXC();
		CompileShadowVertexShader_DXC();
		CompileShadowFragmentShader_DXC();
		CompileSkyboxVertexShader_DXC();
		CompileSkyboxFragmentShader_DXC();
		CompileFullScreenVertexShader_DXC();
		CompileBloomBlurFragmentShader_DXC();
		CompileBloomCompositeFragmentShader_DXC();
		//CompileShadowVertexShader(compiler, options);
		//CompileShadowFragmentShader(compiler, options);
		//CompileSkyboxVertexShader(compiler, options);
		//CompileSkyboxFragmentShader(compiler, options);
		// Free runtime shader compiler resources
		//shaderc_compile_options_release(options);
		//shaderc_compiler_release(compiler);
	}

	void CreateDescriptorPool() {
		VkDescriptorPoolSize poolSize[4] = {};
		poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize[0].descriptorCount = descriptorSets.size();

		poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize[1].descriptorCount = texData.size() + cubeTexData.size();

		poolSize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize[2].descriptorCount = descriptorSets.size();

		poolSize[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize[3].descriptorCount = descriptorSets.size();

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 4;
		poolInfo.pPoolSizes = poolSize;
		poolInfo.maxSets = descriptorSets.size() + 2;

		vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
	}

	void CreateBloomDescriptorSets() {
		unsigned int maxFrames = 0;
		vlk.GetSwapchainImageCount(maxFrames);

		bloomVertSets.resize(maxFrames);
		bloomHorizSets.resize(maxFrames);
		bloomCompositeSets.resize(maxFrames);

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = maxFrames * 4;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;
		poolCreateInfo.maxSets = maxFrames * 3;

		vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &bloomDescriptorPool);

		for (unsigned int i = 0; i < maxFrames; i++) {
			VkDescriptorSetAllocateInfo setAllocateInfo{};
			setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			setAllocateInfo.descriptorPool = bloomDescriptorPool;
			setAllocateInfo.descriptorSetCount = 1;
			setAllocateInfo.pSetLayouts = &bloomBlurDescriptorSetLayout;
			vkAllocateDescriptorSets(device, &setAllocateInfo, &bloomVertSets[i]);
			vkAllocateDescriptorSets(device, &setAllocateInfo, &bloomHorizSets[i]);

			VkDescriptorSetAllocateInfo compositeSetAllocateInfo = setAllocateInfo;
			compositeSetAllocateInfo.pSetLayouts = &bloomCompositeDescriptorSetLayout;
			vkAllocateDescriptorSets(device, &compositeSetAllocateInfo, &bloomCompositeSets[i]);
		}

		VkSampler postSampler = samplers[bloomSamplerIndex];

		VkDescriptorImageInfo sceneViewInfo{};
		sceneViewInfo.sampler = postSampler;
		sceneViewInfo.imageView = sceneHDR.image.imageView;
		sceneViewInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo vertInfo{};
		vertInfo.sampler = postSampler;
		vertInfo.imageView = bloomVert.image.imageView;
		vertInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo horizInfo{};
		horizInfo.sampler = postSampler;
		horizInfo.imageView = bloomHoriz.image.imageView;
		horizInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		for (unsigned int i = 0; i < maxFrames; i++) {
			VkWriteDescriptorSet sceneSample{};
			sceneSample.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			sceneSample.dstSet = bloomVertSets[i];
			sceneSample.dstBinding = 0;
			sceneSample.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			sceneSample.descriptorCount = 1;
			sceneSample.pImageInfo = &sceneViewInfo;

			vkUpdateDescriptorSets(device, 1, &sceneSample, 0, nullptr);

			VkWriteDescriptorSet vertSample = sceneSample;
			vertSample.dstSet = bloomHorizSets[i];
			vertSample.pImageInfo = &vertInfo;
			
			vkUpdateDescriptorSets(device, 1, &vertSample, 0, nullptr);

			VkDescriptorImageInfo compositeImageInfos[2] = { sceneViewInfo, horizInfo };
			VkWriteDescriptorSet compositeDescriptorSets[2]{};
			compositeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			compositeDescriptorSets[0].dstSet = bloomCompositeSets[i];
			compositeDescriptorSets[0].dstBinding = 0;
			compositeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			compositeDescriptorSets[0].descriptorCount = 1;
			compositeDescriptorSets[0].pImageInfo = &compositeImageInfos[0];

			compositeDescriptorSets[1] = compositeDescriptorSets[0];
			compositeDescriptorSets[1].dstBinding = 1;
			compositeDescriptorSets[1].pImageInfo = &compositeImageInfos[1];

			vkUpdateDescriptorSets(device, 2, compositeDescriptorSets, 0, nullptr);
		}
	}

	void AllocateDescriptorSets() {

		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorSetLayout;

		for (int i = 0; i < descriptorSets.size(); i++) {
			vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSets[i]);
		}

		VkDescriptorSetAllocateInfo texAllocateInfo = {};
		texAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		texAllocateInfo.descriptorPool = descriptorPool;
		texAllocateInfo.descriptorSetCount = 1;
		texAllocateInfo.pSetLayouts = &textureDescriptorSetLayout;

		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_descriptor_allocate_info_ext = {};
		variable_descriptor_allocate_info_ext.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variable_descriptor_allocate_info_ext.descriptorSetCount = 1;
		unsigned int descriptorCounts = (unsigned int)texData.size();
		variable_descriptor_allocate_info_ext.pDescriptorCounts = &descriptorCounts;

		texAllocateInfo.pNext = &variable_descriptor_allocate_info_ext;

		vkAllocateDescriptorSets(device, &texAllocateInfo, &textureDescriptorSet);

		VkDescriptorSetAllocateInfo cubeTexAllocateInfo = {};
		cubeTexAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		cubeTexAllocateInfo.descriptorPool = descriptorPool;
		cubeTexAllocateInfo.descriptorSetCount = 1;
		cubeTexAllocateInfo.pSetLayouts = &cubeTextureDescriptorSetLayout;

		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT cube_variable_descriptor_allocate_info_ext = {};
		cube_variable_descriptor_allocate_info_ext.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		cube_variable_descriptor_allocate_info_ext.descriptorSetCount = 1;
		unsigned int cubeDescriptorCounts = (unsigned int)cubeTexData.size();
		cube_variable_descriptor_allocate_info_ext.pDescriptorCounts = &cubeDescriptorCounts;

		cubeTexAllocateInfo.pNext = &cube_variable_descriptor_allocate_info_ext;

		vkAllocateDescriptorSets(device, &cubeTexAllocateInfo, &cubeTextureDescriptorSet);
	}

	void UpdateDescriptorSets() {
		for (int i = 0; i < descriptorSets.size(); i++) {
			VkDescriptorBufferInfo bufferInfo[3] = {};
			bufferInfo[0].buffer = uniformBuffers[i].handle;
			bufferInfo[0].offset = 0;
			bufferInfo[0].range = sizeof(SHADER_SCENE_DATA);

			bufferInfo[1].buffer = storageBuffers[i].handle;
			bufferInfo[1].offset = 0;
			bufferInfo[1].range = sizeof(INSTANCE_DATA) * objectData.size();

			bufferInfo[2].buffer = materialStorageBuffers[i].handle;
			bufferInfo[2].offset = 0;
			bufferInfo[2].range = sizeof(Material) * materialData.size();

			VkWriteDescriptorSet write[3] = {};
			write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[0].dstSet = descriptorSets[i];
			write[0].dstBinding = 0;
			write[0].dstArrayElement = 0;
			write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write[0].descriptorCount = 1;
			write[0].pBufferInfo = &bufferInfo[0];

			write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[1].dstSet = descriptorSets[i];
			write[1].dstBinding = 1;
			write[1].dstArrayElement = 0;
			write[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write[1].descriptorCount = 1;
			write[1].pBufferInfo = &bufferInfo[1];

			write[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[2].dstSet = descriptorSets[i];
			write[2].dstBinding = 2;
			write[2].dstArrayElement = 0;
			write[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write[2].descriptorCount = 1;
			write[2].pBufferInfo = &bufferInfo[2];

			vkUpdateDescriptorSets(device, 3, write, 0, nullptr);
		}

		std::vector<VkDescriptorImageInfo> imageInfo(texData.size());
		for (int i = 0; i < texData.size(); i++) {
			bool isShadow = (i >= (int)sceneData.shadowIndex) && (i < (int)sceneData.shadowIndex + SHADOW_CASCADE_COUNT);
			imageInfo[i].imageLayout = isShadow ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[i].imageView = texData[i].image.imageView;
			imageInfo[i].sampler = samplers[texData[i].sampler];
		}
		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = textureDescriptorSet;
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = texData.size();
		write.pImageInfo = imageInfo.data();

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		
		std::vector<VkDescriptorImageInfo> cubeInfo(cubeTexData.size());
		for (int i = 0; i < cubeTexData.size(); i++) {
			//bool isShadow = (i >= (int)sceneData.shadowIndex) && (i < (int)sceneData.shadowIndex + SHADOW_CASCADE_COUNT);
			cubeInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			cubeInfo[i].imageView = cubeTexData[i].image.imageView;
			cubeInfo[i].sampler = samplers[cubeTexData[i].sampler];
		}
		VkWriteDescriptorSet cubeWrite = {};
		cubeWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cubeWrite.dstSet = cubeTextureDescriptorSet;
		cubeWrite.dstBinding = 0;
		cubeWrite.dstArrayElement = 0;
		cubeWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		cubeWrite.descriptorCount = cubeInfo.size();
		cubeWrite.pImageInfo = cubeInfo.data();

		vkUpdateDescriptorSets(device, 1, &cubeWrite, 0, nullptr);


	}

//	shaderc_compile_options_t CreateCompileOptions()
//	{
//		shaderc_compile_options_t retval = shaderc_compile_options_initialize();
//		shaderc_compile_options_set_source_language(retval, shaderc_source_language_hlsl);
//		shaderc_compile_options_set_invert_y(retval, false);	
//#ifndef NDEBUG
//		shaderc_compile_options_set_generate_debug_info(retval);
//#endif
//		return retval;
//	}
//
//	void CompileVertexShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
//	{
//		std::string vertexShaderSource = ReadFileIntoString("../VertexShader.hlsl");
//		
//		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
//			compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
//			shaderc_vertex_shader, "main.vert", "main", options);
//
//		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
//		{
//			PrintLabeledDebugString("Vertex Shader Errors: \n", shaderc_result_get_error_message(result));
//			abort(); //Vertex shader failed to compile! 
//			return;
//		}
//
//		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
//			(char*)shaderc_result_get_bytes(result), &vertexShader);
//
//		shaderc_result_release(result); // done
//	}
//
//	void CompileShadowVertexShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
//	{
//		std::string vertexShaderSource = ReadFileIntoString("../ShadowVertexShader.hlsl");
//
//		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
//			compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
//			shaderc_vertex_shader, "main.vert", "main", options);
//
//		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
//		{
//			PrintLabeledDebugString("Vertex Shader Errors: \n", shaderc_result_get_error_message(result));
//			abort(); //Vertex shader failed to compile! 
//			return;
//		}
//
//		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
//			(char*)shaderc_result_get_bytes(result), &shadowVertexShader);
//
//		shaderc_result_release(result); // done
//	}

	void CompileVertexShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../VertexShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"vs_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&vertexShader);
	}

	void CompileShadowVertexShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../ShadowVertexShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"vs_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&shadowVertexShader);
	}

	void CompileSkyboxVertexShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../SkyboxVertexShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"vs_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&skyboxVertexShader);
	}

	void CompileFullScreenVertexShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../FullscreenVertexShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"vs_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&fullscreenVertShader);
	}

	void CompileFragmentShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../FragmentShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"ps_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&fragmentShader);
	}

	void CompileShadowFragmentShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../ShadowFragmentShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"ps_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&shadowFragmentShader);
	}

	void CompileSkyboxFragmentShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../SkyboxFragmentShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"ps_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&skyboxFragmentShader);
	}

	void CompileBloomBlurFragmentShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../BloomBlurFragmentShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"ps_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&bloomBlurShader);
	}

	void CompileBloomCompositeFragmentShader_DXC()
	{
		std::string fragmentShaderSource = ReadFileIntoString("../BloomCompositeFragmentShader.hlsl");

		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hr)) { abort(); }
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcBlobEncoding> sourceBlob;
		hr = utils->CreateBlob(fragmentShaderSource.data(),
			(uint32_t)fragmentShaderSource.size(),
			CP_UTF8, &sourceBlob);
		if (FAILED(hr)) { abort(); }

		ComPtr<IDxcIncludeHandler> includeHandler;
		utils->CreateDefaultIncludeHandler(&includeHandler);

		std::vector<LPCWSTR> args = {
			L"-E", L"main",
			L"-T", L"ps_6_6",
			L"-spirv",
			L"-fspv-target-env=vulkan1.1",
			L"-fvk-use-dx-layout",
			L"-O3"
		};

		DxcBuffer src{};
		src.Ptr = sourceBlob->GetBufferPointer();
		src.Size = sourceBlob->GetBufferSize();
		src.Encoding = DXC_CP_UTF8;

		ComPtr<IDxcResult> result;
		hr = compiler->Compile(&src, args.data(), (uint32_t)args.size(),
			includeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr) || !result) { abort(); }

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			PrintLabeledDebugString("DXC Fragment Shader Errors:\n", errors->GetStringPointer());
			abort();
		}
		ComPtr<IDxcBlob> spirv;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
		if (!spirv) { abort(); }

		GvkHelper::create_shader_module(device,
			(uint32_t)spirv->GetBufferSize(),
			(char*)spirv->GetBufferPointer(),
			&bloomCompositeShader);
	}
	

	//void CompileShadowFragmentShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	//{
	//	std::string fragmentShaderSource = ReadFileIntoString("../ShadowFragmentShader.hlsl");

	//	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
	//		compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
	//		shaderc_fragment_shader, "main.frag", "main", options);

	//	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
	//	{
	//		PrintLabeledDebugString("Fragment Shader Errors: \n", shaderc_result_get_error_message(result));
	//		abort(); //Fragment shader failed to compile! 
	//		return;
	//	}

	//	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
	//		(char*)shaderc_result_get_bytes(result), &shadowFragmentShader);

	//	shaderc_result_release(result); // done
	//}

	//void CompileSkyboxVertexShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	//{
	//	std::string vertexShaderSource = ReadFileIntoString("../SkyboxVertexShader.hlsl");

	//	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
	//		compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
	//		shaderc_vertex_shader, "main.vert", "main", options);

	//	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
	//	{
	//		PrintLabeledDebugString("Vertex Shader Errors: \n", shaderc_result_get_error_message(result));
	//		abort(); //Vertex shader failed to compile! 
	//		return;
	//	}

	//	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
	//		(char*)shaderc_result_get_bytes(result), &skyboxVertexShader);

	//	shaderc_result_release(result); // done
	//}

	//void CompileSkyboxFragmentShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	//{
	//	std::string fragmentShaderSource = ReadFileIntoString("../SkyboxFragmentShader.hlsl");

	//	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
	//		compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
	//		shaderc_fragment_shader, "main.frag", "main", options);

	//	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
	//	{
	//		PrintLabeledDebugString("Fragment Shader Errors: \n", shaderc_result_get_error_message(result));
	//		abort(); //Fragment shader failed to compile! 
	//		return;
	//	}

	//	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
	//		(char*)shaderc_result_get_bytes(result), &skyboxFragmentShader);

	//	shaderc_result_release(result); // done
	//}

	// Create Pipeline & Layout (Thanks Tiny!)
	void InitializeGraphicsPipeline()
	{
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};	

		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";

		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = fragmentShader;
		stage_create_info[1].pName = "main";


		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();
		VkVertexInputBindingDescription vertex_binding_description[4] = {};
		CreateVkVertexInputBindingDescription(vertex_binding_description);

		VkVertexInputAttributeDescription vertex_attribute_descriptions[4];
		vertex_attribute_descriptions[0].binding = 0;
		vertex_attribute_descriptions[0].location = 0;
		vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_descriptions[0].offset = 0;

		vertex_attribute_descriptions[1].binding = 1;
		vertex_attribute_descriptions[1].location = 1;
		vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_descriptions[1].offset = 0;

		vertex_attribute_descriptions[2].binding = 2;
		vertex_attribute_descriptions[2].location = 2;
		vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attribute_descriptions[2].offset = 0;

		vertex_attribute_descriptions[3].binding = 3;
		vertex_attribute_descriptions[3].location = 3;
		vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_attribute_descriptions[3].offset = 0;

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(vertex_binding_description, 4, vertex_attribute_descriptions, 4);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkPipelineRasterizationStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);
		
		VkDynamicState dynamic_states[2] = 
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		CreatePipelineLayout();

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);
	}

	void InitializeShadowGraphicsPipeline()
	{
		VkPipelineShaderStageCreateInfo stage_create_info[1] = {};

		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = shadowVertexShader;
		stage_create_info[0].pName = "main";


		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();
		VkVertexInputBindingDescription vertex_binding_description[4] = {};
		CreateVkVertexInputBindingDescription(vertex_binding_description);

		VkVertexInputAttributeDescription vertex_attribute_descriptions[4];
		vertex_attribute_descriptions[0].binding = 0;
		vertex_attribute_descriptions[0].location = 0;
		vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_descriptions[0].offset = 0;

		vertex_attribute_descriptions[1].binding = 1;
		vertex_attribute_descriptions[1].location = 1;
		vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_descriptions[1].offset = 0;

		vertex_attribute_descriptions[2].binding = 2;
		vertex_attribute_descriptions[2].location = 2;
		vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attribute_descriptions[2].offset = 0;

		vertex_attribute_descriptions[3].binding = 3;
		vertex_attribute_descriptions[3].location = 3;
		vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_attribute_descriptions[3].offset = 0;

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(vertex_binding_description, 4, vertex_attribute_descriptions, 4);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkShadowPipelineRasterizationStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();

		VkDynamicState dynamic_states[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 1;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = 0;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = shadowRenderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &shadowPipeline);
	}

	void InitializeOffscreenGraphicsPipeline()
	{
		{
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};

		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";

		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = fragmentShader;
		stage_create_info[1].pName = "main";

		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();
		VkVertexInputBindingDescription vertex_binding_description[4] = {};
		CreateVkVertexInputBindingDescription(vertex_binding_description);

		VkVertexInputAttributeDescription vertex_attribute_descriptions[4];
		vertex_attribute_descriptions[0].binding = 0;
		vertex_attribute_descriptions[0].location = 0;
		vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_descriptions[0].offset = 0;

		vertex_attribute_descriptions[1].binding = 1;
		vertex_attribute_descriptions[1].location = 1;
		vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_descriptions[1].offset = 0;

		vertex_attribute_descriptions[2].binding = 2;
		vertex_attribute_descriptions[2].location = 2;
		vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attribute_descriptions[2].offset = 0;

		vertex_attribute_descriptions[3].binding = 3;
		vertex_attribute_descriptions[3].location = 3;
		vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_attribute_descriptions[3].offset = 0;

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(vertex_binding_description, 4, vertex_attribute_descriptions, 4);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkPipelineRasterizationStateCreateInfo();
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_GREATER;
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);
		VkDynamicState dynamic_states[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = offscreenRenderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &sceneOffscreenPipeline);
		}
		{
			VkPipelineShaderStageCreateInfo stage_create_info[2] = {};

			// Create Stage Info for Vertex Shader
			stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			stage_create_info[0].module = skyboxVertexShader;
			stage_create_info[0].pName = "main";

			// Create Stage Info for Fragment Shader
			stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stage_create_info[1].module = skyboxFragmentShader;
			stage_create_info[1].pName = "main";


			VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();

			VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(nullptr, 0, nullptr, 0);
			VkViewport viewport = CreateViewportFromWindowDimensions();
			VkRect2D scissor = CreateScissorFromWindowDimensions();
			VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
			VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkPipelineRasterizationStateCreateInfo();
			VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
			VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
			depth_stencil_create_info.depthTestEnable = VK_TRUE;
			depth_stencil_create_info.depthWriteEnable = VK_FALSE;
			depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
			VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
			VkPipelineColorBlendStateCreateInfo color_blend_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);
			color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			color_blend_attachment_state.blendEnable = VK_FALSE;
			color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			color_blend_create_info.attachmentCount = 1;
			color_blend_create_info.pAttachments = &color_blend_attachment_state;
			VkDynamicState dynamic_states[2] =
			{
				// By setting these we do not need to re-create the pipeline on Resize
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

			// Pipeline State... (FINALLY) 
			VkGraphicsPipelineCreateInfo pipeline_create_info = {};
			pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_create_info.stageCount = 2;
			pipeline_create_info.pStages = stage_create_info;
			pipeline_create_info.pInputAssemblyState = &assembly_create_info;
			pipeline_create_info.pVertexInputState = &input_vertex_info;
			pipeline_create_info.pViewportState = &viewport_create_info;
			pipeline_create_info.pRasterizationState = &rasterization_create_info;
			pipeline_create_info.pMultisampleState = &multisample_create_info;
			pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
			pipeline_create_info.pColorBlendState = &color_blend_create_info;
			pipeline_create_info.pDynamicState = &dynamic_create_info;
			pipeline_create_info.layout = pipelineLayout;
			pipeline_create_info.renderPass = offscreenRenderPass;
			pipeline_create_info.subpass = 0;
			pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

			vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &skyboxOffscreenPipeline);
		}
	}

	void InitializeBloomGraphicsPipeline() {
		if (bloomBlurDescriptorSetLayout == VK_NULL_HANDLE) {
			InitializeBloomDescriptorSets();
		}
		if (bloomBlurPipelineLayout == VK_NULL_HANDLE) {
			InitializeBloomBlurPipelineLayout();
		}
		if (bloomCompositePipelineLayout == VK_NULL_HANDLE) {
			InitializeBloomCompositePipelineLayout();
		}
		
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};

		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = fullscreenVertShader;
		stage_create_info[0].pName = "main";

		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = bloomBlurShader;
		stage_create_info[1].pName = "main";

		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(nullptr, 0, nullptr, 0);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkShadowPipelineRasterizationStateCreateInfo();
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.cullMode = VK_CULL_MODE_NONE;
		rasterization_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization_create_info.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		depth_stencil_create_info.depthTestEnable = VK_FALSE;
		depth_stencil_create_info.depthWriteEnable = VK_FALSE;
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);
		VkDynamicState dynamic_states[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = bloomBlurPipelineLayout;
		pipeline_create_info.renderPass = bloomRenderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &bloomVertPipeline);
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &bloomHorizPipeline);

		VkPipelineShaderStageCreateInfo composite_stage_create_info[2]{};
		composite_stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		composite_stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		composite_stage_create_info[0].module = fullscreenVertShader;
		composite_stage_create_info[0].pName = "main";

		composite_stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		composite_stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		composite_stage_create_info[1].module = bloomCompositeShader;
		composite_stage_create_info[1].pName = "main";

		VkGraphicsPipelineCreateInfo composite_pipeline_create_info = pipeline_create_info;
		composite_pipeline_create_info.pStages = composite_stage_create_info;
		composite_pipeline_create_info.layout = bloomCompositePipelineLayout;
		composite_pipeline_create_info.renderPass = renderPass;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &composite_pipeline_create_info, nullptr, &bloomCompositePipeline);
	}

	void InitializeBloomDescriptorSets() {
		VkDescriptorSetLayoutBinding blurBinding{};
		blurBinding.binding = 0;
		blurBinding.descriptorCount = 1;
		blurBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		blurBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = &blurBinding;
		vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &bloomBlurDescriptorSetLayout);

		VkDescriptorSetLayoutBinding compositeBindings[2]{};
		compositeBindings[0] = blurBinding;
		compositeBindings[0].binding = 0;

		compositeBindings[1] = blurBinding;
		compositeBindings[1].binding = 1;

		VkDescriptorSetLayoutCreateInfo compositeLayoutCreateInfo{};
		compositeLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		compositeLayoutCreateInfo.bindingCount = 2;
		compositeLayoutCreateInfo.pBindings = compositeBindings;
		vkCreateDescriptorSetLayout(device, &compositeLayoutCreateInfo, nullptr, &bloomCompositeDescriptorSetLayout);
	}

	void InitializeBloomBlurPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(BloomPushConstants);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &bloomBlurDescriptorSetLayout;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &bloomBlurPipelineLayout);
	}

	void InitializeBloomCompositePipelineLayout() {
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &bloomCompositeDescriptorSetLayout;
		vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &bloomCompositePipelineLayout);
	}

	void InitializeSkyboxGraphicsPipeline()
	{
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};

		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = skyboxVertexShader;
		stage_create_info[0].pName = "main";

		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = skyboxFragmentShader;
		stage_create_info[1].pName = "main";


		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(nullptr, 0, nullptr, 0);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkShadowPipelineRasterizationStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_FALSE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);
		color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		VkDynamicState dynamic_states[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &skyboxPipeline);
	}

	VkPipelineShaderStageCreateInfo CreateVertexShaderStageCreateInfo()
	{
		VkPipelineShaderStageCreateInfo retval;
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		retval.stage = VK_SHADER_STAGE_VERTEX_BIT;
		retval.module = vertexShader;
		retval.pName = "main";
		return retval;
	}

	VkPipelineInputAssemblyStateCreateInfo CreateVkPipelineInputAssemblyStateCreateInfo()
	{
		VkPipelineInputAssemblyStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		retval.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		retval.primitiveRestartEnable = false;
		return retval;
	}

	void CreateVkVertexInputBindingDescription(VkVertexInputBindingDescription(&retval)[4])
	{
		retval[0] = {};
		retval[0].binding = 0;
		retval[0].stride = posBindingDescriptionStride;
		retval[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		retval[1] = {};
		retval[1].binding = 1;
		retval[1].stride = nrmBindingDescriptionStride;
		retval[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		retval[2] = {};
		retval[2].binding = 2;
		retval[2].stride = uvBindingDescriptionStride;
		retval[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		retval[3] = {};
		retval[3].binding = 3;
		retval[3].stride = tangentBindingDescriptionStride;
		retval[3].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
	}

	VkPipelineVertexInputStateCreateInfo CreateVkPipelineVertexInputStateCreateInfo(
		VkVertexInputBindingDescription* inputBindingDescriptions, unsigned int bindingCount,
		VkVertexInputAttributeDescription* vertexAttributeDescriptions, unsigned int attributeCount)
	{
		VkPipelineVertexInputStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		retval.vertexBindingDescriptionCount = bindingCount;
		retval.pVertexBindingDescriptions = inputBindingDescriptions;
		retval.vertexAttributeDescriptionCount = attributeCount;
		retval.pVertexAttributeDescriptions = vertexAttributeDescriptions;
		return retval;
	}

	VkViewport CreateViewportFromWindowDimensions()
	{
		VkViewport retval = {};
		retval.x = 0;
		retval.y = 0;
		retval.width = static_cast<float>(windowWidth);
		retval.height = static_cast<float>(windowHeight);
		retval.minDepth = 0.0f;
		retval.maxDepth = 1.0f;
		return retval;
	}

	VkRect2D CreateScissorFromWindowDimensions()
	{
		VkRect2D retval = {};
		retval.offset.x = 0;
		retval.offset.y = 0;
		retval.extent.width = windowWidth;
		retval.extent.height = windowHeight;
		return retval;
	}

	VkPipelineViewportStateCreateInfo CreateVkPipelineViewportStateCreateInfo(VkViewport* viewports, unsigned int viewportCount, VkRect2D* scissors, unsigned int scissorCount)
	{
		VkPipelineViewportStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		retval.viewportCount = viewportCount;
		retval.pViewports = viewports;
		retval.scissorCount = scissorCount;
		retval.pScissors = scissors;
		return retval;
	}

	VkPipelineRasterizationStateCreateInfo CreateVkPipelineRasterizationStateCreateInfo()
	{
		VkPipelineRasterizationStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		retval.rasterizerDiscardEnable = VK_FALSE;
		retval.polygonMode = VK_POLYGON_MODE_FILL;
		retval.lineWidth = 1.0f;
		retval.cullMode = VK_CULL_MODE_BACK_BIT;
		retval.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		retval.depthClampEnable = VK_FALSE;
		retval.depthBiasEnable = VK_FALSE;
		retval.depthBiasClamp = 0.0f;
		retval.depthBiasConstantFactor = 0.0f;
		retval.depthBiasSlopeFactor = 0.0f;
		return retval;
	}

	VkPipelineRasterizationStateCreateInfo CreateVkShadowPipelineRasterizationStateCreateInfo()
	{
		VkPipelineRasterizationStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		retval.rasterizerDiscardEnable = VK_FALSE;
		retval.polygonMode = VK_POLYGON_MODE_FILL;
		retval.lineWidth = 1.0f;
		retval.cullMode = VK_CULL_MODE_NONE;
		retval.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		retval.depthClampEnable = VK_FALSE;
		retval.depthBiasEnable = VK_TRUE;
		retval.depthBiasClamp = 0.0f;
		retval.depthBiasConstantFactor = 0.0f;
		retval.depthBiasSlopeFactor = 0.0f;
		return retval;
	}

	VkPipelineMultisampleStateCreateInfo CreateVkPipelineMultisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		retval.sampleShadingEnable = VK_FALSE;
		retval.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		retval.minSampleShading = 1.0f;
		retval.pSampleMask = VK_NULL_HANDLE;
		retval.alphaToCoverageEnable = VK_FALSE;
		retval.alphaToOneEnable = VK_FALSE;
		return retval;
	}

	VkPipelineDepthStencilStateCreateInfo CreateVkPipelineDepthStencilStateCreateInfo()
	{
		VkPipelineDepthStencilStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		retval.depthTestEnable = VK_TRUE;
		retval.depthWriteEnable = VK_TRUE;
		retval.depthCompareOp = VK_COMPARE_OP_GREATER;
		retval.depthBoundsTestEnable = VK_FALSE;
		retval.minDepthBounds = 1.0f;
		retval.maxDepthBounds = 0.0f;
		retval.stencilTestEnable = VK_FALSE;
		return retval;
	}

	VkPipelineColorBlendAttachmentState CreateVkPipelineColorBlendAttachmentState()
	{
		VkPipelineColorBlendAttachmentState retval = {};
		retval.colorWriteMask = 0xF;
		retval.blendEnable = VK_FALSE;
		retval.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		retval.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		retval.colorBlendOp = VK_BLEND_OP_ADD;
		retval.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		retval.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		retval.alphaBlendOp = VK_BLEND_OP_ADD;
		return retval;
	}

	VkPipelineColorBlendStateCreateInfo CreateVkPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState* attachments, unsigned int attachmentCount)
	{
		VkPipelineColorBlendStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		retval.logicOpEnable = VK_FALSE;
		retval.logicOp = VK_LOGIC_OP_COPY;
		retval.attachmentCount = attachmentCount;
		retval.pAttachments = attachments;
		retval.blendConstants[0] = 0.0f;
		retval.blendConstants[1] = 0.0f;
		retval.blendConstants[2] = 0.0f;
		retval.blendConstants[3] = 0.0f;
		return retval;
	}

	VkPipelineDynamicStateCreateInfo CreateVkPipelineDynamicStateCreateInfo(VkDynamicState* dynamicStates, unsigned int dynamicStateCount)
	{
		VkPipelineDynamicStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		retval.dynamicStateCount = dynamicStateCount;
		retval.pDynamicStates = dynamicStates;
		return retval;
	}

	void CreatePipelineLayout()
	{
		VkDescriptorSetLayoutBinding descriptor_set_layout_binding[3] = {};
		descriptor_set_layout_binding[0].binding = 0;
		descriptor_set_layout_binding[0].descriptorCount = 1;
		descriptor_set_layout_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_set_layout_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptor_set_layout_binding[0].pImmutableSamplers = nullptr;

		descriptor_set_layout_binding[1].binding = 1;
		descriptor_set_layout_binding[1].descriptorCount = 1;
		descriptor_set_layout_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_set_layout_binding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptor_set_layout_binding[1].pImmutableSamplers = nullptr;

		descriptor_set_layout_binding[2].binding = 2;
		descriptor_set_layout_binding[2].descriptorCount = 1;
		descriptor_set_layout_binding[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_set_layout_binding[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptor_set_layout_binding[2].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
		descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_layout_create_info.bindingCount = 3;
		descriptor_set_layout_create_info.pBindings = descriptor_set_layout_binding;
		
		vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &descriptorSetLayout);

		VkDescriptorSetLayoutBinding tex_descriptor_set_layout_binding = {};
		tex_descriptor_set_layout_binding.binding = 0;
		tex_descriptor_set_layout_binding.descriptorCount = texData.size();
		tex_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		tex_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		tex_descriptor_set_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo tex_descriptor_set_layout_create_info = {};
		tex_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		tex_descriptor_set_layout_create_info.bindingCount = 1;
		tex_descriptor_set_layout_create_info.pBindings = &tex_descriptor_set_layout_binding;
		
		VkDescriptorBindingFlagsEXT binding_flags_ext = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT tex_descriptor_set_layout_binding_flags = {};
		tex_descriptor_set_layout_binding_flags.bindingCount = 1;
		tex_descriptor_set_layout_binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		tex_descriptor_set_layout_binding_flags.pBindingFlags = &binding_flags_ext;

		tex_descriptor_set_layout_create_info.pNext = &tex_descriptor_set_layout_binding_flags;

		vkCreateDescriptorSetLayout(device, &tex_descriptor_set_layout_create_info, nullptr, &textureDescriptorSetLayout);

		VkDescriptorSetLayoutBinding cube_tex_descriptor_set_layout_binding = {};
		cube_tex_descriptor_set_layout_binding.binding = 0;
		cube_tex_descriptor_set_layout_binding.binding = 0;
		cube_tex_descriptor_set_layout_binding.descriptorCount = cubeTexData.size();
		cube_tex_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		cube_tex_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo cube_tex_descriptor_set_layout_create_info = {};
		cube_tex_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		cube_tex_descriptor_set_layout_create_info.bindingCount = 1;
		cube_tex_descriptor_set_layout_create_info.pBindings = &cube_tex_descriptor_set_layout_binding;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT cube_tex_descriptor_set_layout_binding_flags = {};
		cube_tex_descriptor_set_layout_binding_flags.bindingCount = 1;
		cube_tex_descriptor_set_layout_binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		cube_tex_descriptor_set_layout_binding_flags.pBindingFlags = &binding_flags_ext;

		cube_tex_descriptor_set_layout_create_info.pNext = &cube_tex_descriptor_set_layout_binding_flags;

		vkCreateDescriptorSetLayout(device, &cube_tex_descriptor_set_layout_create_info, nullptr, &cubeTextureDescriptorSetLayout);

		VkDescriptorSetLayout setLayouts[] = { descriptorSetLayout,textureDescriptorSetLayout, cubeTextureDescriptorSetLayout };

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.size = sizeof(PushConstants);
		pushConstantRange.offset = 0;
		
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 3; 
		pipeline_layout_create_info.pSetLayouts = setLayouts;
		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges = &pushConstantRange;

		vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipelineLayout);
	}

	void BindShutdownCallback()
	{
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
	}

	void QueueBloomCommand(unsigned int frame) {
		vkWaitForFences(device, 1, &bloomFences[frame], VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &bloomFences[frame]);
		vkResetCommandBuffer(bloomCommandBuffers[frame], 0);

		VkCommandBuffer commandBuffer = bloomCommandBuffers[frame];

		VkCommandBufferBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &begin);

		{
			VkClearValue clears[2]{};
			clears[0].color = { 0,0,0,1 };
			clears[1].depthStencil = { 0.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = offscreenRenderPass;
			renderPassBeginInfo.framebuffer = sceneHDR.framebuffer;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = { windowWidth, windowHeight };
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clears;

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = CreateViewportFromWindowDimensions();
			VkRect2D scissor = CreateScissorFromWindowDimensions();

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			VkDescriptorSet sceneDescriptorSets[] = { descriptorSets[frame], textureDescriptorSet, cubeTextureDescriptorSet };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, sceneDescriptorSets, 0, nullptr);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxOffscreenPipeline);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneOffscreenPipeline);

			

			VkBuffer buffers[] = { geometryBuffer.handle, geometryBuffer.handle, geometryBuffer.handle , geometryBuffer.handle };
			BindGeometryBuffers(commandBuffer);
			for (int i = 0; i < (int)primitivesToDraw.size(); i++) {
				VkDeviceSize offsets[] = { bufferByteOffsets[primitivesToDraw[i].instanceIndex][0], bufferByteOffsets[primitivesToDraw[i].instanceIndex][1], bufferByteOffsets[primitivesToDraw[i].instanceIndex][2] ,bufferByteOffsets[primitivesToDraw[i].instanceIndex][3] };
				vkCmdBindVertexBuffers(commandBuffer, 0, 4, buffers, offsets);
			
				vkCmdDrawIndexed(commandBuffer, primitivesToDraw[i].indexCount, 1, primitivesToDraw[i].firstIndex, primitivesToDraw[i].vertexOffset, primitivesToDraw[i].instanceIndex);
			}

			vkCmdEndRenderPass(commandBuffer);
		}

		const unsigned int halfWidth = (unsigned int)(windowWidth * 0.5f);
		const unsigned int halfHeight = (unsigned int)(windowHeight * 0.5f);

		{
			VkClearValue clear{};
			clear.color = { 0,0,0,1 };

			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = bloomRenderPass;
			renderPassBeginInfo.framebuffer = bloomVert.framebuffer;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = { halfWidth, halfHeight };
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clear;

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = CreateViewportFromWindowDimensions();
			viewport.width = (float)halfWidth;
			viewport.height = (float)halfHeight;

			VkRect2D scissor = CreateScissorFromWindowDimensions();
			scissor.extent = { halfWidth, halfHeight };
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomVertPipeline);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomBlurPipelineLayout, 0, 1, &bloomVertSets[frame], 0, nullptr);

			bloomPushConstants.direction = 0;
			bloomPushConstants.texelSize = { 1.0f / (float)windowWidth, 1.0f / (float)windowHeight };
			vkCmdPushConstants(commandBuffer, bloomBlurPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomPushConstants), &bloomPushConstants);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			vkCmdEndRenderPass(commandBuffer);
		}

		VkImageMemoryBarrier barrier = CreateImageMemoryBarrier(bloomVert.image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		{
			VkClearValue clear{};
			clear.color = { 0,0,0,1 };

			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = bloomRenderPass;
			renderPassBeginInfo.framebuffer = bloomHoriz.framebuffer;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = { halfWidth, halfHeight };
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clear;

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = CreateViewportFromWindowDimensions();
			viewport.width = (float)halfWidth;
			viewport.height = (float)halfHeight;

			VkRect2D scissor = CreateScissorFromWindowDimensions();
			scissor.extent = { halfWidth, halfHeight };
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomHorizPipeline);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomBlurPipelineLayout, 0, 1, &bloomHorizSets[frame], 0, nullptr);

			bloomPushConstants.direction = 1;
			bloomPushConstants.texelSize = { 1.0f / (float)halfWidth , 1.0f / (float)halfHeight };
			vkCmdPushConstants(commandBuffer, bloomBlurPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomPushConstants), &bloomPushConstants);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			vkCmdEndRenderPass(commandBuffer);
		}

		vkEndCommandBuffer(commandBuffer);
	}

	void SubmitBloomCommand(unsigned int frame) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &bloomCommandBuffers[frame];
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, bloomFences[frame]);
		vkWaitForFences(device, 1, &bloomFences[frame], VK_TRUE, UINT64_MAX);
	}

	void UpdateCamera() {

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration<float>(now - lastFrameTime).count();
		lastFrameTime = now;

		bool focused;
		win.IsFocus(focused);
		if (!focused) return;
		float x = 0.0f;
#if defined(_WIN32)
		x = 0;
		inputProxy.GetState(G_BUTTON_LEFT, x);

		if (!cursorCaptured && x > 0) {
			LockCursorToWindow();
			SetCursorPos(windowWidth * 0.5f, windowHeight * 0.5f);
			return;
		}
		inputProxy.GetState(G_KEY_ESCAPE, x);
		if (cursorCaptured && x > 0) {
			UnlockCursor();
			return;
		}
		if (cursorCaptured) {
			SetCursorPos(windowWidth * 0.5f, windowHeight * 0.5f);
		}
#endif	

		if (!cursorCaptured) {
			return;
		}




		GW::MATH::GMATRIXF viewMatrix = {};
		mathProxy.InverseF(sceneData.viewMatrix, viewMatrix);

		GW::MATH::GVECTORF translateVector = { 0,0,0,0 };
		DIRECTION_INPUT kInputs;
		DIRECTION_INPUT cInputs;

		float y = 0.0f;
		inputProxy.GetState(G_KEY_SPACE, kInputs.up);
		inputProxy.GetState(G_KEY_LEFTSHIFT, kInputs.down);
		controllerProxy.GetState(0, G_RIGHT_TRIGGER_AXIS, cInputs.up);
		controllerProxy.GetState(0, G_LEFT_TRIGGER_AXIS, cInputs.down);
		y = kInputs.up - kInputs.down + cInputs.up - cInputs.down;

		x = 0.0f;
		inputProxy.GetState(G_KEY_D, kInputs.right);
		inputProxy.GetState(G_KEY_A, kInputs.left);
		controllerProxy.GetState(0, G_LX_AXIS, cInputs.right);
		x = kInputs.right - kInputs.left + cInputs.right;

		float z = 0.0f;
		inputProxy.GetState(G_KEY_W, kInputs.fwd);
		inputProxy.GetState(G_KEY_S, kInputs.back);
		controllerProxy.GetState(0, G_LY_AXIS, cInputs.fwd);
		z = kInputs.fwd - kInputs.back + cInputs.fwd;

		viewMatrix.row4.y += y * config.cameraSpeed * dt;

		translateVector.x = x * dt * config.cameraSpeed;
		translateVector.z = z * dt * config.cameraSpeed;
		translateVector.y = 0;



		mathProxy.TranslateLocalF(viewMatrix, translateVector, viewMatrix);
		unsigned int windowHeight, windowWidth;
		win.GetClientHeight(windowHeight);

		x = 0, y = 0;
		if (inputProxy.GetMouseDelta(x, y) != GW::GReturn::SUCCESS || !focused) {
			x = 0, y = 0;
		}

		controllerProxy.GetState(0, G_RY_AXIS, cInputs.up);
		float thumbSpeed = G_PI_F * dt;
		float pitchDelta = (dt * config.lookSens * G_DEGREE_TO_RADIAN(config.fovDegrees) * y / windowHeight) + cInputs.up * -thumbSpeed;
		float newPitch = pitchDelta + cumulPitch;
		float pitchLimit = G_PI_F * 0.5f;
		if (newPitch > pitchLimit) newPitch = pitchLimit;
		if (newPitch < -pitchLimit) newPitch = -pitchLimit;
		float totalPitch = newPitch - cumulPitch;
		cumulPitch = newPitch;
		GW::MATH::GMATRIXF pitchMatrix;
		mathProxy.RotateXLocalF(GW::MATH::GIdentityMatrixF, totalPitch, pitchMatrix);
		mathProxy.MultiplyMatrixF(pitchMatrix, viewMatrix, viewMatrix);

		win.GetClientWidth(windowWidth);
		float aspectRatio = (windowWidth / (float)windowHeight);
		controllerProxy.GetState(0, G_RX_AXIS, cInputs.right);
		float totalYaw = G_DEGREE_TO_RADIAN(config.fovDegrees) * aspectRatio * config.lookSens * dt * x / windowWidth + cInputs.right * thumbSpeed;
		GW::MATH::GMATRIXF yawMatrix;
		GW::MATH::GVECTORF position = viewMatrix.row4;

		mathProxy.RotateYGlobalF(GW::MATH::GIdentityMatrixF, totalYaw, yawMatrix);
		mathProxy.MultiplyMatrixF(viewMatrix, yawMatrix, viewMatrix);
		viewMatrix.row4 = position;
		sceneData.cameraWorldPosition = float4{ position.x,position.y,position.z, 1 };
		//manipulation
		mathProxy.InverseF(viewMatrix, viewMatrix);
		sceneData.viewMatrix = viewMatrix;



	}


public:
	/*void Render(unsigned int currentFrame)
	{
		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer(currentFrame);

		SetUpSkyboxPipeline(commandBuffer);

		
		VkDescriptorSet sets[] = { descriptorSets[currentFrame], textureDescriptorSet, cubeTextureDescriptorSet };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, sets, 0, nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		SetUpPipeline(commandBuffer);
		VkBuffer buffers[] = { geometryHandle, geometryHandle,geometryHandle,geometryHandle };
		for (int i = 0; i < primitivesToDraw.size(); i++) {
			VkDeviceSize offsets[] = { bufferByteOffsets[primitivesToDraw[i].instanceIndex][0], bufferByteOffsets[primitivesToDraw[i].instanceIndex][1], bufferByteOffsets[primitivesToDraw[i].instanceIndex][2] ,bufferByteOffsets[primitivesToDraw[i].instanceIndex][3] };
			vkCmdBindVertexBuffers(commandBuffer, 0, 4, buffers, offsets);
			
			vkCmdDrawIndexed(commandBuffer, primitivesToDraw[i].indexCount, 1, primitivesToDraw[i].firstIndex, primitivesToDraw[i].vertexOffset, primitivesToDraw[i].instanceIndex);
		}
	}*/

	void RenderFrame();

	

	void LockCursorToWindow() {
#if defined(_WIN32) 
		bool captured = false;
		win.IsFocus(captured);
		if (!captured) return;
		GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE universalWindowHandle = {};
		win.GetWindowHandle(universalWindowHandle);
		HWND hwnd = static_cast<HWND>(universalWindowHandle.window);

		RECT client;
		GetClientRect(hwnd, &client);

		POINT ul{ client.left, client.top };
		POINT lr{ client.right, client.bottom };

		ClientToScreen(hwnd, &ul);
		ClientToScreen(hwnd, &lr);

		RECT clip{ ul.x, ul.y, lr.x, lr.y };
		ClipCursor(&clip);
		cursorCaptured = true;
		while (ShowCursor(FALSE) >= 0);
#endif

	}

	void UnlockCursor() {
#if defined(_WIN32)
		bool captured = false;
		win.IsFocus(captured);
		if (!captured) return;
		ClipCursor(nullptr);
		cursorCaptured = false;
		while (ShowCursor(TRUE) < 0);
#endif
	}

	


	

private:

	void Render(unsigned int currentFrame) {
		UpdateWindowDimensions();

		QueueBloomCommand(currentFrame);
		SubmitBloomCommand(currentFrame);

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer(currentFrame);

		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomCompositePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomCompositePipelineLayout, 0, 1, &bloomCompositeSets[currentFrame], 0, nullptr);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

	void SubmitShadowPass(unsigned int frame) {
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkCommandBuffer shadowCommandBuffer = shadowCommandBuffers[frame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &shadowCommandBuffer;
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);
	}

	void RenderShadowPass(unsigned int frame) {
		VkCommandBuffer commandBuffer = shadowCommandBuffers[frame];
		vkResetCommandBuffer(commandBuffer, 0);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkClearValue clearValue = {};
		clearValue.depthStencil = { 0.0f, 1 };
		VkRenderPassBeginInfo renderPassBegin = {};
		renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBegin.renderPass = shadowRenderPass;
		renderPassBegin.renderArea.offset = { 0,0 };
		renderPassBegin.renderArea.extent = { shadowMapSize, shadowMapSize };
		renderPassBegin.clearValueCount = 1;
		renderPassBegin.pClearValues = &clearValue;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)shadowMapSize;
		viewport.height = (float)shadowMapSize;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0,0 };
		scissor.extent = { shadowMapSize, shadowMapSize };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (int j = 0; j < SHADOW_CASCADE_COUNT; j++) {
			renderPassBegin.framebuffer = shadowCascades[j].frameBuffer;
			vkCmdBeginRenderPass(commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
			BindGeometryBuffers(commandBuffer);
			//vlk.GetSwapchainCurrentFrame(frame);
			VkDescriptorSet sets[] = { descriptorSets[frame], textureDescriptorSet, cubeTextureDescriptorSet };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, sets, 0, nullptr);
			pc.cascadeIndex = j;
			VkBuffer buffers[] = { geometryBuffer.handle, geometryBuffer.handle,geometryBuffer.handle,geometryBuffer.handle };

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

			for (int i = 0; i < primitivesToDraw.size(); i++) {
				VkDeviceSize offsets[] = { bufferByteOffsets[primitivesToDraw[i].instanceIndex][0], bufferByteOffsets[primitivesToDraw[i].instanceIndex][1], bufferByteOffsets[primitivesToDraw[i].instanceIndex][2] ,bufferByteOffsets[primitivesToDraw[i].instanceIndex][3] };
				vkCmdBindVertexBuffers(commandBuffer, 0, 4, buffers, offsets);

				vkCmdDrawIndexed(commandBuffer, primitivesToDraw[i].indexCount, 1, primitivesToDraw[i].firstIndex, primitivesToDraw[i].vertexOffset, primitivesToDraw[i].instanceIndex);
			}
			vkCmdEndRenderPass(commandBuffer);
		}
		vkEndCommandBuffer(commandBuffer);
	}

	void UpdateSceneData(unsigned int currentFrame) {
		RotateSun(2.0f);
		UpdateCascades();
		/*for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
			mathProxy.MultiplyMatrixF(sceneData.projectionMatrix, sceneData.viewMatrix, sceneData.lightViewProjectionMatrices[i]);
		}*/

		GvkHelper::write_to_buffer(device, uniformBuffers[currentFrame].memory, &sceneData, sizeof(sceneData));


		GvkHelper::write_to_buffer(device, storageBuffers[currentFrame].memory, objectData.data(), sizeof(INSTANCE_DATA) * objectData.size());

		GvkHelper::write_to_buffer(device, materialStorageBuffers[currentFrame].memory, materialData.data(), sizeof(Material) * materialData.size());

	}

	void UpdateCascades() {
		float cascadeSplits[SHADOW_CASCADE_COUNT];

		float splitLambda = 0.7f;
		float nearClip = config.nearPlane;
		float farClip = __min(config.farPlane, config.shadowDistance);
		float clipRange = farClip - nearClip;
		float minZ = nearClip;
		float maxZ = nearClip + clipRange;
		float range = maxZ - minZ;
		float ratio = maxZ / minZ;
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
			float p = (i + 1) / static_cast<float>(SHADOW_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = splitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		float lastSplitDistance = 0.0f;
		for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
			float splitDistance = cascadeSplits[i];

			float splitDistanceN = splitDistance;
			float lastSplitN = lastSplitDistance;
			float overlapWorld = 0.75f;
			float overlapN = overlapWorld / clipRange;
			if (i > 0) lastSplitN = __max(0.0f, lastSplitN - overlapN);
			splitDistanceN = __min(1.0f, splitDistanceN + overlapN);

			GW::MATH::GVECTORF frustrumCorners[8] = {
				{-1, 1, 1, 1}, {1, 1, 1, 1}, {1,-1,1,1}, {-1,-1,1,1}, {-1,1,0,1}, {1,1,0,1}, {1,-1,0,1}, {-1,-1,0,1}
			};
			GW::MATH::GMATRIXF viewPerspective = {};
			GW::MATH::GMATRIXF flip = GW::MATH::GIdentityMatrixF;
			flip.data[10] = -1.0f;
			GW::MATH::GMATRIXF shadowProjection;
			float aspectRatio = 0.0f;
			vlk.GetAspectRatio(aspectRatio);
			float fov = G_DEGREE_TO_RADIAN(config.fovDegrees);
			mathProxy.ProjectionVulkanLHF(fov, aspectRatio, farClip, nearClip, shadowProjection);
			mathProxy.MultiplyMatrixF(sceneData.viewMatrix, shadowProjection, viewPerspective);
			//mathProxy.MultiplyMatrixF(viewPerspective, flip, viewPerspective);
			mathProxy.InverseF(viewPerspective, viewPerspective);
			//mathProxy.TransposeF(viewPerspective, viewPerspective);
			for (int j = 0; j < 8; j++) {
				GW::MATH::GVECTORF inverseCorner = {};
				mathProxy.VectorXMatrixF(viewPerspective, frustrumCorners[j], inverseCorner);
				inverseCorner.x /= inverseCorner.w;
				inverseCorner.y /= inverseCorner.w;
				inverseCorner.z /= inverseCorner.w;
				inverseCorner.w = 1;
				frustrumCorners[j] = inverseCorner;
			}
			/*for (int j = 0; j < 8; j++) {
				frustrumCorners[j].z *= -1.0f;
			}*/
			for (int j = 0; j < 4; j++) {
				GW::MATH::GVECTORF distance = { frustrumCorners[j + 4].x - frustrumCorners[j].x, frustrumCorners[j + 4].y - frustrumCorners[j].y, frustrumCorners[j + 4].z - frustrumCorners[j].z };
				GW::MATH::GVECTORF distanceSplitDistance = { distance.x * splitDistanceN, distance.y * splitDistanceN, distance.z * splitDistanceN };
				frustrumCorners[j + 4] = { frustrumCorners[j].x + distanceSplitDistance.x, frustrumCorners[j].y + distanceSplitDistance.y, frustrumCorners[j].z + distanceSplitDistance.z, 1 };
				GW::MATH::GVECTORF distanceLastSplitDistance = { distance.x * lastSplitN, distance.y * lastSplitN, distance.z * lastSplitN };
				frustrumCorners[j] = { frustrumCorners[j].x + distanceLastSplitDistance.x, frustrumCorners[j].y + distanceLastSplitDistance.y, frustrumCorners[j].z + distanceLastSplitDistance.z, 1 };
			}
			
			GW::MATH::GVECTORF frustCenter = { 0,0,0,0 };
			for (int j = 0; j < 8; j++) {
				frustCenter = { frustCenter.x + frustrumCorners[j].x, frustCenter.y + frustrumCorners[j].y, frustCenter.z + frustrumCorners[j].z };
			}
			frustCenter = { frustCenter.x / 8.0f, frustCenter.y / 8.0f, frustCenter.z / 8.0f };
			frustCenter.w = 1;
			float radius = 0.0f;
			for (int j = 0; j < 8; j++) {
				GW::MATH::GVECTORF distance = { frustrumCorners[j].x - frustCenter.x, frustrumCorners[j].y - frustCenter.y, frustrumCorners[j].z - frustCenter.z };
				float magnitude;
				GetMagnitudeV3(distance, magnitude);
				radius = __max(radius, magnitude);
			}
			//radius = std::ceil(radius * 16.0f) / 16.0f;

			GW::MATH::GVECTORF maxExtents = { radius, radius, radius };
			GW::MATH::GVECTORF minExtents = { -radius, -radius, -radius };

			GW::MATH::GVECTORF lightDirection = { sceneData.lightDirection.x, sceneData.lightDirection.y, sceneData.lightDirection.z };
			NormalizeV3(lightDirection, lightDirection);

			//lightDirection.z *= -1.0f;

			GW::MATH::GMATRIXF lightViewMatrix = GW::MATH::GIdentityMatrixF;
			GW::MATH::GVECTORF eye = { frustCenter.x - lightDirection.x * radius, frustCenter.y - lightDirection.y * radius, frustCenter.z - lightDirection.z * radius, 1 };
			GW::MATH::GVECTORF target = { frustCenter.x,frustCenter.y,frustCenter.z,1 };
			GW::MATH::GVECTORF up = { 0,1,0,0 };
			mathProxy.LookAtLHF(eye, target, up, lightViewMatrix);
			float3 minLightSpace = { FLT_MAX, FLT_MAX, FLT_MAX };
			float3 maxLightSpace = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
			GW::MATH::GMATRIXF lightViewTranspose = {};
			//mathProxy.TransposeF(lightViewMatrix, lightViewTranspose);
			for (int j = 0; j < 8; j++) {
				
				GW::MATH::GVECTORF cornerLightSpace = {};
				mathProxy.VectorXMatrixF(lightViewMatrix, frustrumCorners[j], cornerLightSpace);
				/*if (j == 0) std::cout << "Corner light space: " << cornerLightSpace.x << "," << cornerLightSpace.y << "," << cornerLightSpace.z << std::endl;*/
				minLightSpace.x = __min(minLightSpace.x, cornerLightSpace.x);
				maxLightSpace.x = __max(maxLightSpace.x, cornerLightSpace.x);
				minLightSpace.y = __min(minLightSpace.y, cornerLightSpace.y);
				maxLightSpace.y = __max(maxLightSpace.y, cornerLightSpace.y);
				minLightSpace.z = __min(minLightSpace.z, cornerLightSpace.z);
				maxLightSpace.z = __max(maxLightSpace.z, cornerLightSpace.z);
			}
			GW::MATH::GVECTORF centerLightSpace = {};
			mathProxy.VectorXMatrixF(lightViewMatrix, frustCenter, centerLightSpace);
			
			radius = 0.0f;
			float minZ = FLT_MAX;
			float maxZ = -FLT_MAX;

			for (int j = 0; j < 8; ++j) {
				GW::MATH::GVECTORF centered = {};
				mathProxy.VectorXMatrixF(lightViewMatrix, frustrumCorners[j], centered);
				float2 d = { centered.x - centerLightSpace.x , centered.y - centerLightSpace.y };
				radius = __max(radius, sqrtf(d.x * d.x + d.y * d.y));
				minZ = __min(minZ, centered.z);
				maxZ = __max(maxZ, centered.z);
			}

			//radius = std::ceilf(radius * 16.0f) / 16.0f;

			const float pcfScale = 0.75f;
			const float pcfRange = 1.0f; 

			float worldUnitsPerTexel = (2.0f * radius) / static_cast<float>(shadowMapSize);
			float paddingTexels = (pcfRange * pcfScale) + 2.0f;
			float pcfPadding = worldUnitsPerTexel * paddingTexels;

			radius += pcfPadding;

			worldUnitsPerTexel = (2.0f * radius) / static_cast<float>(shadowMapSize);
			radius = std::ceil(radius / worldUnitsPerTexel) * worldUnitsPerTexel;
			worldUnitsPerTexel = (2.0f * radius) / static_cast<float>(shadowMapSize);
			centerLightSpace.x = roundf(centerLightSpace.x / worldUnitsPerTexel) * worldUnitsPerTexel;
			centerLightSpace.y = roundf(centerLightSpace.y / worldUnitsPerTexel) * worldUnitsPerTexel;

			float minX = centerLightSpace.x - radius;
			float maxX = centerLightSpace.x + radius;
			float minY = centerLightSpace.y - radius;
			float maxY = centerLightSpace.y + radius;

			minZ -= radius * 2.5f;
			maxZ += radius * 2.5f;
			GW::MATH::GMATRIXF lightProjectionMatrix = {};
			sceneData.cascadeDepthRanges.xyzw[i] = maxZ - minZ;
			sceneData.cascadeWorldUnitsPerTexel.xyzw[i] = worldUnitsPerTexel;
			CreateOrthoProjection(minX, maxX, minY, maxY, minZ, maxZ, lightProjectionMatrix);
			shadowCascades[i].splitDepth = (config.nearPlane + splitDistance * clipRange);
			mathProxy.MultiplyMatrixF(lightViewMatrix, lightProjectionMatrix, shadowCascades[i].viewProjectionMatrix);
			//mathProxy.TransposeF(shadowCascades[i].viewProjectionMatrix, shadowCascades[i].viewProjectionMatrix);
			sceneData.lightViewProjectionMatrices[i] = shadowCascades[i].viewProjectionMatrix;
			sceneData.cascadeSplits.xyzw[i] = shadowCascades[i].splitDepth;

			lastSplitDistance = cascadeSplits[i];
			
			/*std::cout << "Center Light Space: " << centerLightSpace.x << "," << centerLightSpace.y << "," << centerLightSpace.z << std::endl;
			std::cout << "Min light space: " << minLightSpace.x << "," << minLightSpace.y << "," << minLightSpace.z << " | Max light space: " << maxLightSpace.x << "," << maxLightSpace.y << "," << maxLightSpace.z << std::endl;*/
		}
		/*for (int i = 0; i < 4; i++) {
			std::cout << "Cascade split " << i << ": " << shadowCascades[i].splitDepth << std::endl;
		}*/
	}

	void CreateOrthoProjection(float left, float right, float bottom, float top, float n, float f, GW::MATH::GMATRIXF& outMatrix) {
		float invW = 1.0f / (right - left);
		float invH = 1.0f / (top - bottom);
		float invD = 1.0f / (f - n);
		outMatrix = {
			2.0f * invW, 0, 0, 0,
			0, -2.0f * invH, 0, 0,
			0,0, -invD, 0,
			-(right + left) *invW, -(bottom + top) * invH, f * invD, 1
		};
	}

	void NormalizeV3(GW::MATH::GVECTORF vector, GW::MATH::GVECTORF& outVector) {
		float magnitude;
		GetMagnitudeV3(vector, magnitude);
		outVector = { vector.x / magnitude, vector.y / magnitude, vector.z / magnitude };
	}

	void GetMagnitudeV3(GW::MATH::GVECTORF vector, float& outFloat) {
		outFloat = sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
	}

	void GetMagnitudeV4(GW::MATH::GVECTORF vector, float& outFloat) {
		outFloat = sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w*vector.w);
	}

	void MultiplyMatrixByVector(GW::MATH::GMATRIXF matrix, GW::MATH::GVECTORF vector, GW::MATH::GVECTORF& outVector) {
		outVector = {
			matrix.row1.x * vector.x + matrix.row1.y * vector.y + matrix.row1.z * vector.z + matrix.row1.w * vector.w,
			matrix.row2.x * vector.x + matrix.row2.y * vector.y + matrix.row2.z * vector.z + matrix.row2.w * vector.w,
			matrix.row3.x * vector.x + matrix.row3.y * vector.y + matrix.row3.z * vector.z + matrix.row3.w * vector.w,
			matrix.row4.x * vector.x + matrix.row4.y * vector.y + matrix.row4.z * vector.z + matrix.row4.w * vector.w
		};
	}

	void RotateSun(float degPerSecond) {
		float timeElapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

		GW::MATH::GMATRIXF rotationMatrix = GW::MATH::GIdentityMatrixF;
		mathProxy.RotateYGlobalF(rotationMatrix, G_DEGREE_TO_RADIAN(degPerSecond) * timeElapsed, rotationMatrix);
		sceneData.skyboxRot = G_DEGREE_TO_RADIAN(degPerSecond) * timeElapsed;
		GW::MATH::GVECTORF sunDirection = { originalSunDirection.x,originalSunDirection.y,originalSunDirection.z,originalSunDirection.w };
		mathProxy.VectorXMatrixF(rotationMatrix, sunDirection, sunDirection);
		sceneData.lightDirection = { sunDirection.x, sunDirection.y, sunDirection.z, sunDirection.w };
	}

	VkCommandBuffer GetCurrentCommandBuffer(unsigned int currentFrame)
	{

		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentFrame, (void**)&commandBuffer);
		return commandBuffer;
	}

	void SetUpPipeline(VkCommandBuffer& commandBuffer)
	{
		UpdateWindowDimensions();
		SetViewport(commandBuffer);
		SetScissor(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		BindGeometryBuffers(commandBuffer);
	}

	void SetUpSkyboxPipeline(VkCommandBuffer& commandBuffer)
	{
		UpdateWindowDimensions();
		SetViewport(commandBuffer);
		SetScissor(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);	
	}

	void SetViewport(const VkCommandBuffer& commandBuffer)
	{
		VkViewport viewport = CreateViewportFromWindowDimensions();
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	void SetScissor(const VkCommandBuffer& commandBuffer)
	{
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void BindGeometryBuffers(VkCommandBuffer& commandBuffer)
	{
		VkDeviceSize offsets[] = { bufferByteOffsets[0][0], bufferByteOffsets[0][1], bufferByteOffsets[0][2], bufferByteOffsets[0][3]};
		VkBuffer buffers[] = { geometryBuffer.handle, geometryBuffer.handle,geometryBuffer.handle,geometryBuffer.handle };
		vkCmdBindIndexBuffer(commandBuffer, geometryBuffer.handle, 0, indexType );
		vkCmdBindVertexBuffers(commandBuffer, 0, 4, buffers, offsets);
		
	}

	void DestroyBuffer(std::vector<VkBuffer>& buffer) {
		for (int i = 0; i < buffer.size(); i++) {
			vkDestroyBuffer(device, buffer[i], nullptr);
		}
	}

	void FreeMemory(std::vector<VkDeviceMemory>& data) {
		for (int i = 0; i < data.size(); i++) {
			vkFreeMemory(device, data[i], nullptr);
		}
	}

	void FreeTextureData(std::vector<TextureData>& data) {
		for (auto& texture : data) {
			if (texture.ownsBuffer) {
				if (texture.buffer != VK_NULL_HANDLE) {
					vkDestroyBuffer(device, texture.buffer, nullptr);
					texture.buffer = VK_NULL_HANDLE;
				}
				if (texture.bufferMemory != VK_NULL_HANDLE) {
					vkFreeMemory(device, texture.bufferMemory, nullptr);
					texture.bufferMemory = VK_NULL_HANDLE;
				}
			}
			if (texture.ownsImage)
			{
				texture.image.Destroy(device);
			}
		}
	}

	void DestroySamplers(std::vector<VkSampler>& data) {
		for (int i = 0; i < data.size(); i++) {
			vkDestroySampler(device, data[i], nullptr);
		}
	}

	//Cleanup callback function (passed to VKSurface, will be called when the pipeline shuts down)
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);

		vkDestroyPipeline(device, shadowPipeline, nullptr);
		vkDestroyPipeline(device, sceneOffscreenPipeline, nullptr);
		vkDestroyPipeline(device, skyboxOffscreenPipeline, nullptr);
		vkDestroyPipeline(device, bloomHorizPipeline, nullptr);
		vkDestroyPipeline(device, bloomVertPipeline, nullptr);
		vkDestroyPipeline(device, bloomCompositePipeline, nullptr);
		vkDestroyPipelineLayout(device, bloomCompositePipelineLayout, nullptr);
		vkDestroyPipelineLayout(device, bloomBlurPipelineLayout, nullptr);

		for (auto& cascade : shadowCascades) {
			cascade.Destroy(device);
		}

		for (int i = 0; i < bloomFences.size(); i++) {
			vkDestroyFence(device, bloomFences[i], nullptr);
		}
		
		vkDestroyRenderPass(device, shadowRenderPass, nullptr);
		vkDestroyRenderPass(device, offscreenRenderPass, nullptr);
		vkDestroyRenderPass(device, bloomRenderPass, nullptr);

		// Release allocated buffers, shaders & pipeline
		geometryBuffer.Destroy(device);
		for (auto& buffer : uniformBuffers) {
			buffer.Destroy(device);
		}
		for (auto& buffer : storageBuffers) {
			buffer.Destroy(device);
		}
		for (auto& buffer : materialStorageBuffers) {
			buffer.Destroy(device);
		}
		
		
		vkDestroyCommandPool(device, shadowCommandPool, nullptr);
		vkDestroyCommandPool(device, bloomCommandPool, nullptr);
		
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, fragmentShader, nullptr);
		vkDestroyShaderModule(device, shadowVertexShader, nullptr);
		vkDestroyShaderModule(device, shadowFragmentShader, nullptr);
		vkDestroyShaderModule(device, skyboxFragmentShader, nullptr);
		vkDestroyShaderModule(device, skyboxVertexShader, nullptr);
		vkDestroyShaderModule(device, fullscreenVertShader, nullptr);
		vkDestroyShaderModule(device, bloomBlurShader, nullptr);
		vkDestroyShaderModule(device, bloomCompositeShader, nullptr);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipeline(device, skyboxPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, textureDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, cubeTextureDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(device, bloomBlurDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, bloomCompositeDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, bloomDescriptorPool, nullptr);
		
		FreeTextureData(texData);
		FreeTextureData(cubeTexData);
		DestroySamplers(samplers);

		sceneHDR.Destroy(device);
		sceneDepth.Destroy(device);
		bloomHoriz.Destroy(device);
		bloomVert.Destroy(device);

		alive = false;
	}
};
