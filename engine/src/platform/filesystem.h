
#pragma once
#include "core/defines.h"

#include <fstream>

namespace C3D
{
	enum FileModes : u8
	{
		FileModeRead	= 0x1,
		FileModeWrite	= 0x2,
		FileModeBinary  = 0x4,
	};

	class C3D_API File
	{
	public:
		File();
		~File();

		static [[nodiscard]] bool Exists(const string& path);

		bool Open(const string& path, u8 mode);

		bool Close();

		bool ReadLine(string& line);

		bool WriteLine(const string& line);

		bool Read(u64 dataSize, void* outData, u64* outBytesRead);

		template<typename T>
		bool Read(T* data, u64 count = 1);

		bool ReadAll(char* outBytes, u64* outBytesRead);

		// Deprecated
		bool Write(u64 dataSize, const void* data, u64* outBytesWritten);

		template<typename T>
		bool Write(const T* data, u64 count = 1);

		bool Size(u64* outSize);

		bool isValid;
		u64 bytesWritten;
		u64 bytesRead;

		std::string currentPath;
	private:
		std::fstream m_file;
	};

	template <typename T>
	bool File::Read(T* data, const u64 count)
	{
		if (!isValid) return false;

		m_file.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(sizeof(T) * count));
		bytesRead += m_file.gcount();
		return true;
	}

	template <typename T>
	bool File::Write(const T* data, const u64 count)
	{
		if (!isValid) return false;

		// Get the position of the cursor at the start
		const auto before = m_file.tellp();
		// Write all the bytes
		m_file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(sizeof(T) * count));
		// Flush to ensure that the bytes are written even if the engine crashes before closing this file
		m_file.flush();
		// Check the position after writing
		const auto after = m_file.tellp();
		// The total bytes written will be equal to the current cursor position - the starting cursor position
		bytesWritten += after - before;

		return true;
	}

	class C3D_API FileSystem
	{
	public:
		static void DirectoryFromPath(char* dest, const char* path);

		static void FileNameFromPath(char* dest, const char* path, bool includeExtension = false);
	};
}