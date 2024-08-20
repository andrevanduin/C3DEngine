
#include "game.h"

#include <containers/cstring.h>
#include <core/colors.h>
#include <core/console/console.h>
#include <core/events/event_context.h>
#include <core/frame_data.h>
#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <renderer/renderer_types.h>
#include <resources/skybox.h>
#include <systems/UI/2D/ui2d_system.h>
#include <systems/audio/audio_system.h>
#include <systems/cameras/camera_system.h>
#include <systems/cvars/cvar_system.h>
#include <systems/events/event_system.h>
#include <systems/geometry/geometry_system.h>
#include <systems/input/input_system.h>
#include <systems/lights/light_system.h>
#include <systems/resources/resource_system.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include <glm/gtx/matrix_decompose.hpp>

#include "math/ray.h"
#include "passes/editor_pass.h"
#include "passes/scene_pass.h"
#include "passes/skybox_pass.h"
#include "resources/debug/debug_box_3d.h"
#include "resources/debug/debug_line_3d.h"
#include "resources/loaders/simple_scene_loader.h"
#include "resources/scenes/simple_scene.h"
#include "resources/scenes/simple_scene_config.h"
#include "test_env_types.h"

constexpr const char* INSTANCE_NAME = "TEST_ENV";

TestEnv::TestEnv(C3D::ApplicationState* state) : Application(state), m_state(reinterpret_cast<GameState*>(state)) {}

bool TestEnv::OnBoot()
{
    INFO_LOG("Booting TestEnv.");

    m_state->fontConfig.autoRelease = false;

    // Default bitmap font config
    C3D::BitmapFontConfig bmpFontConfig;
    bmpFontConfig.name         = "Ubuntu Mono 21px";
    bmpFontConfig.resourceName = "UbuntuMono21px";
    bmpFontConfig.size         = 21;
    m_state->fontConfig.bitmapFontConfigs.PushBack(bmpFontConfig);

    // Default system font config
    C3D::SystemFontConfig systemFontConfig;
    systemFontConfig.name         = "Noto Sans";
    systemFontConfig.resourceName = "NotoSansCJK";
    systemFontConfig.defaultSize  = 20;
    m_state->fontConfig.systemFontConfigs.PushBack(systemFontConfig);

    m_state->fontConfig.maxBitmapFontCount = 101;
    m_state->fontConfig.maxSystemFontCount = 101;

    if (!ConfigureRendergraph())
    {
        ERROR_LOG("Failed to create Rendergraph.");
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
    // Register our simple scene loader so we can use it to load our simple scene
    const auto simpleSceneLoader = Memory.New<C3D::ResourceLoader<SimpleSceneConfig>>(C3D::MemoryType::ResourceLoader);
    Resources.RegisterLoader(simpleSceneLoader);

    if (!m_state->frameGraph.LoadResources())
    {
        ERROR_LOG("Failed to load resources for Framegraph.");
        return false;
    }

    m_state->camera = Cam.Acquire("WORLD_CAM");
    m_state->camera->SetPosition({ 5.83f, 4.35f, 18.68f });
    m_state->camera->SetEulerRotation({ -29.43f, -42.41f, 0.0f });

    m_state->wireframeCamera = Cam.Acquire("WIREFRAME_CAM");
    m_state->wireframeCamera->SetPosition({ 8.0f, 0.0f, 10.0f });
    m_state->wireframeCamera->SetEulerRotation({ 0.0f, -90.0f, 0.0f });

    // Set the allocator for the dynamic array that contains our world geometries to our frame allocator
    auto gameFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);
    gameFrameData->worldGeometries.SetAllocator(frameData.allocator);

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

    CVars.Create("moveSpeed", m_state->moveSpeed, [this](const CVar& cvar) { m_state->moveSpeed = cvar.GetValue<f64>(); });
    CVars.Create("moveSpeedFast", m_state->moveSpeed, [this](const CVar& cvar) { m_state->moveSpeedFast = cvar.GetValue<f64>(); });

    m_state->testMusic = Audio.LoadStream("Woodland Fantasy");

    Audio.SetMasterVolume(0.1f);
    Audio.Play(m_state->testMusic, true);

    return true;
}

