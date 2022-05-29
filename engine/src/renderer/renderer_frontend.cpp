
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"

#include "math/c3d_math.h"

#include "renderer/vulkan/renderer_vulkan.h"

// TEMP
#include "services/services.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// TEMP END

namespace C3D
{
	void RenderSystem::CreateDefaultTexture(Texture* texture)
	{
		Memory::Zero(texture, sizeof(Texture));
		texture->generation = INVALID_ID;
	}

	bool RenderSystem::LoadTexture(const string& name, Texture* texture) const
	{
		// TODO: Should be able to be located anywhere

		constexpr i32 requiredChannelCount = 4;
		stbi_set_flip_vertically_on_load(true);

		// TODO: try different extensions
		const auto fullPath = "assets/textures/" + name + ".png";

		// Use a temporary texture to load into
		Texture temp;

		u8* data = stbi_load(fullPath.c_str(), reinterpret_cast<i32*>(&temp.width), reinterpret_cast<i32*>(&temp.height), 
			reinterpret_cast<i32*>(&temp.channelCount), requiredChannelCount);

		temp.channelCount = requiredChannelCount;
		if (data)
		{
			const u32 currentGeneration = texture->generation;
			texture->generation = INVALID_ID;

			const u64 totalSize = temp.width * temp.height * requiredChannelCount;
			bool hasTransparency = false;
			for (u64 i = 0; i < totalSize; i += requiredChannelCount)
			{
				if (const u8 a = data[i + 3]; a < 255)
				{
					hasTransparency = true;
					break;
				}
			}

			if (stbi_failure_reason())
			{
				Logger::Warn("loadTexture() failed to load file {}: {}", fullPath, stbi_failure_reason());
			}

			CreateTexture(name, true, temp.width, temp.height, temp.channelCount, data, hasTransparency, &temp);

			// Take a copy of the old texture
			Texture old = *texture;
			// And assign our newly loaded to the provided texture
			*texture = temp;
			// Destroy the old texture
			DestroyTexture(&old);

			if (currentGeneration == INVALID_ID) 
			{
				// Texture is new so it's generation starts at 0
				texture->generation = 0;
			}
			else
			{
				// Texture already existed so we increment it's generation
				texture->generation = currentGeneration + 1;
			}

			// Cleanup our data
			stbi_image_free(data);
			return true;
		}

		if (stbi_failure_reason())
		{
			Logger::Warn("LoadTexture() failed to load file: {}: {}", fullPath, stbi_failure_reason());
		}

		return false;
	}

	// TEMP
	bool RenderSystem::OnDebugEvent(u16 code, void* sender, EventContext data)
	{
		const char* names[3] = { "cobblestone", "paving", "paving2" };
		static i8 choice = 2;
		choice++;
		choice %= 3;

		LoadTexture(names[choice], &m_testDiffuseTexture);
		return true;
	}
	// END TEMP

	RenderSystem::RenderSystem()
		: m_projection(), m_view(), m_nearClip(0.1f), m_farClip(1000.0f), m_defaultTexture(), m_testDiffuseTexture()
	{
	}

	bool RenderSystem::Init(Application* application)
	{
		Logger::PushPrefix("RENDERER");

		if (!CreateBackend(RendererBackendType::Vulkan))
		{
			Logger::Fatal("Failed to create Vulkan Renderer Backend");
			return false;
		}

		Logger::Info("Created Vulkan Renderer Backend");

		if (!m_backend->Init(application, &m_defaultTexture))
		{
			Logger::Fatal("Failed to Initialize Renderer Backend.");
			return false;
		}

		const auto appState = application->GetState();

		const auto aspectRatio = static_cast<float>(appState->width) / static_cast<float>(appState->height);

		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);

		// NOTE: Create a default texture, a 256x256 blue/white checkerboard pattern
		// this is done in code to eliminate dependencies
		Logger::Trace("Create default texture...");
		constexpr u32 textureDimensions = 256;
		constexpr u32 channels = 4;
		constexpr u32 pixelCount = textureDimensions * textureDimensions;

		u8 pixels[pixelCount * channels]{};
		Memory::Set(pixels, 255, sizeof(u8) * pixelCount * channels);

