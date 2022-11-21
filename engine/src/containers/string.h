
#pragma once
#include <iostream>
#include <ostream>
#include <fmt/core.h>

#include "core/defines.h"
#include "core/memory.h"
#include "containers/dynamic_array.h"

#include "services/services.h"

namespace C3D
{
	constexpr u8 SSO_CAPACITY = 16;
	constexpr u8 SSO_THRESHOLD = SSO_CAPACITY - 1;
	constexpr u8 MEMORY_TYPE = SSO_CAPACITY - 1;

	constexpr char SSO_USE_STACK = '\0';
	constexpr char SSO_USE_HEAP = '\1';

	constexpr const char* TRUE_VALUE = "true";
	constexpr const char* FALSE_VALUE = "false";

	// Times how much the string is increased every time a resize is required
	constexpr f64 STRING_RESIZE_FACTOR = 1.5;

	class String
	{
	public:
		using value_type = char;

	private:
		void Init(const u64 length)
		{
			m_size = length;

			if (length > SSO_THRESHOLD)
			{
				// The length of the string is larger than our threshold for SSO so we use dynamic allocation
				const auto capacity = length + 1; // Extra byte for our '\0' char
				m_sso.capacity = capacity;
				m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;

				m_data = Memory.Allocate<char>(capacity, MemoryType::C3DString);
				Logger::Trace("[STRING] - Init() with capacity = {}", capacity);
			}
			else
			{
				// The length of the string is smaller than our threshold for SSO so we use the stack
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
				m_data = m_sso.data;
			}
		}

		void Resize(const u64 requiredSize)
		{
			if (m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP)
			{
				// The string is already allocated on the heap
				if (requiredSize > m_sso.capacity)
				{
					// The new capacity is larger than the old so we need to allocate more space
					const auto newCapacity = static_cast<u64>(std::ceil(static_cast<f64>(requiredSize) * STRING_RESIZE_FACTOR));
					Logger::Trace("[STRING] - Resize() with capacity = {}", newCapacity);

					// Allocate enough space for the new data
					const auto newData = Memory.Allocate<char>(newCapacity, MemoryType::C3DString);
					// Copy over the old data (including the final '0' byte)
					std::memcpy(newData, m_data, m_size + 1);
					// Now we free our current pointer
					Memory.Free(m_data, m_sso.capacity, MemoryType::C3DString);
					// And set our data pointer to the newly allocated block
					m_data = newData;
					// Finally we store our new capacity
					m_sso.capacity = newCapacity;
				}
			}
			else
			{
				// The string is currently on the stack
				if (requiredSize > SSO_THRESHOLD)
				{
					// Our new capacity is too large for SSO
					const auto newCapacity = static_cast<u64>(std::ceil(static_cast<f64>(requiredSize) * STRING_RESIZE_FACTOR));
					Logger::Trace("[STRING] - Resize() with capacity = {}", newCapacity);

					// Allocate enough space for the new capacity
					m_data = Memory.Allocate<char>(newCapacity, MemoryType::C3DString);
					// Copy over the old data from the stack to the heap (including the final '0' byte)
					std::memcpy(m_data, m_sso.data, m_size + 1);
					// Now we set our MEMORY_TYPE to HEAP since we just grew past our stack limit
					m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
					// Finally we store our capacity since on the stack we weren't using it yet
					m_sso.capacity = newCapacity;
				}
			}
		}

		void Free()
		{
			// We only need to cleanup if we are using the heap
			if (m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP && m_data)
			{
				Memory.Free(m_data, m_sso.capacity, MemoryType::C3DString);
				m_data = nullptr;
			}
		}

