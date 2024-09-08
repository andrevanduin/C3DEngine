
#include "cson_parser_tests.h"

#include <parsers/cson_parser.h>
#include <platform/file_system.h>
#include <string/string.h>

#include "../expect.h"

namespace CSONParser
{
    C3D::String GetFileContent(const char* name)
    {
        C3D::File file;
        C3D::String input;
        if (!file.Open(name, C3D::FileModeRead))
        {
            AssertFail("Failed to open file.");
        }
        if (!file.ReadAll(input))
        {
            AssertFail("Failed to ReadAll() from file.");
        }
        return input;
    }

    TEST(CSONParserShouldParseBasicObjects)
    {
        const char* fileName = "basic_object.cson";
        auto input           = GetFileContent(fileName);

        C3D::CSONParser parser;
        auto actual = parser.Parse(input);

        // We expect 4 preoperties in the root object
        ExpectEqual(4, actual.properties.Size());
        // We expect the first property name to be "string_key"
        ExpectEqual(C3D::String("string_key"), actual.properties[0].name);
        // We expect the first property value to be "value"
        ExpectEqual(C3D::String("value"), actual.properties[0].GetString());
        // We expect the second property name to be "int_key"
        ExpectEqual(C3D::String("int_key"), actual.properties[1].name);
        // We expect the second property value to be 5
        ExpectEqual(5, actual.properties[1].GetI64());
        // We expect the third property name to be "float_key"
        ExpectEqual(C3D::String("float_key"), actual.properties[2].name);
        // We expect the third property value to be 12.002
        ExpectFloatEqual(12.002, actual.properties[2].GetF64());
        // We expect the third property name to be "bool_key"
        ExpectEqual(C3D::String("bool_key"), actual.properties[3].name);
        // We expect the third property value to be 12.002
        ExpectTrue(actual.properties[3].GetBool());
    }

    void VerifyIntArray(const C3D::DynamicArray<i64>& values, const C3D::CSONArray& array)
    {
        ExpectEqual(values.Size(), array.properties.Size());

        for (u32 i = 0; i < values.Size(); ++i)
        {
            ExpectEqual(values[i], array.properties[i].GetI64());
        }
    }

    void VerifyF64Array(const C3D::DynamicArray<f64>& values, const C3D::CSONArray& array)
    {
        ExpectEqual(values.Size(), array.properties.Size());

        for (u32 i = 0; i < values.Size(); ++i)
        {
            ExpectEqual(values[i], array.properties[i].GetF64());
        }
    }

    TEST(CSONParserShouldParseObjectsWithArrays)
    {
        const char* fileName = "object_with_arrays.cson";
        auto input           = GetFileContent(fileName);

        C3D::CSONParser parser;
        auto actual = parser.Parse(input);

        // We expect 4 preoperties in the root object
        ExpectEqual(4, actual.properties.Size());
        // We expect the first property name to be "string_key"
        ExpectEqual(C3D::String("string_key"), actual.properties[0].name);
        // We expect the first property value to be "value"
        ExpectEqual(C3D::String("a string to test stuff"), actual.properties[0].GetString());
        // We expect the second property name to be "int_array_key"
        ExpectEqual(C3D::String("int_array_key"), actual.properties[1].name);
        // We expect the second property value be an array of ints like [ 1, 2, 3, 4, 5 ]
        VerifyIntArray({ 1, 2, 3, 4, 5 }, actual.properties[1].GetArray());
        // We expect the second property name to be "float_array_key"
        ExpectEqual(C3D::String("float_array_key"), actual.properties[2].name);
        // We expect the third property value to be an array of floats like [ 1.05, 1.9, 12.0481 ]
        VerifyF64Array({ 1.05, 1.9, 12.0481 }, actual.properties[2].GetArray());
        // We expect the fourth property name to be "empty_array_key"
        ExpectEqual(C3D::String("empty_array_key"), actual.properties[3].name);
        // We expect the fourth property value to be an empty array
        ExpectTrue(actual.properties[3].GetArray().IsEmpty());
    }

