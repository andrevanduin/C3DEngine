
#pragma once
#include "resource_loader.h"

namespace C3D
{
	struct ShaderConfig;

	class ShaderLoader final : public ResourceLoader
	{
	public:
		ShaderLoader();

		bool Load(const char* name, Resource* outResource) override;

		void Unload(Resource* resource) override;

	private:
		void ParseStages(ShaderConfig* data, const string& value) const;
		void ParseStageFiles(ShaderConfig* data, const string& value) const;

		void ParseAttribute(ShaderConfig* data, const string& value) const;
		void ParseUniform(ShaderConfig* data, const string& value) const;
	};
}