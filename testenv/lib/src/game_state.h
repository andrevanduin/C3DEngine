
#pragma once
#include <UI/2D/internal/ui_rendergraph.h>
#include <application.h>
#include <audio/audio_types.h>
#include <ecs/entity.h>
#include <frame_data.h>
#include <identifiers/uuid.h>
#include <renderer/passes/shadow_map_pass.h>
#include <renderer/rendergraph/forward_rendergraph.h>
#include <renderer/viewport.h>
#include <resources/debug/debug_line_3d.h>
#include <resources/mesh.h>
#include <resources/scenes/scene.h>
#include <systems/events/event_system.h>

#include "editor/editor_gizmo.h"
#include "graphs/editor_rendergraph.h"

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
    u32 id = INVALID_ID;
    C3D::Handle<C3D::Transform> transform;
};

struct GameState final : C3D::ApplicationState
{
    C3D::Camera* camera          = nullptr;

    ReloadState reloadState = ReloadState::Done;

    f64 moveSpeed = 25.0, moveSpeedFast = 100.0;

    // TEMP
    C3D::Scene scene;
    EditorGizmo gizmo;
    bool dragging = false;

    u32 renderMode = 0;

    C3D::DynamicArray<C3D::DebugLine3D> testLines;
    C3D::DynamicArray<C3D::DebugBox3D> testBoxes;

    C3D::Viewport worldViewport;
    C3D::Viewport uiViewport;

    C3D::ForwardRendergraph mainRendergraph;
    EditorRendergraph editorRendergraph;
    C3D::UI2DRendergraph ui2dRendergraph;

    SelectedObject selectedObject;

    C3D::AudioHandle testMusic;

    C3D::PointLight* pLights[4];

    C3D::Handle<C3D::UI_2D::Component> debugInfoPanel, debugInfoLabel;
    C3D::Handle<C3D::UI_2D::Component> textbox;

    u32 hoveredObjectId = INVALID_ID;

    C3D::DynamicArray<C3D::RegisteredEventCallback> registeredCallbacks;
    // TEMP
};
