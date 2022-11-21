
#include "application.h"
#include "logger.h"
#include "memory.h"
#include "input.h"
#include "clock.h"
#include "containers/string.h"

#include <SDL2/SDL.h>

#include "events/event.h"
#include "platform/platform.h"

#include "services/services.h"

// TEMP
#include "core/c3d_string.h"
#include "renderer/transform.h"
//

#include "identifier.h"
#include "utils.h"
#include "renderer/renderer_frontend.h"
#include "resources/loaders/bitmap_font_loader.h"
#include "systems/geometry_system.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"
#include "systems/camera_system.h"
#include "systems/font_system.h"
#include "systems/render_view_system.h"
#include "systems/jobs/job_system.h"

namespace C3D
{
	Application::Application(const ApplicationConfig& config)
		: m_logger("APPLICATION"), m_meshes{}, m_uiMeshes{}
	{
		C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the application twice");
		
		Logger::Init();
		m_logger.Info("Starting");

		Services::InitMemory(config.memorySystemConfig);

		m_state.name = config.name;
		m_state.width = config.width;
		m_state.height = config.height;

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			m_logger.Fatal("Failed to initialize SDL: {}", SDL_GetError());
		}

		m_logger.Info("Successfully initialized SDL");

		m_window = SDL_CreateWindow(("C3DEngine - " + config.name).c_str(), config.x, config.y, config.width, config.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (m_window == nullptr)
		{
			m_logger.Fatal("Failed to create a Window: {}", SDL_GetError());
		}

		m_logger.Info("Successfully created SDL Window");

		auto threadCount = Platform::GetProcessorCount();
		if (threadCount <= 1)
		{
			m_logger.Fatal("System reported {} threads. C3DEngine requires at least 1 thread besides the main thread.", threadCount);
		}
		else
		{
			m_logger.Info("System reported {} threads (including main thread).", threadCount);
		}

		constexpr auto maxThreadCount = 15;
		if (threadCount - 1 > maxThreadCount)
		{
			m_logger.Info("Available threads on this system is greater than {} (). Capping used threads at {}", maxThreadCount, (threadCount - 1), maxThreadCount);
			threadCount = maxThreadCount;
		}

		constexpr auto rendererMultiThreaded = false;

		u32 jobThreadTypes[15];
		for (u32& jobThreadType : jobThreadTypes) jobThreadType = JobTypeGeneral;

		if (threadCount == 1 || !rendererMultiThreaded)
		{
			jobThreadTypes[0] |= (JobTypeGpuResource | JobTypeResourceLoad);
		}
		else if (threadCount == 2)
		{
			jobThreadTypes[0] |= JobTypeGpuResource;
			jobThreadTypes[1] |= JobTypeResourceLoad;
		}
		else
		{
			jobThreadTypes[0] = JobTypeGpuResource;
			jobThreadTypes[1] = JobTypeResourceLoad;
		}

		JobSystemConfig	jobSystemConfig { maxThreadCount, jobThreadTypes };
		
		constexpr ResourceSystemConfig		resourceSystemConfig	{ 32, "../../../../assets" };
		constexpr ShaderSystemConfig		shaderSystemConfig		{ 128, 128, 31, 31 };
		constexpr TextureSystemConfig		textureSystemConfig		{ 65536 };
		constexpr MaterialSystemConfig		materialSystemConfig	{ 4096 };
		constexpr GeometrySystemConfig		geometrySystemConfig	{ 4096 };
		constexpr CameraSystemConfig		cameraSystemConfig		{ 61 };
		constexpr RenderViewSystemConfig	viewSystemConfig		{ 251 };

		FontSystemConfig fontSystemConfig;
		fontSystemConfig.autoRelease = false;
		fontSystemConfig.defaultBitmapFontCount = 1;

		BitmapFontConfig bmpFontConfig = {};
		bmpFontConfig.name = "Ubuntu Mono 21px";
		bmpFontConfig.resourceName = "UbuntuMono21px";
		bmpFontConfig.size = 21;

		fontSystemConfig.bitmapFontConfigs = &bmpFontConfig;

		fontSystemConfig.defaultSystemFontCount = 0;
		fontSystemConfig.systemFontConfigs = nullptr;
		fontSystemConfig.maxBitmapFontCount = 101;
		fontSystemConfig.maxSystemFontCount = 101;

		Identifier::Init();

		Services::Init(this, jobSystemConfig, resourceSystemConfig, shaderSystemConfig, textureSystemConfig, 
			cameraSystemConfig, viewSystemConfig, fontSystemConfig);

		Event.Register(SystemEventCode::Resized, new EventCallback(this, &Application::OnResizeEvent));
		Event.Register(SystemEventCode::Minimized,  new EventCallback(this, &Application::OnMinimizeEvent));
		Event.Register(SystemEventCode::FocusGained, new EventCallback(this, &Application::OnFocusGainedEvent));

		auto callback = new EventCallback(this, &Application::OnDebugEvent);
		Event.Register(SystemEventCode::Debug0, callback);
		Event.Register(SystemEventCode::Debug1, callback);

		Event.Register(SystemEventCode::KeyPressed, new StaticEventCallback(&OnKeyEvent));
		Event.Register(SystemEventCode::KeyReleased, new StaticEventCallback(&OnKeyEvent));
		Event.Register(SystemEventCode::ObjectHoverIdChanged, new EventCallback(this, &Application::OnEvent));

		// Load render views
		RenderViewConfig skyboxConfig{};
		skyboxConfig.type = RenderViewKnownType::Skybox;
		skyboxConfig.width = 1280;
		skyboxConfig.height = 720;
		skyboxConfig.name = "skybox";
		skyboxConfig.passCount = 1;
		skyboxConfig.viewMatrixSource = RenderViewViewMatrixSource::SceneCamera;

		RenderPassConfig skyboxPasses[1];
		skyboxPasses[0].name = "RenderPass.Builtin.Skybox";
		skyboxPasses[0].renderArea = { 0, 0, 1280, 720 };
		skyboxPasses[0].clearColor = { 0, 0, 0.2f, 1.0f };
		skyboxPasses[0].clearFlags = ClearColorBuffer;
		skyboxPasses[0].depth = 1.0f;
		skyboxPasses[0].stencil = 0;

		RenderTargetAttachmentConfig skyboxTargetAttachment = {};
		skyboxTargetAttachment.type = RenderTargetAttachmentTypeColor;
		skyboxTargetAttachment.source = RenderTargetAttachmentSourceDefault;
		skyboxTargetAttachment.loadOperation = RenderTargetAttachmentLoadOperationDontCare;
		skyboxTargetAttachment.storeOperation = RenderTargetAttachmentStoreOperationStore;
		skyboxTargetAttachment.presentAfter = false;

		skyboxPasses[0].target.attachmentCount = 1;
		skyboxPasses[0].target.attachments = &skyboxTargetAttachment;
		skyboxPasses[0].renderTargetCount = Renderer.GetWindowAttachmentCount();

		skyboxConfig.passes = skyboxPasses;

		if (!Views.Create(skyboxConfig))
		{
			m_logger.Fatal("Failed to create view '{}'.", skyboxConfig.name);
		}

		RenderViewConfig worldConfig{};
		worldConfig.type = RenderViewKnownType::World;
		worldConfig.width = 1280;
		worldConfig.height = 720;
		worldConfig.name = "world";
		worldConfig.passCount = 1;
		worldConfig.viewMatrixSource = RenderViewViewMatrixSource::SceneCamera;

		RenderPassConfig worldPasses[1];
		worldPasses[0].name = "RenderPass.Builtin.World";
		worldPasses[0].renderArea = { 0, 0, 1280, 720 };
		worldPasses[0].clearColor = { 0, 0, 0.2f, 1.0f };
		worldPasses[0].clearFlags = ClearDepthBuffer | ClearStencilBuffer;
		worldPasses[0].depth = 1.0f;
		worldPasses[0].stencil = 0;

		RenderTargetAttachmentConfig worldTargetAttachments[2]{};
		worldTargetAttachments[0].type = RenderTargetAttachmentTypeColor;
		worldTargetAttachments[0].source = RenderTargetAttachmentSourceDefault;
		worldTargetAttachments[0].loadOperation = RenderTargetAttachmentLoadOperationLoad;
		worldTargetAttachments[0].storeOperation = RenderTargetAttachmentStoreOperationStore;
		worldTargetAttachments[0].presentAfter = false;

		worldTargetAttachments[1].type = RenderTargetAttachmentTypeDepth;
		worldTargetAttachments[1].source = RenderTargetAttachmentSourceDefault;
		worldTargetAttachments[1].loadOperation = RenderTargetAttachmentLoadOperationDontCare;
		worldTargetAttachments[1].storeOperation = RenderTargetAttachmentStoreOperationStore;
		worldTargetAttachments[1].presentAfter = false;

		worldPasses[0].target.attachmentCount = 2;
		worldPasses[0].target.attachments = worldTargetAttachments;
		worldPasses[0].renderTargetCount = Renderer.GetWindowAttachmentCount();

		worldConfig.passes = worldPasses;

		if (!Views.Create(worldConfig))
		{
			m_logger.Fatal("Failed to create view '{}'.", worldConfig.name);
		}

		RenderViewConfig uiViewConfig{};
		uiViewConfig.type = RenderViewKnownType::UI;
		uiViewConfig.width = 1280;
		uiViewConfig.height = 720;
		uiViewConfig.name = "ui";
		uiViewConfig.passCount = 1;
		uiViewConfig.viewMatrixSource = RenderViewViewMatrixSource::SceneCamera;

		RenderPassConfig uiPasses[1];
		uiPasses[0].name = "RenderPass.Builtin.UI";
		uiPasses[0].renderArea = { 0, 0, 1280, 720 };
		uiPasses[0].clearColor = { 0, 0, 0.2f, 1.0f };
		uiPasses[0].clearFlags = ClearNone;
		uiPasses[0].depth = 1.0f;
		uiPasses[0].stencil = 0;

		RenderTargetAttachmentConfig uiAttachment = {};
		uiAttachment.type = RenderTargetAttachmentTypeColor;
		uiAttachment.source = RenderTargetAttachmentSourceDefault;
		uiAttachment.loadOperation = RenderTargetAttachmentLoadOperationLoad;
		uiAttachment.storeOperation = RenderTargetAttachmentStoreOperationStore;
		uiAttachment.presentAfter = true;

		uiPasses[0].target.attachmentCount = 1;
		uiPasses[0].target.attachments = &uiAttachment;
		uiPasses[0].renderTargetCount = Renderer.GetWindowAttachmentCount();

		uiViewConfig.passes = uiPasses;

		if (!Views.Create(uiViewConfig))
		{
			m_logger.Fatal("Failed to create view '{}'.", uiViewConfig.name);
		}

		// Init material and geometry system since they need the shader created above
		if (!Services::InitMaterialSystem(materialSystemConfig))
		{
			m_logger.Fatal("Failed to init Material system.");
		}
		if (!Services::InitGeometrySystem(geometrySystemConfig))
		{
			m_logger.Fatal("Failed to init Geometry system.");
		}

		// TEMP
		// Create test ui text objects
		if (!m_testText.Create(UITextType::Bitmap, "Ubuntu Mono 21px", 21, "Some test text 123,\nyesyes!\n\tKaas!"))
		{
			m_logger.Fatal("Failed to load basic ui bitmap text.");
		}

		m_testText.SetPosition({ 10, 640, 0 });

		auto& cubeMap = m_skybox.cubeMap;
		cubeMap.magnifyFilter = cubeMap.minifyFilter = TextureFilter::ModeLinear;
		cubeMap.repeatU = cubeMap.repeatV = cubeMap.repeatW = TextureRepeat::ClampToEdge;
		cubeMap.use = TextureUse::CubeMap;
		if (!Renderer.AcquireTextureMapResources(&cubeMap))
		{
			m_logger.Fatal("Unable to acquire resources for cube map texture.");
		}
		cubeMap.texture = Textures.AcquireCube("skybox", true);
		
		GeometryConfig skyboxCubeConfig = Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "skybox_cube", "");
		skyboxCubeConfig.materialName[0] = '\0';

