
#pragma once
#include "core/defines.h"
#include "resources/texture.h"

#include <unordered_map>

#include "core/logger.h"

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

		bool LoadTexture(const string& name, Texture* texture) const;
		static void DestroyTexture(Texture* texture);

		LoggerInstance m_logger;
		bool m_initialized;

		TextureSystemConfig m_config;

		Texture m_defaultTexture;

		Texture* m_registeredTextures;
		std::unordered_map<string, TextureReference> m_registeredTextureTable;
	};
}