		/* @brief
		 * Private constructor to build a string of given length with given capacity.
		 * It is up to the caller to initialize the string with actual data.
		 */
		String(const u64 size, const u64 capacity)
			: m_data(nullptr), m_size(size)
		{
			if (capacity > SSO_THRESHOLD)
			{
				// We need to allocate the memory on the heap.
				m_data = Memory.Allocate<char>(capacity, MemoryType::C3DString);
				// Keep track of the capacity
				m_sso.capacity = capacity;
				// Set the flag bit to Heap
				m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
			}
			else
			{
				// The capacity is small enough for SSO
				m_data = m_sso.data;
				// Set the flag bit to stack
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
			}
		}

	public:

		/* @brief Creates empty string with 1 null byte. (Will use SSO) */
		String()
		{
			// Point our data pointer to the stack memory
			m_data = m_sso.data;
			// Set the size to 0 since the string is empty
			m_size = 0;

			m_sso.data[0] = '\0';
			m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
		}

		/* @brief Creates empty string with 1 null byte. (Will use SSO) */
		explicit String(decltype(nullptr))
		{
			// Point our data pointer to the stack memory
			m_data = m_sso.data;
			// Set the size to 0 since we got passed a nullptr
			m_size = 0;

			m_sso.data[0] = '\0';
			m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
		}

		String(const char* value)
			: m_data(nullptr), m_size(0)
		{
			if (value == nullptr)
			{
				m_size = 0;
				m_sso.data[0] = '\0';
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
				return;
			}

			Init(std::strlen(value));
			std::memcpy(m_data, value, m_size);
		}

		String(const char* value, const u64 size)
			: m_data(nullptr), m_size(size)
		{
			Init(size);
			std::memcpy(m_data, value, m_size);
		}

		explicit String(const bool value)
			: m_data(nullptr), m_size(0)
		{
			if (value)
			{
				Init(4); // We need space for 4 chars "true"
				std::memcpy(m_data, TRUE_VALUE, m_size);
			}
			else
			{
				Init(5); // We need space for 5 char "false"
				std::memcpy(m_data, FALSE_VALUE, m_size);
			}
		}

		explicit String(const u32 value)
			: m_data(nullptr), m_size(0)
		{
			const auto buffer = std::to_string(value);
			Init(buffer.size());
			std::memcpy(m_data, buffer.data(), m_size);
		}

		explicit String(const i32 value)
			: m_data(nullptr), m_size(0)
		{
			const auto buffer = std::to_string(value);
			Init(buffer.size());
			std::memcpy(m_data, buffer.data(), m_size);
		}

		explicit String(const u64 value)
			: m_data(nullptr), m_size(0)
		{
			const auto buffer = std::to_string(value);
			Init(buffer.size());
			std::memcpy(m_data, buffer.data(), m_size);
		}

		explicit String(const i64 value)
			: m_data(nullptr), m_size(0)
		{
			const auto buffer = fmt::format("{}", value);
			Init(buffer.size());
			std::memcpy(m_data, buffer.data(), m_size);
		}

		explicit String(const f32 value, const char* format = "{}")
			: m_data(nullptr), m_size(0)
		{
			const auto buffer = fmt::format(format, value);
			Init(buffer.size());
			std::memcpy(m_data, buffer.data(), m_size);
		}

		explicit String(const f64 value, const char* format = "{}")
			: m_data(nullptr), m_size(0)
		{
			const auto buffer = fmt::format(format, value);
			Init(buffer.size());
			std::memcpy(m_data, buffer.data(), m_size);
		}

		String(const String& other)
			: m_data(nullptr), m_size(other.m_size)
		{
			// Check 'other' is stack or heap allocated
			if (other.m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP) // Heap allocated
			{
				// Reserve enough capacity for the space that the other string is taking (Extra byte for '\0' character).
				m_sso.capacity = m_size + 1;
				// Set our MEMORY_TYPE bit to use the heap
				m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
				// Allocate enough space on the heap for our entire capacity
				m_data = Memory.Allocate<char>(m_sso.capacity, MemoryType::C3DString);
				// Copy over the contents from 'other' (we use our capacity instead of our size since we also want the trailing '\0' byte from 'other')
				std::memcpy(m_data, other.m_data, m_sso.capacity);
			}
			else // Stack allocated
			{
				// We point our data pointer to our stack space
				m_data = m_sso.data;
				// Set our MEMORY_TYPE bit to use the stack
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
				// We can copy over the data from 'other' (we use size + 1 to also copy the '\0' byte)
				std::memcpy(&m_sso.data, &other.m_sso.data, m_size + 1);
			}
		}

