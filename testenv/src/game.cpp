
#include "game.h"

#include <glm/gtx/matrix_decompose.hpp>

#include <core/identifier.h>
#include <core/logger.h>
#include <core/input.h>
#include <core/events/event.h>
#include <core/events/event_context.h>
#include <core/metrics/metrics.h>
#include <core/console/console.h>

#include <containers/cstring.h>

#include <services/system_manager.h>

#include <systems/camera_system.h>
#include <systems/shader_system.h>
#include <systems/texture_system.h>

#include <renderer/renderer_types.h>

#include "math/frustum.h"
#include "systems/render_view_system.h"

TestEnv::TestEnv(const C3D::ApplicationConfig& config)
	: Engine(config), m_camera(nullptr), m_testCamera(nullptr), m_carMesh(nullptr), m_sponzaMesh(nullptr),
	  m_primitiveMeshes{}, m_modelsLoaded(false), m_planes(), m_hoveredObjectId(0)
{}

bool TestEnv::OnBoot()
{
	m_logger.Info("OnBoot() - Booting TestEnv");

	// Setup the frame allocator
	m_frameAllocator.Create("FRAME_ALLOCATOR", MebiBytes(64));

	m_config.fontConfig.autoRelease = false;

	// Default bitmap font config
	C3D::BitmapFontConfig bmpFontConfig;
	bmpFontConfig.name = "Ubuntu Mono 21px";
	bmpFontConfig.resourceName = "UbuntuMono21px";
	bmpFontConfig.size = 21;
	m_config.fontConfig.bitmapFontConfigs.PushBack(bmpFontConfig);

	// Default system font config
	C3D::SystemFontConfig systemFontConfig;
	systemFontConfig.name = "Noto Sans";
	systemFontConfig.resourceName = "NotoSansCJK";
	systemFontConfig.defaultSize = 20;
	m_config.fontConfig.systemFontConfigs.PushBack(systemFontConfig);

	m_config.fontConfig.maxBitmapFontCount = 101;
	m_config.fontConfig.maxSystemFontCount = 101;

	// Config our render views. TODO: Read this from a file
	if (!ConfigureRenderViews())
	{
		m_logger.Error("OnBoot() - Failed to create render views.");
		return false;
	}

	return true;
}

