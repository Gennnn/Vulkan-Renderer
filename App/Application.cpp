#include "Application.h"

using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

bool Application::Initialize(const AppConfig& _appConfig, const RendererConfig& _rendererConfig) {
	appConfig = _appConfig;
	rendererConfig = _rendererConfig;

	if (!CreateAppWindow(appConfig)) return false;
	if (!CreateVulkanSurface()) return false;

	renderer = new Renderer(window, vulkan, rendererConfig);

	if (!LoadStartupScene(appConfig, rendererConfig)) return false;

	renderer->SetScene(&scene);

	if (!renderer->InitializeGraphicsForScene(scene)) return false;

	if (!renderer || !renderer->alive) {
		std::cout << "Renderer failed to initialize." << std::endl;
		return false;
	}

	return true;
}

bool Application::LoadStartupScene(const AppConfig& _appConfig, const RendererConfig& _rendererConfig) {
	SceneLoaderInfo loaderInfo;

	loaderInfo.defaultFOV = _rendererConfig.fovDegrees;
	loaderInfo.defaultNear = _rendererConfig.nearPlane;
	loaderInfo.defaultFar = _rendererConfig.farPlane;
	loaderInfo.defaultAspect = static_cast<float>(_appConfig.windowWidth) / static_cast<float>(_appConfig.windowHeight);

	importedScene = importer.Load(_rendererConfig.startupScenePath);

	scene = sceneLoader.BuildSceneFromImported(importedScene, loaderInfo);

	ApplyyEnvironmentToScene(_rendererConfig);

	return true;
}

static TextureID AddSceneTexture(Scene& scene, const std::string& path, bool isCubeMap, bool isSrgb);

void Application::ApplyyEnvironmentToScene(const RendererConfig& rendererConfig) {
	scene.Environment().brdfLut = AddSceneTexture(scene, rendererConfig.brdfLutPath, false, false);
	scene.Environment().diffuseIrradiance = AddSceneTexture(scene, rendererConfig.diffuseIrradiancePath, true, false);
	scene.Environment().specularPrefilter = AddSceneTexture(scene, rendererConfig.specularPrefilterPath, true, false);
	scene.Environment().skybox = AddSceneTexture(scene, rendererConfig.skyboxPath, true, false);
}

bool Application::CreateAppWindow(const AppConfig& config) {
	GWindowStyle style = config.windowedBordered ? GWindowStyle::WINDOWEDBORDERED : GWindowStyle::WINDOWEDBORDERLESS;

	if (!+ window.Create(config.windowX, config.windowY, config.windowWidth, config.windowHeight, style)) {
		std::cout << "Failed to create window." << std::endl;
		return false;
	}

	window.SetWindowName(config.windowTitle);
	windowEvents.Create([&](const GW::GEvent& e)
		{
			GW::SYSTEM::GWindow::Events eventType;
			if (+e.Read(eventType)) {
				if (eventType == GWindow::Events::RESIZE) {

				}
			}
		});
	window.Register(windowEvents);
	return true;
}

bool Application::CreateVulkanSurface() {
#ifndef NDEBUG
	const char* debugLayers[] = { "VK_LAYER_KHRONOS_validation" };

	const char* setting_debug_action[] = { "VK_DBG_LAYER_ACTION_LOG_MSG", "VK_DBG_LAYER_ACTION_DEBUG_OUTPUT", /*"VK_DBG_LAYER_ACTION_BREAK"*/ };

	const VkLayerSettingEXT layerSettings = {
		debugLayers[0], "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT, static_cast<uint32_t>(std::size(setting_debug_action)), setting_debug_action
	};

	if (!+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT | GW::GRAPHICS::BINDLESS_SUPPORT, static_cast<unsigned int>(std::size(debugLayers)), debugLayers, 1, &layerSettings, 0, nullptr, 0, nullptr, false)) {
		std::cout << "Failed to create VLK surface." << std::endl;
		return false;
	}
#else
	if (!+ vulkan.Create(
		window,
		GW::GRAPHICS::DEPTH_BUFFER_SUPPORT | GW::GRAPHICS::BINDLESS_SUPPORT))
	{
		std::cout << "Failed to create Vulkan surface." << std::endl;
		return false;
	}
#endif
	return true;
}

void Application::Run()
{
	while (+window.ProcessWindowEvents() && renderer && renderer->alive)
	{
		renderer->RenderFrame();
	}
}

void Application::Shutdown()
{
	if (renderer)
	{
		delete renderer;
		renderer = nullptr;
	}
}

static TextureID AddSceneTexture(Scene& scene, const std::string& path, bool isCubeMap, bool isSrgb) {
	if (path.empty()) return InvalidSceneIndex;

	TextureID id = static_cast<TextureID>(scene.Textures().size());

	TextureAsset texture;
	texture.sourcePath = path;
	texture.isCubeMap = isCubeMap;
	texture.isSrgb = isSrgb;

	scene.Textures().push_back(texture);

	return id;
}