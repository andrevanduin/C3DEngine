
#include "RenderPass.h"

namespace C3D
{
	PassObject* MeshPass::Get(const Handle<PassObject> handle)
	{
		return &objects[handle.handle];
	}
}
