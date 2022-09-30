
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
		ResourceLoader();

		bool Load(const char* name, ShaderResource* outResource) const;

		void Unload(ShaderResource* resource) const;

	private:
		void ParseStages(ShaderConfig* data, const string& value) const;
		void ParseStageFiles(ShaderConfig* data, const string& value) const;

		void ParseAttribute(ShaderConfig* data, const string& value) const;
		void ParseUniform(ShaderConfig* data, const string& value) const;
	};
}
