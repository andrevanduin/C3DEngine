
#pragma once
#include <functional>
#include <variant>

#include "containers/array.h"
#include "containers/cstring.h"
#include "core/function/function.h"

namespace C3D
{
    enum class CVarType : u8
    {
        U8,
        I8,
        U16,
        I16,
        U32,
        I32,
        U64,
        I64,
        F32,
        F64,
        Bool,
    };

    static constexpr const char* ToString(CVarType type)
    {
        switch (type)
        {
            case CVarType::I8:
                return "i8";
            case CVarType::I16:
                return "i16";
            case CVarType::I32:
                return "i32";
            case CVarType::I64:
                return "i64";
            case CVarType::U8:
                return "u8";
            case CVarType::U16:
                return "u16";
            case CVarType::U32:
                return "u32";
            case CVarType::U64:
                return "u64";
            case CVarType::F32:
                return "f32";
            case CVarType::F64:
                return "f64";
            case CVarType::Bool:
                return "bool";
            default:
                return "UNKOWN";
        }
    }

    class CVar;

    using CVarName              = CString<128>;
    using CVarOnChangedCallback = std::function<void(const CVar&)>;
    using CVarValue             = std::variant<u8, i8, u16, i16, u32, i32, u64, i64, f32, f64, bool>;

    class CVar
    {
    public:
        template <typename T>
        CVar(const CVarName& name, T value) : m_name(name), m_value(value)
        {}

        bool AddOnChangeCallback(CVarOnChangedCallback&& callback);

        template <typename T>
        void SetValue(T value)
        {
            if (!std::holds_alternative<T>(m_value))
            {
                INSTANCE_FATAL_LOG("CVAR", "Tried setting with value of invalid type.");
            }

            m_value = value;
            for (auto& cb : m_onChangeCallbacks)
            {
                if (cb) cb(*this);
            }
        }

        template <typename T>
        T GetValue() const
        {
            if (!std::holds_alternative<T>(m_value))
            {
                INSTANCE_FATAL_LOG("CVAR", "Tried getting value of invalid type.");
            }
            return std::get<T>(m_value);
        }

        const CVarName& GetName() const { return m_name; }
        const CVarType GetType() const { return static_cast<CVarType>(m_value.index()); }

        CString<256> AsString() const;

    private:
        CVarName m_name;
        CVarValue m_value;
        Array<CVarOnChangedCallback, 4> m_onChangeCallbacks;
    };
}  // namespace C3D
