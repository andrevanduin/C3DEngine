
#include "cvar.h"

namespace C3D
{
    bool CVar::AddOnChangeCallback(CVarOnChangedCallback&& callback)
    {
        for (auto& cb : m_onChangeCallbacks)
        {
            if (!cb)
            {
                cb = std::move(callback);
                return true;
            }
        }

        ERROR_LOG("Failed to add callback since there are already 4 callbacks present for this CVar.");
        return false;
    }

    CString<256> CVar::AsString() const
    {
        CString<256> str;
        auto valueStr = std::visit([](auto&& arg) { return std::to_string(arg); }, m_value);
        str.FromFormat("{} {} = {}", ToString(GetType()), m_name, valueStr);
        return str;
    }

}  // namespace C3D