bool TestEnv::OnCreate()
{
	// TEMP
	// Create test ui text objects
	if (!m_testText.Create(C3D::UITextType::Bitmap, "Ubuntu Mono 21px", 21, "Some test text 123,\nyesyes!\n\tKaas!"))
	{
		m_logger.Fatal("Failed to load basic ui bitmap text.");
	}

	m_testText.SetPosition({ 10, 640, 0 });
	
	Console->OnInit();
	Console->RegisterCommand("exit", this, &TestEnv::ShutdownCommand);
	Console->RegisterCommand("vsync", &VSyncCommand);

	auto& cubeMap = m_skybox.cubeMap;
	cubeMap.magnifyFilter = cubeMap.minifyFilter = C3D::TextureFilter::ModeLinear;
	cubeMap.repeatU = cubeMap.repeatV = cubeMap.repeatW = C3D::TextureRepeat::ClampToEdge;
	cubeMap.use = C3D::TextureUse::CubeMap;
	if (!Renderer.AcquireTextureMapResources(&cubeMap))
	{
		m_logger.Fatal("Unable to acquire resources for cube map texture.");
		return false;
	}
	cubeMap.texture = Textures.AcquireCube("skybox", true);

	C3D::GeometryConfig skyboxCubeConfig = Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "skybox_cube", "");
	skyboxCubeConfig.materialName[0] = '\0';

	m_skybox.g = Geometric.AcquireFromConfig(skyboxCubeConfig, true);
	m_skybox.frameNumber = INVALID_ID_U64;

	Geometric.DisposeConfig(&skyboxCubeConfig);

	C3D::Shader* skyboxShader = Shaders.Get("Shader.Builtin.Skybox");
	C3D::TextureMap* maps[1] = { &m_skybox.cubeMap };
	if (!Renderer.AcquireShaderInstanceResources(skyboxShader, maps, &m_skybox.instanceId))
	{
		m_logger.Fatal("Unable to acquire shader resources for skybox texture.");
		return false;
	}

	// World meshes
	for (u32 i = 0; i < 10; i++)
	{
		m_meshes[i].generation = INVALID_ID_U8;
		m_uiMeshes[i].generation = INVALID_ID_U8;
	}

	u8 meshCount = 0;

	// Our First cube
	// Load up a cube configuration, and load geometry for it
	C3D::GeometryConfig gConfig = Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");

	C3D::Mesh* cubeMesh = &m_meshes[meshCount];
	cubeMesh->geometries.PushBack(Geometric.AcquireFromConfig(gConfig, true));
	cubeMesh->transform = C3D::Transform();
	cubeMesh->generation = 0;
	cubeMesh->uniqueId = C3D::Identifier::GetNewId(cubeMesh);

	meshCount++; 

	// Cleanup the allocations that we just did
	Geometric.DisposeConfig(&gConfig);

	// A second cube
	// Load up a cube configuration, and load geometry for it
	gConfig = Geometric.GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube_2", "test_material");

	C3D::Mesh* cubeMesh2 = &m_meshes[meshCount];
	cubeMesh2->geometries.PushBack(Geometric.AcquireFromConfig(gConfig, true));
	cubeMesh2->transform = C3D::Transform(vec3(0.0f, 10.0f, 0.0f));
	cubeMesh2->transform.SetParent(&cubeMesh->transform);
	cubeMesh2->generation = 0;
	cubeMesh2->uniqueId = C3D::Identifier::GetNewId(cubeMesh2);

	meshCount++;

	// Cleanup the other allocations that we just did
	Geometric.DisposeConfig(&gConfig);

	// A third cube
	gConfig = Geometric.GenerateCubeConfig(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube_3", "test_material");

	C3D::Mesh* cubeMesh3 = &m_meshes[meshCount];
	cubeMesh3->geometries.PushBack(Geometric.AcquireFromConfig(gConfig, true));
	cubeMesh3->transform = C3D::Transform(vec3(0.0f, 5.0f, 0.0f));
	cubeMesh3->transform.SetParent(&cubeMesh2->transform);
	cubeMesh3->generation = 0;
	cubeMesh3->uniqueId = C3D::Identifier::GetNewId(cubeMesh3);
	 
	meshCount++;

	// Cleanup the other allocations that we just did
	Geometric.DisposeConfig(&gConfig);

	m_carMesh = &m_meshes[meshCount];
	m_carMesh->transform = C3D::Transform({ 15.0f, 0.0f, 1.0f });
	m_carMesh->uniqueId = C3D::Identifier::GetNewId(m_carMesh);
	meshCount++;

	m_sponzaMesh = &m_meshes[meshCount];
	m_sponzaMesh->transform = C3D::Transform({ 15.0f, 0.0f, 1.0f }, mat4(1.0f), { 0.05f, 0.05f, 0.05f });
	m_sponzaMesh->uniqueId = C3D::Identifier::GetNewId(m_sponzaMesh);

	meshCount++;

	// Load up our test ui geometry
	C3D::GeometryConfig<C3D::Vertex2D, u32> uiConfig{};
	uiConfig.vertices = C3D::DynamicArray<C3D::Vertex2D>();
	uiConfig.vertices.Resize(4);

	uiConfig.indices = C3D::DynamicArray<u32>(6);

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

	m_uiMeshes[0].uniqueId = C3D::Identifier::GetNewId(&m_uiMeshes[0]);
	m_uiMeshes[0].geometries.PushBack(Geometric.AcquireFromConfig(uiConfig, true));
	m_uiMeshes[0].generation = 0;

	m_primitiveRenderer.OnCreate();

	//auto rotation = angleAxis(C3D::DegToRad(45.0f), vec3(0.0f, 0.0f, 1.0f));
	//m_uiMeshes[0].transform.Rotate(rotation);
	//m_uiMeshes[0].transform.SetPosition({ 0})

	// TEMP END

	m_camera = Cam.GetDefault();
	m_testCamera = new C3D::Camera();
	m_testCamera->SetPosition({ 10.5f, 5.0f, 9.5f });


	m_camera->SetPosition({ 10.5f, 5.0f, 9.5f });

	// TEMP
	Event.Register(C3D::SystemEventCode::Debug0, this, &TestEnv::OnDebugEvent);
	Event.Register(C3D::SystemEventCode::Debug1, this, &TestEnv::OnDebugEvent);
	Event.Register(C3D::SystemEventCode::ObjectHoverIdChanged, this, &TestEnv::OnEvent);
	// TEMP END

	for (auto& mesh : m_primitiveMeshes)
	{
		mesh = nullptr;
	}

	//m_primitiveMeshes[0] = m_primitiveRenderer.AddBox({ 0, 0, 0 }, { 5, 5, 5 });
	//m_primitiveMeshes[0]->transform.SetParent(&cubeMesh->transform);

	m_frameData.worldGeometries.SetAllocator(&m_frameAllocator);

	return true;
}

