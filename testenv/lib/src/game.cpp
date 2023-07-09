
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
#include <systems/render_views/render_view_system.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <map>

TestEnv::TestEnv(C3D::ApplicationState* state) : Application(state), m_state(reinterpret_cast<GameState*>(state)) {}

bool TestEnv::OnBoot()
{
    m_logger.Info("OnBoot() - Booting TestEnv");

    m_state->fontConfig.autoRelease = false;

    // Default bitmap font config
    C3D::BitmapFontConfig bmpFontConfig;
    bmpFontConfig.name = "Ubuntu Mono 21px";
    bmpFontConfig.resourceName = "UbuntuMono21px";
    bmpFontConfig.size = 21;
    m_state->fontConfig.bitmapFontConfigs.PushBack(bmpFontConfig);

    // Default system font config
    C3D::SystemFontConfig systemFontConfig;
    systemFontConfig.name = "Noto Sans";
    systemFontConfig.resourceName = "NotoSansCJK";
    systemFontConfig.defaultSize = 20;
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
    // TEMP
    // Create test ui text objects
    if (!m_state->testText.Create(m_pSystemsManager, C3D::UITextType::Bitmap, "Ubuntu Mono 21px", 21,
                                  "Some test text 123,\nyesyes!\n\tKaas!"))
    {
        m_logger.Fatal("OnRun() - Failed to load basic ui bitmap text.");
    }

    m_state->testText.SetPosition({10, 640, 0});

    if (!m_state->simpleScene.Create(m_pSystemsManager))
    {
        m_logger.Error("OnRun() - Failed to create simple scene");
        return false;
    }

    // World meshes
    for (u32 i = 0; i < 10; i++)
    {
        m_state->meshes[i].generation = INVALID_ID_U8;
        m_state->uiMeshes[i].generation = INVALID_ID_U8;
    }

    // Skybox
    C3D::SkyboxConfig skyboxConfig = {};
    skyboxConfig.cubeMapName = "skybox";
    if (!m_state->skybox.Create(m_pSystemsManager, skyboxConfig))
    {
        m_logger.Error("OnRun() - Failed to create skybox");
        return false;
    }

    if (!m_state->simpleScene.AddSkybox(&m_state->skybox))
    {
        m_logger.Error("OnRun() - Failed add skybox to simple scene");
        return false;
    }

    // First cube
    C3D::Mesh* cube0Mesh = &m_state->meshes[0];
    C3D::MeshConfig cube0Config = {};
    cube0Config.geometryConfigs.PushBack(
        Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material"));

    if (!cube0Mesh->Create(m_pSystemsManager, cube0Config))
    {
        m_logger.Error("OnRun() - Failed to create cube mesh 0");
        return false;
    }

    if (!m_state->simpleScene.AddMesh(cube0Mesh))
    {
        m_logger.Error("OnRun() - Failed to add cube0Mesh to simple scene");
        return false;
    }

    // Second cube
    C3D::Mesh* cube1Mesh = &m_state->meshes[1];
    cube1Mesh->transform = C3D::Transform(vec3(0.0f, 10.0f, 0.0f));
    cube1Mesh->transform.SetParent(&cube0Mesh->transform);
    C3D::MeshConfig cube1Config = {};
    cube1Config.geometryConfigs.PushBack(
        Geometric.GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube2", "test_material"));

    if (!cube1Mesh->Create(m_pSystemsManager, cube1Config))
    {
        m_logger.Error("OnRun() - Failed to create cube mesh 2");
        return false;
    }

    if (!m_state->simpleScene.AddMesh(cube1Mesh))
    {
        m_logger.Error("OnRun() - Failed to add cube2Mesh to simple scene");
        return false;
    }

    // Third cube
    C3D::Mesh* cube2Mesh = &m_state->meshes[2];
    cube2Mesh->transform = C3D::Transform(vec3(0.0f, 4.0f, 0.0f));
    cube2Mesh->transform.SetParent(&cube1Mesh->transform);
    C3D::MeshConfig cube2Config = {};
    cube2Config.geometryConfigs.PushBack(
        Geometric.GenerateCubeConfig(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube3", "test_material"));

    if (!cube2Mesh->Create(m_pSystemsManager, cube2Config))
    {
        m_logger.Error("OnRun() - Failed to create cube mesh 2");
        return false;
    }

    if (!m_state->simpleScene.AddMesh(cube2Mesh))
    {
        m_logger.Error("OnRun() - Failed to add cube2Mesh to simple scene");
        return false;
    }

    m_state->dirLight = {vec4(0.4f, 0.4f, 0.2f, 1.0f), vec4(-0.57735f, -0.57735f, -0.57735f, 0.0f)};
    m_state->pLights[0] = {vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(-5.5f, 2.0f, -5.5f, 0.0f), 1.0f, 0.35f, 0.44f, 0.0f};
    m_state->pLights[1] = {vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(5.5f, 2.0f, -5.5f, 0.0f), 1.0f, 0.35f, 0.44f, 0.0f};
    m_state->pLights[2] = {vec4(0.0f, 0.0f, 1.0f, 1.0f), vec4(4.5f, 2.0f, 5.5f, 0.0f), 1.0f, 0.35f, 0.44f, 0.0f};

    if (!m_state->simpleScene.AddDirectionalLight(&m_state->dirLight))
    {
        m_logger.Error("OnRun() - Failed to add directional light to simple scene");
        return false;
    }

    if (!m_state->simpleScene.AddPointLight(&m_state->pLights[0]))
    {
        m_logger.Error("OnRun() - Failed to add point light to simple scene");
        return false;
    }

    if (!m_state->simpleScene.AddPointLight(&m_state->pLights[1]))
    {
        m_logger.Error("OnRun() - Failed to add point light to simple scene");
        return false;
    }

    if (!m_state->simpleScene.AddPointLight(&m_state->pLights[2]))
    {
        m_logger.Error("OnRun() - Failed to add point light to simple scene");
        return false;
    }

    if (!m_state->simpleScene.Initialize())
    {
        m_logger.Error("OnRun() - Failed to initialize simple scene");
        return false;
    }

    if (!m_state->simpleScene.Load())
    {
        m_logger.Error("OnRun() - Failed to load simple scene");
        return false;
    }

    m_state->carMesh = &m_state->meshes[3];
    m_state->carMesh->transform = C3D::Transform({15.0f, 0.0f, 1.0f});

    m_state->sponzaMesh = &m_state->meshes[4];
    m_state->sponzaMesh->transform = C3D::Transform({15.0f, 0.0f, 1.0f}, mat4(1.0f), {0.05f, 0.05f, 0.05f});

    // Load up our test ui geometry
    C3D::UIGeometryConfig uiConfig = {};
    uiConfig.vertices.Resize(4);
    uiConfig.indices.Resize(6);

    uiConfig.materialName = "test_ui_material";
    uiConfig.name = "test_ui_geometry";

    constexpr f32 w = 128.0f;
    constexpr f32 h = 32.0f;

    uiConfig.vertices[0].position.x = 0.0f;
    uiConfig.vertices[0].position.y = 0.0f;
    uiConfig.vertices[0].texture.x = 0.0f;
    uiConfig.vertices[0].texture.y = 0.0f;

    uiConfig.vertices[1].position.x = w;
    uiConfig.vertices[1].position.y = h;
    uiConfig.vertices[1].texture.x = 1.0f;
    uiConfig.vertices[1].texture.y = 1.0f;

    uiConfig.vertices[2].position.x = 0.0f;
    uiConfig.vertices[2].position.y = h;
    uiConfig.vertices[2].texture.x = 0.0f;
    uiConfig.vertices[2].texture.y = 1.0f;

    uiConfig.vertices[3].position.x = w;
    uiConfig.vertices[3].position.y = 0.0f;
    uiConfig.vertices[3].texture.x = 1.0f;
    uiConfig.vertices[3].texture.y = 0.0f;

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
    m_state->camera->SetPosition({10.5f, 5.0f, 9.5f});

    // Set the allocator for the dynamic array that contains our world geometries to our frame allocator
    auto gameFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);
    gameFrameData->worldGeometries.SetAllocator(frameData.frameAllocator);

    return true;
}

void TestEnv::OnUpdate(C3D::FrameData& frameData)
{
    // Get our application specific frame data
    auto appFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);

    static u64 allocCount = 0;
    const u64 prevAllocCount = allocCount;
    allocCount = Metrics.GetAllocCount();

    const auto deltaTime = frameData.deltaTime;
    if (!m_pConsole->IsOpen())
    {
        if (Input.IsKeyPressed(C3D::KeyM))
        {
            C3D::Logger::Info("Allocations: {} of which {} happened this frame", allocCount,
                              allocCount - prevAllocCount);
            Metrics.PrintMemoryUsage(true);
        }

        if (Input.IsKeyUp('p') && Input.WasKeyDown('p'))
        {
            const auto pos = m_state->camera->GetPosition();
            C3D::Logger::Debug("Position({:.2f}, {:.2f}, {:.2f})", pos.x, pos.y, pos.z);
        }

        // Renderer Debug functions
        if (Input.IsKeyPressed('1'))
        {
            C3D::EventContext context = {};
            context.data.i32[0] = C3D::RendererViewMode::Default;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyPressed('2'))
        {
            C3D::EventContext context = {};
            context.data.i32[0] = C3D::RendererViewMode::Lighting;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyPressed('3'))
        {
            C3D::EventContext context = {};
            context.data.i32[0] = C3D::RendererViewMode::Normals;
            Event.Fire(C3D::EventCodeSetRenderMode, this, context);
        }

        if (Input.IsKeyDown('a') || Input.IsKeyDown(C3D::KeyArrowLeft))
        {
            m_state->camera->AddYaw(1.0 * deltaTime);
        }

        if (Input.IsKeyDown('d') || Input.IsKeyDown(C3D::KeyArrowRight))
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

        static constexpr f64 temp_move_speed = 50.0;

        if (Input.IsKeyDown('w'))
        {
            m_state->camera->MoveForward(temp_move_speed * deltaTime);
        }

        // TEMP
        if (Input.IsKeyPressed('t'))
        {
            C3D::Logger::Debug("Swapping Texture");
            C3D::EventContext context = {};
            Event.Fire(C3D::EventCodeDebug0, this, context);
        }
        // TEMP END

        if (Input.IsKeyDown('s'))
        {
            m_state->camera->MoveBackward(temp_move_speed * deltaTime);
        }

        if (Input.IsKeyDown('q'))
        {
            m_state->camera->MoveLeft(temp_move_speed * deltaTime);
        }

        if (Input.IsKeyDown('e'))
        {
            m_state->camera->MoveRight(temp_move_speed * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeySpace))
        {
            m_state->camera->MoveUp(temp_move_speed * deltaTime);
        }

        if (Input.IsKeyDown(C3D::KeyX))
        {
            m_state->camera->MoveDown(temp_move_speed * deltaTime);
        }
    }

    // Rotate
    quat rotation = angleAxis(0.2f * static_cast<f32>(deltaTime), vec3(0.0f, 1.0f, 0.0f));
    quat rotation2 = angleAxis(-0.2f * static_cast<f32>(deltaTime), vec3(0.0f, 1.0f, 0.0f));

    m_state->meshes[0].transform.Rotate(rotation);
    m_state->meshes[1].transform.Rotate(rotation2);
    m_state->meshes[2].transform.Rotate(rotation);

    if (m_state->meshes[3].generation != INVALID_ID_U8)
    {
        // m_state->meshes[3].transform.Rotate(rotation);
    }

    const auto absTime = OS.GetAbsoluteTime();
    const auto sinTime = (C3D::Sin(absTime) + 1) / 2;  // 0 - > 1
    const auto sinTime2 = C3D::Sin(absTime);           // -1 -> 1

    const C3D::HSV hsv(sinTime, 1.0f, 1.0f);
    const auto rgba = C3D::HsvToRgba(hsv);

    m_state->pLights[0].color = vec4(rgba.r, rgba.g, rgba.b, rgba.a);
    m_state->pLights[0].position.x += sinTime2;
    m_state->pLights[0].linear = 0.5f;
    m_state->pLights[0].quadratic = 0.2f;

    if (m_state->pLights[0].position.x < 20.0f) m_state->pLights[0].position.x = 20.0f;
    if (m_state->pLights[0].position.x > 60.0f) m_state->pLights[0].position.x = 60.0f;

    const auto fWidth = static_cast<f32>(m_state->width);
    const auto fHeight = static_cast<f32>(m_state->height);

    const auto cam = Cam.GetDefault();
    const auto pos = cam->GetPosition();
    const auto rot = cam->GetEulerRotation();

    const auto mouse = Input.GetMousePosition();
    // Convert to NDC
    const auto mouseNdcX = C3D::RangeConvert(static_cast<f32>(mouse.x), 0.0f, fWidth, -1.0f, 1.0f);
    const auto mouseNdcY = C3D::RangeConvert(static_cast<f32>(mouse.y), 0.0f, fHeight, -1.0f, 1.0f);

    const auto leftButton = Input.IsButtonDown(C3D::Buttons::ButtonLeft);
    const auto middleButton = Input.IsButtonDown(C3D::Buttons::ButtonMiddle);
    const auto rightButton = Input.IsButtonDown(C3D::Buttons::ButtonRight);

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
        "{:<10} : DrawCount: {} FPS: {} FrameTime: {:.3f}ms VSync: {} AbsTime: {:.3f}",
        "Cam", pos.x, pos.y, pos.z, C3D::RadToDeg(rot.x), C3D::RadToDeg(rot.y), C3D::RadToDeg(rot.z), "Mouse",
        mouseNdcX, mouseNdcY, leftButton, middleButton, rightButton, hoveredBuffer, "Renderer",
        frameData.drawnMeshCount, Metrics.GetFps(), Metrics.GetFrameTime(),
        Renderer.IsFlagEnabled(C3D::FlagVSyncEnabled) ? "Yes" : "No", absTime);

    m_state->testText.SetText(buffer.Data());
}

bool TestEnv::OnRender(C3D::RenderPacket& packet, C3D::FrameData& frameData)
{
    // Get our application specific frame data
    auto appFrameData = static_cast<GameFrameData*>(frameData.applicationFrameData);
    // Pre-Allocate enough space for 4 views (and default initialize them)
    packet.views.Resize(4);

    // FIXME: Read this from a config
    packet.views[0].view = Views.Get("skybox");
    packet.views[0].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[1].view = Views.Get("world");
    packet.views[1].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[2].view = Views.Get("ui");
    packet.views[2].geometries.SetAllocator(frameData.frameAllocator);

    packet.views[3].view = Views.Get("pick");
    packet.views[3].geometries.SetAllocator(frameData.frameAllocator);

    if (!m_state->simpleScene.PopulateRenderPacket(frameData, packet))
    {
        m_logger.Error("OnRender() - Failed to populate render packet for simple scene");
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

    if (!Views.BuildPacket(Views.Get("ui"), frameData.frameAllocator, &uiPacket, &packet.views[2]))
    {
        m_logger.Error("Failed to build packet for view: 'ui'");
        return false;
    }

    // Pick
    C3D::PickPacketData pickPacket = {};
    pickPacket.uiMeshData = uiPacket.meshData;
    // TODO: This index is hardcoded currently
    pickPacket.worldMeshData = &packet.views[1].geometries;
    pickPacket.texts = uiPacket.texts;
    if (!Views.BuildPacket(Views.Get("pick"), frameData.frameAllocator, &pickPacket, &packet.views[3]))
    {
        m_logger.Error("Failed to build packet for view: 'pick'");
        return false;
    }

    // TEMP END
    return true;
}

void TestEnv::OnResize()
{
    // TEMP
    m_state->testText.SetPosition({10, m_state->height - 80, 0});
    m_state->uiMeshes[0].transform.SetPosition({m_state->width - 130, 10, 0});
    // TEMP END
}

void TestEnv::OnShutdown()
{
    // TEMP
    m_state->skybox.Destroy();
    m_state->testText.Destroy();

    for (auto& mesh : m_state->meshes)
    {
        if (mesh.generation != INVALID_ID_U8)
        {
            mesh.Unload();
        }
    }

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
    m_state->m_registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeDebug1, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnDebugEvent(code, sender, context);
    });
    m_state->m_registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeDebug2, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnDebugEvent(code, sender, context);
    });
    m_state->m_registeredCallbacks.PushBack(cb);

    cb = Event.Register(C3D::EventCodeObjectHoverIdChanged,
                        [this](const u16 code, void* sender, const C3D::EventContext& context) {
                            return OnEvent(code, sender, context);
                        });
    m_state->m_registeredCallbacks.PushBack(cb);

    m_pConsole->RegisterCommand("load_scene", [this](const C3D::DynamicArray<C3D::ArgName>&, C3D::String&) {
        Event.Fire(C3D::EventCodeDebug1, this, {});
        return true;
    });

    m_pConsole->RegisterCommand("unload_scene", [this](const C3D::DynamicArray<C3D::ArgName>&, C3D::String&) {
        Event.Fire(C3D::EventCodeDebug2, this, {});
        return true;
    });
}

