#pragma once
#include "../Core/GatewareConfig.h"
#include "RenderConfig.h"
#include "AppConfig.h"
#include "../Render/Renderer.h"
class Application {
public:
	bool Initialize(const AppConfig& appConfig, const RendererConfig& rendererConfig);
	void Run();
	void Shutdown();
private:
	bool LoadStartupScene(const AppConfig& appConfig, const RendererConfig& rendererConfig);
	void ApplyyEnvironmentToScene(const RendererConfig& rendererConfig);
	bool CreateAppWindow(const AppConfig& appConfig);
	bool CreateVulkanSurface();
private:
	AppConfig appConfig;
	RendererConfig rendererConfig;

	GW::SYSTEM::GWindow window;
	GW::CORE::GEventResponder windowEvents;
	GW::GRAPHICS::GVulkanSurface vulkan;

	ImportedScene importedScene;
	Scene scene;

	GltfImporter importer;
	SceneLoader sceneLoader;

	Renderer* renderer = nullptr;
};