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
	bool CreateAppWindow(const AppConfig& appConfig);
	bool CreateVulkanSurface();
private:
	AppConfig appConfig;
	RendererConfig rendererConfig;

	GW::SYSTEM::GWindow window;
	GW::CORE::GEventResponder windowEvents;
	GW::GRAPHICS::GVulkanSurface vulkan;

	Renderer* renderer = nullptr;
};