
#pragma once
#include "core/exceptions.h"
#include "core/logger.h"
#include "platform/file_system.h"
#include "resource_loader.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    template <typename T>
    class C3D_API BaseTextLoader
    {
    public:
        bool LoadAndParseFile(const char* name, const char* typePath, const char* extension, T& resource) const
        {
            if (std::strlen(name) == 0)
            {
                Logger::Error("[BASE_TEXT_LOADER] - LoadAndParseFile() - Provided name was empty.");
                return false;
            }

            auto fullPath = String::FromFormat("{}/{}/{}.{}", Resources.GetBasePath(), typePath, name, extension);
            auto fileName = String::FromFormat("{}.{}", name, extension);

            resource.fullPath = fullPath;
            resource.name     = name;

            File file;
            if (!file.Open(resource.fullPath, FileModeRead))
            {
                Logger::Error("[BASE_TEXT_LOADER] - LoadAndParseFile() - Unable to open file for reading: '{}'.", resource.fullPath);
                return false;
            }

            String line;
            // Prepare for strings of up to 512 characters so we don't needlessly resize
            line.Reserve(512);

            u32 lineNumber = 1;

            try
            {
                while (file.ReadLine(line))
                {
                    line.Trim();

                    // Skip blank lines and comments
                    if (line.Empty() || line.First() == '#')
                    {
                        lineNumber++;
                        continue;
                    }

                    // First line should be version
                    if (resource.version == 0)
                    {
                        resource.version = ParseVersion(line);
                        // Set the resource's default values based on the parser version
                        // Implementation is defined by the loader that inherits from this base_loader
                        SetDefaults(resource);
                        lineNumber++;
                        continue;
                    }

                    if (line.StartsWith('['))
                    {
                        ParseTagInternal(line, resource);
                    }
                    else
                    {
                        // Split on the '=' symbol
                        const auto& splits = line.Split('=');
                        if (splits.Size() == 2)
                        {
                            ParseNameValuePair(splits[0], splits[1], resource);
                        }
                        else
                        {
                            throw Exception("Incorrect amount of '=' symbols found");
                        }
                    }

                    lineNumber++;
                }
            }
            catch (const std::exception& exc)
            {
                Logger::Error("[BASE_TEXT_LOADER] - LoadAndParseFile() - Failed to parse file: '{}'.\n {} on line: {}.", resource.fullPath,
                              exc.what(), lineNumber);
                return false;
            }

            file.Close();
            return true;
        }

    private:
        virtual void SetDefaults(T& resource) const                                                 = 0;
        virtual void ParseNameValuePair(const String& name, const String& value, T& resource) const = 0;
        virtual void ParseTag(const String& name, bool isOpeningTag, T& resource) const             = 0;

        u8 ParseVersion(const String& line) const
        {
            const auto& splits = line.Split('=');
            if (splits.Size() != 2)
            {
                throw Exception("Invalid version definition. The first line should be: version = <parser version>.");
            }

            const auto& name  = splits[0];
            const auto& value = splits[1];

            if (!name.IEquals("version"))
            {
                throw Exception("Invalid version definition. The first line should be: version = <parser version>.");
            }

            return value.ToU8();
        }

        void ParseTagInternal(const String& line, T& resource) const
        {
            if (!line.EndsWith(']'))
            {
                throw Exception("Invalid Tag specification. A tag should be specified as: [TAG_NAME].");
            }

            auto isClosing = line.Contains('/');
            String name    = line.SubStr(isClosing ? 2 : 1, line.Size() - 1);
            ParseTag(name, !isClosing, resource);
        }
    };
}  // namespace C3D
