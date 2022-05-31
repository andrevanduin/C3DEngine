
#include "texture_system.h"

#include "core/logger.h"
#include "core/memory.h"

#include "core/c3d_string.h"

#include "services/services.h"

// TODO: Resource loader
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace C3D
{
	TextureSystem::TextureSystem()
		: m_config(), m_initialized(false), m_defaultTexture(), m_registeredTextures(nullptr)
	{
	}

	bool TextureSystem::Init(const TextureSystemConfig& config)
	{
		if (config.maxTextureCount == 0)
		{
			Logger::PrefixError("TEXTURE_SYSTEM", "config.maxTextureCount must be > 0");
			return false;
		}

		m_config = config;

		// Allocate enough memory for the max number of textures that we will be using
		m_registeredTextures = Memory::Allocate<Texture>(config.maxTextureCount, MemoryType::Texture);
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
		Logger::PrefixInfo("TEXTURE_SYSTEM", "Destroying all loaded textures");
		for (u32 i = 0; i < m_config.maxTextureCount; i++)
		{
			if (const auto tex = &m_registeredTextures[i]; tex->generation != INVALID_ID)
			{
				Renderer.DestroyTexture(tex);
			}
		}

		// Free the memory that was storing all the textures
		Memory::Free(m_registeredTextures, sizeof(Texture) * m_config.maxTextureCount, MemoryType::Texture);

		Logger::PrefixInfo("TEXTURE_SYSTEM", "Destroying default textures");
		DestroyDefaultTextures();
	}

	Texture* TextureSystem::Acquire(const std::string& name, const bool autoRelease)
	{
		// If the default texture is requested we return it. But we warn about it since it should be
		// retrieved with GetDefaultTexture()
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			Logger::PrefixWarn("TEXTURE_SYSTEM", "Acquire called for {} texture. Use GetDefaultTexture() for this", DEFAULT_TEXTURE_NAME);
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
				Logger::PrefixFatal("TEXTURE_SYSTEM", "No more free space for textures. Adjust the configuration to allow more");
				return nullptr;
			}

			if (!LoadTexture(name, tex))
			{
				Logger::PrefixError("TEXTURE_SYSTEM", "Failed to load texture {}", name);
				return nullptr;
			}

			tex->id = ref.handle;
			Logger::PrefixTrace("TEXTURE_SYSTEM", "Texture {} did not exist yet. Created and the refCount is now {}", name, ref.referenceCount);
		}
		else
		{
			Logger::PrefixTrace("TEXTURE_SYSTEM", "Texture {} already exists. The refCount is now {}", name, ref.referenceCount);
		}

		return &m_registeredTextures[ref.handle];
	}

	void TextureSystem::Release(const string& name)
	{
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			Logger::PrefixWarn("TEXTURE_SYSTEM", "Tried to release {}. This happens on shutdown automatically", DEFAULT_TEXTURE_NAME);
			return;
		}

		const auto it = m_registeredTextureTable.find(name);
		if (it == m_registeredTextureTable.end())
		{
			Logger::PrefixWarn("TEXTURE_SYSTEM", "Tried to release a texture that does not exist: {}", name);
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

			Logger::PrefixInfo("TEXTURE_SYSTEM", "Released texture {}. The texture was unloaded because refCount = 0 and autoRelease = true", name);
			return;
		}

		Logger::PrefixInfo("TEXTURE_SYSTEM", "Released texture {}. The texture now has a refCount = {} (autoRelease = {})", name, ref.referenceCount, ref.autoRelease);
	}

	Texture* TextureSystem::GetDefaultTexture()
	{
		if (!m_initialized)
		{
			Logger::PrefixError("TEXTURE_SYSTEM", "GetDefaultTexture() was called before initialization. Returned nullptr");
			return nullptr;
		}

		return &m_defaultTexture;
	}

	bool TextureSystem::CreateDefaultTextures()
	{
		// NOTE: Create a default texture, a 256x256 blue/white checkerboard pattern
		// this is done in code to eliminate dependencies
		Logger::PrefixTrace("TEXTURE_SYSTEM", "Create default texture...");
		constexpr u32 textureDimensions = 256;
		constexpr u32 channels = 4;
		constexpr u32 pixelCount = textureDimensions * textureDimensions;

		const auto pixels = new u8[pixelCount * channels];
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

		StringNCopy(m_defaultTexture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
		m_defaultTexture.width = textureDimensions;
		m_defaultTexture.height = textureDimensions;
		m_defaultTexture.channelCount = channels;
		m_defaultTexture.generation = INVALID_ID;
		m_defaultTexture.hasTransparency = false;

		Renderer.CreateTexture(pixels, &m_defaultTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultTexture.generation = INVALID_ID;

		// Cleanup our pixel array
		delete[] pixels;
		return true;
	}

	void TextureSystem::DestroyDefaultTextures()
	{
		DestroyTexture(&m_defaultTexture);
	}

	bool TextureSystem::LoadTexture(const string& name, Texture* texture)
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
				stbi__err(nullptr , 0); // Clear the stbi error so next load doesn't fail
				return false;
			}

			StringNCopy(temp.name, name.c_str(), TEXTURE_NAME_MAX_LENGTH);
			temp.generation = INVALID_ID;
			temp.hasTransparency = hasTransparency;

			Renderer.CreateTexture(data, &temp);

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

			// Cleanup our data
			stbi_image_free(data);
			return true;
		}

		if (stbi_failure_reason())
		{
			Logger::Warn("LoadTexture() failed to load file: {}: {}", fullPath, stbi_failure_reason());
			stbi__err(nullptr, 0); // Clear the stbi error so next load doesn't fail
		}

		return false;
	}

	void TextureSystem::DestroyTexture(Texture* texture)
	{
		// Cleanup the backend resources for this texture
		Renderer.DestroyTexture(texture);

		// Zero out the memory for the texture
		Memory::Zero(texture->name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
		Memory::Zero(texture, sizeof(Texture));

		// Invalidate the id and generation
		texture->id = INVALID_ID;
		texture->generation = INVALID_ID;
	}
}
