#include "App/Application.h"
#include "App/AppConfig.h"
#include "Render/RenderConfig.h"

// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

// lets pop a window and use Vulkan to clear to a red screen
int main()
{
	AppConfig appConfig;
	appConfig.windowWidth = 1024;
	appConfig.windowHeight = 768;
	appConfig.windowTitle = "Vulkan Renderer";

	RendererConfig rendererConfig;
	rendererConfig.startupScenePath = "../Sponza/glTF/Sponza.gltf";

	rendererConfig.brdfLutPath = "../Models/Textures/lut_ggx.png";
	rendererConfig.diffuseIrradiancePath = "../Models/Textures/diffuse.ktx2";
	rendererConfig.specularPrefilterPath = "../Models/Textures/specular.ktx2";
	rendererConfig.skyboxPath = "../Models/Textures/skybox_rgba8.ktx2";

	rendererConfig.fovDegrees = 65.0f;
	rendererConfig.nearPlane = 0.1f;
	rendererConfig.farPlane = 10000.0f;

	rendererConfig.shadowMapSize = 2048;
	rendererConfig.shadowDistance = 80.0f;

	rendererConfig.enableBloom = true;
	rendererConfig.enableShadows = true;

	Application app;

	if (!app.Initialize(appConfig, rendererConfig))
		return -1;

	app.Run();
	app.Shutdown();

	return 0;
}
