
#include "cson_types.h"

namespace C3D
{
    const static String empty = "";
    const static CSONObject emptyObject(CSONObjectType::Object);
    const static CSONArray emptyArray(CSONObjectType::Array);
    const static vec4 emptyVec = {0,0,0,0};

    CSONProperty::CSONProperty(u32 num) : value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(i32 num) : value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(u64 num) : value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(i64 num) : value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(f32 num) : value(static_cast<f64>(num)) {}
    CSONProperty::CSONProperty(f64 num) : value(static_cast<f64>(num)) {}
    CSONProperty::CSONProperty(bool b) : value(b) {}
    CSONProperty::CSONProperty(const String& str) : value(str) {}
    CSONProperty::CSONProperty(const CSONObject& obj) : value(obj) {}

    CSONProperty::CSONProperty(const String& name, u32 num) : name(name), value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(const String& name, i32 num) : name(name), value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(const String& name, u64 num) : name(name), value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(const String& name, i64 num) : name(name), value(static_cast<i64>(num)) {}
    CSONProperty::CSONProperty(const String& name, f32 num) : name(name), value(static_cast<f64>(num)) {}
    CSONProperty::CSONProperty(const String& name, f64 num) : name(name), value(static_cast<f64>(num)) {}
    CSONProperty::CSONProperty(const String& name, bool b) : name(name), value(b) {}
    CSONProperty::CSONProperty(const String& name, const String& str) : name(name), value(str) {}
    CSONProperty::CSONProperty(const String& name, const vec4& v)
        : name(name)
    {
        auto array = CSONArray(CSONObjectType::Array);
        array.properties.EmplaceBack(v.x);
        array.properties.EmplaceBack(v.y);
        array.properties.EmplaceBack(v.z);
        array.properties.EmplaceBack(v.w);
        value = array;
    }

    CSONProperty::CSONProperty(const String& name, const CSONObject& obj) : name(name), value(obj) {}

    u32 CSONProperty::GetType() const { return value.index(); }

    bool CSONProperty::IsBasicType() const
    {
        return value.index() == PropertyTypeBool || value.index() == PropertyTypeF64 || value.index() == PropertyTypeI64;
    }

    bool CSONProperty::GetBool() const
    {
        if (std::holds_alternative<bool>(value))
        {
            return std::get<bool>(value);
        }
        ERROR_LOG("Property: '{}' does not hold a bool. Returning false.", name);
        return false;
    }

    i64 CSONProperty::GetI64() const
    {
        if (std::holds_alternative<i64>(value))
        {
            return std::get<i64>(value);
        }
        ERROR_LOG("Property: '{}' does not hold a i64. Returning 0.", name);
        return 0;
    }

    f64 CSONProperty::GetF64() const
    {
        if (std::holds_alternative<f64>(value))
        {
            return std::get<f64>(value);
        }
        ERROR_LOG("Property: '{}' does not hold a f64. Returning 0.0.", name);
        return 0.0;
    }

    f32 CSONProperty::GetF32() const
    {
        if (std::holds_alternative<f64>(value))
        {
          return static_cast<f32>(std::get<f64>(value));
        }
        ERROR_LOG("Property: '{}' does not hold a f64. Returning 0.0.", name);
        return 0.0;
    }

    const String& CSONProperty::GetString() const
    {
        if (std::holds_alternative<String>(value))
        {
            return std::get<String>(value);
        }
        ERROR_LOG("Property: '{}' does not hold a String. Returning empty string.", name);
        return empty;
    }

    const CSONObject& CSONProperty::GetObject() const
    {
        if (std::holds_alternative<CSONObject>(value))
        {
            return std::get<CSONObject>(value);
        }
        ERROR_LOG("Property: '{}' does not hold a CSONObject. Returning empty CSONObject.", name);
        return emptyObject;
    }

    const CSONArray& CSONProperty::GetArray() const
    {
        if (std::holds_alternative<CSONArray>(value))
        {
            return std::get<CSONArray>(value);
        }
        ERROR_LOG("Property: '{}' does not hold a CSONArray. Returning empty CSONArray.", name);
        return emptyArray;
    }

    vec4 CSONProperty::GetVec4() const
    {
        if (std::holds_alternative<CSONArray>(value))
        {
            const auto& array = std::get<CSONArray>(value);

            if (array.properties.Size() != 4)
            {
                ERROR_LOG("Property: '{}' does not hold a 4 element array.", name);
                return emptyVec;
            }

            const auto x = array.properties[0].GetF64();
            const auto y = array.properties[1].GetF64();
            const auto z = array.properties[2].GetF64();
            const auto w = array.properties[3].GetF64();
            return vec4(x, y, z, w);
        }

        ERROR_LOG("Property: '{}' does not hold an array.", name);
        return emptyVec;
    }

}  // namespace C3D
