
#pragma once
#include <renderer/rendergraph/rendergraph.h>

#include "passes/editor_pass.h"

using namespace C3D;

namespace C3D
{
    class LinearAllocator;
    class SimpleScene;
}  // namespace C3D

struct EditorRendergraphConfig
{
    const LinearAllocator* pFrameAllocator = nullptr;
};

class EditorRendergraph : public Rendergraph<EditorRendergraphConfig>
{
public:
    bool Create(const C3D::String& name, const EditorRendergraphConfig& config) override;

    bool OnPrepareRender(FrameData& frameData, const Viewport& currentViewport, Camera* currentCamera, const C3D::SimpleScene& scene);

    void SetGizmo(EditorGizmo* gizmo);

private:
    EditorPass m_editorPass;
    EditorGizmo* m_pGizmo = nullptr;
};