		String(String&& other) noexcept
			: m_data(nullptr), m_size(other.m_size)
		{
			// Check 'other' is stack or heap allocated
			if (other.m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP) // Heap allocated
			{
				// We can simply copy the capacity over
				m_sso.capacity = other.m_sso.capacity;
				// Set our MEMORY_TYPE bit to use the heap
				m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
				// We take ownership of the data pointer
				m_data = other.m_data;
				// Set 'other' MEMORY_TYPE to USE_STACK so we don't double free
				other.m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
			}
			else // Stack allocated
			{
				// We point our data pointer to our stack space
				m_data = m_sso.data;
				// Set our MEMORY_TYPE bit to use the stack
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
				// We can copy over the data from 'other' (we use size + 1 to also copy the '\0' byte)
				std::memcpy(&m_sso.data, &other.m_sso.data, m_size + 1);
			}
		}

		String& operator=(const String& other)
		{
			if (this == &other) return *this;

			// Copy over the size
			m_size = other.m_size;

			// Check 'other' is stack or heap allocated
			if (other.m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP) // Heap allocated
			{
				// Reserve enough capacity for the space that the other string is taking (Extra byte for '\0' character).
				m_sso.capacity = m_size + 1;
				// Set our MEMORY_TYPE bit to use the heap
				m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
				// Allocate enough space on the heap for our entire capacity
				m_data = Memory.Allocate<char>(m_sso.capacity, MemoryType::C3DString);
				// Copy over the contents from 'other' (we use our capacity instead of our size since we also want the trailing '\0' byte from 'other')
				std::memcpy(m_data, other.m_data, m_sso.capacity);
			}
			else // Stack allocated
			{
				// We point our data pointer to our stack space
				m_data = m_sso.data;
				// Set our MEMORY_TYPE bit to use the stack
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
				// We can copy over the data from 'other' (we use size + 1 to also copy the '\0' byte)
				std::memcpy(&m_sso.data, &other.m_sso.data, m_size + 1);
			}

			return *this;
		}

		String& operator=(String&& other) noexcept
		{
			// We need to free this string if there is any dynamically allocated memory
			Free();

			// Copy over the size
			m_size = other.m_size;

			// Check 'other' is stack or heap allocated
			if (other.m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP) // Heap allocated
			{
				
				// We can simply copy the capacity over
				m_sso.capacity = other.m_sso.capacity;
				// Set our MEMORY_TYPE bit to use the heap
				m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
				// We take ownership of the data pointer
				m_data = other.m_data;
				// Set 'other' MEMORY_TYPE to USE_STACK so we don't double free
				other.m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
			}
			else // Stack allocated
			{
				// We point our data pointer to our stack space
				m_data = m_sso.data;
				// Set our MEMORY_TYPE bit to use the stack
				m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
				// We can copy over the data from 'other' (we use size + 1 to also copy the '\0' byte)
				std::memcpy(&m_sso.data, &other.m_sso.data, m_size + 1);
			}
			return *this;
		}

		~String()
		{
			Free();
		}

