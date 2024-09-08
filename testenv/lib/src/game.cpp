
#include "game.h"

#include <colors.h>
#include <console/console.h>
#include <frame_data.h>
#include <logger/logger.h>
#include <math/ray.h>
#include <metrics/metrics.h>
#include <renderer/renderer_types.h>
#include <resources/managers/scene_manager.h>
#include <resources/scenes/scene.h>
#include <resources/scenes/scene_config.h>
#include <resources/skybox.h>
#include <string/cstring.h>
#include <systems/UI/2D/ui2d_system.h>
#include <systems/audio/audio_system.h>
#include <systems/cameras/camera_system.h>
#include <systems/cvars/cvar_system.h>
#include <systems/events/event_context.h>
#include <systems/events/event_system.h>
#include <systems/geometry/geometry_system.h>
#include <systems/input/input_system.h>
#include <systems/lights/light_system.h>
#include <systems/resources/resource_system.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include <glm/gtx/matrix_decompose.hpp>

#include "passes/editor_pass.h"
#include "resources/debug/debug_box_3d.h"
#include "resources/debug/debug_line_3d.h"

TestEnv::TestEnv(C3D::ApplicationState* state) : Application(state), m_state(reinterpret_cast<GameState*>(state)) {}

bool TestEnv::OnBoot()
{
    INFO_LOG("Booting TestEnv.");

    if (!CreateRendergraphs())
    {
        ERROR_LOG("Failed to create Rendergraphs.");
        return false;
    }

    // Setup viewports
    C3D::Rect2D worldViewportRect = { 0.0f, 0.0f, 1280.0f - 40, 720.0f };
    if (!m_state->worldViewport.Create(worldViewportRect, C3D::DegToRad(45.0f), 0.1f, 1000.0f,
                                       C3D::RendererProjectionMatrixType::Perspective))
    {
        ERROR_LOG("Failed to create World Viewport.");
        return false;
    }

    C3D::Rect2D uiViewportRect = { 0.0f, 0.0f, 1280.0f, 720.0f };
    if (!m_state->uiViewport.Create(uiViewportRect, 0.0f, -100.0f, 100.0f, C3D::RendererProjectionMatrixType::Orthographic))
    {
        ERROR_LOG("Failed to create UI Viewport.");
        return false;
    }

    return true;
}

bool TestEnv::OnRun(C3D::FrameData& frameData)
{
    if (!InitializeRendergraphs())
    {
        ERROR_LOG("Failed to initialize Rendergraphs.");
        return false;
    }

    m_state->camera = Cam.Acquire("WORLD_CAM");
    m_state->camera->SetPosition({ 5.83f, 4.35f, 18.68f });
    m_state->camera->SetEulerRotation({ -29.43f, -42.41f, 0.0f });

    m_state->wireframeCamera = Cam.Acquire("WIREFRAME_CAM");
    m_state->wireframeCamera->SetPosition({ 8.0f, 0.0f, 10.0f });
    m_state->wireframeCamera->SetEulerRotation({ 0.0f, -90.0f, 0.0f });

    // Create, initialize and load our editor gizmo
    if (!m_state->gizmo.Create())
    {
        ERROR_LOG("Failed to create Editor Gizmo.");
        return false;
    }

    if (!m_state->gizmo.Initialize())
    {
        ERROR_LOG("Failed to initialize Editor Gizmo.");
        return false;
    }

    if (!m_state->gizmo.Load())
    {
        ERROR_LOG("Failed to load Editor Gizmo.");
        return false;
    }

    m_state->editorRendergraph.SetGizmo(&m_state->gizmo);

    auto font = Fonts.Acquire("Ubuntu Mono 21px", C3D::FontType::Bitmap, 32);

    auto config = C3D::UI_2D::Config::DefaultPanel();
    config.size = u16vec2(300, 80);

    m_state->debugInfoPanel = UI2D.AddPanel(config);

    config          = C3D::UI_2D::Config::DefaultLabel();
    config.position = vec2(15, 10);
    config.text     = "DebugInfo";
    config.font     = font;

    m_state->debugInfoLabel = UI2D.AddLabel(config);

    UI2D.SetParent(m_state->debugInfoLabel, m_state->debugInfoPanel);

    config          = C3D::UI_2D::Config::DefaultTextbox();
    config.position = vec2(400, 250);
    config.text     = "DEFAULT_TEXT_THAT_IS_A_LITTLE_LARGER_THAN_THE_BOUNDS";
    config.size     = u16vec2(150, 30);
    config.font     = font;

    m_state->textbox = UI2D.AddTextbox(config);

    /*
    UI2D.AddOnHoverStartHandler(m_state->button, [](const C3D::UI_2D::OnHoverEventContext& ctx) {
        INFO_LOG("Button hover started at: ({}, {}).", ctx.x, ctx.y);
        return true;
    });
    UI2D.AddOnHoverEndHandler(m_state->button, [](const C3D::UI_2D::OnHoverEventContext& ctx) {
        INFO_LOG("Button hover ended at: ({}, {}).", ctx.x, ctx.y);
        return true;
    });
    */

    CVars.Create("moveSpeed", m_state->moveSpeed, [this](const C3D::CVar& cvar) { m_state->moveSpeed = cvar.GetValue<f64>(); });
    CVars.Create("moveSpeedFast", m_state->moveSpeed, [this](const C3D::CVar& cvar) { m_state->moveSpeedFast = cvar.GetValue<f64>(); });

    m_state->testMusic = Audio.LoadStream("Woodland Fantasy");

    Audio.SetMasterVolume(0.1f);
    Audio.Play(m_state->testMusic, true);

    return true;
}