void TestEnv::OnUpdate(const f64 deltaTime)
{
	// Destroy all our frame data
	m_frameData.worldGeometries.Destroy();
	// Reset our frame allocator (freeing all memory used previous frame)
	m_frameAllocator.FreeAll();

	static u64 allocCount = 0;
	const u64 prevAllocCount = allocCount;
	allocCount = Metrics.GetAllocCount();

	if (!Console->IsOpen())
	{
		if (Input.IsKeyPressed(C3D::KeyM))
		{
			C3D::Logger::Debug("Allocations: {} of which {} happened this frame", allocCount, allocCount - prevAllocCount);
			Metrics.PrintMemoryUsage(true);
		}

		if (Input.IsKeyUp('p') && Input.WasKeyDown('p'))
		{
			const auto pos = m_camera->GetPosition();
			C3D::Logger::Debug("Position({:.2f}, {:.2f}, {:.2f})", pos.x, pos.y, pos.z);
		}

		if (Input.IsKeyPressed('v'))
		{
			const auto current = Renderer.IsFlagEnabled(C3D::FlagVSyncEnabled);
			Renderer.SetFlagEnabled(C3D::FlagVSyncEnabled, !current);
		}

		// Renderer Debug functions
		if (Input.IsKeyPressed('1'))
		{
			C3D::EventContext context = {};
			context.data.i32[0] = C3D::RendererViewMode::Default;
			Event.Fire(C3D::SystemEventCode::SetRenderMode, this, context);
		}

		if (Input.IsKeyPressed('2'))
		{
			C3D::EventContext context = {};
			context.data.i32[0] = C3D::RendererViewMode::Lighting;
			Event.Fire(C3D::SystemEventCode::SetRenderMode, this, context);
		}

		if (Input.IsKeyPressed('3'))
		{
			C3D::EventContext context = {};
			context.data.i32[0] = C3D::RendererViewMode::Normals;
			Event.Fire(C3D::SystemEventCode::SetRenderMode, this, context);
		}

		if (Input.IsKeyDown('a') || Input.IsKeyDown(C3D::KeyArrowLeft))
		{
			m_camera->AddYaw(1.0 * deltaTime);
		}

		if (Input.IsKeyDown('d') || Input.IsKeyDown(C3D::KeyArrowRight))
		{
			m_camera->AddYaw(-1.0 * deltaTime);
		}

		if (Input.IsKeyDown(C3D::KeyArrowUp))
		{
			m_camera->AddPitch(1.0 * deltaTime);
		}

		if (Input.IsKeyDown(C3D::KeyArrowDown))
		{
			m_camera->AddPitch(-1.0 * deltaTime);
		}

		static constexpr f64 temp_move_speed = 50.0;

		if (Input.IsKeyDown('w'))
		{
			m_camera->MoveForward(temp_move_speed * deltaTime);
		}

		// TEMP
		if (Input.IsKeyPressed('t'))
		{
			C3D::Logger::Debug("Swapping Texture");
			C3D::EventContext context = {};
			Event.Fire(C3D::SystemEventCode::Debug0, this, context);
		}

		if (Input.IsKeyPressed('l'))
		{
			C3D::EventContext context = {};
			Event.Fire(C3D::SystemEventCode::Debug1, this, context);
		}
		// TEMP END

		if (Input.IsKeyDown('s'))
		{
			m_camera->MoveBackward(temp_move_speed * deltaTime);
		}

		if (Input.IsKeyDown('q'))
		{
			m_camera->MoveLeft(temp_move_speed * deltaTime);
		}

		if (Input.IsKeyDown('e'))
		{
			m_camera->MoveRight(temp_move_speed * deltaTime);
		}

		if (Input.IsKeyDown(C3D::KeySpace))
		{
			m_camera->MoveUp(temp_move_speed * deltaTime);
		}

		if (Input.IsKeyDown(C3D::KeyX))
		{
			m_camera->MoveDown(temp_move_speed * deltaTime);
		}
	}

	Console->OnUpdate();

	// Rotate 
	quat rotation = angleAxis(0.5f * static_cast<f32>(deltaTime), vec3(0.0f, 1.0f, 0.0f));
	m_meshes[0].transform.Rotate(rotation);
	m_meshes[1].transform.Rotate(rotation);
	m_meshes[2].transform.Rotate(rotation);

	if (m_meshes[3].generation != INVALID_ID_U8)
	{
		m_meshes[3].transform.Rotate(rotation);
	}

	const auto fWidth = static_cast<f32>(m_state.width);
	const auto fHeight = static_cast<f32>(m_state.height);

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
	if (m_hoveredObjectId != INVALID_ID)
	{
		hoveredBuffer.FromFormat("{}", m_hoveredObjectId);
	}
	else
	{
		hoveredBuffer = "None";
	}

	// x, y and z
	vec3 forward	= m_testCamera->GetForward();
	vec3 right		= m_testCamera->GetRight();
	vec3 up			= m_testCamera->GetUp();

	// TODO: Get camera fov, aspect etc.
	mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), fWidth / fHeight, 1.0f, 1000.0f);
	mat4 viewMatrix = m_camera->GetViewMatrix();

	m_cameraFrustum.Create(m_camera->GetPosition(), forward, right, up, fWidth / fHeight, glm::radians(45.0f), 1.0f, 1000.0f);

	//static C3D::Plane3D p = { { 5.0f, 10.0f, 1.0f }, { 0.5f, 1.0f, 0.f } };
	/*m_planes[0] = m_primitiveRenderer.AddPlane(m_cameraFrustum.sides[0], { 1, 0, 0, 1 });
	m_planes[1] = m_primitiveRenderer.AddPlane(m_cameraFrustum.sides[1], { 0, 1, 0, 1 });
	m_planes[2] = m_primitiveRenderer.AddPlane(m_cameraFrustum.sides[2], { 0, 0, 1, 1 });
	m_planes[3] = m_primitiveRenderer.AddPlane(m_cameraFrustum.sides[3], { 1, 1, 0, 1 });
	m_planes[4] = m_primitiveRenderer.AddPlane(m_cameraFrustum.sides[4], { 1, 1, 1, 1 });
	m_planes[5] = m_primitiveRenderer.AddPlane(m_cameraFrustum.sides[5], { 0, 1, 1, 1 });*/

	// Reserve a reasonable amount of space for our world geometries
	m_frameData.worldGeometries.Reserve(512);

	u32 drawCount = 0;
	u32 count = 0;

	for (auto& mesh : m_meshes)
	{
		//auto meshSet = false;

		if (mesh.generation != INVALID_ID_U8)
		{
			mat4 model = mesh.transform.GetWorld();
			
			for (const auto geometry : mesh.geometries)
			{
				m_frameData.worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId);
				drawCount++;

				/*
				// Bounding sphere calculations
				vec3 extentsMin = vec4(geometry->extents.min, 1.0f) * model;
				vec3 extentsMax = vec4(geometry->extents.max, 1.0f) * model;

				f32 min = C3D::Min(extentsMin.x, extentsMin.y, extentsMin.z);
				f32 max = C3D::Max(extentsMax.x, extentsMax.y, extentsMax.z);
				f32 diff = C3D::Abs(max - min);
				f32 radius = diff * 0.5f;

				// Translate / Scale the center
				const vec3 center = vec4(geometry->center, 1.0f) * model;

				if (m_cameraFrustum.IntersectsWithSphere({ center, radius }))
				{
					m_frameData.worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId);
					drawCount++;
				}

				// AABB Calculation
				const vec3 extentsMin = vec4(geometry->extents.min, 1.0f);
				const vec3 extentsMax = vec4(geometry->extents.max, 1.0f);
				const vec3 center = vec4(geometry->center, 1.0f);

				const vec3 halfExtents = 
				{
					C3D::Abs(extentsMax.x - center.x),
					C3D::Abs(extentsMax.y - center.y),
					C3D::Abs(extentsMax.z - center.z),
				};

				if (!m_hasPrimitives[count])
				{
					// Only add primitives for the mesh once
					auto pMesh = m_primitiveRenderer.AddBox(geometry->center, halfExtents);
					pMesh->transform.SetParent(&mesh.transform);

					m_primitiveMeshes[m_primitiveMeshCount] = pMesh;
					m_primitiveMeshCount++;
					meshSet = true;
				}

				if (m_cameraFrustum.IntersectsWithAABB({ geometry->center, halfExtents }))
				{
					m_frameData.worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId);
					drawCount++;
				}*/
			}
		}

		/*
		if (meshSet)
		{
			m_hasPrimitives[count] = true;
		}*/
		count++;
	}

	C3D::CString<256> buffer;
	buffer.FromFormat(
		"{:<10} : Pos({:.3f}, {:.3f}, {:.3f}) Rot({:.3f}, {:.3f}, {:.3f})\n"
		"{:<10} : Pos({:.2f}, {:.2f}) Buttons({}, {}, {}) Hovered: {}\n"
		"{:<10} : DrawCount: {} FPS: {} FrameTime: {:.3f}ms VSync: {}",
		"Cam", pos.x, pos.y, pos.z, C3D::RadToDeg(rot.x), C3D::RadToDeg(rot.y), C3D::RadToDeg(rot.z),
		"Mouse", mouseNdcX, mouseNdcY, leftButton, middleButton, rightButton, hoveredBuffer,
		"Renderer", drawCount, Metrics.GetFps(), Metrics.GetFrameTime(), Renderer.IsFlagEnabled(C3D::FlagVSyncEnabled) ? "Yes" : "No"
	);

	m_testText.SetText(buffer.Data());
}