		m_skybox.g = Geometric.AcquireFromConfig(skyboxCubeConfig, true);
		m_skybox.frameNumber = INVALID_ID_U64;

		Geometric.DisposeConfig(&skyboxCubeConfig);

		Shader* skyboxShader = Shaders.Get("Shader.Builtin.Skybox");
		TextureMap* maps[1] = { &m_skybox.cubeMap };
		if (!Renderer.AcquireShaderInstanceResources(skyboxShader, maps, &m_skybox.instanceId))
		{
			m_logger.Fatal("Unable to acquire shader resources for skybox texture.");
		}

		// World meshes
		for (u32 i = 0; i < 10; i++)
		{
			m_meshes[i].generation = INVALID_ID_U8;
			m_uiMeshes[i].generation = INVALID_ID_U8;
		}

		u8 meshCount = 0;

		Mesh* cubeMesh = &m_meshes[meshCount];
		cubeMesh->geometryCount = 1;
		cubeMesh->geometries = Memory.Allocate<Geometry*>(MemoryType::Array);

		// Load up a cube configuration, and load geometry for it
		GeometryConfig gConfig = Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");

		cubeMesh->geometries[0] = Geometric.AcquireFromConfig(gConfig, true);
		cubeMesh->transform = Transform();
		cubeMesh->generation = 0;
		cubeMesh->uniqueId = Identifier::GetNewId(cubeMesh);