		for (u64 row = 0; row < textureDimensions; row++)
		{
			for (u64 col = 0; col < textureDimensions; col++)
			{
				const u64 index = (row * textureDimensions) + col;
				const u64 indexChannel = index * channels;
				if (row % 2)
				{
					if (col % 2)
					{
						pixels[indexChannel + 0] = 0;
						pixels[indexChannel + 1] = 0;
					}
				}
				else
				{
					if (!(col % 2))
					{
						pixels[indexChannel + 0] = 0;
						pixels[indexChannel + 1] = 0;
					}
				}
			}
		}

		// TEMP
		m_onDebugEventCallback = new EventCallback(this, &RenderSystem::OnDebugEvent);
		Event.Register(SystemEventCode::Debug0, m_onDebugEventCallback);
		// TEMP END


		CreateTexture("default", false, textureDimensions, textureDimensions, channels, pixels, false, &m_defaultTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultTexture.generation = INVALID_ID;

		// TODO: Load other textures
		CreateDefaultTexture(&m_testDiffuseTexture);


		Logger::Info("Initialized Vulkan Renderer Backend");
		Logger::PopPrefix();
		return true;
	}

	void RenderSystem::Shutdown()
	{
		Logger::PrefixInfo("RENDERER", "Shutting Down");

		// TEMP
		Event.UnRegister(SystemEventCode::Debug0, m_onDebugEventCallback);
		delete m_onDebugEventCallback;
		// TEMP END

		DestroyTexture(&m_defaultTexture);
		DestroyTexture(&m_testDiffuseTexture);

		m_backend->Shutdown();
		DestroyBackend();
	}

	void RenderSystem::SetView(const mat4 view)
	{
		m_view = view;
	}

	void RenderSystem::OnResize(const u16 width, const u16 height)
	{
		const auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);

		m_backend->OnResize(width, height);
	}

	bool RenderSystem::DrawFrame(const RenderPacket* packet)
	{
		if (!BeginFrame(packet->deltaTime)) return true;

		m_backend->UpdateGlobalState(m_projection, m_view, vec3(0.0f), vec4(1.0f), 0);

		static f32 angle = 0.0f;
		//angle += 0.001f;

		mat4 model = translate(vec3(0, 0, 0));
		model = rotate(model, angle, vec3(0, 0, -1.0f));

		GeometryRenderData data = { 0 /* TODO: actual object id */, model };
		data.textures[0] = &m_testDiffuseTexture;

		m_backend->UpdateObject(data);

		if (!EndFrame(packet->deltaTime))
		{
			Logger::Error("RenderSystem::EndFrame() failed.");
			return false;
		}
		return true;
	}

	bool RenderSystem::BeginFrame(const f32 deltaTime) const
	{
		return m_backend->BeginFrame(deltaTime);
	}

	bool RenderSystem::EndFrame(const f32 deltaTime) const
	{
		const bool result = m_backend->EndFrame(deltaTime);
		m_backend->state.frameNumber++;
		return result;
	}

	void RenderSystem::CreateTexture(const string& name, const bool autoRelease, const i32 width, const i32 height, const i32 channelCount,
		const u8* pixels, const bool hasTransparency, Texture* outTexture) const
	{
		return m_backend->CreateTexture(name, autoRelease, width, height, channelCount, pixels, hasTransparency, outTexture);
	}

	void RenderSystem::DestroyTexture(Texture* texture) const
	{
		return m_backend->DestroyTexture(texture);
	}

	bool RenderSystem::CreateBackend(const RendererBackendType type)
	{
		if (type == RendererBackendType::Vulkan)
		{
			m_backend = Memory::New<RendererVulkan>(MemoryType::Renderer);
			return true;
		}

		Logger::Error("Tried Creating an Unsupported Renderer Backend: {}", ToUnderlying(type));
		return false;
	}

	void RenderSystem::DestroyBackend() const
	{
		if (m_backend->type == RendererBackendType::Vulkan)
		{
			Memory::Free(m_backend, sizeof(RendererVulkan), MemoryType::Renderer);
		}
	}
}
