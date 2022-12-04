
#pragma once
#include "core/defines.h"
#include "containers/string.h"

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
		File(const File& other) = delete;
		File(File&& other) = delete;

		File& operator=(const File& other) = delete;
		File& operator=(File&& other) = delete;

		~File();

		static [[nodiscard]] bool Exists(const String& path);

		bool Open(const String& path, u8 mode);

		bool Close();

		bool ReadLine(string& line);
		bool ReadLine(String& line, char delimiter = '\n');

		bool WriteLine(const String& line);

		bool Read(u64 dataSize, void* outData, u64* outBytesRead);

		template<typename T>
		bool Read(T* data, u64 count = 1);

		template<typename T>
		bool Read(DynamicArray<T>& data);

		bool ReadAll(char* outBytes, u64* outBytesRead);
		bool ReadAll(String& outChars);

		// Deprecated
		bool Write(u64 dataSize, const void* data, u64* outBytesWritten);

		template<typename T>
		bool Write(const T* data, u64 count = 1);

		template<typename T>
		bool Write(const DynamicArray<T>& data);

		bool Size(u64* outSize = nullptr);

		[[nodiscard]] u64 GetSize() const { return m_size; }

		bool isValid;
		u64 bytesWritten;
		u64 bytesRead;

		String currentPath;
	private:
		u64 m_size;
		std::fstream m_file;
	};

	template <typename T>
	bool File::Read(T* data, const u64 count)
	{
		static_assert(!std::is_pointer_v<T>, "T must not be a pointer.");
		static_assert(!std::is_array_v<T>, "T must not be an array. Pass a pointer to the first element instead.");

		if (!isValid) return false;

		m_file.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(sizeof(T) * count));
		bytesRead += m_file.gcount();
		return true;
	}

	template <typename T>
	bool File::Read(DynamicArray<T>& data)
	{
		static_assert(std::is_pod_v<T>, "This method can only be used with simple types.");

		u64 size;
		if (!Read(&size)) return false;

		// No need to keep reading if this is 0
		if (size == 0) return true;

		data.Resize(size);
		if (!Read(data.GetData(), size)) return false;
			
		return true;
	}

	template <typename T>
	bool File::Write(const T* data, const u64 count)
	{
		static_assert(!std::is_pointer_v<T>, "T must not be a pointer.");
		static_assert(!std::is_array_v<T>, "T must not be an array. Pass a pointer to the first element instead.");

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

	template <typename T>
	bool File::Write(const DynamicArray<T>& data)
	{
		// Write out the size
		const u64 size = data.Size();
		if (!Write(&size)) return false;
		// Write out the elements in the array (if there are any)
		if (size != 0)
		{
			if (!Write(data.GetData(), size)) return false;
		}
		return true;
	}

	class C3D_API FileSystem
	{
	public:
		static void DirectoryFromPath(char* dest, const char* path);

		static void FileNameFromPath(char* dest, const char* path, bool includeExtension = false);
	};
}