		meshCount++;

		// Cleanup the allocations that we just did
		Geometric.DisposeConfig(&gConfig);

		// A second cube
		Mesh* cubeMesh2 = &m_meshes[meshCount];
		cubeMesh2->geometryCount = 1;
		cubeMesh2->geometries = Memory.Allocate<Geometry*>(MemoryType::Array);

		// Load up a cube configuration, and load geometry for it
		gConfig = Geometric.GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube_2", "test_material");

		cubeMesh2->geometries[0] = Geometric.AcquireFromConfig(gConfig, true);
		cubeMesh2->transform = Transform(vec3(20.0f, 0.0f, 1.0f));
		cubeMesh2->transform.SetParent(&cubeMesh->transform);
		cubeMesh2->generation = 0;
		cubeMesh2->uniqueId = Identifier::GetNewId(cubeMesh2);

		meshCount++;

		// Cleanup the other allocations that we just did
		Geometric.DisposeConfig(&gConfig);

		// A third cube
		Mesh* cubeMesh3 = &m_meshes[meshCount];
		cubeMesh3->geometryCount = 1;
		cubeMesh3->geometries = Memory.Allocate<Geometry*>(MemoryType::Array);

		gConfig = Geometric.GenerateCubeConfig(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube_3", "test_material");

		cubeMesh3->geometries[0] = Geometric.AcquireFromConfig(gConfig, true);
		cubeMesh3->transform = Transform(vec3(5.0f, 0.0f, 1.0f));
		cubeMesh3->transform.SetParent(&cubeMesh2->transform);
		cubeMesh3->generation = 0;
		cubeMesh3->uniqueId = Identifier::GetNewId(cubeMesh3);

		meshCount++;

		// Cleanup the other allocations that we just did
		Geometric.DisposeConfig(&gConfig);

		carMesh = &m_meshes[meshCount];
		carMesh->transform = Transform({ 15.0f, 0.0f, 1.0f });
		carMesh->uniqueId = Identifier::GetNewId(carMesh);
		meshCount++;

		sponzaMesh = &m_meshes[meshCount];
		sponzaMesh->transform = Transform({ 15.0f, 0.0f, 1.0f }, mat4(1.0f), { 0.05f, 0.05f, 0.05f });
		sponzaMesh->uniqueId = Identifier::GetNewId(sponzaMesh);

		meshCount++;

		// Load up our test ui geometry
		GeometryConfig<Vertex2D, u32> uiConfig{};
		uiConfig.vertices = DynamicArray<Vertex2D>();
		uiConfig.vertices.Resize(4);

		uiConfig.indices = DynamicArray<u32>(6);

		StringNCopy(uiConfig.materialName, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
		StringNCopy(uiConfig.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

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

		m_uiMeshes[0].uniqueId = Identifier::GetNewId(&m_uiMeshes[0]);
		m_uiMeshes[0].geometryCount = 1;
		m_uiMeshes[0].geometries = Memory.Allocate<Geometry*>(MemoryType::Array);
		m_uiMeshes[0].geometries[0] = Geometric.AcquireFromConfig(uiConfig, true);
		m_uiMeshes[0].generation = 0;
		// TEMP END

		m_state.initialized = true;
		m_state.lastTime = 0;

		Application::OnResize(m_state.width, m_state.height);
		Renderer.OnResize(m_state.width, m_state.height);
	}

	Application::~Application() = default;

	void Application::Run()
	{
		m_state.running = true;

		Clock clock;

		clock.Start();
		clock.Update();

		m_state.lastTime = clock.GetElapsed();

		OnCreate();

		f64 runningTime = 0;
		u16 frameCount = 0;
		constexpr f64 targetFrameSeconds = 1.0 / 60.0;

		{
			const auto memStr = Utils::GenerateMemoryUsageString(Memory);
			m_logger.Info(memStr.Data());

			RenderPacket packet = {};
			packet.views.Resize(3); // Ensure enough space for 3 views so we only allocate once

			SkyboxPacketData skyboxData = {};
			skyboxData.box = &m_skybox;

			MeshPacketData worldMeshData = {};
			worldMeshData.meshes.Reserve(10);

			UIPacketData uiPacketData = {};
			uiPacketData.meshData.meshes.Reserve(10);
			uiPacketData.texts.PushBack(&m_testText);

			while (m_state.running)
			{
				HandleSdlEvents();

				if (!m_state.suspended)
				{
					clock.Update();
					const f64 currentTime = clock.GetElapsed();
					const f64 delta = currentTime - m_state.lastTime;
					const f64 frameStartTime = Platform::GetAbsoluteTime();

					Jobs.Update();

					OnUpdate(delta);
					OnRender(delta);

					// Rotate 
					quat rotation = angleAxis(0.5f * static_cast<f32>(delta), vec3(0.0f, 1.0f, 0.0f));
					m_meshes[0].transform.Rotate(rotation);
					m_meshes[1].transform.Rotate(rotation);
					m_meshes[2].transform.Rotate(rotation);

					packet.deltaTime = static_cast<f32>(delta);

					// Skybox
					if (!Views.BuildPacket(Views.Get("skybox"), &skyboxData, &packet.views[0]))
					{
						m_logger.Error("Failed to build packet for view 'skybox'.");
						return;
					}

					worldMeshData.meshes.Clear();
					for (auto& mesh : m_meshes)
					{
						if (mesh.generation != INVALID_ID_U8)
						{
							worldMeshData.meshes.PushBack(&mesh);
						}
					}

					// World
					if (!Views.BuildPacket(Views.Get("world"), &worldMeshData, &packet.views[1]))
					{
						m_logger.Error("Failed to build packet for view 'world_opaque'.");
						return;
					}

					uiPacketData.meshData.meshes.Clear();
					for (auto& mesh : m_uiMeshes)
					{
						if (mesh.generation != INVALID_ID_U8)
						{
							uiPacketData.meshData.meshes.PushBack(&mesh);
						}
					}

					const auto cam = Cam.GetDefault();
					const auto pos = cam->GetPosition();
					const auto rot = cam->GetEulerRotation();

					char buffer[256]{};
					sprintf_s(buffer, "Camera:\nPosition = (%.3f, %.3f, %.3f)\nRotation = (%.3f, %.3f, %.3f)", pos.x, pos.y, pos.z, RadToDeg(rot.x), RadToDeg(rot.y), RadToDeg(rot.z));
					uiPacketData.texts[0]->SetText(buffer);

					// Ui
					if (!Views.BuildPacket(Views.Get("ui"), &uiPacketData, &packet.views[2]))
					{
						m_logger.Error("Failed to build packet for view 'ui'.");
						return;
					}

					if (!Renderer.DrawFrame(&packet))
					{
						m_logger.Warn("DrawFrame() failed");
					}

					for (auto& view : packet.views)
					{
						view.geometries.Clear();
					}

					const f64 frameEndTime = Platform::GetAbsoluteTime();
					const f64 frameElapsedTime = frameEndTime - frameStartTime;
					runningTime += frameElapsedTime;
					const f64 remainingSeconds = targetFrameSeconds - frameElapsedTime;

					if (remainingSeconds > 0)
					{
						/*const u64 remainingMs = static_cast<u64>(remainingSeconds) * 1000;

						constexpr bool limitFrames = false;
						/if (remainingMs > 0 && limitFrames)
						{
							Platform::SleepMs(remainingMs - 1);
						}*/

						frameCount++;
					}

					Input.Update(delta);

					m_state.lastTime = currentTime;
				}
			}
		}

		Shutdown();
	}

	void Application::Quit()
	{
		m_state.running = false;
	}

	void Application::GetFrameBufferSize(u32* width, u32* height) const
	{
		*width = m_state.width;
		*height = m_state.height;
	}

	SDL_Window* Application::GetWindow() const
	{
		return m_window;
	}

	const ApplicationState* Application::GetState() const
	{
		return &m_state;
	}

	void Application::Shutdown()
	{
		C3D_ASSERT_MSG(m_state.initialized, "Tried to Shutdown application that hasn't been initialized")

		m_logger.Info("Shutdown()");
		m_logger.Info("UnRegistering events");

		Event.UnRegister(SystemEventCode::Resized, new EventCallback(this, &Application::OnResizeEvent));
		Event.UnRegister(SystemEventCode::Minimized, new EventCallback(this, &Application::OnMinimizeEvent));
		Event.UnRegister(SystemEventCode::FocusGained, new EventCallback(this, &Application::OnFocusGainedEvent));
		// TEMP
		Event.UnRegister(SystemEventCode::Debug0, new EventCallback(this, &Application::OnDebugEvent));
		// TEMP END

		Event.UnRegister(SystemEventCode::KeyPressed, new StaticEventCallback(&OnKeyEvent));
		Event.UnRegister(SystemEventCode::KeyReleased, new StaticEventCallback(&OnKeyEvent));

		// TEMP
		// TODO: Implement skybox destroy.
		for (auto& mesh : m_meshes)
		{
			if (mesh.generation != INVALID_ID_U8 && mesh.geometries && mesh.geometryCount > 0)
			{
				Memory.Free(mesh.geometries, mesh.geometryCount * sizeof(Geometry*), MemoryType::Array);
				mesh.geometries = nullptr;
				mesh.geometryCount = 0;
			}
		}

		for (auto& mesh : m_uiMeshes)
		{
			if (mesh.generation != INVALID_ID_U8 && mesh.geometries && mesh.geometryCount > 0)
			{
				Memory.Free(mesh.geometries, mesh.geometryCount * sizeof(Geometry*), MemoryType::Array);
				mesh.geometries = nullptr;
				mesh.geometryCount = 0;
			}
		}

		Renderer.ReleaseTextureMapResources(&m_skybox.cubeMap);

		m_testText.Destroy();
		// TEMP END

		Services::Shutdown();
		Identifier::Destroy();

		SDL_DestroyWindow(m_window);

		m_state.initialized = false;
	}

	void Application::HandleSdlEvents()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			// TODO: ImGUI event process here

			switch (e.type)
			{
				case SDL_QUIT:
					m_state.running = false;
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					Services::GetInput().ProcessKey(e.key.keysym.sym, e.type == SDL_KEYDOWN);
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					Services::GetInput().ProcessButton(e.button.button, e.type == SDL_MOUSEBUTTONDOWN);
					break;
				case SDL_MOUSEMOTION:
					Services::GetInput().ProcessMouseMove(e.motion.x, e.motion.y);
					break;
				case SDL_MOUSEWHEEL:
					Services::GetInput().ProcessMouseWheel(e.wheel.y);
					break;
				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						EventContext context{};
						context.data.u16[0] = static_cast<u16>(e.window.data1);
						context.data.u16[1] = static_cast<u16>(e.window.data2);
						Event.Fire(SystemEventCode::Resized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
					{
						constexpr EventContext context{};
						Event.Fire(SystemEventCode::Minimized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_ENTER && m_state.suspended)
					{
						EventContext context;
						context.data.u16[0] = static_cast<u16>(m_state.width);
						context.data.u16[1] = static_cast<u16>(m_state.height);
						Event.Fire(SystemEventCode::FocusGained, nullptr, context);
					}
					break;
				case SDL_TEXTINPUT:
					// TODO: Possibly change this is the future. But currently this spams the console if letters are pressed
					break;
				default:
					m_logger.Trace("Unhandled SDL Event: {}", e.type);
					break;
			}
		}
	}

	bool Application::OnResizeEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::Resized)
		{
			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			// We only update out width and height if they actually changed
			if (width != m_state.width || height != m_state.height)
			{
				m_logger.Debug("Window Resize: {} {}", width, height);

				if (width == 0 || height == 0)
				{
					m_logger.Warn("Invalid width or height");
					return true;
				}

				m_state.width = width;
				m_state.height = height;

				OnResize(width, height);
				Renderer.OnResize(width, height);

				m_testText.SetPosition({ 10, height - 80, 0 });
			}
		}

		return false;
	}

