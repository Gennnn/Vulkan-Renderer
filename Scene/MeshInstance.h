#pragma once

#include <cstdint>
#include "Transform.h"
#include "SceneTypes.h"

struct MeshInstance {
	MeshID meshId = InvalidSceneIndex;
	Transform transform;

	MaterialID materialOverride = InvalidSceneIndex;
};