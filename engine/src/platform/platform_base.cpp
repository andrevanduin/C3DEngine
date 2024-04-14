
#include "platform_base.h"

#include "platform.h"

namespace C3D
{
    void ParseWindowFlags(WindowConfig& config)
    {
        // Check the appliation flags and set some properties based on those
        if (config.flags & WindowFlagFullScreen)
        {
            config.width  = Platform::GetPrimaryScreenWidth();
            config.height = Platform::GetPrimaryScreenWidth();
        }

        if (config.flags & WindowFlagCenter || config.flags & WindowFlagCenterHorizontal)
        {
            config.x = (Platform::GetPrimaryScreenWidth() / 2) - (config.width / 2);
        }

        if (config.flags & WindowFlagCenter || config.flags & WindowFlagCenterVertical)
        {
            config.y = (Platform::GetPrimaryScreenHeight() / 2) - (config.height / 2);
        }
    }
}  // namespace C3D