#pragma once

#include "GatewareConfig.h"

struct DirectionalLight
{
    GW::MATH::GVECTORF direction = { -0.577f, -0.577f, -0.577f, 0.0f };
    GW::MATH::GVECTORF color = { 1.0f, 0.95f, 0.85f, 1.0f };
    GW::MATH::GVECTORF ambient = { 0.1f, 0.1f, 0.12f, 1.0f };

    float intensity = 1.0f;
};