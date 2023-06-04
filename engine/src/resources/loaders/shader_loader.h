
#pragma once
#include "resource_loader.h"
#include "resources/shader.h"

namespace C3D
{
	struct ShaderConfig;

	struct ShaderResource final : Resource
	{
		ShaderConfig config;
	};

	template <>
	class ResourceLoader<ShaderResource> final : public IResourceLoader
	{
	public:
		explicit ResourceLoader(const Engine* engine);

		bool Load(const char* name, ShaderResource& resource) const;

		void Unload(ShaderResource& resource) const;

	private:
		void ParseStages(ShaderConfig& data, const String& value) const;
		void ParseStageFiles(ShaderConfig& data, const String& value) const;

		void ParseAttribute(ShaderConfig& data, const String& value) const;
		void ParseUniform(ShaderConfig& data, const String& value) const;
		static void ParseTopology(ShaderConfig& data, const String& value);

		static void ParseCullMode(ShaderConfig& data, const String& value);
	};
}
