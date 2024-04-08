
#include "text_component.h"

#include "core/colors.h"
#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/fonts/font_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr u64 VERTICES_PER_QUAD     = 4;
    constexpr u64 INDICES_PER_QUAD      = 6;

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

    bool TextComponent::Initialize(Component& self, FontHandle f, const String& t)
    {
        font = f;
        text = t;

        if (!font)
        {
            ERROR_LOG("Failed to Acquire Font: '{}'.", f);
            return false;
        }

        auto& fontSystem   = self.GetSystem<FontSystem>();
        auto& renderSystem = self.GetSystem<RenderSystem>();
        auto& uiSystem     = self.GetSystem<UI2DSystem>();

        auto textSize = text.SizeUtf8();
        // Ensure that text size is larger than 0 since we can't create an empty buffer
        if (textSize < 1) textSize = 1;

        // Acquire shader instance resources
        TextureMap* maps[1] = { &fontSystem.GetFontData(font).atlas };
        if (!renderSystem.AcquireShaderInstanceResources(uiSystem.GetShader(), 1, maps, &renderable.instanceId))
        {
            ERROR_LOG("Failed to Acquire Shader Instance resources.");
            return false;
        }

        // Verify that our atlas has all the required glyphs
        if (!fontSystem.VerifyAtlas(font, text))
        {
            ERROR_LOG("Font atlas verification failed.");
            return false;
        }

        vertices = DynamicArray<Vertex2D>();
        indices  = DynamicArray<u32>();

        Regenerate(self);
        return true;
    }

    void TextComponent::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& fontSystem   = self.GetSystem<FontSystem>();
        auto& shaderSystem = self.GetSystem<ShaderSystem>();
        auto& uiSystem     = self.GetSystem<UI2DSystem>();
        auto& renderer     = self.GetSystem<RenderSystem>();

        auto& fontData = fontSystem.GetFontData(font);

        // Apply instance
        bool needsUpdate = renderable.frameNumber != frameData.frameNumber || renderable.drawIndex != frameData.drawIndex;

        shaderSystem.BindInstance(renderable.instanceId);

        shaderSystem.SetUniformByIndex(locations.properties, &WHITE);
        shaderSystem.SetUniformByIndex(locations.diffuseTexture, &fontData.atlas);
        shaderSystem.ApplyInstance(needsUpdate);

        // Sync frame number
        renderable.frameNumber = frameData.frameNumber;
        renderable.drawIndex   = frameData.drawIndex;

        renderer.SetStencilWriteMask(0x0);
        renderer.SetStencilTestingEnabled(false);

        // Apply local
        auto model = self.GetWorld();
        shaderSystem.SetUniformByIndex(locations.model, &model);

        renderer.DrawGeometry(renderable.renderData);
    }

    void TextComponent::Regenerate(Component& self)
    {
        const u64 utf8Size = text.SizeUtf8();

        const u64 vertexCount      = utf8Size * VERTICES_PER_QUAD;
        const u64 indexCount       = utf8Size * INDICES_PER_QUAD;
        const u64 vertexBufferSize = sizeof(Vertex2D) * vertexCount;
        const u64 indexBufferSize  = sizeof(u32) * indexCount;

        if (utf8Size > capacity)
        {
            // New text is larger then the current capacity we allocated for
            auto& renderSystem = self.GetSystem<RenderSystem>();

            if (capacity > 0)
            {
                // If we previously had some data we need to free it
                const u64 oldVertexBufferSize = sizeof(Vertex2D) * capacity * VERTICES_PER_QUAD;
                const u64 oldIndexBufferSize  = sizeof(u32) * capacity * INDICES_PER_QUAD;

                // So first we free the current allocation for vertices
                if (!renderSystem.FreeInRenderBuffer(RenderBufferType::Vertex, oldVertexBufferSize,
                                                     renderable.renderData.vertexBufferOffset))
                {
                    ERROR_LOG("Failed to free in Render's Vertex Buffer with size: {} and offset: {}.", oldVertexBufferSize,
                              renderable.renderData.vertexBufferOffset);
                }
                // And our indices
                if (!renderSystem.FreeInRenderBuffer(RenderBufferType::Index, oldIndexBufferSize, renderable.renderData.indexBufferOffset))
                {
                    ERROR_LOG("Failed to free in Render's Index Buffer with size: {} and offset: {}.", oldIndexBufferSize,
                              renderable.renderData.indexBufferOffset);
                }
            }

            // Then we allocate new bigger capacity for our vertices
            if (!renderSystem.AllocateInRenderBuffer(RenderBufferType::Vertex, vertexBufferSize, renderable.renderData.vertexBufferOffset))
            {
                ERROR_LOG("Failed to allocate in Render's Vertex Buffer width size: {}", vertexBufferSize);
            }
            // And our indices
            if (!renderSystem.AllocateInRenderBuffer(RenderBufferType::Index, indexBufferSize, renderable.renderData.indexBufferOffset))
            {
                ERROR_LOG("Failed to allocate in Render's Index Buffer width size: {}", indexBufferSize);
            }

            // Save off our new max capacity
            capacity = utf8Size;
        }

        f32 x = 0, y = 0;

        auto width  = 0;
        auto height = 0;

        // Clear our temp data
        vertices.Clear();
        indices.Clear();

        // Make sure we have enough storage for our temp data
        vertices.Reserve(vertexCount);
        indices.Reserve(indexCount);

        auto& fontSystem = self.GetSystem<FontSystem>();
        auto& data       = fontSystem.GetFontData(font);

        for (u32 c = 0, uc = 0; c < text.Size(); ++c)
        {
            i32 codepoint = text[c];

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
            codepoint  = text.ToCodepoint(c, advance);

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

                if (maxX > width) width = maxX;
                if (maxY > height) height = maxY;

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

                vertices.EmplaceBack(vec2(minX, minY), vec2(tMinX, tMinY));
                vertices.EmplaceBack(vec2(maxX, maxY), vec2(tMaxX, tMaxY));
                vertices.EmplaceBack(vec2(minX, maxY), vec2(tMinX, tMaxY));
                vertices.EmplaceBack(vec2(maxX, minY), vec2(tMaxX, tMinY));

                // Increment our x by the xAdvance plus any potential kerning
                x += static_cast<f32>(glyph->xAdvance) + GetFontKerningAmount(data, text, codepoint, c + advance, utf8Size);
            }
            else
            {
                ERROR_LOG("Failed find codepoint. Skipping this glyph.");
                continue;
            }

            indices.PushBack(uc * 4 + 2);
            indices.PushBack(uc * 4 + 1);
            indices.PushBack(uc * 4 + 0);
            indices.PushBack(uc * 4 + 3);
            indices.PushBack(uc * 4 + 0);
            indices.PushBack(uc * 4 + 1);

            // Increment our character index (subtracting 1 since our loop will increment every iteration by 1)
            c += advance - 1;
            uc++;
        }

        // Set our newly calculated size
        self.SetSize(u16vec2(width, height));

        if (utf8Size > 0)
        {
            auto& renderSystem = self.GetSystem<RenderSystem>();

            // Load up our vertex and index buffer data
            if (!renderSystem.LoadRangeInRenderBuffer(RenderBufferType::Vertex, renderable.renderData.vertexBufferOffset, vertexBufferSize,
                                                      vertices.GetData()))
            {
                ERROR_LOG("Failed to LoadRange() for vertex buffer.");
            }
            if (!renderSystem.LoadRangeInRenderBuffer(RenderBufferType::Index, renderable.renderData.indexBufferOffset, indexBufferSize,
                                                      indices.GetData()))
            {
                ERROR_LOG("Failed to LoadRange() for index buffer.");
            }
        }

        renderable.renderData.vertexSize      = sizeof(Vertex2D);
        renderable.renderData.indexSize       = sizeof(u32);
        renderable.renderData.vertexCount     = vertices.Size();
        renderable.renderData.indexCount      = indices.Size();
        renderable.renderData.windingInverted = false;
    }

    void TextComponent::Destroy(Component& self)
    {
        auto& renderSystem = self.GetSystem<RenderSystem>();

        text.Destroy();
        vertices.Destroy();
        indices.Destroy();

        if (renderable.instanceId != INVALID_ID)
        {
            auto& renderSystem = self.GetSystem<RenderSystem>();
            auto& uiSystem     = self.GetSystem<UI2DSystem>();

            renderSystem.ReleaseShaderInstanceResources(uiSystem.GetShader(), renderable.instanceId);
        }

        const auto vertexSize = capacity * VERTICES_PER_QUAD * sizeof(Vertex2D);
        if (!renderSystem.FreeInRenderBuffer(RenderBufferType::Vertex, vertexSize, renderable.renderData.vertexBufferOffset))
        {
            ERROR_LOG("Failed to Free in Renderer's Vertex buffer.");
        }

        const auto indexSize = capacity * INDICES_PER_QUAD * sizeof(u32);
        if (!renderSystem.FreeInRenderBuffer(RenderBufferType::Index, indexSize, renderable.renderData.indexBufferOffset))
        {
            ERROR_LOG("Failed to Free in Renderer's Index buffer.");
        }
    }
}  // namespace C3D::UI_2D