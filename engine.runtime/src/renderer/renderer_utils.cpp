
#include "renderer_utils.h"

namespace C3D
{
    bool UniformTypeIsASampler(ShaderUniformType type)
    {
        switch (type)
        {
            case Uniform_Sampler1D:
            case Uniform_Sampler2D:
            case Uniform_Sampler3D:
            case Uniform_SamplerCube:
            case Uniform_Sampler1DArray:
            case Uniform_Sampler2DArray:
            case Uniform_SamplerCubeArray:
                return true;
            default:
                return false;
        }
    }
}  // namespace C3D