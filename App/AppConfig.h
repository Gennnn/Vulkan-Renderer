#pragma once
#include <cstdint>

struct AppConfig {
	uint32_t windowX = 0;
	uint32_t windowY = 0;
	uint32_t windowWidth = 1280;
	uint32_t windowHeight = 720;
	const char* windowTitle = "Vulkan Renderer";
	bool windowedBordered = true;
};