
#include "text_component.h"

#include "core/colors.h"
#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/fonts/font_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"

namespace C3D::UI_2D
{
    constexpr u64 VERTICES_PER_QUAD = 4;
    constexpr u64 INDICES_PER_QUAD  = 6;

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

    bool TextComponent::Initialize(Component& self, const Config& config)
    {
        font  = config.font;
        text  = config.text;
        color = config.color;

        if (!font)
        {
            ERROR_LOG("Failed to Acquire Font: '{}'.", font);
            return false;
        }

        auto textSize = text.SizeUtf8();
        // Ensure that text size is larger than 0 since we can't create an empty buffer
        if (textSize < 1) textSize = 1;

        // Acquire shader instance resources
        auto& shader = UI2D.GetShader();

        TextureMap* maps[1] = { &Fonts.GetFontData(font).atlas };

        ShaderInstanceUniformTextureConfig textureConfig;
        textureConfig.uniformLocation = shader.GetUniformIndex("diffuseTexture");
        textureConfig.textureMaps     = maps;

        ShaderInstanceResourceConfig instanceConfig;
        instanceConfig.uniformConfigs     = &textureConfig;
        instanceConfig.uniformConfigCount = 1;

        if (!Renderer.AcquireShaderInstanceResources(shader, instanceConfig, renderable.instanceId))
        {
            ERROR_LOG("Failed to Acquire Shader Instance resources.");
            return false;
        }

        // Verify that our atlas has all the required glyphs
        if (!Fonts.VerifyAtlas(font, text))
        {
            ERROR_LOG("Font atlas verification failed.");
            return false;
        }

        vertices = DynamicArray<Vertex2D>();
        indices  = DynamicArray<u32>();

        RecalculateGeometry(self);

        return true;
    }

    void TextComponent::OnPrepareRender(Component& self)
    {
        if (isDirty)
        {
            // We need enough vertex and index buffer space for our new pending capacity
            const u64 vertexBufferSize = sizeof(Vertex2D) * vertices.Size();
            const u64 indexBufferSize  = sizeof(u32) * indices.Size();

            const u32 neededCharacterCapacity = vertices.Size() / VERTICES_PER_QUAD;
            bool needsRealloc                 = neededCharacterCapacity > characterCapacity;

            if (needsRealloc)
            {
                // Our new data is larger than our previous data so we must reallocate in our buffers
                if (characterCapacity > 0)
                {
                    // We previously had some data which we need to free
                    const u64 oldVertexBufferSize = sizeof(Vertex2D) * characterCapacity * VERTICES_PER_QUAD;
                    const u64 oldIndexBufferSize  = sizeof(u32) * characterCapacity * INDICES_PER_QUAD;

                    // So first we free the current allocation for vertices
                    if (!Renderer.FreeInRenderBuffer(RenderBufferType::Vertex, oldVertexBufferSize,
                                                     renderable.renderData.vertexBufferOffset))
                    {
                        ERROR_LOG("Failed to free in Render's Vertex Buffer with size: {} and offset: {}.", oldVertexBufferSize,
                                  renderable.renderData.vertexBufferOffset);
                    }
                    // And our indices
                    if (!Renderer.FreeInRenderBuffer(RenderBufferType::Index, oldIndexBufferSize, renderable.renderData.indexBufferOffset))
                    {
                        ERROR_LOG("Failed to free in Render's Index Buffer with size: {} and offset: {}.", oldIndexBufferSize,
                                  renderable.renderData.indexBufferOffset);
                    }
                }

                // We allocate new bigger capacity for our vertices and override our newVertexBufferOffset
                if (!Renderer.AllocateInRenderBuffer(RenderBufferType::Vertex, vertexBufferSize, renderable.renderData.vertexBufferOffset))
                {
                    ERROR_LOG("Failed to allocate in Render's Vertex Buffer width size: {}", vertexBufferSize);
                }
                // And our indices (and override our newIndexBufferOffset)
                if (!Renderer.AllocateInRenderBuffer(RenderBufferType::Index, indexBufferSize, renderable.renderData.indexBufferOffset))
                {
                    ERROR_LOG("Failed to allocate in Render's Index Buffer width size: {}", indexBufferSize);
                }

                // And store our new capacity and vertex/index buffer offset
                characterCapacity = neededCharacterCapacity;
            }

            // Only load ranges if we actually have data
            if (vertexBufferSize > 0)
            {
                // Load up our vertex and index buffer data
                if (!Renderer.LoadRangeInRenderBuffer(RenderBufferType::Vertex, renderable.renderData.vertexBufferOffset, vertexBufferSize,
                                                      vertices.GetData(), true))
                {
                    ERROR_LOG("Failed to LoadRange() for vertex buffer.");
                }
                if (!Renderer.LoadRangeInRenderBuffer(RenderBufferType::Index, renderable.renderData.indexBufferOffset, indexBufferSize,
                                                      indices.GetData(), true))
                {
                    ERROR_LOG("Failed to LoadRange() for index buffer.");
                }
            }

            renderable.renderData.vertexCount = vertices.Size();
            renderable.renderData.indexCount  = indices.Size();

            isDirty = false;
        }
    }

