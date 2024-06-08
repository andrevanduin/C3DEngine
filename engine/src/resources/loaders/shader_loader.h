
#pragma once
#include "base_text_loader.h"
#include "resource_loader.h"
#include "resources/shaders/shader_types.h"

namespace C3D
{
    template <>
    class C3D_API ResourceLoader<ShaderConfig> final : public IResourceLoader, BaseTextLoader<ShaderConfig>
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, ShaderConfig& resource) const;

        static void Unload(ShaderConfig& resource);

    private:
        enum class ParserTagType
        {
            None,
            General,
            Stages,
            Attributes,
            Uniforms
        };

        enum class ParserUniformScope
        {
            None,
            Global,
            Instance,
            Local
        };

        void SetDefaults(ShaderConfig& resource) const override;
        void ParseNameValuePair(const String& name, const String& value, ShaderConfig& resource) const override;
        void ParseTag(const String& name, bool isOpeningTag, ShaderConfig& resource) const override;

        void ParseGeneral(const String& name, const String& value, ShaderConfig& resource) const;
        void ParseStages(const String& name, const String& value, ShaderConfig& resource) const;
        void ParseAttribute(const String& name, const String& value, ShaderConfig& resource) const;
        void ParseUniform(const String& name, const String& value, ShaderConfig& resource) const;

        mutable ParserTagType m_currentTagType;
        mutable ParserUniformScope m_currentUniformScope;

        static void ParseTopology(ShaderConfig& data, const String& value);
        static void ParseCullMode(ShaderConfig& data, const String& value);
    };
}  // namespace C3D
