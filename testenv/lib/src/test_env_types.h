
#pragma once
#include <core/defines.h>
#include <resources/debug/debug_box_3d.h>

enum TestEnvPacketViews
{
    TEST_ENV_VIEW_WORLD        = 0,
    TEST_ENV_VIEW_EDITOR_WORLD = 1,
    TEST_ENV_VIEW_WIREFRAME    = 2,
    TEST_ENV_VIEW_UI           = 3,
    TEST_ENV_VIEW_PICK         = 4,
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

struct LightDebugData
{
    C3D::DebugBox3D box;
};