void TestEnv::OnLibraryUnload()
{
    for (auto& cb : m_state->m_registeredCallbacks)
    {
        Event.Unregister(cb);
    }
    m_state->m_registeredCallbacks.Clear();

    m_pConsole->UnregisterCommand("load_scene");
    m_pConsole->UnregisterCommand("unload_scene");
}

bool TestEnv::ConfigureRenderViews() const
{
    // Skybox View
    C3D::RenderViewConfig skyboxConfig{};
    skyboxConfig.type = C3D::RenderViewKnownType::Skybox;
    skyboxConfig.width = 1280;
    skyboxConfig.height = 720;
    skyboxConfig.name = "skybox";
    skyboxConfig.passCount = 1;
    skyboxConfig.viewMatrixSource = C3D::RenderViewViewMatrixSource::SceneCamera;

    C3D::RenderPassConfig skyboxPass{};
    skyboxPass.name = "RenderPass.Builtin.Skybox";
    skyboxPass.renderArea = {0, 0, 1280, 720};
    skyboxPass.clearColor = {0, 0, 0.2f, 1.0f};
    skyboxPass.clearFlags = C3D::ClearColorBuffer;
    skyboxPass.depth = 1.0f;
    skyboxPass.stencil = 0;

    C3D::RenderTargetAttachmentConfig skyboxTargetAttachment = {};
    skyboxTargetAttachment.type = C3D::RenderTargetAttachmentType::Color;
    skyboxTargetAttachment.source = C3D::RenderTargetAttachmentSource::Default;
    skyboxTargetAttachment.loadOperation = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    skyboxTargetAttachment.storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    skyboxTargetAttachment.presentAfter = false;

    skyboxPass.target.attachments.PushBack(skyboxTargetAttachment);
    skyboxPass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    skyboxConfig.passes.PushBack(skyboxPass);

    m_state->renderViews.PushBack(skyboxConfig);

    // World View
    C3D::RenderViewConfig worldConfig{};
    worldConfig.type = C3D::RenderViewKnownType::World;
    worldConfig.width = 1280;
    worldConfig.height = 720;
    worldConfig.name = "world";
    worldConfig.passCount = 1;
    worldConfig.viewMatrixSource = C3D::RenderViewViewMatrixSource::SceneCamera;

    C3D::RenderPassConfig worldPass;
    worldPass.name = "RenderPass.Builtin.World";
    worldPass.renderArea = {0, 0, 1280, 720};
    worldPass.clearColor = {0, 0, 0.2f, 1.0f};
    worldPass.clearFlags = C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
    worldPass.depth = 1.0f;
    worldPass.stencil = 0;

    C3D::RenderTargetAttachmentConfig worldTargetAttachments[2]{};
    worldTargetAttachments[0].type = C3D::RenderTargetAttachmentType::Color;
    worldTargetAttachments[0].source = C3D::RenderTargetAttachmentSource::Default;
    worldTargetAttachments[0].loadOperation = C3D::RenderTargetAttachmentLoadOperation::Load;
    worldTargetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    worldTargetAttachments[0].presentAfter = false;

    worldTargetAttachments[1].type = C3D::RenderTargetAttachmentType::Depth;
    worldTargetAttachments[1].source = C3D::RenderTargetAttachmentSource::Default;
    worldTargetAttachments[1].loadOperation = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    worldTargetAttachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    worldTargetAttachments[1].presentAfter = false;

    worldPass.target.attachments.PushBack(worldTargetAttachments[0]);
    worldPass.target.attachments.PushBack(worldTargetAttachments[1]);
    worldPass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    worldConfig.passes.PushBack(worldPass);

    m_state->renderViews.PushBack(worldConfig);

    // UI View
    C3D::RenderViewConfig uiViewConfig{};
    uiViewConfig.type = C3D::RenderViewKnownType::UI;
    uiViewConfig.width = 1280;
    uiViewConfig.height = 720;
    uiViewConfig.name = "ui";
    uiViewConfig.passCount = 1;
    uiViewConfig.viewMatrixSource = C3D::RenderViewViewMatrixSource::SceneCamera;

    C3D::RenderPassConfig uiPass;
    uiPass.name = "RenderPass.Builtin.UI";
    uiPass.renderArea = {0, 0, 1280, 720};
    uiPass.clearColor = {0, 0, 0.2f, 1.0f};
    uiPass.clearFlags = C3D::ClearNone;
    uiPass.depth = 1.0f;
    uiPass.stencil = 0;

    C3D::RenderTargetAttachmentConfig uiAttachment = {};
    uiAttachment.type = C3D::RenderTargetAttachmentType::Color;
    uiAttachment.source = C3D::RenderTargetAttachmentSource::Default;
    uiAttachment.loadOperation = C3D::RenderTargetAttachmentLoadOperation::Load;
    uiAttachment.storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    uiAttachment.presentAfter = true;

    uiPass.target.attachments.PushBack(uiAttachment);
    uiPass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    uiViewConfig.passes.PushBack(uiPass);

    m_state->renderViews.PushBack(uiViewConfig);

    // Pick View
    C3D::RenderViewConfig pickViewConfig = {};
    pickViewConfig.type = C3D::RenderViewKnownType::Pick;
    pickViewConfig.width = 1280;
    pickViewConfig.height = 720;
    pickViewConfig.name = "pick";
    pickViewConfig.passCount = 2;
    pickViewConfig.viewMatrixSource = C3D::RenderViewViewMatrixSource::SceneCamera;

    C3D::RenderPassConfig pickPasses[2] = {};

    pickPasses[0].name = "RenderPass.Builtin.WorldPick";
    pickPasses[0].renderArea = {0, 0, 1280, 720};
    // HACK: Clear to white for better visibility (should be 0 since it's invalid id)
    pickPasses[0].clearColor = {1.0f, 1.0f, 1.0f, 1.0f};
    pickPasses[0].clearFlags =
        C3D::RenderPassClearFlags::ClearColorBuffer | C3D::RenderPassClearFlags::ClearDepthBuffer;
    pickPasses[0].depth = 1.0f;
    pickPasses[0].stencil = 0;

    C3D::RenderTargetAttachmentConfig worldPickTargetAttachments[2];
    worldPickTargetAttachments[0].type = C3D::RenderTargetAttachmentType::Color;
    worldPickTargetAttachments[0].source = C3D::RenderTargetAttachmentSource::View;
    worldPickTargetAttachments[0].loadOperation = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    worldPickTargetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    worldPickTargetAttachments[0].presentAfter = false;

    worldPickTargetAttachments[1].type = C3D::RenderTargetAttachmentType::Depth;
    worldPickTargetAttachments[1].source = C3D::RenderTargetAttachmentSource::View;
    worldPickTargetAttachments[1].loadOperation = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    worldPickTargetAttachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    worldPickTargetAttachments[1].presentAfter = false;

    pickPasses[0].target.attachments.PushBack(worldPickTargetAttachments[0]);
    pickPasses[0].target.attachments.PushBack(worldPickTargetAttachments[1]);
    pickPasses[0].renderTargetCount = 1;

    pickPasses[1].name = "RenderPass.Builtin.UIPick";
    pickPasses[1].renderArea = {0, 0, 1280, 720};
    pickPasses[1].clearColor = {1.0f, 1.0f, 1.0f, 1.0f};
    pickPasses[1].clearFlags = C3D::RenderPassClearFlags::ClearNone;
    pickPasses[1].depth = 1.0f;
    pickPasses[1].stencil = 0;

    C3D::RenderTargetAttachmentConfig uiPickTargetAttachment{};
    uiPickTargetAttachment.type = C3D::RenderTargetAttachmentType::Color;
    uiPickTargetAttachment.source = C3D::RenderTargetAttachmentSource::View;
    uiPickTargetAttachment.loadOperation = C3D::RenderTargetAttachmentLoadOperation::Load;
    uiPickTargetAttachment.storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    uiPickTargetAttachment.presentAfter = false;

    pickPasses[1].target.attachments.PushBack(uiPickTargetAttachment);
    pickPasses[1].renderTargetCount = 1;

    pickViewConfig.passes.PushBack(pickPasses[0]);
    pickViewConfig.passes.PushBack(pickPasses[1]);

    m_state->renderViews.PushBack(pickViewConfig);

    return true;
}

