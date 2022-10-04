
#pragma once
#include "core/defines.h"
#include "services/services.h"

#include <type_traits>
#include <algorithm>

#include "core/memory.h"

namespace C3D
{
	constexpr u64 MULTIPLIER = 97;

	template<class T>
	class __declspec(dllexport) HashTable
	{
	public:
		HashTable();
		HashTable(const HashTable& other) = delete;
		HashTable(HashTable&& other) = delete;

		HashTable& operator=(const HashTable& other) = delete;
		HashTable& operator=(HashTable&& other) = delete;

		~HashTable();

		bool Create(u32 elementCount);

		bool Fill(const T& value);

		void Destroy();

		bool Set(const char* name, T* value);

		T Get(const char* name);

		static u64 GetMemoryRequirement(u64 elementCount);
	private:
		u64 Hash(const char* name) const;

		u32 m_elementCount;
		T* m_elements;
	};

	template <class T>
	HashTable<T>::HashTable() : m_elementCount(0), m_elements(nullptr) {}

	template <class T>
	HashTable<T>::~HashTable()
	{
		Destroy();
	}

	template <class T>
	bool HashTable<T>::Create(const u32 elementCount)
	{
		if (elementCount == 0)
		{
			Logger::Error("[HASHTABLE] - Element count must be a positive non-zero value.", elementCount);
			return false;
		}
		
		if (elementCount < 128)
		{
			Logger::Warn("[HASHTABLE] - Element count of {} is low. This might cause collisions!", elementCount);
		}

		m_elementCount = elementCount;
		m_elements = Memory.Allocate<T>(elementCount, MemoryType::HashTable);
		return true;
	}

	template <class T>
	bool HashTable<T>::Fill(const T& value)
	{
		if (std::is_pointer<T>())
		{
			Logger::Error("[HASHTABLE] - Fill() - Should not be used with pointer types");
			return false;
		}
		std::fill_n(m_elements, m_elementCount, value);
		return true;
	}

	template <class T>
	void HashTable<T>::Destroy()
	{
		if (m_elements && m_elementCount != 0)
		{
			Memory.Free(m_elements, static_cast<u64>(m_elementCount) * sizeof(T), MemoryType::HashTable);
			m_elementCount = 0;
			m_elements = nullptr;
		}
	}

	template <class T>
	bool HashTable<T>::Set(const char* name, T* value)
	{
		if (!name || !value)
		{
			Logger::Error("[HASHTABLE] - Set() - requires valid name and value.");
			return false;
		}

		u64 index = Hash(name);
		if (std::is_pointer<T>())
		{
			// If we are storing pointers we dereference it
			m_elements[index] = *value;
		}
		else
		{
			// Else we are storing values so we copy
			Memory.Copy(&m_elements[index], value, sizeof(T));
		}
		return true;
	}

	template <class T>
	T HashTable<T>::Get(const char* name)
	{
		if (!name)
		{
			Logger::Error("[HASHTABLE] - Get() - requires valid name.");
			return m_elements[0];
		}
		return m_elements[Hash(name)];
	}

	template <class T>
	u64 HashTable<T>::GetMemoryRequirement(const u64 elementCount)
	{
		return sizeof(T) * static_cast<u64>(elementCount);
	}

	template <class T>
	u64 HashTable<T>::Hash(const char* name) const
	{
		u64 hash = 0;
		while (*name)
		{
			// Add every char in the name to the previous value of hash multiplied by a prime
			hash = hash * MULTIPLIER + *name;
			name++;
		}
		// Mod against element count to make sure hash is valid index in our array
		return hash % m_elementCount;
	}
}
