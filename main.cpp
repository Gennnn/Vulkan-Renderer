// Simple basecode showing how to create a window and attatch a vulkan surface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries

// Ignore some graphics libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this

#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT

#define KHRONOS_STATIC
// With what we want & what we don't defined we can include the API
#include "Gateware.h"
#include "TinyGLTF/tiny_gltf.h"
#include "Defines.h"
#include "ktx/include/ktxvulkan.h"
#include "Materials/TextureUtilsKTX.h"
#include <dxcapi.h>
#include <wrl/client.h>
#include "Render/Renderer.h"


// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

// lets pop a window and use Vulkan to clear to a red screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GVulkanSurface vulkan;

	if (+win.Create(0, 0, 1024, 768, GWindowStyle::WINDOWEDBORDERED))
	{
		// TODO: Part 1a
		VkClearValue clrAndDepth[2];
		clrAndDepth[0].color = { {0.2f, 0.0f, 0.45f, 1} }; 
		clrAndDepth[1].depthStencil = { 0.0f, 1u };
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			/*if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clrAndDepth[0].color.float32[2] += 0.01f; */
			});
		win.Register(msgs);
		win.SetWindowName("Ethan Kuchta - Lab 7 (Vulkan) - Part 4 Complete");

#ifndef NDEBUG
		const char* debugLayers[] = {
			"VK_LAYER_KHRONOS_validation", // standard validation layer
		};
		const char* setting_debug_action[] = { // debug actions for the validation layer
			"VK_DBG_LAYER_ACTION_LOG_MSG",
			"VK_DBG_LAYER_ACTION_DEBUG_OUTPUT",
			"VK_DBG_LAYER_ACTION_BREAK"
		};
		const VkLayerSettingEXT lyr_sett = { // extended settings for the validation layer
			debugLayers[0], "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT,
			static_cast<uint32_t>(std::size(setting_debug_action)), setting_debug_action
		};
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT | BINDLESS_SUPPORT, sizeof(debugLayers) / sizeof(debugLayers[0]),
			debugLayers, 1, &lyr_sett, 0, nullptr, 0, nullptr, false))
#else
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT | GW::GRAPHICS::BINDLESS_SUPPORT))
#endif
		{
			Renderer renderer(win, vulkan);
			/*auto best = FindBrightestTexel_KTX2_RGBA8Cubemap("../Models/Textures/skybox_rgba8.ktx2", true, 3);
			float u = (2.0f * (best.x + 0.5f) / 64) - 1.0f;
			float v = (2.0f * (best.y + 0.5f) / 64) - 1.0f;
			FaceUVToDir(best.face, u, v);
			auto mipView = GetCubemapMipPixelsRGBA8("../Models/Textures/skybox_rgba8.ktx2",3);
			AccumulateSunTintRing(mipView.facePixels[best.face], 64, 64, best.x, best.y, 2, 5, true);
			printf("Brightest: face=%d x=%d y=%d lum=%f rgbLin=(%f,%f,%f)\n",
				best.face, best.x, best.y, best.lum, best.r, best.g, best.b);*/
			while (+win.ProcessWindowEvents() && renderer.alive)
			{
				renderer.RenderFrame();
			}
		}
	}
	return 0; // that's all folks
}
