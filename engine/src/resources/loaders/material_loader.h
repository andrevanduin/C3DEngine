
#pragma once
#include "base_text_loader.h"
#include "resources/materials/material_types.h"

namespace C3D
{
    template <>
    class C3D_API ResourceLoader<MaterialConfig> final : public IResourceLoader, BaseTextLoader<MaterialConfig>
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, MaterialConfig& resource) const;

        static void Unload(MaterialConfig& resource);

    private:
        enum ParserTagType
        {
            Global,
            Map,
            Prop
        };

        void SetDefaults(MaterialConfig& resource) const override;
        void ParseNameValuePair(const String& name, const String& value, MaterialConfig& resource) const override;
        void ParseTag(const String& name, bool isOpeningTag, MaterialConfig& resource) const override;

        void ParseGlobal(const String& name, const String& value, MaterialConfig& resource) const;
        void ParseGlobalV2(const String& name, const String& value, MaterialConfig& resource) const;

        void ParseMap(const String& name, const String& value, MaterialConfig& resource) const;
        void ParseProp(const String& name, const String& value, MaterialConfig& resource) const;

        mutable ParserTagType m_currentTagType;
    };
}  // namespace C3D
