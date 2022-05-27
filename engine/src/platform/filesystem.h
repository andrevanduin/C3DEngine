
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
		static [[nodiscard]] bool Exists(const string& path);

		bool Open(const string& path, u8 mode);

		bool Close();

		bool ReadLine(string& line);

		bool WriteLine(const string& line);

		bool Read(u64 dataSize, void* outData, u64* outBytesRead);

		bool ReadAllBytes(char** outBytes, u64* outBytesRead);

		bool Write(u64 dataSize, const void* data, u64* outBytesWritten);

		bool isValid = false;
	private:
		std::fstream m_file;
	};
}