		/* @brief Reserve enough space in the string to hold the provided capacity. */
		void Reserve(const u64 capacity)
		{
			if (m_sso.data[MEMORY_TYPE] == SSO_USE_HEAP)
			{
				// The string is already allocated on the heap
				if (capacity > m_sso.capacity)
				{
					// The new capacity is larger than the old so we need to allocate more space

					// Allocate enough space for the new data
					const auto newData = Memory.Allocate<char>(capacity, MemoryType::C3DString);
					// Copy over the old data (including the final '0' byte)
					if (m_size > 0) std::memcpy(newData, m_data, m_size + 1);
					// Now we free our current pointer
					Memory.Free(m_data, m_sso.capacity, MemoryType::C3DString);
					// And set our data pointer to the newly allocated block
					m_data = newData;
				}
			}
			else
			{
				// The string is currently on the stack
				if (capacity > SSO_THRESHOLD)
				{
					// Our new capacity is too large for SSO

					// Allocate enough space for the new capacity
					m_data = Memory.Allocate<char>(capacity, MemoryType::C3DString);
					// Copy over the old data from the stack to the heap (including the final '0' byte)
					if (m_size > 0) std::memcpy(m_data, m_sso.data, m_size + 1);
					// Now we set our MEMORY_TYPE to HEAP since we just grew past our stack limit
					m_sso.data[MEMORY_TYPE] = SSO_USE_HEAP;
					// Finally we store our capacity since on the stack we weren't using it yet
					m_sso.capacity = capacity;
				}
			}
		}

		/* @brief Clear out the string so it's empty. Does not change the capacity. */
		void Clear()
		{
			m_size = 0;
			m_data[0] = '\0';
		}

		/* @brief Completely destroy the string and it's contents.
		 * This sets the size to 0 and frees it's internal memory (if any is allocated).
		 */
		void Destroy()
		{
			Free();

			// Point our data pointer to the stack memory
			m_data = m_sso.data;
			// Set the size to 0 since the string is empty
			m_size = 0;

			m_sso.data[0] = '\0';
			m_sso.data[MEMORY_TYPE] = SSO_USE_STACK;
		}

		/* @brief Builds a string from the format and the provided arguments.
		 * Internally uses fmt::format to build out the string.
		 */
		template <typename... Args>
		static String FromFormat(const char* format, Args&&... args)
		{
			String buffer;
			fmt::format_to(std::back_inserter(buffer), format, std::forward<Args>(args)...);
			return buffer;
		}

		/* @brief Builds this string from the format and the provided arguments.
		 * Internally uses fmt::format to build out the string.
		 * The formatted output will appended to the back of the string.
		 */
		template <typename... Args>
		void Format(const char* format, Args&&... args)
		{
			fmt::format_to(std::back_inserter(*this), format, std::forward<Args>(args)...);
		}

		/* @brief Append the provided string to the end of this string. */
		void Append(const String& other)
		{
			// Calculate the totalSize that we will require for our appended string
			const u64 requiredSize = m_size + other.m_size;
			// Resize our internal buffer if it is needed (including '\0' byte)
			Resize(requiredSize + 1);
			// Copy data from the other string into our newly allocated buffer
			std::memcpy(m_data + m_size, other.Data(), other.m_size);
			// Add '\0' character at the end
			m_data[requiredSize] = '\0';
			// Store our new size
			m_size = requiredSize;
		}

		/* @brief Append the provided const char* to the end of this string. */
		void Append(const char* other)
		{
			// Get the length of the other string
			const u64 otherLength = std::strlen(other);
			// Calculate the totalSize that we will require for our appended string
			const u64 requiredSize = m_size + otherLength;
			// Resize our internal buffer if it is needed (including '\0' byte)
			Resize(requiredSize + 1);
			// Copy data from the other string into our newly allocated buffer
			std::memcpy(m_data + m_size, other, otherLength);
			// Add '\0' character at the end
			m_data[requiredSize] = '\0';
			// Store our new size
			m_size = requiredSize;
		}

		/* @brief Append the provided const char to the end of this string. */
		void Append(const char c)
		{
			// Calculate the totalSize that we will require for our appended string
			const u64 requiredSize = m_size + 1;
			// Resize our internal buffer if it is needed (including '\0' byte)
			Resize(requiredSize + 1);
			// Copy data from the other string into our newly allocated buffer
			m_data[m_size] = c;
			// Add '\0' character at the end
			m_data[requiredSize] = '\0';
			// Store our new size
			m_size = requiredSize;
		}

