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
	RenderShadowPass(frame);
	SubmitShadowPass(frame);

	VkClearValue clrAndDepth[2];
	clrAndDepth[0].color = { 0.2f, 0.0f, 0.45f, 1.0f };
	clrAndDepth[1].depthStencil = { 0.0f, 1u };

	if (+vlk.StartFrame(2, clrAndDepth)) {
		Render(frame);
		vlk.EndFrame(true);
	}
}