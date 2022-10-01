
#pragma once
#include "core/defines.h"
#include "resources/texture.h"

#include <unordered_map>

#include "containers/hash_table.h"
#include "core/logger.h"

namespace C3D
{
	constexpr auto DEFAULT_TEXTURE_NAME = "default";
	constexpr auto DEFAULT_DIFFUSE_TEXTURE_NAME = "defaultDiffuse";
	constexpr auto DEFAULT_SPECULAR_TEXTURE_NAME = "defaultSpecular";
	constexpr auto DEFAULT_NORMAL_TEXTURE_NAME = "defaultNormal";

	struct TextureSystemConfig
	{
		u32 maxTextureCount;
	};

	struct TextureReference
	{
		TextureReference() : referenceCount(0), handle(INVALID_ID), autoRelease(false) {}

		u64 referenceCount;
		u32 handle;
		bool autoRelease;
	};

	class TextureSystem final : public System<TextureSystemConfig>
	{
	public:
		TextureSystem();

		bool Init(const TextureSystemConfig& config);
		void Shutdown();

		Texture* Acquire(const char* name, bool autoRelease);
		Texture* AcquireWritable(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency);

		void Release(const char* name);

		Texture* WrapInternal(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency, bool isWritable, bool registerTexture, void* internalData);

		static bool SetInternal(Texture* t, void* internalData);

		bool Resize(Texture* t, u32 width, u32 height, bool regenerateInternalData);

		Texture* GetDefault();
		Texture* GetDefaultDiffuse();
		Texture* GetDefaultSpecular();
		Texture* GetDefaultNormal();

		bool CreateDefaultTextures();
	private:
		void DestroyDefaultTextures();

		bool LoadTexture(const char* name, Texture* texture) const;
		static void DestroyTexture(Texture* texture);

		bool ProcessTextureReference(const char* name, i8 referenceDiff, bool autoRelease, bool skipLoad, u32* outTextureId);

		bool m_initialized;

		Texture m_defaultTexture;
		Texture m_defaultDiffuseTexture;
		Texture m_defaultSpecularTexture;
		Texture m_defaultNormalTexture;

		Texture* m_registeredTextures;
		HashTable<TextureReference> m_registeredTextureTable;
	};
}
