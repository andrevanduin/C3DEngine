
#pragma once
#include "defines.h"
#include "math/math_types.h"

namespace C3D
{
	/*
	 * @brief Gets the length of the string
	 *
	 * @param str The string you want to know the length off
	 *
	 * @return the length of the string
	 */
	u32 StringLength(const string& str);

	/*
	 * @brief Compares two strings case-sensitive
	 *
	 * @param left The string you want to compare
	 * @param right The string you want to compare to
	 *
	 * @return a bool indicating if the strings match case-sensitively
	 */
	bool Equals(const string& left, const string& right);

	/*
	 * @brief Compares two strings case-insensitive
	 *
	 * @param left The string you want to compare
	 * @param right The string you want to compare to
	 *
	 * @return a bool indicating if the string match case-insensitively
	 */
	bool IEquals(const string& left, const string& right);

	/*
	 * @brief Copies the string in source to dest up to a given length. Does no perform any allocation.
	 *
	 * @param dest The destination string
	 * @param source The source string
	 * @param length The maximum number of characters that will be copied
	 */
	void StringNCopy(char* dest, const char* source, i64 length);

	/*
	 * @brief Trims all the whitespace on the left of the provided string.
	 *
	 * @param str The string you want to have left trimmed
	 */
	void LTrim(string& str);

	/*
	 * @brief Trims all the whitespace on the right of the provided string.
	 *
	 * @param str The string you want to have right trimmed
	 */
	void RTrim(string& str);

	/*
	 * @brief Trims all the whitespace of the provided string.
	 *
	 * @param str The string you want to have trimmed
	 */
	void Trim(string& str);

	/*
	 * @brief Empties the provided string by setting the first char to 0.
	 *
	 * @param str The string to be emptied
	 *
	 * @ return A pointer to the string
	 */
	char* StringEmpty(char* str);

	C3D_API i32 StringFormat(char* dest, const char* format, ...);

	C3D_API i32 StringFormatV(char* dest, const char* format, void* vaList);

	C3D_API char* StringDuplicate(const char* str);

	/*
	 * @brief Attempts to parse a vec4 from a string
	 *
	 * @param str The string to parse from. It is expected to be space-delimited (i.e. "1.0 2.0 3.0 4.0")
	 * @param outVector A pointer to the vector that we should write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToVec4(const char* str, vec4* outVec);

	/*
	 * @brief Attempts to parse a vec3 from a string
	 *
	 * @param str The string to parse from. It is expected to be space-delimited (i.e. "1.0 2.0 3.0")
	 * @param outVector A pointer to the vector that we should write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToVec3(const char* str, vec3* outVec);

	/*
	 * @brief Attempts to parse a vec2 from a string
	 *
	 * @param str The string to parse from. It is expected to be space-delimited (i.e. "1.0 2.0")
	 * @param outVector A pointer to the vector that we should write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToVec2(const char* str, vec2* outVec);

	/*
	 * @brief Attempts to parse a f32 from a string
	 *
	 * @param str The string to parse from.
	 * @param f A pointer to the float to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToF32(const char* str, f32* f);

	/*
	 * @brief Attempts to parse a f64 from a string
	 *
	 * @param str The string to parse from.
	 * @param f A pointer to the float to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToF64(const char* str, f64* f);

	/*
	 * @brief Attempts to parse a u8 from a string
	 *
	 * @param str The string to parse from.
	 * @param u A pointer to the u8 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToU8(const char* str, u8* u);

	/*
	 * @brief Attempts to parse a u16 from a string
	 *
	 * @param str The string to parse from.
	 * @param u A pointer to the u16 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToU16(const char* str, u16* u);

	/*
	 * @brief Attempts to parse a u32 from a string
	 *
	 * @param str The string to parse from.
	 * @param u A pointer to the u32 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToU32(const char* str, u32* u);

	/*
	 * @brief Attempts to parse a u64 from a string
	 *
	 * @param str The string to parse from.
	 * @param u A pointer to the u64 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToU64(const char* str, u64* u);

	/*
	 * @brief Attempts to parse a i8 from a string
	 *
	 * @param str The string to parse from.
	 * @param i A pointer to the i8 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToI8(const char* str, i8* i);

	/*
	 * @brief Attempts to parse a i16 from a string
	 *
	 * @param str The string to parse from.
	 * @param i A pointer to the i16 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToI16(const char* str, i16* i);

	/*
	 * @brief Attempts to parse a i32 from a string
	 *
	 * @param str The string to parse from.
	 * @param i A pointer to the i32 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToI32(const char* str, i32* i);

	/*
	 * @brief Attempts to parse a i64 from a string
	 *
	 * @param str The string to parse from.
	 * @param i A pointer to the i64 to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToI64(const char* str, i64* i);

	/*
	 * @brief Attempts to parse a bool from a string
	 *
	 * @param str The string to parse from. "true" or 1 is considered true; anything else is false.
	 * @param b A pointer to the bool to write to.
	 * @return true if parsed successfully; otherwise false
	 */
	bool StringToBool(const char* str, bool* b);
}