    void TextComponent::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& fontData = Fonts.GetFontData(font);

        // Apply instance
        bool needsUpdate = renderable.frameNumber != frameData.frameNumber || renderable.drawIndex != frameData.drawIndex;

        Shaders.BindInstance(renderable.instanceId);
        Shaders.SetUniformByIndex(locations.properties, &color);
        Shaders.SetUniformByIndex(locations.diffuseTexture, &fontData.atlas);
        Shaders.ApplyInstance(frameData, needsUpdate);

        // Sync frame number
        renderable.frameNumber = frameData.frameNumber;
        renderable.drawIndex   = frameData.drawIndex;

        // Apply local
        auto model = self.GetWorld();
        model[3][0] += offsetX;
        model[3][1] += offsetY;

        Shaders.BindLocal();
        Shaders.SetUniformByIndex(locations.model, &model);
        Shaders.ApplyLocal(frameData);

        Renderer.DrawGeometry(renderable.renderData);
    }

    void TextComponent::RecalculateGeometry(Component& self)
    {
        const u64 utf8Size = text.SizeUtf8();

        const u64 vertexCount      = utf8Size * VERTICES_PER_QUAD;
        const u64 indexCount       = utf8Size * INDICES_PER_QUAD;
        const u64 vertexBufferSize = sizeof(Vertex2D) * vertexCount;
        const u64 indexBufferSize  = sizeof(u32) * indexCount;

        f32 x = 0, y = 0;

        maxX = 0;
        maxY = 0;

        // Clear our temp data
        vertices.Clear();
        indices.Clear();

        // Make sure we have enough storage for our temp data
        vertices.Reserve(vertexCount);
        indices.Reserve(indexCount);

        auto& data = Fonts.GetFontData(font);

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
                const f32 minX    = x + static_cast<f32>(glyph->xOffset);
                const f32 minY    = y + static_cast<f32>(glyph->yOffset);
                const f32 curMaxX = minX + static_cast<f32>(glyph->width);
                const f32 curMaxY = minY + static_cast<f32>(glyph->height);

                if (curMaxX > maxX) maxX = curMaxX;
                if (curMaxY > maxY) maxY = curMaxY;

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
                vertices.EmplaceBack(vec2(curMaxX, curMaxY), vec2(tMaxX, tMaxY));
                vertices.EmplaceBack(vec2(minX, curMaxY), vec2(tMinX, tMaxY));
                vertices.EmplaceBack(vec2(curMaxX, minY), vec2(tMaxX, tMinY));

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

        renderable.renderData.vertexSize      = sizeof(Vertex2D);
        renderable.renderData.indexSize       = sizeof(u32);
        renderable.renderData.windingInverted = false;
        isDirty                               = true;
    }

    void TextComponent::SetText(Component& self, const char* _text)
    {
        text = _text;
        RecalculateGeometry(self);
    }

    void TextComponent::Insert(Component& self, u32 index, char c)
    {
        text.Insert(index, c);
        RecalculateGeometry(self);
    }

    void TextComponent::Insert(Component& self, u32 index, const String& t)
    {
        text.Insert(index, t);
        RecalculateGeometry(self);
    }

    void TextComponent::RemoveAt(Component& self, u32 index)
    {
        text.RemoveAt(index);
        RecalculateGeometry(self);
    }

    void TextComponent::RemoveRange(Component& self, u32 characterIndexStart, u32 characterIndexEnd, bool regenerate)
    {
        text.RemoveRange(characterIndexStart, characterIndexEnd);
        if (regenerate)
        {
            RecalculateGeometry(self);
        }
    }

    void TextComponent::Destroy(Component& self)
    {
        text.Destroy();
        vertices.Destroy();
        indices.Destroy();

        if (renderable.instanceId != INVALID_ID)
        {
            Renderer.ReleaseShaderInstanceResources(UI2D.GetShader(), renderable.instanceId);
        }

        const auto vertexSize = characterCapacity * VERTICES_PER_QUAD * sizeof(Vertex2D);
        if (!Renderer.FreeInRenderBuffer(RenderBufferType::Vertex, vertexSize, renderable.renderData.vertexBufferOffset))
        {
            ERROR_LOG("Failed to Free in Renderer's Vertex buffer.");
        }

        const auto indexSize = characterCapacity * INDICES_PER_QUAD * sizeof(u32);
        if (!Renderer.FreeInRenderBuffer(RenderBufferType::Index, indexSize, renderable.renderData.indexBufferOffset))
        {
            ERROR_LOG("Failed to Free in Renderer's Index buffer.");
        }
    }
}  // namespace C3D::UI_2D