bool TestEnv::OnRender(C3D::RenderPacket& packet, f64 deltaTime)
{
	// TEMP
	// Ensure that we will be using our frame allocator for this packet's view
	packet.views.SetAllocator(&m_frameAllocator);
	// Pre-Allocate enough space for 5 views (and default initialize them)
	packet.views.Resize(4);

	// Skybox
	C3D::SkyboxPacketData skyboxData = { &m_skybox };
	if (!Views.BuildPacket(Views.Get("skybox"), m_frameAllocator, &skyboxData, &packet.views[0]))
	{
		m_logger.Error("Failed to build packet for view: 'skybox'");
		return false;
	}

	// World
	if (!Views.BuildPacket(Views.Get("world"), m_frameAllocator, &m_frameData.worldGeometries, &packet.views[1]))
	{
		m_logger.Error("Failed to build packet for view: 'world'");
		return false;
	}

	// UI
	C3D::UIPacketData uiPacket = {};
	uiPacket.meshData.meshes.SetAllocator(&m_frameAllocator);
	for (auto& mesh : m_uiMeshes)
	{
		if (mesh.generation != INVALID_ID_U8)
		{
			uiPacket.meshData.meshes.PushBack(&mesh);
		}
	}
	uiPacket.texts.SetAllocator(&m_frameAllocator);
	uiPacket.texts.PushBack(&m_testText);

	Console->OnRender(uiPacket);

	if (!Views.BuildPacket(Views.Get("ui"), m_frameAllocator, &uiPacket, &packet.views[2]))
	{
		m_logger.Error("Failed to build packet for view: 'ui'");
		return false;
	}

	// Pick
	C3D::PickPacketData pickPacket = {};
	pickPacket.uiMeshData = uiPacket.meshData;
	pickPacket.worldMeshData = &m_frameData.worldGeometries;
	pickPacket.texts = uiPacket.texts;
	if (!Views.BuildPacket(Views.Get("pick"), m_frameAllocator, &pickPacket, &packet.views[3]))
	{
		m_logger.Error("Failed to build packet for view: 'pick'");
		return false;
	}

	// TEMP END
	return true;
}