		/* @brief Added to support using default std::back_inserter(). */
		void push_back(const char c)
		{
			Append(c);
		}

		/* @brief Removes the last count characters from the string.
		 * If count > String::Size(), String::Size() characters will be removed (leaving you with an empty string).
		 */
		void RemoveLast(const u64 count = 1)
		{
			u64 newSize = 0;
			if (count < m_size)
			{
				newSize = m_size - count;
			}

			m_data[newSize] = '\0';
			m_size = newSize;
		}

		/* @brief Splits the string at the given delimiter. */
		[[nodiscard]] DynamicArray<String> Split(const char delimiter, const bool trimEntries = true, const bool skipEmpty = true) const
		{
			DynamicArray<String> elements;
			String current;
			for (u64 i = 0; i < m_size; i++)
			{
				if (m_data[i] == delimiter)
				{
					if (!skipEmpty || !current.Empty())
					{
						if (trimEntries) current.Trim();

						elements.PushBack(current);
						current.Clear();
					}
				}
				else
				{
					current.Append(m_data[i]);
				}
			}

			if (!current.Empty())
			{
				if (trimEntries) current.Trim();
				elements.PushBack(current);
			}
			return elements;
		}

		/* @brief Removes all starting whitespace characters from the string. */
		void TrimLeft()
		{
			u64 newStart = 0;
			// Find the first non-space character
			while (std::isspace(m_data[newStart]))
			{
				newStart++;
			}
			// If the first character is a non-space character we do nothing
			if (newStart == 0) return;
			// Decrement the size by however many characters we have removed
			m_size -= newStart;
			// Copy over the remaining characters
			std::memcpy(m_data, m_data + newStart, m_size);
			// Add a null termination character
			m_data[m_size] = '\0';
		}

		/* @brief Removes all the trailing whitespace characters from the string. */
		void TrimRight()
		{
			u64 newSize = m_size;
			// Find the first no-space character at the end
			while (std::isspace(m_data[newSize - 1]))
			{
				newSize--;
			}
			// If the last character is a non-space character we do nothing
			if (newSize == m_size) return;
			// We save off our new size
			m_size = newSize;
			// Set the null termination character to end our string
			m_data[m_size] = '\0';
		}

		/* @brief Remove all the starting and trailing whitespace characters from the string. */
		void Trim()
		{
			TrimLeft();
			TrimRight();
		}

		/* @brief Checks if string starts with provided character sequence. */
		[[nodiscard]] bool StartsWith(const String& sequence) const
		{
			// If our string is shorter than the sequence our string can not start with the sequence.
			if (m_size < sequence.Size()) return false;
			
			return std::equal(begin(), begin() + sequence.Size(), sequence.begin(), sequence.end());
		}

		/* @brief Checks if string starts with provided character. */
		[[nodiscard]] bool StartsWith(const char c) const
		{
			// If we have an empty string it can not start with a char.
			if (m_size == 0) return false;

			return m_data[0] == c;
		}

		/* @brief Checks if string ends in the provided character sequence. */
		[[nodiscard]] bool EndsWith(const String& sequence) const
		{
			// If our string is shorter than the sequence our string can not start with the sequence.
			if (m_size < sequence.Size()) return false;

			return std::equal(end() - sequence.Size(), end(), sequence.begin(), sequence.end());
		}

		/* @brief Checks if string ends with the provided character. */
		[[nodiscard]] bool EndsWith(const char c) const
		{
			// If we have an empty string it can not end with a char.
			if (m_size == 0) return false;

			return m_data[m_size - 1] == c;
		}

		[[nodiscard]] bool Contains(const char c) const
		{
			return std::find(begin(), end(), c) != end();
		}

		/* @brief Check if const char pointer matches case-sensitive. */
		[[nodiscard]] bool Equals(const char* other) const
		{
			return std::strcmp(m_data, other) == 0;
		}

