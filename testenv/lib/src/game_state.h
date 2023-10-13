
#pragma once
#include <core/application.h>
#include <core/frame_data.h>
#include <math/frustum.h>
#include <resources/debug/debug_line_3d.h>
#include <resources/mesh.h>
#include <resources/ui_text.h>
#include <systems/events/event_system.h>

#include "editor/editor_gizmo.h"
#include "resources/scenes/simple_scene.h"

namespace C3D
{
    class Camera;
    class PointLight;
}  // namespace C3D

enum class ReloadState
{
    Done,
    Unloading,
    Unloaded,
    Loading
};

struct GameState final : C3D::ApplicationState
{
    C3D::Camera* camera = nullptr;

    C3D::Frustum cameraFrustum;

    ReloadState reloadState = ReloadState::Done;

    // TEMP
    SimpleScene simpleScene;
    EditorGizmo gizmo;

    C3D::DynamicArray<C3D::DebugLine3D> testLines;
    C3D::DynamicArray<C3D::DebugBox3D> testBoxes;

    C3D::PointLight* pLights[4];

    C3D::UIMesh uiMeshes[10];
    C3D::UIText testText;

    u32 hoveredObjectId = INVALID_ID;

    C3D::DynamicArray<C3D::RegisteredEventCallback> registeredCallbacks;
    // TEMP
};

struct GameFrameData final : C3D::ApplicationFrameData
{
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> worldGeometries;
};