void TestEnv::OnUpdate(C3D::FrameData& frameData)
{
    // Get our application specific frame data
    auto appFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);

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

    if (m_state->simpleScene.GetState() == SceneState::Uninitialized && m_state->reloadState == ReloadState::Unloading)
    {
        m_state->reloadState = ReloadState::Loading;
        INFO_LOG("Loading Main Scene...");
        LoadTestScene();
    }

    const auto pos      = m_state->camera->GetPosition();
    const auto nearClip = m_state->worldViewport.GetNearClip();
    const auto farClip  = m_state->worldViewport.GetFarClip();

    if (m_state->simpleScene.GetState() >= SceneState::Loaded)
    {
        if (!m_state->simpleScene.Update(frameData))
        {
            ERROR_LOG("Failed to update main scene.");
        }

        // Update LODs for the scene based on distance from the camera
        m_state->simpleScene.UpdateLodFromViewPosition(frameData, pos, nearClip, farClip);

        m_state->gizmo.Update();

        // Rotate
        quat rotation = angleAxis(0.2f * static_cast<f32>(deltaTime), vec3(0.0f, 1.0f, 0.0f));

        const auto absTime  = OS.GetAbsoluteTime();
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
    // Get our application specific frame data
    auto appFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);

    auto camera = m_state->camera;

    m_state->skyboxPass.Prepare(&m_state->worldViewport, m_state->camera, m_state->simpleScene.GetSkybox());

    // Only when the scene is loaded we prepare the shadow, scene and editor pass
    if (m_state->simpleScene.GetState() == SceneState::Loaded)
    {
        // Prepare our scene for rendering
        m_state->simpleScene.OnPrepareRender(frameData);

        // Prepare the editor gizmo for rendering
        m_state->gizmo.OnPrepareRender(frameData);

        // Prepare debug boxes and lines for rendering
        for (auto& box : m_state->testBoxes)
        {
            box.OnPrepareRender(frameData);
        }

        for (auto& line : m_state->testLines)
        {
            line.OnPrepareRender(frameData);
        }

        // Prepare the shadow pass
        m_state->shadowPass.Prepare(frameData, m_state->worldViewport, m_state->camera);

        // Query meshes and terrains seen by the furthest out cascade since all passes will "see" the same
        // Get all the relevant meshes from the scene
        auto& cullingData = m_state->shadowPass.GetCullingData();

        m_state->simpleScene.QueryMeshes(frameData, cullingData.lightDirection, cullingData.center, cullingData.radius,
                                         cullingData.geometries);
        // Keep track of how many meshes are being used in our shadow pass
        frameData.drawnShadowMeshCount = cullingData.geometries.Size();

        // Get all the relevant terrains from the scene
        m_state->simpleScene.QueryTerrains(frameData, cullingData.lightDirection, cullingData.center, cullingData.radius,
                                           cullingData.terrains);

        // Also keep track of how many terrains are being used in our shadow pass
        frameData.drawnShadowMeshCount += cullingData.terrains.Size();

        // Prepare the scene pass
        m_state->scenePass.Prepare(&m_state->worldViewport, m_state->camera, frameData, m_state->simpleScene, m_state->renderMode,
                                   m_state->testLines, m_state->testBoxes, m_state->shadowPass.GetCascadeData());

        // Prepare the editor pass
        m_state->editorPass.Prepare(&m_state->worldViewport, m_state->camera, &m_state->gizmo);
    }

    UI2D.Prepare(&m_state->uiViewport);

    return true;
}