void TestEnv::AfterRender()
{
	for (const auto mesh : m_planes)
	{
		if (mesh && mesh->generation != INVALID_ID_U8) C3D::PrimitiveRenderer::Dispose(mesh);
	}
}

void TestEnv::OnResize(const u16 width, const u16 height)
{
	m_state.width = width;
	m_state.height = height;

	// TEMP
	m_testText.SetPosition({ 10, height - 80, 0 });
	m_uiMeshes[0].transform.SetPosition({ width - 130, 10, 0 });
	// TEMP END
}

void TestEnv::OnShutdown()
{
	// TEMP
	Console->OnShutDown();

	m_skybox.Destroy();
	m_testText.Destroy();

	for (auto& mesh : m_meshes)
	{
		if (mesh.generation != INVALID_ID_U8)
		{
			mesh.Unload();
		}
	}

	for (auto& mesh : m_uiMeshes)
	{
		if (mesh.generation != INVALID_ID_U8)
		{
			mesh.Unload();
		}
	}

	Event.UnRegister(C3D::SystemEventCode::Debug0, this, &TestEnv::OnDebugEvent);
	Event.UnRegister(C3D::SystemEventCode::Debug1, this, &TestEnv::OnDebugEvent);
	Event.UnRegister(C3D::SystemEventCode::ObjectHoverIdChanged, this, &TestEnv::OnEvent);

	m_frameAllocator.Destroy();
	// TEMP END
}

