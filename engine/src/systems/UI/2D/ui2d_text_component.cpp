
#include "containers/string.h"
#include "renderer/renderer_frontend.h"
#include "systems/fonts/font_system.h"
#include "systems/shaders/shader_system.h"
#include "ui2d_system.h"

namespace C3D
{
    /*
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr u64 VERTICES_PER_QUAD     = 4;
    constexpr u64 INDICES_PER_QUAD      = 6;

    using namespace UI_2D;

    bool UI2DSystem::SetupTextComponent(Entity entity, const String& text, FontHandle font)
    {
        auto& textComponent = m_ecs.AddComponent<TextComponent>(entity);
        textComponent.text  = text;
        textComponent.font  = font;
        if (!textComponent.font)
        {
            ERROR_LOG("Failed to Acquire Font: '{}'.", font);
            return false;
        }

        auto textSize = text.SizeUtf8();
        // Ensure that text size is larger than 0 since we can't create an empty buffer
        if (textSize < 1) textSize = 0;

        // Verify that our atlas has all the required glyphs
        if (!Fonts.VerifyAtlas(textComponent.font, text))
        {
            ERROR_LOG("Font atlas verification failed.");
            return false;
        }

        RegenerateTextComponent(entity);

        return true;
    }

    FontGlyph* GetFontGlyph(const FontData& data, const i32 codepoint)
    {
        for (auto& g : data.glyphs)
        {
            if (g.codepoint == codepoint)
            {
                return &g;
            }
        }
        return nullptr;
    }

    f32 GetFontKerningAmount(const FontData& data, const String& text, const i32 codepoint, const u32 offset, const u64 utf8Size)
    {
        if (offset > utf8Size - 1) return 0;

        u8 advanceNext          = 0;
        const i32 nextCodepoint = text.ToCodepoint(offset, advanceNext);
        if (nextCodepoint == -1) return 0;

        for (const auto& k : data.kernings)
        {
            if (k.codepoint0 == codepoint && k.codepoint1 == nextCodepoint)
            {
                return k.amount;
            }
        }
        return 0;
    }

    void UI2DSystem::RegenerateTextComponent(Entity entity)
    {
        auto& component = m_ecs.GetComponent<TextComponent>(entity);

        const u64 utf8Size = component.text.SizeUtf8();

        const u64 vertexCount      = utf8Size * VERTICES_PER_QUAD;
        const u64 indexCount       = utf8Size * INDICES_PER_QUAD;
        const u64 vertexBufferSize = sizeof(Vertex2D) * vertexCount;
        const u64 indexBufferSize  = sizeof(u32) * indexCount;

        if (utf8Size > component.textCapacity)
        {
            // New text is larger then the current capacity we allocated for

            if (component.textCapacity > 0)
            {
                // If we previously had some data we need to free it
                const u64 oldVertexBufferSize = sizeof(Vertex2D) * component.textCapacity * VERTICES_PER_QUAD;
                const u64 oldIndexBufferSize  = sizeof(u32) * component.textCapacity * INDICES_PER_QUAD;

                // So first we free the current allocation for vertices
                if (!Renderer.FreeInRenderBuffer(RenderBufferType::Vertex, oldVertexBufferSize, component.vertexBufferOffset))
                {
                    ERROR_LOG("Failed to free in Render's Vertex Buffer with size: {} and offset: {}.", oldVertexBufferSize,
                              component.vertexBufferOffset);
                }
                // And our indices
                if (!Renderer.FreeInRenderBuffer(RenderBufferType::Index, oldIndexBufferSize, component.indexBufferOffset))
                {
                    ERROR_LOG("Failed to free in Render's Index Buffer with size: {} and offset: {}.", oldIndexBufferSize,
                              component.indexBufferOffset);
                }
            }

            // Then we allocate new bigger capacity for our vertices
            if (!Renderer.AllocateInRenderBuffer(RenderBufferType::Vertex, vertexBufferSize, component.vertexBufferOffset))
            {
                ERROR_LOG("Failed to allocate in Render's Vertex Buffer width size: {}", vertexBufferSize);
            }
            // And our indices
            if (!Renderer.AllocateInRenderBuffer(RenderBufferType::Index, indexBufferSize, component.indexBufferOffset))
            {
                ERROR_LOG("Failed to allocate in Render's Index Buffer width size: {}", indexBufferSize);
            }

            // Save off our new max capacity
            component.textCapacity = utf8Size;
        }

        f32 x = 0, y = 0;

        component.maxX = 0;
        component.maxY = 0;

        // Clear our temp data
        component.vertices.Clear();
        component.indices.Clear();

        // Make sure we have enough storage for our temp data
        component.vertices.Reserve(vertexCount);
        component.indices.Reserve(indexCount);

        auto& data = Fonts.GetFontData(component.font);

        for (u32 c = 0, uc = 0; c < component.text.Size(); ++c)
        {
            i32 codepoint = component.text[c];

            // Continue to the next line for newlines
            if (codepoint == '\n')
            {
                x = 0;
                y += static_cast<f32>(data.lineHeight);
                continue;
            }

            if (codepoint == '\t')
            {
                x += data.tabXAdvance;
                continue;
            }

            u8 advance = 0;
            codepoint  = component.text.ToCodepoint(c, advance);

            const FontGlyph* glyph = GetFontGlyph(data, codepoint);
            if (!glyph)
            {
                // If we don't have a valid glyph for the codepoint we simply revert to the codepoint = 1 glyph
                // (fallback glyph)
                glyph = GetFontGlyph(data, -1);
            }

            if (glyph)
            {
                const f32 minX = x + static_cast<f32>(glyph->xOffset);
                const f32 minY = y + static_cast<f32>(glyph->yOffset);
                const f32 maxX = minX + static_cast<f32>(glyph->width);
                const f32 maxY = minY + static_cast<f32>(glyph->height);

                if (maxX > component.maxX) component.maxX = maxX;
                if (maxY > component.maxY) component.maxY = maxY;

                const f32 atlasSizeX = static_cast<f32>(data.atlasSizeX);
                const f32 atlasSizeY = static_cast<f32>(data.atlasSizeY);

                const f32 tMinX = static_cast<f32>(glyph->x) / atlasSizeX;
                f32 tMinY       = static_cast<f32>(glyph->y) / atlasSizeY;
                const f32 tMaxX = static_cast<f32>(glyph->x + glyph->width) / atlasSizeX;
                f32 tMaxY       = static_cast<f32>(glyph->y + glyph->height) / atlasSizeY;

                // Flip the y-axis for system text
                if (data.type == FontType::System)
                {
                    tMinY = 1.0f - tMinY;
                    tMaxY = 1.0f - tMaxY;
                }

                component.vertices.EmplaceBack(vec2(minX, minY), vec2(tMinX, tMinY));
                component.vertices.EmplaceBack(vec2(maxX, maxY), vec2(tMaxX, tMaxY));
                component.vertices.EmplaceBack(vec2(minX, maxY), vec2(tMinX, tMaxY));
                component.vertices.EmplaceBack(vec2(maxX, minY), vec2(tMaxX, tMinY));

                // Increment our x by the xAdvance plus any potential kerning
                x += static_cast<f32>(glyph->xAdvance) + GetFontKerningAmount(data, component.text, codepoint, c + advance, utf8Size);
            }
            else
            {
                ERROR_LOG("Failed find codepoint. Skipping this glyph.");
                continue;
            }

            component.indices.PushBack(uc * 4 + 2);
            component.indices.PushBack(uc * 4 + 1);
            component.indices.PushBack(uc * 4 + 0);
            component.indices.PushBack(uc * 4 + 3);
            component.indices.PushBack(uc * 4 + 0);
            component.indices.PushBack(uc * 4 + 1);

            // Increment our character index (subtracting 1 since our loop will increment every iteration by 1)
            c += advance - 1;
            uc++;
        }

        if (utf8Size > 0)
        {
            // Load up our vertex and index buffer data
            if (!Renderer.LoadRangeInRenderBuffer(RenderBufferType::Vertex, component.vertexBufferOffset, vertexBufferSize,
                                                  component.vertices.GetData()))
            {
                ERROR_LOG("Failed to LoadRange() for vertex buffer.");
            }
            if (!Renderer.LoadRangeInRenderBuffer(RenderBufferType::Index, component.indexBufferOffset, indexBufferSize,
                                                  component.indices.GetData()))
            {
                ERROR_LOG("Failed to LoadRange() for index buffer.");
            }
        }
    }

    bool UI2DSystem::SetText(Entity entity, const char* text)
    {
        ASSERT_VALID(entity);

        if (!m_ecs.HasComponent<TextComponent>(entity))
        {
            ERROR_LOG("Entity: {} does not have a TextComponent.", entity);
            return false;
        }

        auto& component = m_ecs.GetComponent<TextComponent>(entity);

        // Create a tmp string
        String tmp = text;
        // If the strings match we don't need to do anything
        if (component.text == tmp)
        {
            return true;
        }

        // Move our tmp string to the component
        component.text = std::move(tmp);
        RegenerateTextComponent(entity);

        return true;
    }

    bool UI2DSystem::SetText(Entity entity, const String& text)
    {
        ASSERT_VALID(entity);

        if (!m_ecs.HasComponent<TextComponent>(entity))
        {
            ERROR_LOG("Entity: {} does not have a TextComponent.", entity);
            return false;
        }

        auto& component = m_ecs.GetComponent<TextComponent>(entity);
        // If the strings match we don't need to do anything
        if (component.text == text)
        {
            return true;
        }

        component.text = text;
        RegenerateTextComponent(entity);

        return true;
    }

    f32 UI2DSystem::GetMaxTextX(Entity entity)
    {
        ASSERT_VALID(entity);
        if (!m_ecs.HasComponent<TextComponent>(entity))
        {
            ERROR_LOG("Entity: {} does not have a TextComponent.", entity);
            return 0.0f;
        }

        auto& component = m_ecs.GetComponent<TextComponent>(entity);
        return component.maxX;
    }

    f32 UI2DSystem::GetMaxTextY(Entity entity)
    {
        ASSERT_VALID(entity);
        if (!m_ecs.HasComponent<TextComponent>(entity))
        {
            ERROR_LOG("Entity: {} does not have a TextComponent.", entity);
            return 0.0f;
        }

        auto& component = m_ecs.GetComponent<TextComponent>(entity);
        return component.maxY;
    }

    void UI2DSystem::DestroyTextComponent(Entity entity)
    {
        auto& component = m_ecs.GetComponent<TextComponent>(entity);
        component.text.Destroy();
        component.vertices.Destroy();
        component.indices.Destroy();

        const auto vertexSize = component.textCapacity * VERTICES_PER_QUAD * sizeof(Vertex2D);
        if (!Renderer.FreeInRenderBuffer(RenderBufferType::Vertex, vertexSize, component.vertexBufferOffset))
        {
            ERROR_LOG("Failed to Free in Renderer's Vertex buffer.");
        }

        const auto indexSize = component.textCapacity * INDICES_PER_QUAD * sizeof(u32);
        if (!Renderer.FreeInRenderBuffer(RenderBufferType::Index, indexSize, component.indexBufferOffset))
        {
            ERROR_LOG("Failed to Free in Renderer's Index buffer.");
        }
    }
    */
}  // namespace C3D