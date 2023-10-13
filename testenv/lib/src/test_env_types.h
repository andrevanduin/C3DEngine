
#pragma once
#include <core/defines.h>

enum TestEnvPacketViews
{
    TEST_ENV_VIEW_SKYBOX       = 0,
    TEST_ENV_VIEW_WORLD        = 1,
    TEST_ENV_VIEW_EDITOR_WORLD = 2,
    TEST_ENV_VIEW_UI           = 3,
    TEST_ENV_VIEW_PICK         = 4,
};

struct DebugColorShaderLocations
{
    u16 projection;
    u16 view;
    u16 model;
};