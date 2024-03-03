
#pragma once
#include <core/application.h>
#include <core/ecs/entity.h>
#include <core/frame_data.h>
#include <core/uuid.h>
#include <renderer/rendergraph/rendergraph.h>
#include <renderer/viewport.h>
#include <resources/debug/debug_line_3d.h>
#include <resources/mesh.h>
#include <resources/ui_text.h>
#include <systems/events/event_system.h>

#include "editor/editor_gizmo.h"
#include "passes/editor_pass.h"
#include "passes/scene_pass.h"
#include "passes/skybox_pass.h"
#include "resources/scenes/simple_scene.h"

namespace C3D
{
    class Camera;
    class PointLight;
    class Transform;
}  // namespace C3D

enum class ReloadState
{
    Done,
    Unloading,
    Unloaded,
    Loading
};

struct SelectedObject
{
    C3D::UUID uuid;
    C3D::Transform* transform = nullptr;
};

struct GameState final : C3D::ApplicationState
{
    C3D::Camera* camera          = nullptr;
    C3D::Camera* wireframeCamera = nullptr;

    ReloadState reloadState = ReloadState::Done;

    // TEMP
    SimpleScene simpleScene;
    EditorGizmo gizmo;
    bool dragging = false;

    u32 renderMode = 0;

    C3D::DynamicArray<C3D::DebugLine3D> testLines;
    C3D::DynamicArray<C3D::DebugBox3D> testBoxes;
    C3D::DynamicArray<C3D::UIText*, C3D::LinearAllocator> texts;

    C3D::Viewport worldViewport;
    C3D::Viewport uiViewport;
    C3D::Viewport wireframeViewport;

    C3D::Rendergraph frameGraph;
    SkyboxPass skyboxPass;
    ScenePass scenePass;
    EditorPass editorPass;

    SelectedObject selectedObject;

    C3D::PointLight* pLights[4];

    C3D::UIMesh uiMeshes[10];
    C3D::UIText testText;

    C3D::Entity panel, button;

    u32 hoveredObjectId = INVALID_ID;

    C3D::DynamicArray<C3D::RegisteredEventCallback> registeredCallbacks;
    // TEMP
};

struct GameFrameData final : C3D::ApplicationFrameData
{
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> worldGeometries;
};