
#include "shader.h"

namespace C3D
{
    u16 Shader::GetUniformIndex(const char* uniformName)
    {
        if (id == INVALID_ID)
        {
            Logger::Error("[{}] - GetUniformIndex() - Called with invalid shader.", name);
            return INVALID_ID_U16;
        }

        if (!uniforms.Has(uniformName))
        {
            Logger::Error("[{}] - GetUniformIndex() - No uniform named: '{}' is registered in this shader.", name,
                          uniformName);
            return INVALID_ID_U16;
        }

        return static_cast<u16>(uniforms.GetIndex(uniformName));
    }
}  // namespace C3D