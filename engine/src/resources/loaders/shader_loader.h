
#pragma once
#include "resource_loader.h"
#include "resources/shaders/shader_types.h"

namespace C3D
{
    template <>
    class C3D_API ResourceLoader<ShaderConfig> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, ShaderConfig& resource) const;

        void Unload(ShaderConfig& resource) const;

    private:
        void ParseStages(ShaderConfig& data, const String& value) const;
        void ParseStageFiles(ShaderConfig& data, const String& value) const;

        void ParseAttribute(ShaderConfig& data, const String& value) const;
        bool ParseUniform(ShaderConfig& data, const String& value) const;
        static void ParseTopology(ShaderConfig& data, const String& value);

        static void ParseCullMode(ShaderConfig& data, const String& value);
    };
}  // namespace C3D
