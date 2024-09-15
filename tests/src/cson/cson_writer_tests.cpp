#include "cson_writer_tests.h"

#include <cson/cson_reader.h>
#include <cson/cson_writer.h>
#include <platform/file_system.h>
#include <string/string.h>

#include "../expect.h"

namespace CSONWriter
{
    void CompareFiles(const char* expectedPath, const char* actualPath)
    {
        C3D::File expectedFile, actualFile;
        if (!expectedFile.Open(expectedPath, C3D::FileModeRead))
        {
            AssertFail("Failed to open expected file.");
        }
        if (!actualFile.Open(actualPath, C3D::FileModeRead))
        {
            AssertFail("Failed to open actual file.");
        }

        C3D::String expected, actual;
        if (!expectedFile.ReadAll(expected))
        {
            AssertFail("Failed to read expected file.");
        }
        if (!actualFile.ReadAll(actual))
        {
            AssertFail("Failed to read actual file.");
        }

        ExpectEqual(expected, actual);
    }

    TEST(CSONWriterShouldWriteBasicObjects)
    {
        // Read the basic object from a file
        C3D::CSONReader reader;
        auto object = reader.ReadFromFile("basic_object.cson");

        // Write it back to a new file so we can compare
        C3D::CSONWriter writer;
        ExpectTrue(writer.WriteToFile(object, "basic_object_written.cson"));

        // Compare the initial file to the written file
        CompareFiles("basic_object.cson", "basic_object_written.cson");
    }

    TEST(CSONWriterShouldWriteObjectsWithArrays)
    {
        // Read the basic object from a file
        C3D::CSONReader reader;
        auto object = reader.ReadFromFile("object_with_arrays.cson");

        // Write it back to a new file so we can compare
        C3D::CSONWriter writer;
        ExpectTrue(writer.WriteToFile(object, "object_with_arrays_written.cson"));

        // Compare the initial file to the written file
        CompareFiles("object_with_arrays.cson", "object_with_arrays_written.cson");
    }

    TEST(CSONWriterShouldWriteNestedObjects)
    {
        // Read the basic object from a file
        C3D::CSONReader reader;
        auto object = reader.ReadFromFile("nested_objects.cson");

        // Write it back to a new file so we can compare
        C3D::CSONWriter writer;
        ExpectTrue(writer.WriteToFile(object, "nested_objects_written.cson"));

        // Compare the initial file to the written file
        CompareFiles("nested_objects.cson", "nested_objects_written.cson");
    }

    TEST(CSONWriterShouldWriteArrayOfObjects)
    {
        // Read the basic object from a file
        C3D::CSONReader reader;
        auto object = reader.ReadFromFile("array_of_objects.cson");

        // Write it back to a new file so we can compare
        C3D::CSONWriter writer;
        ExpectTrue(writer.WriteToFile(object, "array_of_objects_written.cson"));

        // Compare the initial file to the written file
        CompareFiles("array_of_objects.cson", "array_of_objects_written.cson");
    }

    void RegisterTests(TestManager& manager)
    {
        manager.StartType("CSONWriter");
        REGISTER_TEST(CSONWriterShouldWriteBasicObjects, "Writing of basic CSON objects should work.");
        REGISTER_TEST(CSONWriterShouldWriteObjectsWithArrays, "Writing of CSON objects with arrays should work.");
        REGISTER_TEST(CSONWriterShouldWriteNestedObjects, "Writing of nested CSON objects should work.");
        REGISTER_TEST(CSONWriterShouldWriteArrayOfObjects, "Writing of array of CSON objects should work.");
    }
}  // namespace CSONWriter