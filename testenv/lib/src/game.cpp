
#include "game.h"

#include <containers/cstring.h>
#include <core/colors.h>
#include <core/console/console.h>
#include <core/events/event_context.h>
#include <core/frame_data.h>
#include <core/identifier.h>
#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <renderer/renderer_types.h>
#include <resources/skybox.h>
#include <systems/cameras/camera_system.h>
#include <systems/events/event_system.h>
#include <systems/geometry/geometry_system.h>
#include <systems/input/input_system.h>
#include <systems/lights/light_system.h>
#include <systems/render_views/render_view_system.h>
#include <systems/resources/resource_system.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include <glm/gtx/matrix_decompose.hpp>

#include "editor/render_view_editor_world.h"
#include "math/ray.h"
#include "resources/debug/debug_box_3d.h"
#include "resources/debug/debug_line_3d.h"
#include "resources/loaders/simple_scene_loader.h"
#include "resources/scenes/simple_scene.h"
#include "resources/scenes/simple_scene_config.h"
#include "test_env_types.h"
#include "views/render_view_pick.h"
#include "views/render_view_skybox.h"
#include "views/render_view_ui.h"
#include "views/render_view_world.h"

TestEnv::TestEnv(C3D::ApplicationState* state) : Application(state), m_state(reinterpret_cast<GameState*>(state)) {}

bool TestEnv::OnBoot()
{
    m_logger.Info("OnBoot() - Booting TestEnv");

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

    // Config our render views. TODO: Read this from a file
    if (!ConfigureRenderViews())
    {
        m_logger.Error("OnBoot() - Failed to create render views.");
        return false;
    }

    return true;
}

bool TestEnv::OnRun(C3D::FrameData& frameData)
{
    // Register our simple scene loader so we can use it to load our simple scene
    const auto simpleSceneLoader = Memory.New<C3D::ResourceLoader<SimpleSceneConfig>>(C3D::MemoryType::ResourceLoader, m_pSystemsManager);
    Resources.RegisterLoader(simpleSceneLoader);

    // TEMP
    // Create test ui text objects
    if (!m_state->testText.Create("TEST_UI_TEXT", m_pSystemsManager, C3D::UITextType::Bitmap, "Ubuntu Mono 21px", 21,
                                  "Some test text 123,\nyesyes!\n\tKaas!"))
    {
        m_logger.Fatal("OnRun() - Failed to load basic ui bitmap text.");
    }

    m_state->testText.SetPosition({ 10, 640, 0 });

    // Load up our test ui geometry
    C3D::UIGeometryConfig uiConfig = {};
    uiConfig.vertices.Resize(4);
    uiConfig.indices.Resize(6);

    uiConfig.materialName = "test_ui_material";
    uiConfig.name         = "test_ui_geometry";

    constexpr f32 w = 128.0f;
    constexpr f32 h = 32.0f;

    uiConfig.vertices[0].position.x = 0.0f;
    uiConfig.vertices[0].position.y = 0.0f;
    uiConfig.vertices[0].texture.x  = 0.0f;
    uiConfig.vertices[0].texture.y  = 0.0f;

    uiConfig.vertices[1].position.x = w;
    uiConfig.vertices[1].position.y = h;
    uiConfig.vertices[1].texture.x  = 1.0f;
    uiConfig.vertices[1].texture.y  = 1.0f;

    uiConfig.vertices[2].position.x = 0.0f;
    uiConfig.vertices[2].position.y = h;
    uiConfig.vertices[2].texture.x  = 0.0f;
    uiConfig.vertices[2].texture.y  = 1.0f;

    uiConfig.vertices[3].position.x = w;
    uiConfig.vertices[3].position.y = 0.0f;
    uiConfig.vertices[3].texture.x  = 1.0f;
    uiConfig.vertices[3].texture.y  = 0.0f;

    // Counter-clockwise
    uiConfig.indices.PushBack(2);
    uiConfig.indices.PushBack(1);
    uiConfig.indices.PushBack(0);
    uiConfig.indices.PushBack(3);
    uiConfig.indices.PushBack(0);
    uiConfig.indices.PushBack(1);

    m_state->uiMeshes[0].LoadFromConfig(m_pSystemsManager, uiConfig);

    // auto rotation = angleAxis(C3D::DegToRad(45.0f), vec3(0.0f, 0.0f, 1.0f));
    // m_uiMeshes[0].transform.Rotate(rotation);
    // m_uiMeshes[0].transform.SetPosition({ 0})

    // TEMP END

    m_state->camera = Cam.GetDefault();
    m_state->camera->SetPosition({ 16.07f, 4.5f, 25.0f });
    m_state->camera->SetEulerRotation({ -20.0f, 51.0f, 0.0f });

    // Set the allocator for the dynamic array that contains our world geometries to our frame allocator
    auto gameFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);
    gameFrameData->worldGeometries.SetAllocator(frameData.frameAllocator);

    // Create, initialize and load our editor gizmo
    if (!m_state->gizmo.Create(m_pSystemsManager))
    {
        m_logger.Error("OnRun() - Failed to create editor gizmo.");
        return false;
    }

    if (!m_state->gizmo.Initialize())
    {
        m_logger.Error("OnRun() - Failed to initialize editor gizmo.");
        return false;
    }

    if (!m_state->gizmo.Load())
    {
        m_logger.Error("OnRun() - Failed to load editor gizmo.");
        return false;
    }

    return true;
}

