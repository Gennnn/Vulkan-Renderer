#pragma once
#include "../Core/GatewareConfig.h"
#include "RenderConfig.h"
#include "AppConfig.h"
#include "../Render/Renderer.h"
#include "FrameTimer.h"
#include "CameraController.h"
#include "../Platform/InputSystem.h"

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

	InputSystem input;
	CameraController cameraController;
	FrameTimer frameTimer;

	Renderer* renderer = nullptr;
};