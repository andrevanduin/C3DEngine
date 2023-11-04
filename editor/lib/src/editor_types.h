
#pragma once

enum EditorPacketViews
{
    EDITOR_VIEW_WORLD        = 0,
    EDITOR_VIEW_EDITOR_WORLD = 1,
    EDITOR_VIEW_WIREFRAME    = 2,
    EDITOR_VIEW_UI           = 3,
    EDITOR_VIEW_PICK         = 4,
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