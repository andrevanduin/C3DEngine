
#include "texture_system.h"

#include "core/logger.h"
#include "core/c3d_string.h"
#include "core/memory.h"

#include "services/services.h"

#include "renderer/renderer_frontend.h"
#include "resources/resource_types.h"
#include "resources/loaders/image_loader.h"

#include "systems/resource_system.h"
#include "systems/texture_system.h"

#include "jobs/job.h"
#include "jobs/job_system.h"

namespace C3D
{
	TextureSystem::TextureSystem()
		: System("TEXTURE_SYSTEM"), m_initialized(false), m_registeredTextures(nullptr)
	{}

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
		m_registeredTextureTable.Create(config.maxTextureCount);
		// Fill our hashtable with invalid references
		const TextureReference invalidRef;
		m_registeredTextureTable.Fill(invalidRef);

		CreateDefaultTextures();

		m_initialized = true;
		return true;
		;
	}

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
		// Destroy our hashtable
		m_registeredTextureTable.Destroy();

		m_logger.Info("Destroying default textures");
		DestroyDefaultTextures();
	}

	Texture* TextureSystem::Acquire(const char* name, const bool autoRelease)
	{
		// If the default texture is requested we return it. But we warn about it since it should be
		// retrieved with GetDefault()
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			m_logger.Warn("Acquire() - Called for {} texture. Use GetDefault() for this.", DEFAULT_TEXTURE_NAME);
			return &m_defaultTexture;
		}

		u32 id = INVALID_ID;
		if (!ProcessTextureReference(name, TextureType::Type2D, 1, autoRelease, false, &id))
		{
			m_logger.Error("Acquire() - Failed to obtain a new texture id.");
			return nullptr;
		}

		return &m_registeredTextures[id];
	}

	Texture* TextureSystem::AcquireCube(const char* name, bool autoRelease)
	{
		// If the default texture is requested we return it. But we warn about it since it should be
		// retrieved with GetDefault()
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			m_logger.Warn("AcquireCube() - Called for {} texture. Use GetDefault() for this.", DEFAULT_TEXTURE_NAME);
			return &m_defaultTexture;
		}

		u32 id = INVALID_ID;
		if (!ProcessTextureReference(name, TextureType::TypeCube, 1, autoRelease, false, &id))
		{
			m_logger.Error("AcquireCube() - Failed to obtain a new texture id.");
			return nullptr;
		}

		return &m_registeredTextures[id];
	}

	Texture* TextureSystem::AcquireWritable(const char* name, const u32 width, const u32 height, const u8 channelCount, const bool hasTransparency)
	{
		u32 id = INVALID_ID;
		/* Wrapped textures are never auto-released because it means that their
		resources are created and managed somewhere within the renderer internals. */
		if (!ProcessTextureReference(name, TextureType::Type2D, 1, false, true, &id))
		{
			m_logger.Error("AcquireWritable() - Failed to obtain a new texture id.");
			return nullptr;
		}

		Texture* t = &m_registeredTextures[id];
		TextureFlagBits flags = hasTransparency ? HasTransparency : 0;
		flags |= IsWritable;

		t->Set(id, TextureType::Type2D, name, width, height, channelCount, flags);
		Renderer.CreateWritableTexture(t);
		return t;
	}

	void TextureSystem::Release(const char* name)
	{
		if (IEquals(name, DEFAULT_TEXTURE_NAME))
		{
			m_logger.Warn("Tried to release {}. This happens on shutdown automatically", DEFAULT_TEXTURE_NAME);
			return;
		}

		u32 id = INVALID_ID;
		if (!ProcessTextureReference(name, TextureType::Type2D, -1, false, false, &id))
		{
			m_logger.Error("Release() - Failed to release texture: '{}' properly.", name);
		}
	}

	Texture* TextureSystem::WrapInternal(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency, bool isWritable, const bool registerTexture, void* internalData)
	{
		u32 id = INVALID_ID;
		Texture* t = nullptr;
		if (registerTexture)
		{
			// NOTE: Wrapped textures are never auto-released because it means that their
			// resources are created and managed somewhere within the renderer internals.
			if (!ProcessTextureReference(name, TextureType::Type2D, 1, false, true, &id))
			{
				m_logger.Error("WrapInternal() - Failed to obtain a new texture id.");
				return nullptr;
			}
			t = &m_registeredTextures[id];
		}
		else
		{
			t = Memory.Allocate<Texture>(MemoryType::Texture);
			//m_logger.Trace("WrapInternal() - Created texture '{}', but not registering, resulting in an allocation. It is up to the caller to free this memory.", name);
		}

		auto flags = hasTransparency ? HasTransparency : 0;
		flags |= isWritable ? IsWritable : 0;
		flags |= IsWrapped;

		t->Set(id, TextureType::Type2D, name, width, height, channelCount, flags, internalData);
		return t;
	}

	bool TextureSystem::SetInternal(Texture* t, void* internalData)
	{
		if (t)
		{
			t->internalData = internalData;
			t->generation++;
			return true;
		}
		return false;
	}

	bool TextureSystem::Resize(Texture* t, const u32 width, const u32 height, const bool regenerateInternalData) const
	{
		if (t)
		{
			if (!t->IsWritable())
			{
				m_logger.Warn("Resize() - Should not be called on textures that are not writable.");
				return false;
			}

			t->width = width;
			t->height = height;

			if (!t->IsWrapped() && regenerateInternalData)
			{
				Renderer.ResizeTexture(t, width, height);
				return false;
			}

			t->generation++;
			return true;
		}
		return false;
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

	Texture* TextureSystem::GetDefaultDiffuse()
	{
		if (!m_initialized)
		{
			m_logger.Error("GetDefaultDiffuse() was called before initialization. Returned nullptr");
			return nullptr;
		}
		return &m_defaultDiffuseTexture;
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

		m_defaultTexture = Texture(DEFAULT_TEXTURE_NAME, TextureType::Type2D, textureDimensions, textureDimensions, channels);
		Renderer.CreateTexture(pixels, &m_defaultTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultTexture.generation = INVALID_ID;

		// Diffuse texture
		m_logger.Trace("Create default diffuse texture...");
		const auto diffusePixels = new u8[16 * 16 * 4];
		// Default diffuse texture is all white
		Memory.Set(diffusePixels, 255, sizeof(u8) * 16 * 16 * 4); // Default diffuse map is all white

		m_defaultDiffuseTexture = Texture(DEFAULT_DIFFUSE_TEXTURE_NAME, TextureType::Type2D, 16, 16, 4);
		Renderer.CreateTexture(diffusePixels, &m_defaultDiffuseTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultDiffuseTexture.generation = INVALID_ID;

		// Specular texture.
		m_logger.Trace("Create default specular texture...");
		const auto specPixels = new u8[16 * 16 * 4];
		Memory.Set(specPixels, 0, sizeof(u8) * 16 * 16 * 4); // Default specular map is black (no specular)

		m_defaultSpecularTexture = Texture(DEFAULT_SPECULAR_TEXTURE_NAME, TextureType::Type2D, 16, 16, 4);
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

		m_defaultNormalTexture = Texture(DEFAULT_NORMAL_TEXTURE_NAME, TextureType::Type2D, 16, 16, 4);
		Renderer.CreateTexture(normalPixels, &m_defaultNormalTexture);
		// Manually set the texture generation to invalid since this is the default texture
		m_defaultNormalTexture.generation = INVALID_ID;

		// Cleanup our pixel arrays
		delete[] pixels;
		delete[] diffusePixels;
		delete[] specPixels;
		delete[] normalPixels;

		return true;
	}

	void TextureSystem::DestroyDefaultTextures()
	{
		DestroyTexture(&m_defaultTexture);
		DestroyTexture(&m_defaultDiffuseTexture);
		DestroyTexture(&m_defaultSpecularTexture);
		DestroyTexture(&m_defaultNormalTexture);
	}

	bool TextureSystem::LoadTexture(const char* name, Texture* texture) const
	{
		TextureLoadParams params;
		params.resourceName = StringDuplicate(name);
		params.outTexture = texture;
		params.currentGeneration = texture->generation;

		JobInfo info;
		info.entryPoint = LoadJobEntryPoint;
		info.onSuccess = [this](void* r) { LoadJobSuccess(r); };
		info.onFailure = [this](void* r) { LoadJobFailure(r); };
		info.SetData(&params, sizeof(TextureLoadParams));

		Jobs.Submit(info);
		return true;
	}

	bool TextureSystem::LoadCubeTextures(const char* name, const std::array<char[TEXTURE_NAME_MAX_LENGTH], 6>& textureNames, Texture* texture) const
	{
		constexpr ImageResourceParams params { false };

		u8* pixels = nullptr;
		u64 imageSize = 0;

		for (auto i = 0; i < 6; i++)
		{
			const auto textureName = textureNames[i];

			ImageResource res{};
			if (!Resources.Load(textureName, &res, params))
			{
				m_logger.Error("LoadCubeTextures() - Failed to load image resource for texture '{}'", textureName);
				return false;
			}

			if (!res.data.pixels)
			{
				m_logger.Error("LoadCubeTextures() - Failed to load image data for texture '{}'", name);
				return false;
			}

			if (!pixels)
			{
				texture->width = res.data.width;
				texture->height = res.data.height;
				texture->channelCount = res.data.channelCount;
				texture->flags = 0;
				texture->generation = 0;
				StringNCopy(texture->name, name, TEXTURE_NAME_MAX_LENGTH);

				imageSize = static_cast<u64>(texture->width) * texture->height * texture->channelCount;
				pixels = Memory.Allocate<u8>(imageSize * 6, MemoryType::Array); // 6 textures one for every side of the cube
			}
			else
			{
				if (texture->width != res.data.width || texture->height != res.data.height || texture->channelCount != res.data.channelCount)
				{
					m_logger.Error("LoadCubeTextures() - Failed to load. All textures must be the same resolution and bit depth.");
					Memory.Free(pixels, sizeof(u8) * imageSize * 6, MemoryType::Array);
					pixels = nullptr;
					return false;
				}
			}

			// Copy over the pixels to the correct location in the array
			Memory.Copy(pixels + (imageSize * i), res.data.pixels, imageSize);

			// Cleanup our resource
			Resources.Unload(&res);
		}

		// Acquire internal texture resources and upload to the GPU
		Renderer.CreateTexture(pixels, texture);

		Memory.Free(pixels, sizeof(u8) * imageSize * 6, MemoryType::Array);
		pixels = nullptr;

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

	bool TextureSystem::ProcessTextureReference(const char* name, const TextureType type, const i8 referenceDiff, const bool autoRelease, bool skipLoad, u32* outTextureId)
	{
		*outTextureId = INVALID_ID;
		TextureReference ref = m_registeredTextureTable.Get(name);
		if (ref.referenceCount == 0 && referenceDiff > 0)
		{
			// Set autoRelease to the provided value if this is the first time the texture is loaded
			ref.autoRelease = autoRelease;
		}

		// Increment / Decrement our reference count
		ref.referenceCount += referenceDiff;

		char nameCopy[TEXTURE_NAME_MAX_LENGTH];
		StringNCopy(nameCopy, name, TEXTURE_NAME_MAX_LENGTH);

		// If decrementing, this means we want to release
		if (referenceDiff < 0)
		{
			// If reference count is 0 and we want to auto release, we destroy the texture
			if (ref.referenceCount == 0 && ref.autoRelease)
			{
				Texture* texture = &m_registeredTextures[ref.handle];
				DestroyTexture(texture);

				// Reset the reference
				ref.handle = INVALID_ID;
				ref.autoRelease = false;
				m_logger.Trace("ProcessTextureReference() - Released texture '{}'. Texture unloaded because refCount = 0 and autoRelease = true", nameCopy);
			}
			else
			{
				// Otherwise we simply do nothing
				m_logger.Trace("ProcessTextureReference() - Released texture '{}'. Texture now has refCount = {} (autoRelease = {})", nameCopy, ref.referenceCount, ref.autoRelease);
			}
		}
		else
		{
			// Incrementing. Check if the handle is new or not.
			if (ref.handle == INVALID_ID)
			{
				// No texture exists here yet. Find a free index first.
				const u32 count = m_config.maxTextureCount;
				for (u32 i = 0; i < count; i++)
				{
					if (m_registeredTextures[i].id == INVALID_ID)
					{
						// We found a free slot. Use it's index as a handle
						ref.handle = i;
						*outTextureId = i;
						break;
					}
				}

				// Check if we found a free slot
				if (*outTextureId == INVALID_ID)
				{
					m_logger.Fatal("ProcessTextureReference() - TextureSystem was unable to find free texture slot. Adjust configuration to allow for more.");
					return false;
				}

				Texture* texture = &m_registeredTextures[ref.handle];
				texture->type = type;

				// Create a new texture
				if (skipLoad)
				{
					m_logger.Trace("ProcessTextureReference() - Loading skipped for texture '{}' as requested.", nameCopy);
				}
				else
				{
					if (type == TextureType::TypeCube)
					{
						std::array<char[TEXTURE_NAME_MAX_LENGTH], 6> textureNames{};
						StringFormat(textureNames[0], "%s_r", name); // Right texture
						StringFormat(textureNames[1], "%s_l", name); // Left texture
						StringFormat(textureNames[2], "%s_u", name); // Up texture
						StringFormat(textureNames[3], "%s_d", name); // Down texture
						StringFormat(textureNames[4], "%s_f", name); // Front texture
						StringFormat(textureNames[5], "%s_b", name); // Back texture

						if (!LoadCubeTextures(name, textureNames, texture))
						{
							*outTextureId = INVALID_ID;
							m_logger.Error("ProcessTextureReference() - Failed to load cube textures '{}'.", name);
							return false;
						}
					}
					else
					{
						if (!LoadTexture(name, texture))
						{
							*outTextureId = INVALID_ID;
							m_logger.Error("ProcessTextureReference() - Failed to load texture '{}.'", name);
							return false;
						}

					}

					texture->id = ref.handle;
					m_logger.Trace("ProcessTextureReference() - Texture '{}' did not exist yet. Created and refCount is now {}.", name, ref.referenceCount);
				}
			}
			else
			{
				*outTextureId = ref.handle;
				m_logger.Trace("ProcessTextureReference() - Texture '{}' already exists. RefCount is now {}.", name, ref.referenceCount);
			}
		}

		// Whatever happens if we get to this point we should update our hashtable
		m_registeredTextureTable.Set(nameCopy, &ref);
		return true;
	}

	bool TextureSystem::LoadJobEntryPoint(void* data, void* resultData)
	{
		const auto loadParams = static_cast<TextureLoadParams*>(data);

		constexpr ImageResourceParams resourceParams{ true };

		const auto result = Resources.Load(loadParams->resourceName, &loadParams->imageResource, resourceParams);
		if (result)
		{
			const ImageResourceData& resourceData = loadParams->imageResource.data;

			// Use our temporary texture to load into
			loadParams->tempTexture.width = resourceData.width;
			loadParams->tempTexture.height = resourceData.height;
			loadParams->tempTexture.channelCount = resourceData.channelCount;

			loadParams->currentGeneration = loadParams->outTexture->generation;
			loadParams->outTexture->generation = INVALID_ID;

			const u64 totalSize = static_cast<u64>(loadParams->tempTexture.width) * loadParams->tempTexture.height * loadParams->tempTexture.channelCount;
			// Check for transparency
			bool hasTransparency = false;
			for (u64 i = 0; i < totalSize; i += loadParams->tempTexture.channelCount)
			{
				const u8 a = resourceData.pixels[i + 3]; // Get the alpha channel of this pixel
				if (a < 255)
				{
					hasTransparency = true;
					break;
				}
			}

			// Take a copy of the name
			StringNCopy(loadParams->tempTexture.name, loadParams->resourceName, TEXTURE_NAME_MAX_LENGTH);
			loadParams->tempTexture.generation = INVALID_ID;
			loadParams->tempTexture.flags |= hasTransparency ? TextureFlag::HasTransparency : 0;

			// NOTE: The load params are also used as the result data here, only the imageResource field is populated now.
			Memory.Copy(resultData, loadParams, sizeof(TextureLoadParams));
		}

		return result;
	}

	void TextureSystem::LoadJobSuccess(void* data) const
	{
		const auto params = static_cast<TextureLoadParams*>(data);

		// TODO: This still handles the GPU upload. This can't be jobified before our renderer supports multiThreading.
		const ImageResourceData& resourceData = params->imageResource.data;

		// Acquire internal texture resources and upload to GPU.
		Renderer.CreateTexture(resourceData.pixels, &params->tempTexture);
		// Take a copy of the old texture.
		Texture old = *params->outTexture;
		// Assign the temp texture to the pointer
		*params->outTexture = params->tempTexture;
		// Destroy the old texture
		Renderer.DestroyTexture(&old);

		if (params->currentGeneration == INVALID_ID)
		{
			params->outTexture->generation = 0;
		}
		else
		{
			params->outTexture->generation++;
		}

		m_logger.Trace("LoadJobSuccess() - Successfully loaded texture '{}'.", params->resourceName);

		// Cleanup our data
		Resources.Unload(&params->imageResource);
		if (params->resourceName)
		{
			const auto length = StringLength(params->resourceName) + 1;
			Memory.Free(params->resourceName, sizeof(char) * length, MemoryType::String);
			params->resourceName = nullptr;
		}
	}

	void TextureSystem::LoadJobFailure(void* data) const
	{
		const auto params = static_cast<TextureLoadParams*>(data);
		m_logger.Error("LoadJobFailure() - Failed to load texture '{}'.", params->resourceName);
		Resources.Unload(&params->imageResource);
	}
}
