
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/renderer_types.h"
#include "renderer/rendergraph/rendergraph_pass.h"

namespace C3D
{
    class Shader;
    class UIMesh;
    class UIText;
    struct TextureMap;
    struct UI2DRenderData;

    namespace
    {
        struct ShaderLocations
        {
            u16 diffuseMap = INVALID_ID_U16;
            u16 properties = INVALID_ID_U16;
            u16 model      = INVALID_ID_U16;
        };

        struct ShaderUI2DLocations
        {
            u16 projection     = INVALID_ID_U16;
            u16 view           = INVALID_ID_U16;
            u16 diffuseTexture = INVALID_ID_U16;
            u16 properties     = INVALID_ID_U16;
            u16 model          = INVALID_ID_U16;
        };
    }  // namespace

    class C3D_API UI2DPass : public RendergraphPass
    {
    public:
        UI2DPass();
        UI2DPass(const SystemManager* pSystemsManager);

        bool Initialize(const LinearAllocator* frameAllocator) override;
        bool Prepare(Viewport* viewport, Camera* camera, TextureMap* textureAtlas, const UIMesh* meshes,
                     const DynamicArray<UIText*, LinearAllocator>* texts, const DynamicArray<UIRenderData, LinearAllocator>* uiRenderData);
        bool Execute(const FrameData& frameData) override;

    private:
        Shader* m_shader     = nullptr;
        Shader* m_ui2DShader = nullptr;

        const UIMesh* m_pMeshes;
        const DynamicArray<UIText*, LinearAllocator>* m_pTexts;
        const DynamicArray<UIRenderData, LinearAllocator>* m_pUIRenderData;

        TextureMap* m_pTextureAtlas;

        ShaderLocations m_locations;
        ShaderUI2DLocations m_ui2DLocations;
    };
}  // namespace C3D