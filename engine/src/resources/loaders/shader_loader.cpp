
#include "shader_loader.h"

#include "core/c3d_string.h"
#include "platform/filesystem.h"
#include "resources/shader.h"
#include "services/services.h"
#include "systems/resource_system.h"

namespace C3D
{
	ResourceLoader<ShaderResource>::ResourceLoader()
		: IResourceLoader("SHADER_LOADER", MemoryType::Shader, ResourceType::Shader, nullptr, "shaders")
	{}

	bool ResourceLoader<ShaderResource>::Load(const char* name, ShaderResource* outResource) const
	{
		if (StringLength(name) == 0 || !outResource)
		{
			m_logger.Error("Load() - Provided name was empty or no outResource pointer provided");
			return false;
		}

		char fullPath[512];
		const auto formatStr = "%s/%s/%s.%s";
		StringFormat(fullPath, formatStr, Resources.GetBasePath(), typePath, name, "shadercfg");

		File file;
		if (!file.Open(fullPath, FileModeRead))
		{
			m_logger.Error("Load() - Failed to open shader config file for reading: '{}'", fullPath);
			return false;
		}

		outResource->fullPath = StringDuplicate(fullPath);
		outResource->config.cullMode = FaceCullMode::Back;

		outResource->config.renderPassName = nullptr;
		outResource->config.name = nullptr;

		outResource->config.attributes.Reserve(4);
		outResource->config.uniforms.Reserve(8);

		string line;
		u32 lineNumber = 1;
		while (file.ReadLine(line))
		{
			// Trim the line
			Trim(line);

			// Skip Blank lines and comments
			if (line.empty() || line[0] == '#')
			{
				lineNumber++;
				continue;
			}

			// Split into variable and value
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
				outResource->config.name = StringDuplicate(value.data());
			}
			else if (IEquals(varName.data(), "renderPass"))
			{
				outResource->config.renderPassName = StringDuplicate(value.data());
			}
			else if (IEquals(varName.data(), "stages"))
			{
				ParseStages(&outResource->config, value);
			}
			else if (IEquals(varName.data(), "stageFiles"))
			{
				ParseStageFiles(&outResource->config, value);
			}
			else if (IEquals(varName.data(), "cullMode"))
			{
				ParseCullMode(&outResource->config, value);
			}
			else if (IEquals(varName.data(), "attribute"))
			{
				ParseAttribute(&outResource->config, value);
			}
			else if (IEquals(varName.data(), "uniform"))
			{
				ParseUniform(&outResource->config, value);
			}

			lineNumber++;
		}

