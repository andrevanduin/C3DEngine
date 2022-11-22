
#include "filesystem.h"

#include "core/logger.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "containers/string.h"
#include "core/c3d_string.h"

namespace C3D
{
	namespace fs = std::filesystem;

	File::File() : isValid(false), bytesWritten(0), bytesRead(0), m_size(0) {}

	File::~File()
	{
		Close();
	}

	bool File::Exists(const String& path)
	{
		const fs::path f{ path.Data() };
		return exists(f);
	}

	bool File::Open(const String& path, const u8 mode)
	{
		isValid = false;

		std::ios::openmode openMode = 0;

		if (mode & FileModeRead)	openMode |= std::ios::in;
		if (mode & FileModeWrite)	openMode |= std::ios::out;
		if (mode & FileModeBinary)	openMode |= std::ios::binary;

		m_file.open(path.Data(), openMode);

		isValid = m_file.is_open();
		currentPath = path;

		return isValid;
	}

	bool File::Close()
	{
		m_size = 0;
		bytesWritten = 0;
		bytesRead = 0;
		currentPath = "";

		if (m_file.is_open())
		{
			m_file.close();
			isValid = false;
			return true;
		}
		return false;
	}

	bool File::ReadLine(string& line)
	{
		if (!isValid) return false;
		if (!std::getline(m_file, line)) return false;
		return true;
	}

	bool File::ReadLine(String& line, const char delimiter)
	{
		// First we empty out our provided line
		line.Clear();
		// Get the very first character
		char c = static_cast<char>(m_file.get());
		// If it's the end of the file character we just return that we are done
		if (c == EOF) return false;
		// Otherwise we need to add it to our line
		line.Append(c);
		// Then we keep reading characters until we find our delimiter character or EOF
		while (m_file.get(c) && c != delimiter && c != EOF)
		{
			line.Append(c);
		}
		return true;
	}

	bool File::WriteLine(const String& line)
	{
		if (!isValid) return false;
		m_file << line << "\n";
		// Flush to ensure that we don't have data loss if the engine crashes before closing the file
		m_file.flush(); 
		return true;
	}

	bool File::Read(const u64 dataSize, void* outData, u64* outBytesRead)
	{
		if (!isValid) return false;

		*outBytesRead = m_file.readsome(static_cast<char*>(outData), static_cast<std::streamsize>(dataSize));
		if (*outBytesRead != dataSize) return false;

		return true;
	}

	bool File::ReadAll(char* outBytes, u64* outBytesRead)
	{
		if (!isValid) return false;
		// If we have no size yet we get it
		if (m_size == 0) Size();
		// Read the data
		m_file.read(outBytes, static_cast<std::streamsize>(m_size));
		// Get the amount of bytes read
		*outBytesRead = m_file.gcount();
		// If we did not read all bytes we return false
		/*
		if (*outBytesRead != m_size)
		{
			Logger::Error("[FILE] - ReadAll() - File size of {} did not match the read bytes", currentPath);
			return false;
		}*/
		// Otherwise reading was successful 
		return true;
	}

	bool File::Write(const u64 dataSize, const void* data, u64* outBytesWritten)
	{
		if (!isValid) return false;

		// Get the position of the cursor at the start
		const auto before = m_file.tellp();
		// Write all the bytes
		m_file.write(static_cast<const char*>(data), static_cast<std::streamsize>(dataSize));
		// Flush to ensure that the bytes are written even if the engine crashes before closing this file
		m_file.flush();
		// Check the position after writing
		const auto after = m_file.tellp();
		// The total bytes written will be equal to the current cursor position - the starting cursor position
		*outBytesWritten = after - before;
		// If we did not write all the expected bytes we return false
		if (*outBytesWritten != dataSize) return false;

		return true;
	}

	bool File::Size(u64* outSize)
	{
		if (!isValid) return false;

		// Get the file size
		const auto s = std::filesystem::file_size(currentPath.Data());
		// Store it internally
		m_size = s;
		// Save off the size to the outPtr if it is valid
		if (outSize != nullptr) *outSize = s;
		return true;
	}

	void FileSystem::DirectoryFromPath(char* dest, const char* path)
	{
		const i64 length = static_cast<i64>(strlen(path));
		for (i64 i = length; i >= 0; i--)
		{
			char c = path[i];
			if (c == '/' || c == '\\')
			{
				strncpy(dest, path, i + 1);
				dest[i + 1] = 0;
				return;
			}
		}
	}

	void FileSystem::FileNameFromPath(char* dest, const char* path, const bool includeExtension)
	{
		const i64 length = static_cast<i64>(strlen(path));
		if (includeExtension)
		{
			for (i64 i = length; i >= 0; i--)
			{
				const char c = path[i];
				if (c == '/' || c == '\\')
				{
					std::strcpy(dest, path + i + 1);
					return;
				}
			}
		}
		else
		{
			u64 start = 0;
			u64 end = 0;
			for (i64 i = length; i >= 0; i--)
			{
				char c = path[i];
				if (end == 0 && c == '.')
				{
					end = i;
				}
				if (start == 0 && (c == '/' || c == '\\'))
				{
					start = i + 1;
					break;
				}
			}

			StringMid(dest, path, static_cast<i32>(start), static_cast<i32>(end - start));
		}
	}
}
