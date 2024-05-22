
#include "shader.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "SHADER";

    u16 Shader::GetUniformIndex(const char* uniformName) const
    {
        if (id == INVALID_ID)
        {
            INFO_LOG("Shader: '{}' is invalid.", name);
            return INVALID_ID_U16;
        }

        if (!uniforms.Has(uniformName))
        {
            INFO_LOG("No uniform named: '{}' is registered in this shader ('{}').", uniformName, name);
            return INVALID_ID_U16;
        }

        return static_cast<u16>(uniforms.GetIndex(uniformName));
    }

    u16 Shader::GetUniformIndex(const String& uniformName) const { return GetUniformIndex(uniformName.Data()); }
}  // namespace C3D