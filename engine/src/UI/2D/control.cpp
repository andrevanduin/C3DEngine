
#include "control.h"

#include "core/frame_data.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    bool Control::Create() { return true; }
    void Control::Destroy() { DEBUG_LOG("Destroying UI_2D::Control."); }

    bool Control::Load() { return true; }
    bool Control::Unload() { return true; }

    bool Control::Update(const FrameData& frameData) { return true; }
    bool Control::Render(const FrameData& frameData) { return true; }
}  // namespace C3D::UI_2D