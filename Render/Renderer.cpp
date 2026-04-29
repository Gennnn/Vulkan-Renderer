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

void Renderer::RenderFrame() {
	UpdateCamera();

	unsigned int frame = 0;
	vlk.GetSwapchainCurrentFrame(frame);

	UpdateSceneData(frame);

	if (config.enableShadows) {
		RenderShadowPass(frame);
		SubmitShadowPass(frame);
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

	if (+vlk.StartFrame(2, clearValues)) {
		Render(frame);
		vlk.EndFrame(true);
	}
}