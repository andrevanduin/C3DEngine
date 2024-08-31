#pragma once
#include <bitset>

#include "defines.h"

namespace C3D
{
    constexpr u64 MAX_COMPONENTS_TYPES = 64;
    using ComponentID                  = u8;
    using EntityIndex                  = u32;
    using EntityVersion                = u32;
    using ComponentMask                = std::bitset<MAX_COMPONENTS_TYPES>;

}  // namespace C3D