
#include "texture_system.h"

#include "core/logger.h"
#include "core/c3d_string.h"
#include "core/memory.h"

#include "services/services.h"

#include "renderer/renderer_frontend.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"

namespace C3D
{
	TextureSystem::TextureSystem()
		: m_logger("TEXTURE_SYSTEM"), m_initialized(false), m_config(), m_defaultTexture(), m_defaultSpecularTexture(),
		  m_defaultNormalTexture(), m_registeredTextures(nullptr)
	{
	}

	bool TextureSystem::Init(const TextureSystemConfig& config)
	{
		if (config.maxTextureCount == 0)
		{
			m_logger.Error("config.maxTextureCount must be > 0");
			return false;
		}

		m_config = config;

		// Allocate enough memory for the max number of textures that we will be using
		m_registeredTextures = Memory.Allocate<Texture>(config.maxTextureCount, MemoryType::Texture);
		// Set the id of all the textures to invalid
		for (u32 i = 0; i < m_config.maxTextureCount; i++)
		{
			m_registeredTextures[i].id = INVALID_ID;
			m_registeredTextures[i].generation = INVALID_ID;
		}

		// Ensure that we have enough space for all our textures
		m_registeredTextureTable.reserve(config.maxTextureCount);

		CreateDefaultTextures();

		m_initialized = true;
		return true;
;	}

	void TextureSystem::Shutdown()
	{
		m_logger.Info("Destroying all loaded textures");
		for (u32 i = 0; i < m_config.maxTextureCount; i++)
		{
			if (const auto tex = &m_registeredTextures[i]; tex->generation != INVALID_ID)
			{
				Renderer.DestroyTexture(tex);
			}
		}

		// Free the memory that was storing all the textures
		Memory.Free(m_registeredTextures, sizeof(Texture) * m_config.maxTextureCount, MemoryType::Texture);

		m_logger.Info("Destroying default textures");
		DestroyDefaultTextures();
	}

	Texture* TextureSystem::Acquire(const std::string& name, const bool autoRelease)
	{
		// If the default texture is requested we return it. But we warn about it since it should be
		// retrieved with GetDefault()
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			m_logger.Warn("Acquire called for {} texture. Use GetDefault() for this", DEFAULT_TEXTURE_NAME);
			return &m_defaultTexture;
		}

		const auto& it = m_registeredTextureTable.find(name);
		// No Texture found for this name
		if (it == m_registeredTextureTable.end())
		{
			m_registeredTextureTable.emplace(name, TextureReference{ 0, INVALID_ID, autoRelease });
		}

		auto& ref = m_registeredTextureTable[name];
		ref.referenceCount++;

		if (ref.handle == INVALID_ID)
		{
			// No texture exists here yet
			const u32 count = m_config.maxTextureCount;
			Texture* tex = nullptr;
			for (u32 i = 0; i < count; i++)
			{
				if (m_registeredTextures[i].id == INVALID_ID)
				{
					ref.handle = i;
					tex = &m_registeredTextures[i];
					break;
				}
			}

			if (!tex || ref.handle == INVALID_ID)
			{
				m_logger.Fatal("No more free space for textures. Adjust the configuration to allow more");
				return nullptr;
			}

			if (!LoadTexture(name, tex))
			{
				m_logger.Error("Failed to load texture {}", name);
				return nullptr;
			}

			tex->id = ref.handle;
			m_logger.Trace("Texture {} did not exist yet. Created and the refCount is now {}", name, ref.referenceCount);
		}
		else
		{
			m_logger.Trace("Texture {} already exists. The refCount is now {}", name, ref.referenceCount);
		}

