#pragma once

#include "GatewareConfig.h"

struct Transform {
	GW::MATH::GMATRIXF world = GW::MATH::GIdentityMatrixF;
	
	GW::MATH::GVECTORF position = { 0.0f, 0.0f, 0.0f, 1.0f };
	GW::MATH::GVECTORF rotationEuler = { 0, 0, 0, 0 };
	GW::MATH::GVECTORF scale = { 1, 1, 1, 0 };
};