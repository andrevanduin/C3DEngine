
#pragma once
#include "core/defines.h"
#include "resources/texture.h"

#include <unordered_map>

namespace C3D
{
	constexpr auto DEFAULT_TEXTURE_NAME = "default";

	struct TextureSystemConfig
	{
		u32 maxTextureCount;
	};

	struct TextureReference
	{
		u64 referenceCount;
		u32 handle;
		bool autoRelease;
	};

	class TextureSystem
	{
	public:
		TextureSystem();

		bool Init(const TextureSystemConfig& config);
		void Shutdown();

		Texture* Acquire(const string& name, bool autoRelease);
		void Release(const string& name);

		Texture* GetDefaultTexture();

	private:
		bool CreateDefaultTextures();
		void DestroyDefaultTextures();

		static bool LoadTexture(const string& name, Texture* texture);
		static void DestroyTexture(Texture* texture);

		TextureSystemConfig m_config;

		bool m_initialized;

		Texture m_defaultTexture;

		Texture* m_registeredTextures;
		std::unordered_map<string, TextureReference> m_registeredTextureTable;
	};
}