bool TestEnv::ConfigureRenderViews()
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
	skyboxPass.renderArea = { 0, 0, 1280, 720 };
	skyboxPass.clearColor = { 0, 0, 0.2f, 1.0f };
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

	m_config.renderViews.PushBack(skyboxConfig);

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
	worldPass.renderArea = { 0, 0, 1280, 720 };
	worldPass.clearColor = { 0, 0, 0.2f, 1.0f };
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

	m_config.renderViews.PushBack(worldConfig);

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
	uiPass.renderArea = { 0, 0, 1280, 720 };
	uiPass.clearColor = { 0, 0, 0.2f, 1.0f };
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

	m_config.renderViews.PushBack(uiViewConfig);

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
	pickPasses[0].renderArea = { 0, 0, 1280, 720 };
	pickPasses[0].clearColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // HACK: Clear to white for better visibility (should be 0 since it's invalid id)
	pickPasses[0].clearFlags = C3D::RenderPassClearFlags::ClearColorBuffer | C3D::RenderPassClearFlags::ClearDepthBuffer;
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
	pickPasses[1].renderArea = { 0, 0, 1280, 720 };
	pickPasses[1].clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
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

	m_config.renderViews.PushBack(pickViewConfig);

	return true;
}

bool TestEnv::OnEvent(const u16 code, void* sender, const C3D::EventContext& context)
{
	switch (code)
	{
		case C3D::SystemEventCode::ObjectHoverIdChanged:
			m_hoveredObjectId = context.data.u32[0];
			return true;
		default:
			return false;
	}
}

bool TestEnv::OnDebugEvent(const u16 code, void* sender, const C3D::EventContext& context)
{
	if (code == C3D::SystemEventCode::Debug0)
	{
		const char* names[3] = { "cobblestone", "paving", "rock" };
		static i8 choice = 2;

		const char* oldName = names[choice];

		choice++;
		choice %= 3;

		if (C3D::Geometry* g = m_meshes[0].geometries[0])
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
		
	if (code == C3D::SystemEventCode::Debug1)
	{
		if (!m_modelsLoaded)
		{
			m_logger.Debug("OnDebugEvent() - Loading models...");
			m_modelsLoaded = true;

			if (!m_carMesh->LoadFromResource("falcon"))
			{
				m_logger.Error("OnDebugEvent() - Failed to load falcon mesh!");
			}
			if (!m_sponzaMesh->LoadFromResource("sponza"))
			{
				m_logger.Error("OnDebugEvent() - Failed to load sponza mesh!");
			}
		}

		return true;
	}
		
	return false;
}

bool TestEnv::ShutdownCommand(const C3D::DynamicArray<C3D::CString<128>>& args, C3D::CString<256>& output)
{
	Quit();
	output = "Shutting down!";
	return true;
}

bool TestEnv::VSyncCommand(const C3D::DynamicArray<C3D::CString<128>>& args, C3D::CString<256>& output)
{
	if (args.Size() != 2)
	{
		output.FromFormat("Invalid number of arguments provided: {}", args.Size());
		return false;
	}

	if (args[1] == "on")
	{
		Renderer.SetFlagEnabled(C3D::FlagVSyncEnabled, true);
		output = "VSync is set to enabled";
		return true;
	}

	if (args[1] == "off")
	{
		Renderer.SetFlagEnabled(C3D::FlagVSyncEnabled, false);
		output = "VSync is set to disabled";
		return true;
	}

	output.FromFormat("Invalid argument provided \'{}\'", args[1]);
	return false;
}