void TestEnv::OnUpdate(C3D::FrameData& frameData)
{
    // Get our application specific frame data
    auto appFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);

    static u64 allocCount    = 0;
    const u64 prevAllocCount = allocCount;
    allocCount               = Metrics.GetAllocCount();

    const auto deltaTime = frameData.deltaTime;
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

        f64 move_speed = 50.0;
        if (Input.IsKeyDown(C3D::KeyLControl))
        {
            move_speed = 150.0;
        }

        if (Input.IsKeyDown('W'))
        {
            m_state->camera->MoveForward(move_speed * deltaTime);
        }

        // TEMP
        if (Input.IsKeyPressed('T'))
        {
            C3D::Logger::Debug("Swapping Texture");
            C3D::EventContext context = {};
            Event.Fire(C3D::EventCodeDebug0, this, context);
        }
        // TEMP END

        if (Input.IsKeyDown('S'))
        {
            m_state->camera->MoveBackward(move_speed * deltaTime);
        }

        if (Input.IsKeyDown('Q'))
        {
            m_state->camera->MoveLeft(move_speed * deltaTime);
        }

        if (Input.IsKeyDown('E'))
        {
            m_state->camera->MoveRight(move_speed * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeySpace))
        {
            m_state->camera->MoveUp(move_speed * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeyX))
        {
            m_state->camera->MoveDown(move_speed * deltaTime);
        }
    }

    if (!m_state->simpleScene.Update(frameData))
    {
        m_logger.Error("Update() - Failed to update main scene");
    }

    m_state->gizmo.Update();

    if (m_state->simpleScene.GetState() == SceneState::Uninitialized && m_state->reloadState == ReloadState::Unloading)
    {
        m_state->reloadState = ReloadState::Loading;
        m_logger.Info("OnDebugEvent() - Loading Main Scene...");
        LoadTestScene();
    }

    if (m_state->simpleScene.GetState() == SceneState::Loaded)
    {
        // Rotate
        quat rotation = angleAxis(0.2f * static_cast<f32>(deltaTime), vec3(0.0f, 1.0f, 0.0f));

        // m_state->meshes[0].transform.Rotate(rotation);
        // m_state->meshes[1].transform.Rotate(rotation);
        // m_state->meshes[2].transform.Rotate(rotation);

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

    const auto fWidth  = static_cast<f32>(m_state->width);
    const auto fHeight = static_cast<f32>(m_state->height);

    const auto cam = Cam.GetDefault();
    const auto pos = cam->GetPosition();
    const auto rot = cam->GetEulerRotation();

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

    // x, y and z
    // vec3 forward	= m_testCamera->GetForward();
    // vec3 right		= m_testCamera->GetRight();
    // vec3 up			= m_testCamera->GetUp();

    // TODO: Get camera fov, aspect etc.
    // mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), fWidth / fHeight, 1.0f, 1000.0f);
    // mat4 viewMatrix = m_camera->GetViewMatrix();

    // m_cameraFrustum.Create(m_camera->GetPosition(), forward, right, up, fWidth / fHeight, glm::radians(45.0f), 1.0f,
    // 1000.0f);

    C3D::CString<320> buffer;
    buffer.FromFormat(
        "{:<10} : Pos({:.3f}, {:.3f}, {:.3f}) Rot({:.3f}, {:.3f}, {:.3f})\n"
        "{:<10} : Pos({:.2f}, {:.2f}) Buttons({}, {}, {}) Hovered: {}\n"
        "{:<10} : DrawCount: {} FPS: {} FrameTime: {:.3f}ms VSync: {}",
        "Cam", pos.x, pos.y, pos.z, C3D::RadToDeg(rot.x), C3D::RadToDeg(rot.y), C3D::RadToDeg(rot.z), "Mouse", mouseNdcX, mouseNdcY,
        leftButton, middleButton, rightButton, hoveredBuffer, "Renderer", frameData.drawnMeshCount, Metrics.GetFps(),
        Metrics.GetFrameTime(), Renderer.IsFlagEnabled(C3D::FlagVSyncEnabled) ? "Yes" : "No");

    m_state->testText.SetText(buffer.Data());
}