		return &m_registeredTextures[ref.handle];
	}

	void TextureSystem::Release(const string& name)
	{
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			m_logger.Warn("Tried to release {}. This happens on shutdown automatically", DEFAULT_TEXTURE_NAME);
			return;
		}

		const auto it = m_registeredTextureTable.find(name);
		if (it == m_registeredTextureTable.end())
		{
			m_logger.Warn("Tried to release a texture that does not exist: {}", name);
			return;
		}

		auto& ref = it->second;
		ref.referenceCount--;

		if (ref.referenceCount == 0 && ref.autoRelease)
		{
			// This texture is marked for auto release and we are holding no more references to it
			Texture* tex = &m_registeredTextures[ref.handle];

			// Destroy the texture
			DestroyTexture(tex);

			// Remove the reference
			m_registeredTextureTable.erase(it);

			m_logger.Info("Released texture {}. The texture was unloaded because refCount = 0 and autoRelease = true", name);
			return;
		}

		m_logger.Info("Released texture {}. The texture now has a refCount = {} (autoRelease = {})", name, ref.referenceCount, ref.autoRelease);
	}

	Texture* TextureSystem::GetDefault()
	{
		if (!m_initialized)
		{
			m_logger.Error("GetDefault() was called before initialization. Returned nullptr");
			return nullptr;
		}
		return &m_defaultTexture;
	}

	Texture* TextureSystem::GetDefaultSpecular()
	{
		if (!m_initialized)
		{
			m_logger.Error("GetDefaultSpecular() was called before initialization. Returned nullptr");
			return nullptr;
		}
		return &m_defaultSpecularTexture;
	}

	Texture* TextureSystem::GetDefaultNormal()
	{
		if (!m_initialized)
		{
			m_logger.Error("GetDefaultNormal() was called before initialization. Returned nullptr");
			return nullptr;
		}
		return &m_defaultNormalTexture;
	}

	bool TextureSystem::CreateDefaultTextures()
	{
		// NOTE: Create a default texture, a 256x256 blue/white checkerboard pattern
		// this is done in code to eliminate dependencies
		m_logger.Trace("Create default texture...");
		constexpr u32 textureDimensions = 256;
		constexpr u32 channels = 4;
		constexpr u32 pixelCount = textureDimensions * textureDimensions;

		const auto pixels = new u8[pixelCount * channels];
		Memory.Set(pixels, 255, sizeof(u8) * pixelCount * channels);

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

		StringNCopy(m_defaultTexture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
		m_defaultTexture.width = textureDimensions;
		m_defaultTexture.height = textureDimensions;
		m_defaultTexture.channelCount = channels;
		m_defaultTexture.generation = INVALID_ID;
		m_defaultTexture.hasTransparency = false;

		Renderer.CreateTexture(pixels, &m_defaultTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultTexture.generation = INVALID_ID;

		// Specular texture.
		m_logger.Trace("Create default specular texture...");
		const auto specPixels = new u8[16 * 16 * 4];
		// Default specular map is black (no specular)
		Memory.Set(specPixels, 0, sizeof(u8) * 16 * 16 * 4);
		StringNCopy(m_defaultSpecularTexture.name, DEFAULT_SPECULAR_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
		m_defaultSpecularTexture.width = 16;
		m_defaultSpecularTexture.height = 16;
		m_defaultSpecularTexture.channelCount = 4;
		m_defaultSpecularTexture.generation = INVALID_ID;
		m_defaultSpecularTexture.hasTransparency = false;

		Renderer.CreateTexture(specPixels, &m_defaultSpecularTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultSpecularTexture.generation = INVALID_ID;

		// Normal texture.
		m_logger.Trace("Create default normal texture...");
		const auto normalPixels = new u8[16 * 16 * 4];
		Memory.Set(normalPixels, 0, sizeof(u8) * 16 * 16 * 4);

		// Each pixel
		for (u64 row = 0; row < 16; row++)
		{
			for (u64 col = 0; col < 16; col++)
			{
				const u64 index = (row * 16) + col;
				const u64 indexBpp = index * channels;

				// Set blue, z-axis by default and alpha
				normalPixels[indexBpp + 0] = 128;
				normalPixels[indexBpp + 1] = 128;
				normalPixels[indexBpp + 2] = 255;
				normalPixels[indexBpp + 3] = 255;
			}
		}

		StringNCopy(m_defaultNormalTexture.name, DEFAULT_NORMAL_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
		m_defaultNormalTexture.width = 16;
		m_defaultNormalTexture.height = 16;
		m_defaultNormalTexture.channelCount = 4;
		m_defaultNormalTexture.generation = INVALID_ID;
		m_defaultNormalTexture.hasTransparency = false;

		Renderer.CreateTexture(normalPixels, &m_defaultNormalTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultNormalTexture.generation = INVALID_ID;

		// Cleanup our pixel array
		delete[] pixels;
		delete[] specPixels;
		delete[] normalPixels;

		return true;
	}

	void TextureSystem::DestroyDefaultTextures()
	{
		DestroyTexture(&m_defaultTexture);
		DestroyTexture(&m_defaultSpecularTexture);
		DestroyTexture(&m_defaultNormalTexture);
	}

	bool TextureSystem::LoadTexture(const string& name, Texture* texture) const
	{
		Resource imgResource{};
		if (!Resources.Load(name, ResourceType::Image, &imgResource))
		{
			m_logger.Error("Failed to load image resource for texture '{}'", name);
			return false;
		}

		const auto resourceData = static_cast<ImageResourceData*>(imgResource.data);
		if (!resourceData)
		{
			m_logger.Error("Failed to load image data for texture '{}'", name);
			return false;
		}

		Texture temp{};
		temp.width = resourceData->width;
		temp.height = resourceData->height;
		temp.channelCount = resourceData->channelCount;

		const u32 currentGeneration = texture->generation;
		texture->generation = INVALID_ID;

		const u64 totalSize = static_cast<u64>(temp.width) * temp.height * temp.channelCount;
		bool hasTransparency = false;
		for (u64 i = 0; i < totalSize; i += temp.channelCount)
		{
			if (const u8 a = resourceData->pixels[i + 3]; a < 255)
			{
				hasTransparency = true;
				break;
			}
		}

		StringNCopy(temp.name, name.c_str(), TEXTURE_NAME_MAX_LENGTH);
		temp.generation = INVALID_ID;
		temp.hasTransparency = hasTransparency;

		Renderer.CreateTexture(resourceData->pixels, &temp);

		// Take a copy of the old texture
		Texture old = *texture;
		// And assign our newly loaded to the provided texture
		*texture = temp;
		// Destroy the old texture
		Renderer.DestroyTexture(&old);

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

		Resources.Unload(&imgResource);
		return true;
	}

	void TextureSystem::DestroyTexture(Texture* texture)
	{
		// Cleanup the backend resources for this texture
		Renderer.DestroyTexture(texture);

		// Zero out the memory for the texture
		Memory.Zero(texture->name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
		Memory.Zero(texture, sizeof(Texture));

		// Invalidate the id and generation
		texture->id = INVALID_ID;
		texture->generation = INVALID_ID;
	}
}