bool TestEnv::OnRender(C3D::FrameData& frameData)
{
    // Execute our Rendergraph
    if (!m_state->frameGraph.ExecuteFrame(frameData))
    {
        ERROR_LOG("Execute frame failed for the Rendergraph.");
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

    m_state->frameGraph.OnResize(width, height);

    auto debugLabelMaxX = UI2D.GetTextMaxX(m_state->debugInfoLabel);
    auto debugLabelMaxY = UI2D.GetTextMaxY(m_state->debugInfoLabel);

    UI2D.SetSize(m_state->debugInfoPanel, debugLabelMaxX + 30, debugLabelMaxY + 20);
    UI2D.SetPosition(m_state->debugInfoPanel, vec2(0, height - (debugLabelMaxY + 20)));
}

void TestEnv::OnShutdown()
{
    // Unload our simple scene
    m_state->simpleScene.Unload(true);

    // Destroy our Rendergraph
    m_state->frameGraph.Destroy();

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

        if (m_state->simpleScene.GetState() == SceneState::Loaded)
        {
            INFO_LOG("Unloading models...");
            m_state->simpleScene.Unload();
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

bool TestEnv::ConfigureRendergraph() const
{
    if (!m_state->frameGraph.Create("FRAME_RENDERGRAPH", this))
    {
        ERROR_LOG("Failed to create Frame Rendergraph.");
        return false;
    }

    // Add our global sources
    if (!m_state->frameGraph.AddGlobalSource("COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor,
                                             C3D::RendergraphSourceOrigin::Global))
    {
        ERROR_LOG("Failed to add global color buffer source to Rendergraph.");
        return false;
    }
    if (!m_state->frameGraph.AddGlobalSource("DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil,
                                             C3D::RendergraphSourceOrigin::Global))
    {
        ERROR_LOG("Failed to add global depth buffer source to Rendergraph.");
        return false;
    }

    // Skybox pass
    m_state->skyboxPass = SkyboxPass();
    if (!m_state->frameGraph.AddPass("SKYBOX", &m_state->skyboxPass))
    {
        ERROR_LOG("Failed to add SKYBOX pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("SKYBOX", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER sink to Skybox pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("SKYBOX", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor,
                                       C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER source to Skybox pass.");
        return false;
    }
    if (!m_state->frameGraph.Link("COLOR_BUFFER", "SKYBOX", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to link Global COLOR_BUFFER source to SKYBOX COLOR_BUFFER sink.");
        return false;
    }

    // ShadowMap pass
    ShadowMapPassConfig config;
    config.resolution = 4096;

    m_state->shadowPass = ShadowMapPass("SHADOW", config);
    if (!m_state->frameGraph.AddPass("SHADOW", &m_state->shadowPass))
    {
        ERROR_LOG("Failed to add: SHADOW pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("SHADOW", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil,
                                       C3D::RendergraphSourceOrigin::Self))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER to Shadow pass.");
        return false;
    }

    // Scene pass
    m_state->scenePass = ScenePass();
    if (!m_state->frameGraph.AddPass("SCENE", &m_state->scenePass))
    {
        ERROR_LOG("Failed to add SCENE pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("SCENE", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER sink to Scene pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("SCENE", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER sink to Scene pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("SCENE", "SHADOW_MAP"))
    {
        ERROR_LOG("Failed to add SHADOW_MAP_0 sink to Scene pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("SCENE", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor,
                                       C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER source to Scene pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("SCENE", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil,
                                       C3D::RendergraphSourceOrigin::Global))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER source to Scene pass.");
        return false;
    }
    if (!m_state->frameGraph.Link("SKYBOX", "COLOR_BUFFER", "SCENE", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to link SKYBOX COLOR_BUFFER source to SCENE COLOR_BUFFER sink.");
        return false;
    }
    if (!m_state->frameGraph.Link("DEPTH_BUFFER", "SCENE", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to link Global DEPTH_BUFFER source to SCENE DEPTH_BUFFER sink.");
        return false;
    }
    if (!m_state->frameGraph.Link("SHADOW", "DEPTH_BUFFER", "SCENE", "SHADOW_MAP"))
    {
        ERROR_LOG("Failed to link SHADOW  DEPTH_BUFFER source to SCENE SHADOW_MAP sink.");
        return false;
    }

    // Editor pass
    m_state->editorPass = EditorPass();
    if (!m_state->frameGraph.AddPass("EDITOR", &m_state->editorPass))
    {
        ERROR_LOG("Failed to add EDITOR pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("EDITOR", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER sink to Editor pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("EDITOR", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER sink to Editor pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("EDITOR", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor,
                                       C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER source to Editor pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("EDITOR", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil,
                                       C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER source to Editor pass.");
        return false;
    }
    if (!m_state->frameGraph.Link("SCENE", "COLOR_BUFFER", "EDITOR", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to link SCENE COLOR_BUFFER source to EDITOR COLOR_BUFFER sink.");
        return false;
    }
    if (!m_state->frameGraph.Link("SCENE", "DEPTH_BUFFER", "EDITOR", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to link SCENE DEPTH_BUFFER source to EDITOR DEPTH_BUFFER sink.");
        return false;
    }

    // UI Pass
    if (!m_state->frameGraph.AddPass("UI", UI2D.GetPass()))
    {
        ERROR_LOG("Failed to add UI pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("UI", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER sink to UI pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSink("UI", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER sink to UI pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("UI", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor,
                                       C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER source to UI pass.");
        return false;
    }
    if (!m_state->frameGraph.AddSource("UI", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil,
                                       C3D::RendergraphSourceOrigin::Global))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER source to UI pass.");
        return false;
    }
    if (!m_state->frameGraph.Link("EDITOR", "COLOR_BUFFER", "UI", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to link Editor COLOR_BUFFER source to UI COLOR_BUFFER sink.");
        return false;
    }
    if (!m_state->frameGraph.Link("DEPTH_BUFFER", "UI", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to link Global DEPTH_BUFFER source to UI DEPTH_BUFFER sink.");
        return false;
    }

    if (!m_state->frameGraph.Finalize(m_pEngine->GetFrameAllocator()))
    {
        ERROR_LOG("Failed to finalize rendergraph.");
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
    if (m_state->simpleScene.GetState() < SceneState::Loaded)
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
            if (m_state->simpleScene.RayCast(ray, result))
            {
                f32 closestDistance = C3D::F32_MAX;
                for (auto& hit : result.hits)
                {
                    // Create a debug line
                    C3D::DebugLine3D line;
                    if (!line.Create(ray.origin, hit.position, nullptr))
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
                    if (!box.Create(vec3(0.1f), nullptr))
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
                        closestDistance              = hit.distance;
                        m_state->selectedObject.uuid = hit.uuid;
                    }
                }

                const auto selectedUUID = m_state->selectedObject.uuid;
                if (selectedUUID.IsValid())
                {
                    m_state->selectedObject.transform = m_state->simpleScene.GetTransformById(selectedUUID);
                    INFO_LOG("Selected object id = {}.", selectedUUID);
                    m_state->gizmo.SetSelectedObjectTransform(m_state->selectedObject.transform);
                }
            }
            else
            {
                INFO_LOG("Ray MISSED!");

                m_state->selectedObject.transform = nullptr;
                m_state->selectedObject.uuid      = INVALID_ID;
                m_state->gizmo.SetSelectedObjectTransform(nullptr);

                // Create a debug line
                C3D::DebugLine3D line;
                if (!line.Create(origin, origin + (ray.direction * 100.0f), nullptr))
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
        if (m_state->simpleScene.GetState() == SceneState::Uninitialized)
        {
            INFO_LOG("Loading Main Scene...");
            LoadTestScene();
        }

        return true;
    }

    if (code == C3D::EventCodeDebug2)
    {
        if (m_state->simpleScene.GetState() == SceneState::Loaded)
        {
            UnloadTestScene();
        }

        return true;
    }

    return false;
}

bool TestEnv::LoadTestScene()
{
    SimpleSceneConfig sceneConfig;
    Resources.Load("test_scene", sceneConfig);

    if (!m_state->simpleScene.Create(sceneConfig))
    {
        ERROR_LOG("Creating SimpleScene failed.");
        return false;
    }

    if (!m_state->simpleScene.Initialize())
    {
        ERROR_LOG("Initializing SimpleScene failed.");
        return false;
    }

    m_state->pLights[0] = m_state->simpleScene.GetPointLight("point_light_0");

    if (!m_state->simpleScene.Load())
    {
        ERROR_LOG("Loading SimpleScene failed.");
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

    m_state->simpleScene.Unload();
}

C3D::Application* CreateApplication(C3D::ApplicationState* state) { return Memory.New<TestEnv>(C3D::MemoryType::Game, state); }

C3D::ApplicationState* CreateApplicationState()
{
    const auto state           = Memory.New<GameState>(C3D::MemoryType::Game);
    state->name                = "TestEnv";
    state->windowConfig.width  = 1280;
    state->windowConfig.height = 720;
    state->windowConfig.flags  = C3D::WindowFlagCenter;
    state->frameAllocatorSize  = MebiBytes(8);
    state->appFrameDataSize    = sizeof(GameFrameData);
    return state;
}