    TEST(CSONParserShouldParseNestedObjects)
    {
        const char* fileName = "nested_objects.cson";
        auto input           = GetFileContent(fileName);

        C3D::CSONParser parser;
        auto actual = parser.Parse(input);

        // We expect 2 properties in the root object
        ExpectEqual(2, actual.properties.Size());
        // We expect the first property to be named "nested_object"
        ExpectEqual(C3D::String("nested_object"), actual.properties[0].name);
        // We expect the first property to be an object
        auto nestedObject = actual.properties[0].GetObject();
        // Expect the nested object to be correct
        ExpectEqual(C3D::String("key"), nestedObject.properties[0].name);
        ExpectEqual(C3D::String("key2"), nestedObject.properties[1].name);
        ExpectEqual(C3D::String("value"), nestedObject.properties[0].GetString());
        ExpectEqual(5, nestedObject.properties[1].GetI64());
        // We expect the second property to also be an object
        auto nestedObject2 = actual.properties[1].GetObject();
        // Expect the nested object2 to also be correct
        ExpectEqual(C3D::String("array_key"), nestedObject2.properties[0].name);
        VerifyIntArray({ 1, 2, 3 }, nestedObject2.properties[0].GetArray());
    }

    TEST(CSONParserArrayOfObjects)
    {
        const char* fileName = "array_of_objects.cson";
        auto input           = GetFileContent(fileName);

        C3D::CSONParser parser;
        auto actual = parser.Parse(input);

        // We expect 1 property in the root object
        ExpectEqual(1, actual.properties.Size());
        // The first property should be a key of "array_of_objects" and value should be a CSONArray
        auto arrayObject = actual.properties[0];
        // Verify that the key is "array_of_objects"
        ExpectEqual(C3D::String("array_of_objects"), arrayObject.name);
        // Get the value which should be an array
        auto array = arrayObject.GetArray();
        ExpectEqual(C3D::CSONObjectType::Array, array.type);
        // There should be 2 objects in the array
        ExpectEqual(2, array.properties.Size());
        // Get the objects out of the array
        auto obj  = array.properties[0].GetObject();
        auto obj2 = array.properties[1].GetObject();
        // The key an value should be key: value and key2: value2
        ExpectEqual(1, obj.properties.Size());
        ExpectEqual(1, obj2.properties.Size());
        ExpectEqual(C3D::String("key"), obj.properties[0].name);
        ExpectEqual(C3D::String("value"), obj.properties[0].GetString());
        ExpectEqual(C3D::String("key2"), obj2.properties[0].name);
        ExpectEqual(C3D::String("value2"), obj2.properties[0].GetString());
    }

    TEST(CSONParserRootArray)
    {
        const char* fileName = "root_array.cson";
        auto input           = GetFileContent(fileName);

        C3D::CSONParser parser;
        auto actual = parser.Parse(input);

        ExpectEqual(C3D::CSONObjectType::Array, actual.type);
        ExpectEqual(4, actual.properties.Size());

        ExpectEqual(5, actual.properties[0].GetI64());
        ExpectEqual(C3D::String("string"), actual.properties[1].GetString());
        ExpectEqual(1.506, actual.properties[2].GetF64());
        ExpectEqual(true, actual.properties[3].GetBool());
    }

    void RegisterTests(TestManager& manager)
    {
        manager.StartType("CSONParser");
        REGISTER_TEST(CSONParserShouldParseBasicObjects, "Parsing of basic CSON objects should work.");
        REGISTER_TEST(CSONParserShouldParseObjectsWithArrays, "Parsing of CSON objects with arrays should work.");
        REGISTER_TEST(CSONParserShouldParseNestedObjects, "Parsing of nested CSON objects should work.");
        REGISTER_TEST(CSONParserArrayOfObjects, "Parsing of array of CSON objects should work.");
        REGISTER_TEST(CSONParserRootArray, "Parsing should also work when the root is a CSON array.");
    }
}  // namespace CSONParser