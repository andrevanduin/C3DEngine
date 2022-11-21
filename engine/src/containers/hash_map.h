
#pragma once
#include "core/defines.h"
#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	template <typename T>
	struct HashCompute
	{
		size_t operator()(const T& key, const u64 hashSize) const
		{
			return std::hash<T>() (key) % hashSize;
		}
	};

	template <class Key, class Value, class HashFunc = HashCompute<Key>>
	class HashMap
	{
		class HashMapIterator
		{
			void FindNextOccupiedIndex()
			{
				// Iterate over our elements and find the next element that is also occupied
				// We start at the element after our current element (which is at m_index)
				for (auto i = m_index + 1; i < m_map->m_size; i++)
				{
					if (m_map->m_elements[i].occupied)
					{
						// We found a new element that is occupied so we store it's index as our new m_index
						m_index = i;
						return;
					}
				}

				// If we get to this part we could not find a next occupied element
				// so we set our index to the std::end() element
				m_index = m_map->m_size;
			}

		public:
			using DifferenceType = std::ptrdiff_t;
			using Pointer = Value*;
			using Reference = Value&;
			using iterator_category = std::forward_iterator_tag;

			HashMapIterator() : m_index(0), m_map(nullptr) {}

			explicit HashMapIterator(const HashMap<Key, Value, HashFunc>* map)
				: m_index(INVALID_ID_U64), m_map(map)
			{
				FindNextOccupiedIndex();
			}

			HashMapIterator(const HashMap<Key, Value, HashFunc>* map, const u64 currentIndex)
				: m_index(currentIndex), m_map(map)
			{
				FindNextOccupiedIndex();
			}

			// Dereference operator
			Reference operator*() const
			{
				return m_map->m_elements[m_index].element;
			}

			// Pre and post-increment operators
			HashMapIterator& operator++ ()
			{
				FindNextOccupiedIndex();
				return *this;
			}

			HashMapIterator operator++ (int)
			{
				return HashMapIterator(m_map, m_index);
			}

			bool operator== (const HashMapIterator& other)
			{
				return m_map == other.m_map && m_index == other.m_index;
			}

			bool operator!= (const HashMapIterator& other)
			{
				return m_map != other.m_map || m_index != other.m_index;
			}

		private:
			u64 m_index;
			const HashMap<Key, Value, HashFunc>* m_map;
		};

		struct HashMapNode
		{
			bool occupied;
			Value element;
		};

	public:
		HashMap();

		HashMap(const HashMap&) = delete;
		HashMap(HashMap&&) = delete;

		HashMap& operator=(const HashMap&) = delete;
		HashMap& operator=(HashMap&&) = delete;

		~HashMap();

		void Create(u64 size);
		void Destroy();

		void Set(const Key& key, const Value& value);

		Value& Get(const Key& key);

		bool Has(const Key& key);

		Value& operator[] (const Key& key);

		[[nodiscard]] HashMapIterator begin() const;
		[[nodiscard]] HashMapIterator end() const;

	private:
		HashMapNode* m_elements;

		u64 m_size;
		HashFunc m_hashCompute;
	};

	template <class Key, class Value, class HashFunc>
	HashMap<Key, Value, HashFunc>::HashMap()
		: m_elements(nullptr), m_size(0)
	{}

	template <class Key, class Value, class HashFunc>
	HashMap<Key, Value, HashFunc>::~HashMap()
	{
		Destroy();
	}

	template<class Key, class Value, class HashFunc>
	void HashMap<Key, Value, HashFunc>::Create(const u64 size)
	{
		// Allocate enough memory for all the elements and indices pointing to them
		m_elements = Memory.Allocate<HashMapNode>(size, MemoryType::HashMap);
		m_size = size;

		// Invalidate all entries
		for (u64 i = 0; i < m_size; i++)
		{
			m_elements[i].occupied = false;
		}
	}

	template <class Key, class Value, class HashFunc>
	void HashMap<Key, Value, HashFunc>::Destroy()
	{
		// Call the destructor for all elements
		for (u64 i = 0; i < m_size; i++)
		{
			m_elements[i].element.~Value();
		}

		// If this HashMap is created we free all it's dynamically allocated memory
		if (m_size > 0 && m_elements)
		{
			Memory.Free(m_elements, sizeof(HashMapNode) * m_size, MemoryType::HashMap);
			m_elements = nullptr;

			m_size = 0;
		}
	}

	template <class Key, class Value, class HashFunc>
	void HashMap<Key, Value, HashFunc>::Set(const Key& key, const Value& value)
	{
		const auto hash = m_hashCompute(key, m_size);
		m_elements[hash].occupied = true;
		m_elements[hash].element = value;
	}

	template <class Key, class Value, class HashFunc>
	Value& HashMap<Key, Value, HashFunc>::Get(const Key& key)
	{
		const auto hash = m_hashCompute(key, m_size);
		if (!m_elements[hash].occupied)
		{
			throw std::invalid_argument("Key could not be found");
		}
		return m_elements[hash].element;
	}

	template <class Key, class Value, class HashFunc>
	bool HashMap<Key, Value, HashFunc>::Has(const Key& key)
	{
		const auto hash = m_hashCompute(key, m_size);
		return m_elements[hash].occupied;
	}

	template <class Key, class Value, class HashFunc>
	Value& HashMap<Key, Value, HashFunc>::operator[](const Key& key)
	{
		const auto hash = m_hashCompute(key, m_size);
		if (!m_elements[hash].occupied)
		{
			throw std::invalid_argument("Key could not be found");
		}
		return m_elements[hash].element;
	}

	template <class Key, class Value, class HashFunc>
	typename HashMap<Key, Value, HashFunc>::HashMapIterator HashMap<Key, Value, HashFunc>::begin() const
	{
		return HashMapIterator(this);
	}

	template <class Key, class Value, class HashFunc>
	typename HashMap<Key, Value, HashFunc>::HashMapIterator HashMap<Key, Value, HashFunc>::end() const
	{
		return HashMapIterator(this, m_size);
	}
}

template <>
struct std::hash<const char*>
{
	size_t operator() (const char* key) const noexcept
	{
		size_t hash = 0;
		const size_t length = std::strlen(key);

		for (size_t i = 0; i < length; i++)
		{
			hash ^= static_cast<size_t>(key[i]);
			hash *= std::_FNV_prime;
		}
		return hash;
	}
};
