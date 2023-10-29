
#pragma once
#include <core/defines.h>

enum TestEnvPacketViews
{
    TEST_ENV_VIEW_WORLD        = 0,
    TEST_ENV_VIEW_EDITOR_WORLD = 1,
    TEST_ENV_VIEW_UI           = 2,
    TEST_ENV_VIEW_PICK         = 3,
};

struct DebugColorShaderLocations
{
    u16 projection;
    u16 view;
    u16 model;
};

struct SkyboxShaderLocations
{
    u16 projection;
    u16 view;
    u16 cubeMap;
};