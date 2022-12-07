
#include "material_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "platform/filesystem.h"
#include "resources/material.h"
#include "services/services.h"

#include "systems/resource_system.h"

namespace C3D
{
	constexpr auto BUILTIN_SHADER_NAME_MATERIAL = "Shader.Builtin.Material";

	ResourceLoader<MaterialResource>::ResourceLoader()
		: IResourceLoader("MATERIAL_LOADER", MemoryType::MaterialInstance, ResourceType::Material, nullptr, "materials")
	{}

	bool ResourceLoader<MaterialResource>::Load(const char* name, MaterialResource* outResource) const
	{
		if (StringLength(name) == 0 || !outResource) return false;

		char fullPath[512];
		const auto formatStr = "%s/%s/%s.%s";

		// TODO: try different extensions
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name, "mt");

		File file;
		if (!file.Open(fullPath, FileModeRead))
		{
			m_logger.Error("Unable to open material file for reading: '{}'", fullPath);
			return false;
		}

		outResource->fullPath = fullPath;
		outResource->config.autoRelease = true;
		outResource->config.diffuseColor = vec4(1); // Default white
		outResource->config.diffuseMapName[0] = 0;

		StringNCopy(outResource->config.name, name, MATERIAL_NAME_MAX_LENGTH);

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
				StringNCopy(outResource->config.name, value.Data(), MATERIAL_NAME_MAX_LENGTH);
			}
			else if (varName.IEquals("diffuseMapName"))
			{
				StringNCopy(outResource->config.diffuseMapName, value.Data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (varName.IEquals("specularMapName"))
			{
				StringNCopy(outResource->config.specularMapName, value.Data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (varName.IEquals("normalMapName"))
			{
				StringNCopy(outResource->config.normalMapName, value.Data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (varName.IEquals("diffuseColor"))
			{
				if (!StringToVec4(value.Data(), &outResource->config.diffuseColor))
				{
					m_logger.Warn("Error parsing diffuseColor in file '{}'. Using default of white instead", fullPath);
				}
			}
			else if (varName.IEquals("shader"))
			{
				outResource->config.shaderName = value;
			}
			else if (varName.IEquals("shininess"))
			{
				if (!StringToF32(value.Data(), &outResource->config.shininess))
				{
					m_logger.Warn("Error Parsing shininess in file '{}'. Using default of 32.0 instead", fullPath);
					outResource->config.shininess = 32.0f;
				}
			}

			// TODO: more fields

			lineNumber++;
		}

		file.Close();

		outResource->name = name;
		return true;
	}

	void ResourceLoader<MaterialResource>::Unload(MaterialResource* resource)
	{
		resource->config.shaderName.Destroy();
		resource->fullPath.Destroy();
		resource->name.Destroy();
	}
}