bool TestEnv::OnRender(C3D::RenderPacket& packet, C3D::FrameData& frameData)
{
    // Get our application specific frame data
    auto appFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);
    // Pre-Allocate enough space for 4 views (and default initialize them)
    packet.views.Resize(5);

    // FIXME: Read this from a config
    packet.views[TEST_ENV_VIEW_SKYBOX].view = Views.Get("SKYBOX_VIEW");
    packet.views[TEST_ENV_VIEW_SKYBOX].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[TEST_ENV_VIEW_WORLD].view = Views.Get("WORLD_VIEW");
    packet.views[TEST_ENV_VIEW_WORLD].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[TEST_ENV_VIEW_EDITOR_WORLD].view = Views.Get("EDITOR_WORLD_VIEW");
    packet.views[TEST_ENV_VIEW_EDITOR_WORLD].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[TEST_ENV_VIEW_UI].view = Views.Get("UI_VIEW");
    packet.views[TEST_ENV_VIEW_UI].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[TEST_ENV_VIEW_PICK].view = Views.Get("PICK_VIEW");
    packet.views[TEST_ENV_VIEW_PICK].geometries.SetAllocator(frameData.frameAllocator);

    // This method generate the skybox and world packet for us
    if (!m_state->simpleScene.PopulateRenderPacket(frameData, m_state->camera, static_cast<f32>(m_state->width) / m_state->height, packet))
    {
        m_logger.Error("OnRender() - Failed to populate render packet for simple scene");
        return false;
    }

    // HACK: Inject debug geometries into world packet
    if (m_state->simpleScene.GetState() == SceneState::Loaded)
    {
        for (auto& line : m_state->testLines)
        {
            packet.views[TEST_ENV_VIEW_WORLD].debugGeometries.EmplaceBack(line.GetModel(), line.GetGeometry(), INVALID_ID);
        }
        for (auto& box : m_state->testBoxes)
        {
            packet.views[TEST_ENV_VIEW_WORLD].debugGeometries.EmplaceBack(box.GetModel(), box.GetGeometry(), INVALID_ID);
        }
    }

    // Editor world
    EditorWorldPacketData editorWorldPacket = {};
    editorWorldPacket.gizmo                 = &m_state->gizmo;

    auto& editorWorldViewPacket = packet.views[TEST_ENV_VIEW_EDITOR_WORLD];
    if (!Views.BuildPacket(editorWorldViewPacket.view, frameData.frameAllocator, &editorWorldPacket, &editorWorldViewPacket))
    {
        m_logger.Error("OnRender() - Failed to build packet for view: 'editor world'.");
        return false;
    }

    // UI
    C3D::UIPacketData uiPacket = {};
    uiPacket.meshData.meshes.SetAllocator(frameData.frameAllocator);
    for (auto& mesh : m_state->uiMeshes)
    {
        if (mesh.generation != INVALID_ID_U8)
        {
            uiPacket.meshData.meshes.PushBack(&mesh);
        }
    }
    uiPacket.texts.SetAllocator(frameData.frameAllocator);
    uiPacket.texts.PushBack(&m_state->testText);

    m_pConsole->OnRender(uiPacket);

    auto& uiViewPacket = packet.views[TEST_ENV_VIEW_UI];
    if (!Views.BuildPacket(uiViewPacket.view, frameData.frameAllocator, &uiPacket, &uiViewPacket))
    {
        m_logger.Error("OnRender() - Failed to build packet for view: 'ui'.");
        return false;
    }

    // Pick
    C3D::PickPacketData pickPacket = {};
    pickPacket.uiMeshData          = uiPacket.meshData;
    pickPacket.worldMeshData       = &packet.views[TEST_ENV_VIEW_WORLD].geometries;
    pickPacket.terrainData         = &packet.views[TEST_ENV_VIEW_WORLD].terrainGeometries;
    pickPacket.texts               = uiPacket.texts;

    auto& pickViewPacket = packet.views[TEST_ENV_VIEW_PICK];
    if (!Views.BuildPacket(pickViewPacket.view, frameData.frameAllocator, &pickPacket, &pickViewPacket))
    {
        m_logger.Error("OnRender() - Failed to build packet for view: 'pick'.");
        return false;
    }

    // TEMP END
    return true;
}

