
#include "c3d_string.h"

#include <algorithm>
#include <locale>
#include <cctype>
#include <cstdarg>
#include <sstream>

#include "memory/global_memory_system.h"
#include "platform/platform.h"

#ifndef _MSC_VER
#include <strings.h>
#endif

namespace C3D
{
	u64 StringLength(const char* str)
	{
		return strlen(str);
	}

	bool Equals(const char* a, const char* b, const i32 length)
	{
		if (length == -1)
		{
			return strcmp(a, b) == 0;
		}
		return strncmp(a, b, length) == 0;
	}

	bool IEquals(const char* a, const char* b, const i32 length)
	{
		if (length == -1)
		{
#if defined(__GNUC__)
			return strcasecmp(a, b) == 0;
#elif (defined _MSC_VER)
			return _strcmpi(a, b) == 0;
#endif
		}
		else
		{
#if defined(__GNUC)
			return strncasecmp(a, b, length) == 0;
#elif defined _MSC_VER
			return _strnicmp(a, b, length) == 0;
#endif
		}
	}

	void StringNCopy(char* dest, const char* source, const i64 length)
	{
		strncpy(dest, source, length);
	}

	void LTrim(string& str)
	{
		str.erase(
			str.begin(), 
			std::find_if(
				str.begin(), str.end(), [](const unsigned char c) 
				{
					return !std::isspace(c);
				}
			)
		);
	}

	void RTrim(string& str)
	{
		str.erase(
			std::find_if(
				str.rbegin(), str.rend(), [](const unsigned char c)
				{
					return !std::isspace(c);
				}
			).base(), str.end()
		);
	}

	void Trim(string& str)
	{
		LTrim(str);
		RTrim(str);
	}

	void StringMid(char* dest, const char* source, const i32 start, const i32 length)
	{
		if (length == 0) return;

		if (const i64 srcLength = StringLength(source); start >= srcLength)
		{
			dest[0] = 0;
			return;
		}

		if (length > 0)
		{
			for (u64 i = start, j = 0; j < length && source[i]; i++, j++)
			{
				dest[j] = source[i];
			}
			dest[start + length] = 0;
		}
		else
		{
			// Negative value means we keep going to the end
			u64 j = 0;
			for (u64 i = start; source[i]; i++, j++)
			{
				dest[j] = source[i];
			}
			dest[start + j] = 0;
		}
	}

	std::vector<char*> StringSplit(const string& s, const char delimiter, const bool trimEntry, const bool excludeEmpty)
	{
		std::vector<char*> splits;
		std::string split;
		std::istringstream ss(s);
		while (std::getline(ss, split, delimiter))
		{
			if (trimEntry) Trim(split);
			if (!excludeEmpty || !split.empty()) splits.push_back(StringDuplicate(split.data()));
		}
		return splits;
	}

	char* StringEmpty(char* str)
	{
		if (str) str[0] = 0;
		return str;
	}

	i32 StringFormat(char* dest, const char* format, ...)
	{
		if (dest) {
			va_list argPtr;
			va_start(argPtr, format);
			const i32 written = StringFormatV(dest, format, argPtr);
			va_end(argPtr);
			return written;
		}
		return -1;
	}

	i32 StringFormatV(char* dest, const char* format, va_list vaList)
	{
		if (dest) {
			// Big, but can fit on the stack.
			char buffer[16000];
			const auto written = vsnprintf(buffer, sizeof buffer, format, vaList);
			Platform::MemCopy(dest, buffer, written + 1);

			return written;
		}
		return -1;
	}

	char* StringDuplicate(const char* str)
	{
		const u64 length = StringLength(str);
		char* copy = Memory.Allocate<char>(MemoryType::String, length + 1);
		Platform::MemCopy(copy, str, length);
		copy[length] = 0;
		return copy;
	}

	void StringAppend(char* dest, const char* src, const char* append)
	{
		sprintf(dest, "%s%s", src, append);
	}

	void StringAppend(char* dest, const char* src, const i64 append)
	{
		sprintf(dest, "%s%lli", src, append);
	}

	void StringAppend(char* dest, const char* src, const u64 append)
	{
		sprintf(dest, "%s%llu", src, append);
	}

	bool StringToVec4(const char* str, vec4* outVec)
	{
		if (!str) return false;

		Platform::Zero(outVec, sizeof(vec4));
		const auto result = sscanf_s(str, "%f %f %f %f", &outVec->x, &outVec->y, &outVec->z, &outVec->w);
		return result != -1;
	}

	bool StringToVec3(const char* str, vec3* outVec)
	{
		if (!str) return false;

		Platform::Zero(outVec, sizeof(vec3));
		const auto result = sscanf_s(str, "%f %f %f", &outVec->x, &outVec->y, &outVec->z);
		return result != -1;
	}

	bool StringToVec2(const char* str, vec2* outVec)
	{
		if (!str) return false;

		Platform::Zero(outVec, sizeof(vec2));
		const auto result = sscanf_s(str, "%f %f", &outVec->x, &outVec->y);
		return result != -1;
	}

	bool StringToF32(const char* str, f32* f)
	{
		if (!str) return false;

		*f = 0;
		const auto result = sscanf_s(str, "%f", f);
		return result != -1;
	}

	bool StringToF64(const char* str, f64* f)
	{
		if (!str) return false;

		*f = 0;
		const auto result = sscanf_s(str, "%lf", f);
		return result != -1;
	}

	bool StringToU8(const char* str, u8* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%hhu", u);
		return result != -1;
	}

	bool StringToU16(const char* str, u16* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%hi", u);
		return result != -1;
	}

	bool StringToU32(const char* str, u32* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%u", u);
		return result != -1;
	}

	bool StringToU64(const char* str, u64* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%llu", u);
		return result != -1;
	}

	bool StringToI8(const char* str, i8* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%hhi", i);
		return result != -1;
	}

	bool StringToI16(const char* str, i16* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%hi", i);
		return result != -1;
	}

	bool StringToI32(const char* str, i32* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%i", i);
		return result != -1;
	}

	bool StringToI64(const char* str, i64* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%lli", i);
		return result != -1;
	}

	bool StringToBool(const char* str, bool* b)
	{
		if (!str) return false;
		*b = Equals(str, "1") || IEquals(str, "true");
		return *b;
	}
}