		/* @brief Check if const char pointer matches case-insensitive. */
		[[nodiscard]] bool IEquals(const char* other) const
		{
			return _stricmp(m_data, other) == 0;
		}

		/* @brief Check if another string matches case-sensitive. */
		[[nodiscard]] bool Equals(const String& other) const
		{
			return std::equal(begin(), end(), other.begin(), other.end());
		}

		/* @brief Check if another string matches case-insensitive. */
		[[nodiscard]] bool IEquals(const String& other) const
		{
			return std::equal(begin(), end(), other.begin(), other.end(), [](const char a, const char b) { return tolower(a) == tolower(b); });
		}

		/* @brief Returns the utf8 codepoint at the given index.
		 * Will set advance to the amount of characters that need to be skipped to get the next character
		 */
		[[nodiscard]] i32 ToCodepoint(const u64 index, u64& advance) const
		{
			const int codepoint = static_cast<u8>(m_data[index]);
			if (codepoint >= 0 && codepoint < 0x7F)
			{
				// Singe-byte character
				advance = 1;
				return codepoint;
			}

			if ((codepoint & 0xE0) == 0xC0)
			{
				// Double-byte character
				advance = 2;
				return ((codepoint & 0b00011111) << 6) + 
					m_data[index + 1] & 0b00111111;
			}

			if ((codepoint & 0xF0) == 0xE0)
			{
				// Triple-byte character
				advance = 3;
				return ((codepoint & 0b00001111) << 12) +
					((m_data[index + 1] & 0b00111111) << 6) +
					(m_data[index + 2] & 0b00111111);
			}

			if ((codepoint & 0xF8) == 0xF0)
			{
				// Four-byte character
				advance = 4;
				return ((codepoint & 0b00000111) << 18) +
					((m_data[index + 1] & 0b00111111) << 12) +
					((m_data[index + 2] & 0b00111111) << 6) +
					(m_data[index + 3] & 0b00111111);
			}

			Logger::Error("[STRING] - ToCodepoint() - Invalid 5 or 6-byte character in string.");
			advance = 1;
			return -1;
		}

		/* @brief Converts string to an i32 in the provided base. */
		[[nodiscard]] i32 ToI32(const i32 base = 10) const
		{
			return std::strtol(m_data, nullptr, base);
		}

		/* @brief Converts string to an u32 in the provided base. */
		[[nodiscard]] u32 ToU32(const i32 base = 10) const
		{
			return std::strtoul(m_data, nullptr, base);
		}

		/* @brief Converts string to an i16 in the provided base. */
		[[nodiscard]] i16 ToI16(const i32 base = 10) const
		{
			return static_cast<i16>(std::strtol(m_data, nullptr, base));
		}

		/* @brief Converts string to an u16 in the provided base. */
		[[nodiscard]] u16 ToU16(const i32 base = 10) const
		{
			return static_cast<u16>(std::strtoul(m_data, nullptr, base));
		}

		/* @brief Converts string to an i8 in the provided base. */
		[[nodiscard]] i8 ToI8(const i32 base = 10) const
		{
			return static_cast<i8>(std::strtol(m_data, nullptr, base));
		}

		/* @brief Converts string to an u8 in the provided base. */
		[[nodiscard]] u8 ToU8(const i32 base = 10) const
		{
			return static_cast<u8>(std::strtoul(m_data, nullptr, base));
		}

		[[nodiscard]] bool ToBool() const
		{
			if (IEquals("1") || IEquals("true")) return true;
			return false;
		}

		/* @brief Gets the number of characters currently in the string (excluding the null-terminator). */
		[[nodiscard]] u64 Size() const { return m_size; }

