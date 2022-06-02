
#include "c3d_string.h"

#include <algorithm>
#include <locale>
#include <cctype>
#include <cstdarg>

#include "memory.h"
#include "services/services.h"

#ifndef _MSC_VER
#include <strings.h>
#endif

namespace C3D
{
	u32 StringLength(const string& str)
	{
		return static_cast<u32>(str.size());
	}

	bool Equals(const string& left, const string& right)
	{
		return left == right;
	}

	bool IEquals(const string& left, const string& right)
	{
        return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) { return tolower(a) == tolower(b); });
	}

	void StringNCopy(char* dest, const char* source, i64 length)
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

	i32 StringFormatV(char* dest, const char* format, void* vaList)
	{
		if (dest) {
			// Big, but can fit on the stack.
			char buffer[16000];
			const i32 written = vsnprintf(buffer, 16000, format, static_cast<va_list>(vaList));
			buffer[written] = 0;
			Memory.Copy(dest, buffer, written + 1);

			return written;
		}
		return -1;
	}

	char* StringDuplicate(const char* str)
	{
		const u64 length = StringLength(str);
		char* copy = Memory.Allocate<char>(length + 1, MemoryType::String);
		Memory.Copy(copy, str, length);
		copy[length] = 0;
		return copy;
	}

	bool StringToVec4(const char* str, vec4* outVec)
	{
		if (!str) return false;

		Memory.Zero(outVec, sizeof(vec4));
		const auto result = sscanf_s(str, "%f %f %f %f", &outVec->x, &outVec->y, &outVec->z, &outVec->w);
		return result != -1;
	}

	bool StringToVec3(const char* str, vec3* outVec)
	{
		if (!str) return false;

		Memory.Zero(outVec, sizeof(vec3));
		const auto result = sscanf_s(str, "%f %f %f", &outVec->x, &outVec->y, &outVec->z);
		return result != -1;
	}

	bool StringToVec2(const char* str, vec2* outVec)
	{
		if (!str) return false;

		Memory.Zero(outVec, sizeof(vec2));
		const auto result = sscanf_s(str, "%f %f", &outVec->x, &outVec->y);
		return result != -1;
	}

	bool StringToF32(const char* str, f32* f)
	{
		if (!str) return false;

		*f = 0;
		const auto result = sscanf_s(str, "%f", f);
		return result != 1;
	}

	bool StringToF64(const char* str, f64* f)
	{
		if (!str) return false;

		*f = 0;
		const auto result = sscanf_s(str, "%lf", f);
		return result != 1;
	}

	bool StringToU8(const char* str, u8* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%hhu", u);
		return result != 1;
	}

	bool StringToU16(const char* str, u16* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%hi", u);
		return result != 1;
	}

	bool StringToU32(const char* str, u32* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%u", u);
		return result != 1;
	}

	bool StringToU64(const char* str, u64* u)
	{
		if (!str) return false;

		*u = 0;
		const auto result = sscanf_s(str, "%llu", u);
		return result != 1;
	}

	bool StringToI8(const char* str, i8* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%hhi", i);
		return result != 1;
	}

	bool StringToI16(const char* str, i16* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%hi", i);
		return result != 1;
	}

	bool StringToI32(const char* str, i32* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%i", i);
		return result != 1;
	}

	bool StringToI64(const char* str, i64* i)
	{
		if (!str) return false;

		*i = 0;
		const auto result = sscanf_s(str, "%lli", i);
		return result != 1;
	}

	bool StringToBool(const char* str, bool* b)
	{
		if (!str) return false;
		*b = Equals(str, "1") || IEquals(str, "true");
		return *b;
	}
}
