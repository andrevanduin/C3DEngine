
#include "material_loader.h"

#include "core/logger.h"

#include "platform/filesystem.h"
#include "resources/material.h"
#include "systems/system_manager.h"

#include "systems/resource_system.h"

namespace C3D
{
	constexpr auto BUILTIN_SHADER_NAME_MATERIAL = "Shader.Builtin.Material";

	ResourceLoader<MaterialResource>::ResourceLoader()
		: IResourceLoader("MATERIAL_LOADER", MemoryType::MaterialInstance, ResourceType::Material, nullptr, "materials")
	{}

	bool ResourceLoader<MaterialResource>::Load(const char* name, MaterialResource& resource) const
	{
		if (std::strlen(name) == 0) return false;

		// TODO: try different extensions
		auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, "mt");

		File file;
		if (!file.Open(fullPath, FileModeRead))
		{
			m_logger.Error("Unable to open material file for reading: '{}'", fullPath);
			return false;
		}

		resource.fullPath = fullPath;
		resource.config.autoRelease = true;
		resource.config.diffuseColor = vec4(1); // Default white
		resource.config.name = name;

		String line;
		// Prepare for strings of up to 512 characters so we don't needlessly resize
		line.Reserve(512);

		u32 lineNumber = 0;
		while (file.ReadLine(line))
		{
			line.Trim();

			// Skip blank lines and comments
			if (line.Empty() || line.First() == '#')
			{
				lineNumber++;
				continue;
			}

			// Find the '=' symbol
			const auto equalIndex = std::find(line.begin(), line.end(), '=');
			if (equalIndex == line.end())
			{
				m_logger.Warn("Potential formatting issue found in file '': '=' token not found. Skipping line {}.", fullPath, lineNumber);
				lineNumber++;
				continue;
			}

			// Get the variable name (which is all the characters up to the '=' and trim
			auto varName = line.SubStr(line.begin(), equalIndex);
			varName.Trim();

			// Get the value (which is all the characters after the '=' and trim
			auto value = line.SubStr(equalIndex + 1, line.end());
			value.Trim();

			if (varName.IEquals("version"))
			{
				// TODO: version
			}
			else if (varName.IEquals("name"))
			{
				resource.config.name = value.Data();
			}
			else if (varName.IEquals("diffuseMapName"))
			{
				resource.config.diffuseMapName = value.Data();
			}
			else if (varName.IEquals("specularMapName"))
			{
				resource.config.specularMapName = value.Data();
			}
			else if (varName.IEquals("normalMapName"))
			{
				resource.config.normalMapName = value.Data();
			}
			else if (varName.IEquals("diffuseColor"))
			{
				resource.config.diffuseColor = value.ToVec4();
			}
			else if (varName.IEquals("shader"))
			{
				resource.config.shaderName = value;
			}
			else if (varName.IEquals("shininess"))
			{
				resource.config.shininess = value.ToF32();
			}

			// TODO: more fields

			lineNumber++;
		}

		file.Close();

		resource.name = name;
		return true;
	}

	void ResourceLoader<MaterialResource>::Unload(MaterialResource& resource)
	{
		resource.config.shaderName.Destroy();
		resource.fullPath.Destroy();
		resource.name.Destroy();
	}
}