		/* @brief Get the size of the string while keeping UTF8 multi-byte characters into account.
		 * Warning does not support 5 or 6-byte characters!
		 */
		[[nodiscard]] u64 SizeUtf8() const
		{
			u64 size = 0;
			for (u64 i = 0; i < m_size; i++)
			{
				if (m_data[i] >= 0 && m_data[i] < 127) size++;	// 1-byte character
				else if ((m_data[i] & 0xE0) == 0xC0) size += 2;	// 2-byte character
				else if ((m_data[i] & 0xF0) == 0xE0) size += 3; // 3-byte character
				else if ((m_data[i] & 0xF8) == 0xF0) size += 4; // 4-byte character
				else
				{
					Logger::Error("[STRING] - SizeUtf8() - Invalid 5 or 6-byte character in string.");
					return 0;
				}
			}
			return size;
		}

		/* @brief Gets the number of characters currently in the string (excluding the null-terminator). */
		[[nodiscard]] u64 Length() const { return m_size; }

		/* @brief Checks if the string is currently empty. */
		[[nodiscard]] bool Empty() const { return m_size == 0;  }

		/* @brief Returns a pointer to the internal character array. */
		[[nodiscard]] const char* Data() const { return m_data; }

		/* @brief Returns an iterator pointing to the start of the character array. */
		[[nodiscard]] char* begin() const { return m_data; }

		/* @brief Returns an iterator pointing to the element right after the last character in the character array. */
		[[nodiscard]] char* end() const	{ return m_data + m_size; }

		/* @brief Returns the first char in the string by reference. */
		[[nodiscard]] char& First() const { return m_data[0]; }

		/* @brief Returns the last char in the string by reference. */
		[[nodiscard]] char& Last() const { return m_data[m_size - 1]; }

		explicit operator const char* () const { return m_data; }

		/* @brief Returns the char at the provided index by reference. */
		char& operator[](const u64 index) const { return m_data[index]; }

		/* @brief Returns the char at the provided index by reference.
		 * Performs bounds checking internally. 
		 */
		[[nodiscard]] char& At(const u64 index) const
		{
			assert(index < m_size);
			return m_data[index];
		}

		/* @brief Checks if the string is empty. Will return true if the string is empty and false otherwise. */
		bool operator! () const
		{
			return m_size == 0;
		}

		/* @brief Checks if the string is empty. Will return false if the string is empty and true otherwise. */
		explicit operator bool () const
		{
			return m_size != 0;
		}

		/* @brief Operator overload for comparing with a const char pointer. */
		bool operator== (const char* other) const
		{
			return std::strcmp(m_data, other) == 0;
		}

		/* @brief Operator overload for comparing with another string. */
		bool operator== (const String& other) const
		{
			return std::equal(begin(), end(), other.begin(), other.end());
		}

		/* @brief Operator overload for inequality with a const char pointer. */
		bool operator!= (const char* other) const
		{
			return std::strcmp(m_data, other) != 0;
		}

		/* @brief Operator overload for inequality with another string. */
		bool operator!= (const String& other) const
		{
			return !std::equal(begin(), end(), other.begin(), other.end());
		}

		/* @brief Operator overload for appending another string to this string. */
		String& operator+= (const String& other)
		{
			Append(other);
			return *this;
		}

		/* @brief Operator overload for appending const char* to this string. */
		String& operator+= (const char* other)
		{
			Append(other);
			return *this;
		}

		/* @brief Concatenate two strings. */
		String friend operator+ (const String& left, const String& right);
		/* @brief Concatenate two strings with rvalue optimization for the left side. */
		String friend operator+ (String&& left, const String& right);

		/* @brief Concatenate a string with a const char*. */
		String friend operator+ (const String& left, const char* right);
		/* @brief Concatenate a string with a const char*, with rvalue optimization for the left side. */
		String friend operator+ (String&& left, const char* right);
		/* @brief Concatenate a const char* with a string */
		String friend operator+ (const char* left, const String& right);

	private:
		char* m_data;
		u64 m_size;

		/* @brief Our internal SSO data to store strings on the stack if they are small enough.
		 * For large strings we store our current capacity in here and a '\1' in data[SSO_SIZE]
		 * For small strings we store our string in data and a '\0' in data[SSO_SIZE]
		 */
		union
		{
			u64 capacity;
			char data[SSO_CAPACITY];
		} m_sso{};
	};

