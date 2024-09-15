
#pragma once
#include <variant>

#include "containers/dynamic_array.h"
#include "defines.h"
#include "string/string.h"

namespace C3D
{
    enum class CSONTokenizeMode
    {
        Default,
        Whitespace,
        NumericLiteral,
        StringLiteral,
        Comment,
    };

    enum class CSONTokenType : u8
    {
        Unknown,
        Whitespace,
        Comma,
        Colon,
        OperatorPlus,
        OperatorMinus,
        OperatorEquals,
        OperatorSlash,
        OperatorAsterisk,
        OpenSquareBrace,
        CloseSquareBrace,
        OpenCurlyBrace,
        CloseCurlyBrace,
        StringLiteral,
        Integer,
        Float,
        Boolean,
        Comment,
        NewLine,
        EndOfFile,
    };

    struct CSONToken
    {
        CSONTokenType type = CSONTokenType::Unknown;
        u32 start          = 0;
        u32 end            = 0;
        u32 line           = 0;

        CSONToken() {}
        CSONToken(CSONTokenType type, u32 pos, u32 line) : type(type), start(pos), end(pos), line(line) {}
        CSONToken(CSONTokenType type, u32 start, u32 end, u32 line) : type(type), start(start), end(end), line(line) {}
    };

    enum class CSONParseMode
    {
        ObjectOrArray,
        KeyOrEndOfObject,
        Colon,
        Value,
        CommaOrEndOfObject,
        ArrayValueAfterOpen,
        ArrayValueAfterComma,
        ArraySeparatorOrEnd,
        EndOfFile,
    };

    enum class CSONObjectType
    {
        Object,
        Array
    };

    struct CSONProperty;

    /** @brief Represents an object in a CSON file. */
    struct C3D_API CSONObject
    {
        CSONObject* parent = nullptr;

        CSONObjectType type;
        DynamicArray<CSONProperty> properties;

        bool IsEmpty() const { return properties.Empty(); }

        CSONObject(CSONObjectType type) : type(type) {}
    };

    /** @brief A CSONArray is just an object where the properties don't have a name. */
    using CSONArray = CSONObject;

    /** @brief Describes a value of something in a CSON file. Can also be another CSON object. */
    using CSONValue = std::variant<i64, f64, bool, String, CSONObject>;

    enum CSONPropertyValueType
    {
        PropertyTypeI64 = 0,
        PropertyTypeF64,
        PropertyTypeBool,
        PropertyTypeString,
        PropertyTypeObject
    };

    /** @brief Describes a property in a CSON file. For objects the name field will be populated.
     * For properties of an array the name field will be empty. */
    struct C3D_API CSONProperty
    {
        String name;
        CSONValue value;

        u32 GetType() const;
        bool IsBasicType() const;

        bool GetBool() const;
        i64 GetI64() const;
        f64 GetF64() const;
        const String& GetString() const;

        const CSONObject& GetObject() const;
        const CSONArray& GetArray() const;

        CSONProperty(const CSONValue& value) : value(value) {}
        CSONProperty(const String& name) : name(name) {}
    };

}  // namespace C3D

template <>
struct fmt::formatter<C3D::CSONObjectType>
{
    template <typename ParseContext>
    static auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const C3D::CSONObjectType type, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", static_cast<u32>(type));
    }
};