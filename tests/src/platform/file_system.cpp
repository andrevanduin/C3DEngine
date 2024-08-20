
#include "file_system.h"

#include <containers/string.h>
#include <platform/file_system.h>

#include "../expect.h"

u8 FileSystemDirectoryFromPath()
{
    C3D::String path = "/assets/directory/test/text_file.txt";

    C3D::String expectedDirectory = "/assets/directory/test/";
    C3D::String actualDirectory   = C3D::FileSystem::DirectoryFromPath(path);

    ExpectEqual(expectedDirectory, actualDirectory);

    return true;
}

u8 FileNameFromPathWithExtension()
{
    C3D::String path = "/assets/directory/test/text_file.txt";

    C3D::String expectedFileName = "text_file.txt";
    C3D::String actualFileName   = C3D::FileSystem::FileNameFromPath(path, true);

    ExpectEqual(expectedFileName, actualFileName);

    return true;
}

u8 FileNameFromPathWithoutExtension()
{
    C3D::String path = "/assets/directory/test/text_file.txt";

    C3D::String expectedFileName = "text_file";
    C3D::String actualFileName   = C3D::FileSystem::FileNameFromPath(path);

    ExpectEqual(expectedFileName, actualFileName);

    return true;
}

void FileSystem::RegisterTests(TestManager& manager)
{
    manager.StartType("FileSystem");
    REGISTER_TEST(FileSystemDirectoryFromPath, "DirectoryFromPath should correctly return the full directory.");
    REGISTER_TEST(FileNameFromPathWithExtension, "FileNameFromPath should correctly return the file name.");
    REGISTER_TEST(FileNameFromPathWithoutExtension, "FileNameFromPath should remove extension when option is provided.");
}