void TestEnv::OnResize()
{
    // TEMP
    m_state->testText.SetPosition({ 10, m_state->height - 80, 0 });
    m_state->uiMeshes[0].transform.SetPosition({ m_state->width - 130, 10, 0 });
    // TEMP END
}

void TestEnv::OnShutdown()
{
    // TEMP
    m_state->simpleScene.Unload(true);
    m_state->testText.Destroy();

    for (auto& mesh : m_state->uiMeshes)
    {
        if (mesh.generation != INVALID_ID_U8)
        {
            mesh.Unload();
        }
    }
    // TEMP END
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
            m_logger.Info("OnDebugEvent() - Unloading models...");
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
    m_state->registeredCallbacks.Clear();

    m_pConsole->UnregisterCommand("load_scene");
    m_pConsole->UnregisterCommand("unload_scene");
    m_pConsole->UnregisterCommand("reload_scene");
}

bool TestEnv::ConfigureRenderViews() const
{
    // Skybox View
    RenderViewSkybox* skyboxView = Memory.New<RenderViewSkybox>(C3D::MemoryType::RenderView);
    m_state->renderViews.PushBack(skyboxView);

    // World View
    RenderViewWorld* worldView = Memory.New<RenderViewWorld>(C3D::MemoryType::RenderView);
    m_state->renderViews.PushBack(worldView);

    // Editor World View
    RenderViewEditorWorld* editorWorldView = Memory.New<RenderViewEditorWorld>(C3D::MemoryType::RenderView);
    m_state->renderViews.PushBack(editorWorldView);

    // UI View
    RenderViewUi* uiView = Memory.New<RenderViewUi>(C3D::MemoryType::RenderView);
    m_state->renderViews.PushBack(uiView);

    // Pick View
    RenderViewPick* pickView = Memory.New<RenderViewPick>(C3D::MemoryType::RenderView);
    m_state->renderViews.PushBack(pickView);

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
    if (m_state->dragging)
    {
        return false;
    }

    switch (button)
    {
        case C3D::ButtonLeft:
            f32 x = static_cast<f32>(context.data.i16[1]);
            f32 y = static_cast<f32>(context.data.i16[2]);

            if (m_state->simpleScene.GetState() < SceneState::Loaded)
            {
                return false;
            }

            mat4 view         = m_state->camera->GetViewMatrix();
            vec3 origin       = m_state->camera->GetPosition();
            vec2 viewPortSize = vec2(static_cast<f32>(m_state->width), static_cast<f32>(m_state->height));

            // TODO: Get this from the viewport
            mat4 projection = glm::perspective(C3D::DegToRad(45.0f), viewPortSize.x / viewPortSize.y, 0.1f, 4000.0f);

            C3D::Ray ray = C3D::Ray::FromScreen(vec2(x, y), viewPortSize, origin, view, projection);

            C3D::RayCastResult result;
            if (m_state->simpleScene.RayCast(ray, result))
            {
                f32 closestDistance = C3D::F32_MAX;
                for (auto& hit : result.hits)
                {
                    // Create a debug line
                    C3D::DebugLine3D line;
                    if (!line.Create(m_pSystemsManager, ray.origin, hit.position, nullptr))
                    {
                        m_logger.Error("OnButtonUp() - Failed to create debug line.");
                        return false;
                    }
                    if (!line.Initialize())
                    {
                        m_logger.Error("OnButtonUp() - Failed to initialize debug line.");
                        return false;
                    }
                    if (!line.Load())
                    {
                        m_logger.Error("OnButtonUp() - Failed to load debug line.");
                        return false;
                    }
                    // We set the line to yellow for hits
                    line.SetColor(C3D::YELLOW);

                    m_state->testLines.PushBack(line);

                    C3D::DebugBox3D box;
                    if (!box.Create(m_pSystemsManager, vec3(0.1f), nullptr))
                    {
                        m_logger.Error("OnButtonUp() - Failed to create debug box.");
                        return false;
                    }
                    if (!box.Initialize())
                    {
                        m_logger.Error("OnButtonUp() - Failed to initialize debug box.");
                        return false;
                    }
                    if (!box.Load())
                    {
                        m_logger.Error("OnButtonUp() - Failed to load debug box.");
                        return false;
                    }

                    box.SetPosition(hit.position);

                    m_state->testBoxes.PushBack(box);

                    // Keep track of the hit that is closest
                    if (hit.distance < closestDistance)
                    {
                        closestDistance                  = hit.distance;
                        m_state->selectedObject.uniqueId = hit.uniqueId;
                    }
                }

                const auto id = m_state->selectedObject.uniqueId;
                if (id != INVALID_ID)
                {
                    m_state->selectedObject.transform = m_state->simpleScene.GetTransformById(id);
                    m_logger.Info("OnButtonUp() - Selected object id = {}.", id);
                    m_state->gizmo.SetSelectedObjectTransform(m_state->selectedObject.transform);
                }
            }
            else
            {
                m_logger.Info("OnButtonUp() - Ray MISSED!");

                m_state->selectedObject.transform = nullptr;
                m_state->selectedObject.uniqueId  = INVALID_ID;
                m_state->gizmo.SetSelectedObjectTransform(nullptr);

                // Create a debug line
                C3D::DebugLine3D line;
                if (!line.Create(m_pSystemsManager, origin, origin + (ray.direction * 100.0f), nullptr))
                {
                    m_logger.Error("OnButtonUp() - Failed to create debug line.");
                    return false;
                }
                if (!line.Initialize())
                {
                    m_logger.Error("OnButtonUp() - Failed to initialize debug line.");
                    return false;
                }
                if (!line.Load())
                {
                    m_logger.Error("OnButtonUp() - Failed to load debug line.");
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

        vec2 viewPortSize = vec2(static_cast<f32>(m_state->width), static_cast<f32>(m_state->height));
        vec3 origin       = m_state->camera->GetPosition();
        mat4 view         = m_state->camera->GetViewMatrix();
        // TODO: Get this from the viewport
        mat4 projectionMatrix = glm::perspective(C3D::DegToRad(45.0f), viewPortSize.x / viewPortSize.y, 0.1f, 4000.0f);

        const auto ray = C3D::Ray::FromScreen(vec2(x, y), viewPortSize, origin, view, projectionMatrix);
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
        vec2 viewPortSize = vec2(static_cast<f32>(m_state->width), static_cast<f32>(m_state->height));
        vec3 origin       = m_state->camera->GetPosition();
        mat4 view         = m_state->camera->GetViewMatrix();
        // TODO: Get this from the viewport
        mat4 projectionMatrix = glm::perspective(C3D::DegToRad(45.0f), viewPortSize.x / viewPortSize.y, 0.1f, 4000.0f);

        const auto ray = C3D::Ray::FromScreen(vec2(x, y), viewPortSize, origin, view, projectionMatrix);

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
    /*if (code == C3D::EventCodeDebug0)
    {
        const char* names[3] = { "cobblestone", "paving", "rock" };
        static i8 choice     = 2;

        const char* oldName = names[choice];

        choice++;
        choice %= 3;

        if (C3D::Geometry* g = m_state->meshes[0].geometries[0])
        {
            g->material = Materials.Acquire(names[choice]);
            if (!g->material)
            {
                m_logger.Warn("OnDebugEvent() - No material found! Using default material.");
                g->material = Materials.GetDefault();
            }

            Materials.Release(oldName);
        }

        return true;
    }*/

    if (code == C3D::EventCodeDebug1)
    {
        if (m_state->simpleScene.GetState() == SceneState::Uninitialized)
        {
            m_logger.Info("OnDebugEvent() - Loading Main Scene...");
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

    if (!m_state->simpleScene.Create(m_pSystemsManager, sceneConfig))
    {
        m_logger.Error("LoadTestScene() - Creating SimpleScene failed.");
        return false;
    }

    if (!m_state->simpleScene.Initialize())
    {
        m_logger.Error("LoadTestScene() - Initializing SimpleScene failed.");
        return false;
    }

    m_state->pLights[0] = m_state->simpleScene.GetPointLight("point_light_0");

    if (!m_state->simpleScene.Load())
    {
        m_logger.Error("LoadTestScene() - Loading SimpleScene failed.");
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
    const auto state          = Memory.New<GameState>(C3D::MemoryType::Game);
    state->name               = "TestEnv";
    state->width              = 1280;
    state->height             = 720;
    state->x                  = 2560 / 2 - 1280 / 2;
    state->y                  = 1440 / 2 - 720 / 2;
    state->frameAllocatorSize = MebiBytes(8);
    state->appFrameDataSize   = sizeof(GameFrameData);
    return state;
}