	inline std::ostream& operator<< (std::ostream& cout, const String& str)
	{
		cout << str.Data();
		return cout;
	}

	inline String operator+ (const String& left, const String& right)
	{
		// Our new size will be the size of both strings combined
		const u64 size = left.m_size + right.m_size;
		// Allocate memory for this size in our new String (capacity contains 1 extra byte for '\0' char)
		String s(size, size + 1);

		// Copy the contents from the left to the start
		std::memcpy(s.m_data, left.m_data, left.m_size);
		// Copy the contents from the right to right after the left part
		std::memcpy(s.m_data + left.m_size, right.m_data, right.m_size);

		return s;
	}

	inline String operator+(String&& left, const String& right)
	{
		// Our new size will be the size of both strings combined
		const u64 newSize = left.m_size + right.m_size;
		// Instead of creating a new string we can resize left since it is an rvalue anyway
		// Add an extra byte for the '\0' byte
		left.Resize(newSize + 1);
		// Since we kept the left side we only need to copy the right side over
		std::memcpy(left.m_data + left.m_size, right.m_data, right.m_size);
		// Now we update length of the left side
		left.m_size = newSize;
		// We need to add a '\0' byte at the end
		left[left.m_size] = '\0';
		// We can std::move left since it will be destroyed anyway
		return std::move(left);
	}

	inline String operator+(const String& left, const char* right)
	{
		// Get the size of the right
		const u64 rightSize = std::strlen(right);
		// Our new size will be the size of both strings combined
		const u64 size = left.m_size + rightSize;
		// Allocate memory for this size in our new String (capacity contains 1 extra byte for '\0' char)
		String s(size, size + 1);

		// Copy the contents from the left to the start
		std::memcpy(s.m_data, left.m_data, left.m_size);
		// Copy the contents from the right to right after 
		std::memcpy(s.m_data + left.m_size, right, rightSize);

		return s;
	}

	inline String operator+(String&& left, const char* right)
	{
		// Get the size of right
		const u64 rightSize = std::strlen(right);
		// Our new size will be the size of both strings combined
		const u64 newSize = left.m_size + rightSize;
		// Instead of creating a new string we can resize left since it is an rvalue anyway
		// Add an extra byte for the '\0' byte
		left.Resize(newSize + 1);
		// Since we kept the left side we only need to copy the right side over
		std::memcpy(left.m_data + left.m_size, right, rightSize);
		// Now we update length of the left side
		left.m_size = newSize;
		// We need to add a '\0' byte at the end
		left[left.m_size] = '\0';
		// We can std::move left since it will be destroyed anyway
		return std::move(left);
	}

	inline String operator+(const char* left, const String& right)
	{
		// Get the size of left
		const u64 leftSize = std::strlen(left);
		// Our new size will be the size of both strings combined
		const u64 size = leftSize + right.m_size;
		// Allocate memory for this size in our new String (capacity contains 1 extra byte for '\0' char)
		String s(size, size + 1);

		// Copy the contents from the left to the start
		std::memcpy(s.m_data, left, leftSize);
		// Copy the contents from the right to right after 
		std::memcpy(s.m_data + leftSize, right.Data(), right.m_size);
		// We need to add a '\0' byte at the end
		s[s.m_size] = '\0';
		return s;
	}
}

template<>
struct fmt::formatter<C3D::String>
{
	template<typename ParseContext>
	static constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template<typename FormatContext>
	auto format(C3D::String const& str, FormatContext& ctx)
	{
		return fmt::format_to(ctx.out(), "{}", str.Data());
	}
};

template <>
struct std::hash<C3D::String>
{
	size_t operator() (const C3D::String& key) const noexcept
	{
		size_t hash = 0;
		for (const auto c : key)
		{
			hash ^= static_cast<size_t>(c);
			hash *= std::_FNV_prime;
		}
		return hash;
	}
};
