
#include "material_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "platform/filesystem.h"
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

		outResource->fullPath = StringDuplicate(fullPath);
		outResource->config.autoRelease = true;
		outResource->config.diffuseColor = vec4(1); // Default white
		outResource->config.diffuseMapName[0] = 0;

		StringNCopy(outResource->config.name, name, MATERIAL_NAME_MAX_LENGTH);

		string line;
		// Prepare for strings of up to 512 characters so we don't needlessly resize
		line.reserve(512);

		u32 lineNumber = 0;
		while (file.ReadLine(line))
		{
			Trim(line);

			// Skip blank lines and comments
			if (line.empty() || line[0] == '#')
			{
				lineNumber++;
				continue;
			}

			// Find the '=' symbol
			const auto equalIndex = line.find('=');
			if (equalIndex == string::npos)
			{
				m_logger.Warn("Potential formatting issue found in file '': '=' token not found. Skipping line {}.", fullPath, lineNumber);
				lineNumber++;
				continue;
			}

			// Get the variable name (which is all the characters up to the '=' and trim
			auto varName = line.substr(0, equalIndex);
			Trim(varName);

			// Get the value (which is all the characters after the '=' and trim
			auto value = line.substr(equalIndex + 1);
			Trim(value);

			if (IEquals(varName.data(), "version"))
			{
				// TODO: version
			}
			else if (IEquals(varName.data(), "name"))
			{
				StringNCopy(outResource->config.name, value.data(), MATERIAL_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName.data(), "diffuseMapName"))
			{
				StringNCopy(outResource->config.diffuseMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName.data(), "specularMapName"))
			{
				StringNCopy(outResource->config.specularMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName.data(), "normalMapName"))
			{
				StringNCopy(outResource->config.normalMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName.data(), "diffuseColor"))
			{
				if (!StringToVec4(value.data(), &outResource->config.diffuseColor))
				{
					m_logger.Warn("Error parsing diffuseColor in file '{}'. Using default of white instead", fullPath);
				}
			}
			else if (IEquals(varName.data(), "shader"))
			{
				outResource->config.shaderName = StringDuplicate(value.data());
			}
			else if (IEquals(varName.data(), "shininess"))
			{
				if (!StringToF32(value.data(), &outResource->config.shininess))
				{
					m_logger.Warn("Error Parsing shininess in file '{}'. Using default of 32.0 instead", fullPath);
					outResource->config.shininess = 32.0f;
				}
			}

			// TODO: more fields

			lineNumber++;
		}

		file.Close();

		outResource->name = StringDuplicate(name);
		return true;
	}

	void ResourceLoader<MaterialResource>::Unload(const MaterialResource* resource)
	{
		if (resource->config.shaderName)
		{
			const auto count = StringLength(resource->config.shaderName) + 1;
			Memory.Free(resource->config.shaderName, count, MemoryType::String);
		}
	}
}
