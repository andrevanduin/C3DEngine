
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "renderer/render_buffer.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"

namespace C3D
{
    class SystemManager;
    class FontSystem;
    class RenderSystem;
    class ShaderSystem;
    struct FontData;
    struct FontGlyph;
    struct FontKerning;

    enum class UITextType
    {
        Unknown,
        Bitmap,
        System,
    };

    class C3D_API UIText
    {
    public:
        UIText();

        UIText(const UIText&) = delete;
        UIText(UIText&&) = delete;

        UIText& operator=(const UIText&) = delete;
        UIText& operator=(UIText&&) = delete;

        ~UIText();

        bool Create(const SystemManager* pSystemsManager, UITextType fontType, const char* fontName, u16 fontSize,
                    const char* textContent);
        void Destroy();

        void SetPosition(const vec3& pos);
        void SetText(const char* text);

        [[nodiscard]] u32 GetMaxX() const;
        [[nodiscard]] u32 GetMaxY() const;

        void Draw() const;

        u32 uniqueId = INVALID_ID;

        UITextType type = UITextType::Unknown;
        FontData* data = nullptr;

        u32 instanceId = INVALID_ID;
        u64 frameNumber = INVALID_ID_U64;

        Transform transform;

    private:
        void RegenerateGeometry();
        [[nodiscard]] FontGlyph* GetFontGlyph(i32 codepoint) const;
        [[nodiscard]] f32 GetFontKerningAmount(i32 codepoint, u32 offset, u64 utf8Size) const;

        LoggerInstance<16> m_logger;

        RenderBuffer* m_vertexBuffer = nullptr;
        RenderBuffer* m_indexBuffer = nullptr;

        DynamicArray<Vertex2D> m_vertexData;
        DynamicArray<u32> m_indexData;

        f32 m_maxX = 0;
        f32 m_maxY = 0;

        String m_text;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D