void TestEnv::OnUpdate(C3D::FrameData& frameData)
{
    static u64 allocCount    = 0;
    const u64 prevAllocCount = allocCount;
    allocCount               = Metrics.GetAllocCount();

    UI2D.OnUpdate(frameData);

    const auto deltaTime = frameData.timeData.delta;
    if (!m_pConsole->IsOpen())
    {
        if (Input.IsKeyPressed(C3D::KeyM))
        {
            C3D::Logger::Info("Allocations: {} of which {} happened this frame", allocCount, allocCount - prevAllocCount);
            Metrics.PrintMemoryUsage(true);
        }

        if (Input.IsKeyPressed(C3D::KeyP))
        {
            const auto pos = m_state->camera->GetPosition();
            C3D::Logger::Debug("Position({:.2f}, {:.2f}, {:.2f})", pos.x, pos.y, pos.z);
        }

        if (Input.IsKeyPressed(C3D::KeyG))
        {
            EditorGizmoOrientation orientation = m_state->gizmo.GetOrientation();
            if (orientation == EditorGizmoOrientation::Global)
            {
                m_state->gizmo.SetOrientation(EditorGizmoOrientation::Local);
            }
            else
            {
                m_state->gizmo.SetOrientation(EditorGizmoOrientation::Global);
            }
        }

        // Renderer Debug functions
        if (Input.IsKeyPressed(C3D::KeyF1))
        {
            C3D::EventContext context = {};
            context.data.i32[0]       = C3D::RendererViewMode::Default;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyPressed(C3D::KeyF2))
        {
            C3D::EventContext context = {};
            context.data.i32[0]       = C3D::RendererViewMode::Lighting;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyPressed(C3D::KeyF3))
        {
            C3D::EventContext context = {};
            context.data.i32[0]       = C3D::RendererViewMode::Normals;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyPressed(C3D::KeyF4))
        {
            C3D::EventContext context = {};
            context.data.i32[0]       = C3D::RendererViewMode::Cascades;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyPressed(C3D::KeyF5))
        {
            C3D::EventContext context = {};
            context.data.i32[0]       = C3D::RendererViewMode::Wireframe;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        // Gizmo mode keys
        if (Input.IsKeyPressed('1'))
        {
            m_state->gizmo.SetMode(EditorGizmoMode::None);
        }

        if (Input.IsKeyPressed('2'))
        {
            m_state->gizmo.SetMode(EditorGizmoMode::Move);
        }

        if (Input.IsKeyPressed('3'))
        {
            m_state->gizmo.SetMode(EditorGizmoMode::Rotate);
        }

        if (Input.IsKeyPressed('4'))
        {
            m_state->gizmo.SetMode(EditorGizmoMode::Scale);
        }

        if (Input.IsKeyDown('A') || Input.IsKeyDown(C3D::KeyArrowLeft))
        {
            m_state->camera->AddYaw(1.0 * deltaTime);
        }

        if (Input.IsKeyDown('D') || Input.IsKeyDown(C3D::KeyArrowRight))
        {
            m_state->camera->AddYaw(-1.0 * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeyArrowUp))
        {
            m_state->camera->AddPitch(1.0 * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeyArrowDown))
        {
            m_state->camera->AddPitch(-1.0 * deltaTime);
        }

        f64 moveSpeed = m_state->moveSpeed;
        if (Input.IsKeyDown(C3D::KeyLControl))
        {
            moveSpeed = m_state->moveSpeedFast;
        }

        if (Input.IsKeyDown('W'))
        {
            m_state->camera->MoveForward(moveSpeed * deltaTime);
        }

        if (Input.IsKeyDown('S'))
        {
            m_state->camera->MoveBackward(moveSpeed * deltaTime);
        }

        if (Input.IsKeyDown('Q'))
        {
            m_state->camera->MoveLeft(moveSpeed * deltaTime);
        }

        if (Input.IsKeyDown('E'))
        {
            m_state->camera->MoveRight(moveSpeed * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeySpace))
        {
            m_state->camera->MoveUp(moveSpeed * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeyX))
        {
            m_state->camera->MoveDown(moveSpeed * deltaTime);
        }
    }

    if (m_state->scene.GetState() == C3D::SceneState::Uninitialized && m_state->reloadState == ReloadState::Unloading)
    {
        m_state->reloadState = ReloadState::Loading;
        INFO_LOG("Loading Main Scene...");
        LoadTestScene();
    }

    const auto pos      = m_state->camera->GetPosition();
    const auto nearClip = m_state->worldViewport.GetNearClip();
    const auto farClip  = m_state->worldViewport.GetFarClip();

    if (m_state->scene.GetState() >= C3D::SceneState::Loaded)
    {
        if (!m_state->scene.Update(frameData))
        {
            ERROR_LOG("Failed to update main scene.");
        }

        // Update LODs for the scene based on distance from the camera
        m_state->scene.UpdateLodFromViewPosition(frameData, pos, nearClip, farClip);

        m_state->gizmo.Update();

        // Rotate
        quat rotation = angleAxis(0.2f * static_cast<f32>(deltaTime), vec3(0.0f, 1.0f, 0.0f));

        const auto absTime  = Platform::GetAbsoluteTime();
        const auto sinTime  = (C3D::Sin(absTime) + 1) / 2;  // 0  -> 1
        const auto sinTime2 = C3D::Sin(absTime);            // -1 -> 1

        const C3D::HSV hsv(sinTime, 1.0f, 1.0f);
        const auto rgba = C3D::HsvToRgba(hsv);

        m_state->pLights[0]->data.color = vec4(rgba.r, rgba.g, rgba.b, rgba.a);
        m_state->pLights[0]->data.position.z += sinTime2;
        m_state->pLights[0]->data.linear    = 0.5f;
        m_state->pLights[0]->data.quadratic = 0.2f;

        if (m_state->pLights[0]->data.position.z < 10.0f) m_state->pLights[0]->data.position.z = 10.0f;
        if (m_state->pLights[0]->data.position.z > 40.0f) m_state->pLights[0]->data.position.z = 40.0f;

        Lights.InvalidatePointLightCache();
    }

    const auto fWidth  = static_cast<f32>(m_pEngine->GetWindowWidth());
    const auto fHeight = static_cast<f32>(m_pEngine->GetWindowHeight());

    const auto rot = m_state->camera->GetEulerRotation();

    const auto mouse = Input.GetMousePosition();
    // Convert to NDC
    const auto mouseNdcX = C3D::RangeConvert(static_cast<f32>(mouse.x), 0.0f, fWidth, -1.0f, 1.0f);
    const auto mouseNdcY = C3D::RangeConvert(static_cast<f32>(mouse.y), 0.0f, fHeight, -1.0f, 1.0f);

    const auto leftButton   = Input.IsButtonDown(C3D::Buttons::ButtonLeft);
    const auto middleButton = Input.IsButtonDown(C3D::Buttons::ButtonMiddle);
    const auto rightButton  = Input.IsButtonDown(C3D::Buttons::ButtonRight);

    C3D::CString<16> hoveredBuffer;
    if (m_state->hoveredObjectId != INVALID_ID)
    {
        hoveredBuffer.FromFormat("{}", m_state->hoveredObjectId);
    }
    else
    {
        hoveredBuffer = "None";
    }

    C3D::CString<512> buffer;
    buffer.FromFormat(
        "{:<10} : Pos({:.3f}, {:.3f}, {:.3f}) Rot({:.3f}, {:.3f}, {:.3f})\n"
        "{:<10} : Pos({:.2f}, {:.2f}) Buttons({}, {}, {}) Hovered: {}\n"
        "{:<10} : DrawCount: (Mesh: {}, Terrain: {}, ShadowMap: {}) FPS: {} VSync: {}\n"
        "{:<10} : Prepare: {:.4f} Render: {:.4f} Present: {:.4f} Update: {:.4f} Total: {:.4f}",
        "Cam", pos.x, pos.y, pos.z, C3D::RadToDeg(rot.x), C3D::RadToDeg(rot.y), C3D::RadToDeg(rot.z), "Mouse", mouseNdcX, mouseNdcY,
        leftButton, middleButton, rightButton, hoveredBuffer, "Renderer", frameData.drawnMeshCount, frameData.drawnTerrainCount,
        frameData.drawnShadowMeshCount, Metrics.GetFps(), Renderer.IsFlagEnabled(C3D::FlagVSyncEnabled) ? "Yes" : "No", "Timings",
        frameData.timeData.avgPrepareFrameTimeMs, frameData.timeData.avgRenderTimeMs, frameData.timeData.avgPresentTimeMs,
        frameData.timeData.avgUpdateTimeMs, frameData.timeData.avgRunTimeMs);

    UI2D.SetText(m_state->debugInfoLabel, buffer.Data());

    static bool resized = false;
    if (!resized)
    {
        const auto fHeight = static_cast<f32>(m_pEngine->GetWindowHeight());

        auto debugLabelMaxX = UI2D.GetTextMaxX(m_state->debugInfoLabel);
        auto debugLabelMaxY = UI2D.GetTextMaxY(m_state->debugInfoLabel);

        UI2D.SetSize(m_state->debugInfoPanel, debugLabelMaxX + 30, debugLabelMaxY + 20);
        UI2D.SetPosition(m_state->debugInfoPanel, vec2(0, fHeight - (debugLabelMaxY + 20)));
        resized = true;
    }
}

bool TestEnv::OnPrepareRender(C3D::FrameData& frameData)
{
    auto camera = m_state->camera;

    // Prepare debug boxes and lines for rendering
    for (auto& box : m_state->testBoxes)
    {
        box.OnPrepareRender(frameData);
    }

    for (auto& line : m_state->testLines)
    {
        line.OnPrepareRender(frameData);
    }

    if (!m_state->mainRendergraph.OnPrepareRender(frameData, m_state->worldViewport, camera, m_state->scene, m_state->renderMode,
                                                  m_state->testLines, m_state->testBoxes))
    {
        ERROR_LOG("Failed to prepare main rendergraph.");
        return false;
    }

    if (!m_state->editorRendergraph.OnPrepareRender(frameData, m_state->worldViewport, camera, m_state->scene))
    {
        ERROR_LOG("Failed to prepare editor rendergraph.");
        return false;
    }

    if (!m_state->ui2dRendergraph.OnPrepareRender(frameData, m_state->uiViewport))
    {
        ERROR_LOG("Failed to prepare UI2D rendergraph.");
        return false;
    }

    return true;
}

bool TestEnv::OnRender(C3D::FrameData& frameData)
{
    // Execute our Rendergraphs
    if (!m_state->mainRendergraph.ExecuteFrame(frameData))
    {
        ERROR_LOG("Execute frame failed for the Main Rendergraph.");
        return false;
    }

    if (!m_state->editorRendergraph.ExecuteFrame(frameData))
    {
        ERROR_LOG("Execute frame failed for Editor Rendergraph.");
        return false;
    }

    if (!m_state->ui2dRendergraph.ExecuteFrame(frameData))
    {
        ERROR_LOG("Execute frame failed for UI2D Rendergraph.");
        return false;
    }

    return true;
}

void TestEnv::OnResize(u16 width, u16 height)
{
    f32 halfWidth = width * 0.5f;

    // Resize our viewports
    C3D::Rect2D worldViewportRect = { 0.0f, 0.0f, static_cast<f32>(width), static_cast<f32>(height) };
    m_state->worldViewport.Resize(worldViewportRect);

    C3D::Rect2D uiViewportRect = { 0.0f, 0.0f, static_cast<f32>(width), static_cast<f32>(height) };
    m_state->uiViewport.Resize(uiViewportRect);

    if (!m_state->mainRendergraph.OnResize(width, height))
    {
        ERROR_LOG("Failed to resize main rendergaph.");
    }

    if (!m_state->editorRendergraph.OnResize(width, height))
    {
        ERROR_LOG("Failed to resize editor rendergraph.");
    }

    if (!m_state->ui2dRendergraph.OnResize(width, height))
    {
        ERROR_LOG("Failed to resize UI2D rednergraph.");
    }

    auto debugLabelMaxX = UI2D.GetTextMaxX(m_state->debugInfoLabel);
    auto debugLabelMaxY = UI2D.GetTextMaxY(m_state->debugInfoLabel);

    UI2D.SetSize(m_state->debugInfoPanel, debugLabelMaxX + 30, debugLabelMaxY + 20);
    UI2D.SetPosition(m_state->debugInfoPanel, vec2(0, height - (debugLabelMaxY + 20)));
}

void TestEnv::OnShutdown()
{
    // Unload our simple scene
    m_state->scene.Unload(true);

    // Destroy our Rendergraph
    m_state->mainRendergraph.Destroy();
    m_state->editorRendergraph.Destroy();
    m_state->ui2dRendergraph.Destroy();

    // Destroy our test geometry
    for (auto& line : m_state->testLines)
    {
        line.Unload();
        line.Destroy();
    }
    m_state->testLines.Destroy();

    for (auto& box : m_state->testBoxes)
    {
        box.Unload();
        box.Destroy();
    }
    m_state->testBoxes.Destroy();

    // Unload our gizmo
    m_state->gizmo.Unload();

    // Destroy our gizmo
    m_state->gizmo.Destroy();
}

void TestEnv::OnLibraryLoad()
{
    C3D::RegisteredEventCallback cb;

    cb = Event.Register(C3D::EventCodeDebug0, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnDebugEvent(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeDebug1, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnDebugEvent(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeDebug2, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnDebugEvent(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeObjectHoverIdChanged,
                        [this](const u16 code, void* sender, const C3D::EventContext& context) { return OnEvent(code, sender, context); });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeButtonUp, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnButtonUp(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeMouseMoved, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnMouseMoved(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeMouseDragged, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnMouseDragged(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeMouseDraggedStart, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnMouseDragged(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeMouseDraggedEnd, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnMouseDragged(code, sender, context);
    });
    m_state->registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeSetRenderMode, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        switch (const i32 mode = context.data.i32[0])
        {
            case C3D::RendererViewMode::Default:
                DEBUG_LOG("Renderer mode set to default.");
                m_state->renderMode = C3D::RendererViewMode::Default;
                break;
            case C3D::RendererViewMode::Lighting:
                DEBUG_LOG("Renderer mode set to lighting.");
                m_state->renderMode = C3D::RendererViewMode::Lighting;
                break;
            case C3D::RendererViewMode::Normals:
                DEBUG_LOG("Renderer mode set to normals.");
                m_state->renderMode = C3D::RendererViewMode::Normals;
                break;
            case C3D::RendererViewMode::Cascades:
                DEBUG_LOG("Renderer mode set to cascades.");
                m_state->renderMode = C3D::RendererViewMode::Cascades;
                break;
            case C3D::RendererViewMode::Wireframe:
                DEBUG_LOG("Renderer mode set to wireframe.")
                m_state->renderMode = C3D::RendererViewMode::Wireframe;
                break;
            default:
                FATAL_LOG("Unknown render mode.");
                break;
        }
        return true;
    });
    m_state->registeredCallbacks.PushBack(cb);

    m_pConsole->RegisterCommand("load_scene", [this](const C3D::DynamicArray<C3D::ArgName>&, C3D::String&) {
        Event.Fire(C3D::EventCodeDebug1, this, {});
        return true;
    });

    m_pConsole->RegisterCommand("unload_scene", [this](const C3D::DynamicArray<C3D::ArgName>&, C3D::String&) {
        Event.Fire(C3D::EventCodeDebug2, this, {});
        return true;
    });

    m_pConsole->RegisterCommand("reload_scene", [this](const C3D::DynamicArray<C3D::ArgName>&, C3D::String&) {
        m_state->reloadState = ReloadState::Unloading;

        if (m_state->scene.GetState() == C3D::SceneState::Loaded)
        {
            INFO_LOG("Unloading models...");
            m_state->scene.Unload();
        }
        return true;
    });
}

void TestEnv::OnLibraryUnload()
{
    for (auto& cb : m_state->registeredCallbacks)
    {
        Event.Unregister(cb);
    }
    m_state->registeredCallbacks.Destroy();

    m_pConsole->UnregisterCommand("load_scene");
    m_pConsole->UnregisterCommand("unload_scene");
    m_pConsole->UnregisterCommand("reload_scene");
}

bool TestEnv::CreateRendergraphs() const
{
    C3D::ForwardRendergraphConfig mainConfig;
    mainConfig.shadowMapResolution = 4096;
    mainConfig.pFrameAllocator     = m_pEngine->GetFrameAllocator();

    if (!m_state->mainRendergraph.Create("MAIN_RENDERGRAPH", mainConfig))
    {
        ERROR_LOG("Failed to create main rendergraph.");
        return false;
    }

    EditorRendergraphConfig editorConfig;
    editorConfig.pFrameAllocator = m_pEngine->GetFrameAllocator();

    if (!m_state->editorRendergraph.Create("EDITOR_RENDERGRAPH", editorConfig))
    {
        ERROR_LOG("Failed to create editor rendergraph.");
        return false;
    }

    UI2DRendergraphConfig ui2DConfig;
    ui2DConfig.pFrameAllocator = m_pEngine->GetFrameAllocator();

    if (!m_state->ui2dRendergraph.Create("UI2D_RENDERGRAPH", ui2DConfig))
    {
        ERROR_LOG("Failed to create UI2D rendergraph.");
        return false;
    }

    return true;
}

bool TestEnv::InitializeRendergraphs() const
{
    if (!m_state->mainRendergraph.Initialize())
    {
        ERROR_LOG("Failed to initialize main rendergraph.");
        return false;
    }

    if (!m_state->editorRendergraph.Initialize())
    {
        ERROR_LOG("Failed to initialize editor rendergraph.");
        return false;
    }

    if (!m_state->ui2dRendergraph.Initialize())
    {
        ERROR_LOG("Failed to initialize UI2D rendergraph.");
        return false;
    }

    return true;
}

bool TestEnv::OnEvent(const u16 code, void* sender, const C3D::EventContext& context)
{
    switch (code)
    {
        case C3D::EventCodeObjectHoverIdChanged:
            m_state->hoveredObjectId = context.data.u32[0];
            return true;
        default:
            return false;
    }
}

bool TestEnv::OnButtonUp(u16 code, void* sender, const C3D::EventContext& context)
{
    u16 button = context.data.u16[0];

    // If we are dragging we don't need to do any of the logic below
    if (m_state->dragging)
    {
        return false;
    }

    // If our scene is not loaded we also ignore everything below
    if (m_state->scene.GetState() < C3D::SceneState::Loaded)
    {
        return false;
    }

    switch (button)
    {
        case C3D::ButtonLeft:
            f32 x = static_cast<f32>(context.data.i16[1]);
            f32 y = static_cast<f32>(context.data.i16[2]);

            mat4 view   = m_state->camera->GetViewMatrix();
            vec3 origin = m_state->camera->GetPosition();

            const auto& viewport = m_state->worldViewport;

            // Only allow ray casting in the "primary" section of the viewport
            if (!viewport.PointIsInside({ x, y }))
            {
                return false;
            }

            C3D::Ray ray = C3D::Ray::FromScreen(vec2(x, y), viewport.GetRect2D(), origin, view, viewport.GetProjection());

            C3D::RayCastResult result;
            if (m_state->scene.RayCast(ray, result))
            {
                f32 closestDistance = C3D::F32_MAX;
                for (auto& hit : result.hits)
                {
                    // Create a debug line
                    C3D::DebugLine3D line;
                    if (!line.Create(ray.origin, hit.position))
                    {
                        ERROR_LOG("Failed to create debug line.");
                        return false;
                    }
                    if (!line.Initialize())
                    {
                        ERROR_LOG("Failed to initialize debug line.");
                        return false;
                    }
                    if (!line.Load())
                    {
                        ERROR_LOG("Failed to load debug line.");
                        return false;
                    }
                    // We set the line to yellow for hits
                    line.SetColor(C3D::YELLOW);

                    m_state->testLines.PushBack(line);

                    C3D::DebugBox3D box;
                    if (!box.Create(vec3(0.1f)))
                    {
                        ERROR_LOG("Failed to create debug box.");
                        return false;
                    }
                    if (!box.Initialize())
                    {
                        ERROR_LOG("Failed to initialize debug box.");
                        return false;
                    }
                    if (!box.Load())
                    {
                        ERROR_LOG("Failed to load debug box.");
                        return false;
                    }

                    box.SetPosition(hit.position);

                    m_state->testBoxes.PushBack(box);

                    // Keep track of the hit that is closest
                    if (hit.distance < closestDistance)
                    {
                        closestDistance            = hit.distance;
                        m_state->selectedObject.id = hit.id;
                    }
                }

                const auto selectedId = m_state->selectedObject.id;
                if (selectedId != INVALID_ID)
                {
                    m_state->selectedObject.transform = m_state->scene.GetTransformById(selectedId);
                    INFO_LOG("Selected object id = {}.", selectedId);
                    m_state->gizmo.SetSelectedObjectTransform(m_state->selectedObject.transform);
                }
            }
            else
            {
                INFO_LOG("Ray MISSED!");

                m_state->selectedObject.transform.Invalidate();
                m_state->selectedObject.id = INVALID_ID;
                m_state->gizmo.SetSelectedObjectTransform(C3D::Handle<C3D::Transform>());

                // Create a debug line
                C3D::DebugLine3D line;
                if (!line.Create(origin, origin + (ray.direction * 100.0f)))
                {
                    ERROR_LOG("Failed to create debug line.");
                    return false;
                }
                if (!line.Initialize())
                {
                    ERROR_LOG("Failed to initialize debug line.");
                    return false;
                }
                if (!line.Load())
                {
                    ERROR_LOG("Failed to load debug line.");
                    return false;
                }
                // We set the line to magenta for non-hits
                line.SetColor(C3D::MAGENTA);

                m_state->testLines.PushBack(line);
            }

            break;
    }

    return false;
}

bool TestEnv::OnMouseMoved(u16 code, void* sender, const C3D::EventContext& context)
{
    if (code == C3D::EventCodeMouseMoved && !Input.IsButtonDragging(C3D::ButtonLeft))
    {
        // Mouse is being moved but we are not dragging left mouse buttn
        i16 x = context.data.i16[0];
        i16 y = context.data.i16[1];

        mat4 view   = m_state->camera->GetViewMatrix();
        vec3 origin = m_state->camera->GetPosition();

        const auto& viewport = m_state->worldViewport;

        const auto ray = C3D::Ray::FromScreen(vec2(x, y), viewport.GetRect2D(), origin, view, viewport.GetProjection());
        m_state->gizmo.BeginInteraction(EditorGizmoInteractionType::MouseHover, m_state->camera, ray);
        m_state->gizmo.HandleInteraction(ray);
    }
    // Allow other event handlers to handle this event
    return false;
}

bool TestEnv::OnMouseDragged(u16 code, void* sender, const C3D::EventContext& context)
{
    u16 button = context.data.u16[0];
    i16 x      = context.data.i16[1];
    i16 y      = context.data.i16[2];

    if (button == C3D::ButtonLeft)
    {
        // Only do this when we are dragging with our left mouse button
        vec3 origin = m_state->camera->GetPosition();
        mat4 view   = m_state->camera->GetViewMatrix();

        const auto& viewport = m_state->worldViewport;

        const auto ray = C3D::Ray::FromScreen(vec2(x, y), viewport.GetRect2D(), origin, view, viewport.GetProjection());

        if (code == C3D::EventCodeMouseDraggedStart)
        {
            // Drag start so we start our "dragging" interaction
            m_state->gizmo.BeginInteraction(EditorGizmoInteractionType::MouseDrag, m_state->camera, ray);
            m_state->dragging = true;
        }
        else if (code == C3D::EventCodeMouseDragged)
        {
            m_state->gizmo.HandleInteraction(ray);
        }
        else if (code == C3D::EventCodeMouseDraggedEnd)
        {
            m_state->gizmo.EndInteraction();
            m_state->dragging = false;
        }
    }
    return false;
}

bool TestEnv::OnDebugEvent(const u16 code, void*, const C3D::EventContext&)
{
    if (code == C3D::EventCodeDebug1)
    {
        if (m_state->scene.GetState() == C3D::SceneState::Uninitialized)
        {
            INFO_LOG("Loading Main Scene...");
            LoadTestScene();
        }

        return true;
    }

    if (code == C3D::EventCodeDebug2)
    {
        if (m_state->scene.GetState() == C3D::SceneState::Loaded)
        {
            UnloadTestScene();
        }

        return true;
    }

    return false;
}

bool TestEnv::LoadTestScene()
{
    C3D::SceneConfig sceneConfig;
    Resources.Read("test_scene", sceneConfig);

    if (!m_state->scene.Create(sceneConfig))
    {
        ERROR_LOG("Creating Scene failed.");
        return false;
    }

    if (!m_state->scene.Initialize())
    {
        ERROR_LOG("Initializing Scene failed.");
        return false;
    }

    m_state->pLights[0] = m_state->scene.GetPointLight("point_light_0");

    if (!m_state->scene.Load())
    {
        ERROR_LOG("Loading Scene failed.");
        return false;
    }

    m_state->reloadState = ReloadState::Done;
    return true;
}

void TestEnv::UnloadTestScene()
{
    for (auto& line : m_state->testLines)
    {
        line.Unload();
        line.Destroy();
    }
    m_state->testLines.Destroy();

    for (auto& box : m_state->testBoxes)
    {
        box.Unload();
        box.Destroy();
    }
    m_state->testBoxes.Destroy();

    m_state->scene.Unload();
}

C3D::Application* CreateApplication(C3D::ApplicationState* state) { return Memory.New<TestEnv>(C3D::MemoryType::Game, state); }

C3D::ApplicationState* CreateApplicationState() { return Memory.New<GameState>(C3D::MemoryType::Game); }