bool TestEnv::OnEvent(const u16 code, void*, const C3D::EventContext& context) const
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

bool TestEnv::OnDebugEvent(const u16 code, void*, const C3D::EventContext&) const
{
    if (code == C3D::EventCodeDebug0)
    {
        const char* names[3] = {"cobblestone", "paving", "rock"};
        static i8 choice = 2;

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
    }

    if (code == C3D::EventCodeDebug1)
    {
        if (!m_state->modelsLoaded)
        {
            m_logger.Info("OnDebugEvent() - Loading models...");

            C3D::MeshConfig falconConfig = {};
            falconConfig.resourceName = "falcon";

            if (!m_state->carMesh->Create(m_pSystemsManager, falconConfig))
            {
                m_logger.Error("OnDebugEvent() - Failed to create falcon mesh");
                return false;
            }
            if (!m_state->simpleScene.AddMesh(m_state->carMesh))
            {
                m_logger.Error("OnDebugEvent() - Failed to add falcon mesh");
            }

            C3D::MeshConfig sponzaConfig = {};
            sponzaConfig.resourceName = "sponza";

            if (!m_state->sponzaMesh->Create(m_pSystemsManager, sponzaConfig))
            {
                m_logger.Error("OnDebugEvent() - Failed to create sponza mesh");
            }

            if (!m_state->simpleScene.AddMesh(m_state->sponzaMesh))
            {
                m_logger.Error("OnDebugEvent() - Failed to add sponza mesh");
            }

            m_state->modelsLoaded = true;
        }

        return true;
    }

    if (code == C3D::EventCodeDebug2)
    {
        if (m_state->modelsLoaded)
        {
            m_logger.Info("OnDebugEvent() - Unloading models...");

            m_state->simpleScene.Unload();
        }

        m_state->modelsLoaded = false;
    }

    return false;
}

C3D::Application* CreateApplication(C3D::ApplicationState* state)
{
    return Memory.New<TestEnv>(C3D::MemoryType::Game, state);
}

C3D::ApplicationState* CreateApplicationState()
{
    const auto state = Memory.New<GameState>(C3D::MemoryType::Game);
    state->name = "TestEnv";
    state->width = 1280;
    state->height = 720;
    state->x = 2560 / 2 - 1280 / 2;
    state->y = 1440 / 2 - 720 / 2;
    state->frameAllocatorSize = MebiBytes(8);
    state->appFrameDataSize = sizeof(GameFrameData);
    return state;
}
