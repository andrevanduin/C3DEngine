
#include "cson_types.h"

namespace C3D
{
    const static String empty = "";
    const static CSONObject emptyObject(CSONObjectType::Object);
    const static CSONArray emptyArray(CSONObjectType::Array);

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
        ERROR_LOG("Property: '{}' does not hold a i64. Returning 0.0.", name);
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

}  // namespace C3D