	bool Application::OnMinimizeEvent(const u16 code, void* sender, EventContext context)
	{
		if (code == SystemEventCode::Minimized)
		{
			m_logger.Info("Window was minimized - suspending application");
			m_state.suspended = true;
		}

		return false;
	}

	bool Application::OnFocusGainedEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::FocusGained)
		{
			m_logger.Info("Window has regained focus - resuming application");
			m_state.suspended = false;

			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			OnResize(width, height);
			Renderer.OnResize(width, height);
		}

		return false;
	}

	// TEMP
	bool Application::OnDebugEvent(const u16 code, void* sender, EventContext context)
	{
		if (code == SystemEventCode::Debug0)
		{
			const char* names[3] = { "cobblestone", "paving", "paving2" };
			static i8 choice = 2;

			const char* oldName = names[choice];

			choice++;
			choice %= 3;

			Geometry* g = m_meshes[0].geometries[0];
			if (g)
			{
				// Acquire new material
				g->material = Materials.Acquire(names[choice]);
				if (!g->material)
				{
					m_logger.Warn("[ON_DEBUG_EVENT] - No material found. Using default.");
					g->material = Materials.GetDefault();
				}

				Materials.Release(oldName);
			}

			return true;
		}

		if (code == SystemEventCode::Debug1)
		{
			if (!modelsLoaded)
			{
				modelsLoaded = true;
				m_logger.Debug("OnDebugEvent() - Loading models...");

				if (!carMesh->LoadFromResource("falcon"))
				{
					m_logger.Error("OnDebugEvent() - Failed to load falcon mesh.");
				}
				if (!sponzaMesh->LoadFromResource("sponza"))
				{
					m_logger.Error("OnDebugEvent() - Failed to load sponza mesh.");
				}
			}

			return true;
		}
		
		return false;
	}
	// TEMP END

	bool Application::OnKeyEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::KeyPressed)
		{
			const auto key = context.data.u16[0];
			Logger::Debug("Key Pressed: {}", static_cast<char>(key));
		}
		else if (code == SystemEventCode::KeyReleased)
		{
			const auto key = context.data.u16[0];
			Logger::Debug("Key Released: {}", static_cast<char>(key));
		}
		return false;
	}

	bool Application::OnEvent(u16 code, void* sender, EventContext context)
	{
		if (code == SystemEventCode::ObjectHoverIdChanged)
		{
			m_hoveredObjectId = context.data.u32[0];
			return true;
		}
		return false;
	}
}
