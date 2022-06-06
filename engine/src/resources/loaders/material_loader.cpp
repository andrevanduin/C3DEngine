
#include "material_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "platform/filesystem.h"
#include "services/services.h"

#include "systems/resource_system.h"

namespace C3D
{
	constexpr auto BUILTIN_SHADER_NAME_MATERIAL = "Shader.Builtin.Material";

	MaterialLoader::MaterialLoader()
		: ResourceLoader("MATERIAL_LOADER", MemoryType::MaterialInstance, ResourceType::Material, nullptr, "materials")
	{}

	bool MaterialLoader::Load(const string& name, Resource* outResource)
	{
		if (!outResource) return false;

		char fullPath[512];
		const auto formatStr = "%s/%s/%s.%s";

		// TODO: try different extensions
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name.data(), "mt");

		File file;
		if (!file.Open(fullPath, FileModeRead))
		{
			m_logger.Error("Unable to open material file for reading: '{}'", fullPath);
			return false;
		}

		outResource->fullPath = StringDuplicate(fullPath);

		auto* resourceData = Memory.Allocate<MaterialConfig>(MemoryType::MaterialInstance);
		resourceData->shaderName = StringDuplicate(BUILTIN_SHADER_NAME_MATERIAL);
		resourceData->autoRelease = true;
		resourceData->diffuseColor = vec4(1); // Default white
		resourceData->diffuseMapName[0] = 0;

		StringNCopy(resourceData->name, name.data(), MATERIAL_NAME_MAX_LENGTH);

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

			if (IEquals(varName, "version"))
			{
				// TODO: version
			}
			else if (IEquals(varName, "name"))
			{
				StringNCopy(resourceData->name, value.data(), MATERIAL_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName, "diffuseMapName"))
			{
				StringNCopy(resourceData->diffuseMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName, "specularMapName"))
			{
				StringNCopy(resourceData->specularMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName, "normalMapName"))
			{
				StringNCopy(resourceData->normalMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName, "diffuseColor"))
			{
				if (!StringToVec4(value.data(), &resourceData->diffuseColor))
				{
					m_logger.Warn("Error parsing diffuseColor in file '{}'. Using default of white instead", fullPath);
				}
			}
			else if (IEquals(varName, "shader"))
			{
				resourceData->shaderName = StringDuplicate(value.data());
			}
			else if (IEquals(varName, "shininess"))
			{
				if (!StringToF32(value.data(), &resourceData->shininess))
				{
					m_logger.Warn("Error Parsing shininess in file '{}'. Using default of 32.0 instead", fullPath);
					resourceData->shininess = 32.0f;
				}
			}

			// TODO: more fields

			lineNumber++;
		}

		file.Close();

		outResource->data = resourceData;
		outResource->dataSize = sizeof(MaterialConfig);
		outResource->name = name.data();

		return true;
	}
}