		file.Close();
		return true;
	}

	void ResourceLoader<ShaderResource>::Unload(ShaderResource* resource) const
	{
		if (!resource)
		{
			m_logger.Warn("Unload() - Called with nullptr resource");
			return;
		}

		const auto data = &resource->config;

		for (const auto stageFileName : data->stageFileNames)
		{
			const auto count = StringLength(stageFileName) + 1;
			Memory.Free(stageFileName, count, MemoryType::String);
		}
		data->stageFileNames.clear();

		for (const auto stageName : data->stageNames)
		{
			const auto count = StringLength(stageName) + 1;
			Memory.Free(stageName, count, MemoryType::String);
		}
		data->stageNames.clear();
		data->stages.clear();

		// Cleanup attributes
		for (const auto& attribute : data->attributes)
		{
			const auto count = StringLength(attribute.name) + 1;
			Memory.Free(attribute.name, count, MemoryType::String);
		}
		data->attributes.Destroy();

		// Cleanup uniforms
		for (const auto& uniform : data->uniforms)
		{
			const auto count = StringLength(uniform.name);
			Memory.Free(uniform.name, count, MemoryType::String);
		}
		data->uniforms.Destroy();

		Memory.Free(data->renderPassName, StringLength(data->renderPassName) + 1, MemoryType::String);
		Memory.Free(data->name, StringLength(data->name) + 1, MemoryType::String);
	}

	void ResourceLoader<ShaderResource>::ParseStages(ShaderConfig* data, const string& value) const
	{
		data->stageNames = StringSplit(value, ',');
		if (!data->stageFileNames.empty() && data->stageNames.size() != data->stageFileNames.size())
		{
			// We already found the stage file names and the lengths don't match
			m_logger.Error("ParseStages() - Mismatch between the amount of StageNames ({}) and StageFileNames ({})",
				data->stageNames.size(), data->stageFileNames.size());
		}

		for (const auto stageName : data->stageNames)
		{
			if (IEquals(stageName, "frag") || IEquals(stageName, "fragment"))
			{
				data->stages.push_back(ShaderStage::Fragment);
			}
			else if (IEquals(stageName, "vert") || IEquals(stageName, "vertex"))
			{
				data->stages.push_back(ShaderStage::Vertex);
			}
			else if (IEquals(stageName, "geom") || IEquals(stageName, "geometry"))
			{
				data->stages.push_back(ShaderStage::Geometry);
			}
			else if (IEquals(stageName, "comp") || IEquals(stageName, "compute"))
			{
				data->stages.push_back(ShaderStage::Compute);
			}
			else
			{
				m_logger.Error("ParseStages() - Unrecognized stage '{}'", stageName);
			}
		}
	}

	void ResourceLoader<ShaderResource>::ParseStageFiles(ShaderConfig* data, const string& value) const
	{
		data->stageFileNames = StringSplit(value, ',');
		if (!data->stageNames.empty() && data->stageNames.size() != data->stageFileNames.size())
		{
			// We already found the stage names and the lengths don't match
			m_logger.Error("ParseStages() - Mismatch between the amount of StageNames ({}) and StageFileNames ({})",
				data->stageNames.size(), data->stageFileNames.size());
		}
	}

	void ResourceLoader<ShaderResource>::ParseAttribute(ShaderConfig* data, const string& value) const
	{
		const auto fields = StringSplit(value, ',');
		if (fields.size() != 2)
		{
			m_logger.Error("ParseAttribute() - Invalid layout. Attribute field must be 'type,name'. Skipping this line.");
			return;
		}

		ShaderAttributeConfig attribute{};
		if (IEquals(fields[0], "f32")) {
			attribute.type = Attribute_Float32;
			attribute.size = 4;
		}
		else if (IEquals(fields[0], "vec2")) {
			attribute.type = Attribute_Float32_2;
			attribute.size = 8;
		}
		else if (IEquals(fields[0], "vec3")) {
			attribute.type = Attribute_Float32_3;
			attribute.size = 12;
		}
		else if (IEquals(fields[0], "vec4")) {
			attribute.type = Attribute_Float32_4;
			attribute.size = 16;
		}
		else if (IEquals(fields[0], "u8")) {
			attribute.type = Attribute_UInt8;
			attribute.size = 1;
		}
		else if (IEquals(fields[0], "u16")) {
			attribute.type = Attribute_UInt16;
			attribute.size = 2;
		}
		else if (IEquals(fields[0], "u32")) {
			attribute.type = Attribute_UInt32;
			attribute.size = 4;
		}
		else if (IEquals(fields[0], "i8")) {
			attribute.type = Attribute_Int8;
			attribute.size = 1;
		}
		else if (IEquals(fields[0], "i16")) {
			attribute.type = Attribute_Int16;
			attribute.size = 2;
		}
		else if (IEquals(fields[0], "i32")) {
			attribute.type = Attribute_Int32;
			attribute.size = 4;
		}
		else {
			m_logger.Error("ParseAttribute() - Invalid file layout. Attribute type must be be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, or u32.");
			m_logger.Warn("Defaulting to f32.");
			attribute.type = Attribute_Float32;
			attribute.size = 4;
		}

		attribute.nameLength = static_cast<u8>(StringLength(fields[1]));
		attribute.name = StringDuplicate(fields[1]);

		for (const auto field : fields)
		{
			const auto count = StringLength(field) + 1;
			Memory.Free(field, count, MemoryType::String);
		}

		data->attributes.PushBack(attribute);
	}

	void ResourceLoader<ShaderResource>::ParseUniform(ShaderConfig* data, const string& value) const
	{
		const auto fields = StringSplit(value, ',');
		if (fields.size() != 3)
		{
			for (const auto field : fields)
			{
				const auto count = StringLength(field) + 1;
				Memory.Free(field, count, MemoryType::String);
			}

			m_logger.Error("ParseAttribute() - Invalid layout. Uniform field must be 'type,scope,name'. Skipping this line.");
			return;
		}

		ShaderUniformConfig uniform{};
		if (IEquals(fields[0], "f32")) {
			uniform.type = Uniform_Float32;
			uniform.size = 4;
		}
		else if (IEquals(fields[0], "vec2")) {
			uniform.type = Uniform_Float32_2;
			uniform.size = 8;
		}
		else if (IEquals(fields[0], "vec3")) {
			uniform.type = Uniform_Float32_3;
			uniform.size = 12;
		}
		else if (IEquals(fields[0], "vec4")) {
			uniform.type = Uniform_Float32_4;
			uniform.size = 16;
		}
		else if (IEquals(fields[0], "u8")) {
			uniform.type = Uniform_UInt8;
			uniform.size = 1;
		}
		else if (IEquals(fields[0], "u16")) {
			uniform.type = Uniform_UInt16;
			uniform.size = 2;
		}
		else if (IEquals(fields[0], "u32")) {
			uniform.type = Uniform_UInt32;
			uniform.size = 4;
		}
		else if (IEquals(fields[0], "i8")) {
			uniform.type = Uniform_Int8;
			uniform.size = 1;
		}
		else if (IEquals(fields[0], "i16")) {
			uniform.type = Uniform_Int16;
			uniform.size = 2;
		}
		else if (IEquals(fields[0], "i32")) {
			uniform.type = Uniform_Int32;
			uniform.size = 4;
		}
		else if (IEquals(fields[0], "mat4")) {
			uniform.type = Uniform_Matrix4;
			uniform.size = 64;
		}
		else if (IEquals(fields[0], "samp") || IEquals(fields[0], "sampler")) {
			uniform.type = Uniform_Sampler;
			uniform.size = 0;  // Samplers don't have a size.
		}
		else {
			m_logger.Error("ParseUniform(). Invalid file layout. Uniform type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 or mat4.");
			m_logger.Warn("Defaulting to f32.");
			uniform.type = Uniform_Float32;
			uniform.size = 4;
		}

		if (IEquals(fields[1], "global"))
		{
			uniform.scope = ShaderScope::Global;
		}
		else if (IEquals(fields[1], "instance"))
		{
			uniform.scope = ShaderScope::Instance;
		}
		else if (IEquals(fields[1], "local"))
		{
			uniform.scope = ShaderScope::Local;
		}
		else
		{
			m_logger.Error("ParseUniform(). Invalid file layout. Uniform scope must be global, instance or local.");
			m_logger.Warn("Defaulting to global.");
			uniform.scope = ShaderScope::Global;
		}

		uniform.nameLength = static_cast<u8>(StringLength(fields[2]));
		uniform.name = StringDuplicate(fields[2]);

		for (const auto field : fields)
		{
			const auto count = StringLength(field) + 1;
			Memory.Free(field, count, MemoryType::String);
		}

		data->uniforms.PushBack(uniform);
	}

	void ResourceLoader<ShaderResource>::ParseCullMode(ShaderConfig* data, const string& value) const
	{
		if (IEquals(value.data(), "front"))
		{
			data->cullMode = FaceCullMode::Front;
		}
		else if (IEquals(value.data(), "front_and_back"))
		{
			data->cullMode = FaceCullMode::FrontAndBack;
		}
		else if (IEquals(value.data(), "none"))
		{
			data->cullMode = FaceCullMode::None;
		}
		// Default is BACK so we don't have to do anyting
	}
}
