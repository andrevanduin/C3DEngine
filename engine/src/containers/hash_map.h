
#pragma once
#include "core/defines.h"
#include "memory/global_memory_system.h"
#include "platform/platform.h"
#include "systems/system_manager.h"

namespace C3D
{
    template <typename T>
    struct HashCompute
    {
        size_t operator()(const T& key, const u64 hashSize) const { return std::hash<T>()(key) % hashSize; }
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
                    if (m_map->m_nodes[i].occupied)
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
            using DifferenceType    = std::ptrdiff_t;
            using Pointer           = Value*;
            using Reference         = Value&;
            using iterator_category = std::forward_iterator_tag;

            HashMapIterator() : m_index(0), m_map(nullptr) {}

            explicit HashMapIterator(const HashMap* map) : m_index(INVALID_ID_U64), m_map(map) { FindNextOccupiedIndex(); }

            HashMapIterator(const HashMap* map, const u64 currentIndex) : m_index(currentIndex), m_map(map) { FindNextOccupiedIndex(); }

            // Dereference operator
            Reference operator*() const { return m_map->m_nodes[m_index].element; }

            // Pre and post-increment operators
            HashMapIterator& operator++()
            {
                FindNextOccupiedIndex();
                return *this;
            }

            HashMapIterator operator++(int) { return HashMapIterator(m_map, m_index); }

            bool operator==(const HashMapIterator& other) const { return m_map == other.m_map && m_index == other.m_index; }

            bool operator!=(const HashMapIterator& other) const { return m_map != other.m_map || m_index != other.m_index; }

        private:
            u64 m_index;
            const HashMap* m_map;
        };

        struct HashMapNode
        {
            bool occupied;
            Value element;
        };

    public:
        HashMap() : m_nodes(nullptr), m_size(0), m_count(0) {}

        HashMap(const HashMap&) = delete;
        HashMap(HashMap&&)      = delete;

        HashMap& operator=(const HashMap& other)
        {
            if (this == &other) return *this;

            // Allocate enough space for the same amount of nodes as other has
            m_nodes = Memory.Allocate<HashMapNode>(MemoryType::HashMap, other.m_size);
            // Copy over the values from other
            m_size        = other.m_size;
            m_count       = other.m_count;
            m_hashCompute = other.m_hashCompute;

            // Invalidate all nodes in our array except if at this location other has an occupied element
            for (u64 i = 0; i < other.m_size; i++)
            {
                const auto otherNode = other.m_nodes[i];
                if (otherNode.occupied)
                {
                    // The other node is occupied so we need to copy over this node
                    m_nodes[i].occupied = true;
                    m_nodes[i].element  = otherNode.element;
                }
                else
                {
                    // The other node is empty so we don't need to do anything here
                    m_nodes[i].occupied = false;
                }
            }
            return *this;
        }

        HashMap& operator=(HashMap&&) = delete;

        ~HashMap() { Destroy(); }

        void Create(const u64 size)
        {
            // Allocate enough memory for all the elements and indices pointing to them
            m_nodes = Memory.Allocate<HashMapNode>(MemoryType::HashMap, size);
            m_size  = size;

            // Invalidate all entries
            for (u64 i = 0; i < m_size; i++)
            {
                m_nodes[i].occupied = false;
            }
        }

        /**
         * @brief Clears all the entries from the hashmap without destroying underlying memory
         * meaning you can keep using the hashmap as if it was just freshly created
         */
        void Clear()
        {
            for (u64 i = 0; i < m_size; i++)
            {
                m_nodes[i].occupied = false;
                m_nodes[i].element.~Value();
            }
            m_count = 0;
        }

        void Destroy()
        {
            // Call the destructor for all elements
            std::destroy_n(m_nodes, m_size);

            // If this HashMap is created we free all it's dynamically allocated memory
            if (m_size > 0 && m_nodes)
            {
                Memory.Free(m_nodes);
                m_nodes = nullptr;

                m_size  = 0;
                m_count = 0;
            }
        }

        void Set(const Key& key, const Value& value)
        {
            const auto hash = m_hashCompute(key, m_size);
            auto& node      = m_nodes[hash];

            // If the slot was not occupied yet we increment the count
            if (!node.occupied)
            {
                m_count++;
            }

            node.occupied = true;
            node.element  = value;
        }

        void Delete(const Key& key)
        {
            const auto hash = m_hashCompute(key, m_size);
            if (!m_nodes[hash].occupied)
            {
                throw std::invalid_argument("Key could not be found");
            }

            // Set the occupied flag to false
            m_nodes[hash].occupied = false;
            // Call the destructor for the element
            m_nodes[hash].element.~Value();
            // Decrement the count to keep track of how many elements we are currently storing
            m_count--;
        }

        Value& Get(const Key& key)
        {
            const auto hash = m_hashCompute(key, m_size);
            if (!m_nodes[hash].occupied)
            {
                throw std::invalid_argument("Key could not be found");
            }
            return m_nodes[hash].element;
        }

        [[nodiscard]] const Value& Get(const Key& key) const
        {
            const auto hash = m_hashCompute(key, m_size);
            if (!m_nodes[hash].occupied)
            {
                throw std::invalid_argument("Key could not be found");
            }
            return m_nodes[hash].element;
        }

        Value& GetByIndex(u64 index) { return m_nodes[index].element; }

        [[nodiscard]] const Value& GetByIndex(u64 index) const { return m_nodes[index].element; }

        [[nodiscard]] bool Has(const Key& key) const
        {
            const auto hash = m_hashCompute(key, m_size);
            return m_nodes[hash].occupied;
        }

        Value& operator[](const Key& key)
        {
            const auto hash = m_hashCompute(key, m_size);
            if (!m_nodes[hash].occupied)
            {
                throw std::invalid_argument("Key could not be found");
            }
            return m_nodes[hash].element;
        }

        [[nodiscard]] const Value& operator[](const Key& key) const
        {
            const auto hash = m_hashCompute(key, m_size);
            if (!m_nodes[hash].occupied)
            {
                throw std::invalid_argument("Key could not be found");
            }
            return m_nodes[hash].element;
        }

        [[nodiscard]] HashMapIterator begin() const { return HashMapIterator(this); }

        [[nodiscard]] HashMapIterator end() const { return HashMapIterator(this, m_size); }

        [[nodiscard]] u64 Size() const { return m_size; }

        [[nodiscard]] u64 Count() const { return m_count; }

        [[nodiscard]] u64 GetIndex(const Key& key) const { return m_hashCompute(key, m_size); }

    private:
        HashMapNode* m_nodes;

        u64 m_size;
        u64 m_count;
        HashFunc m_hashCompute;
    };
}  // namespace C3D

template <>
struct std::hash<const char*>
{
    size_t operator()(const char* key) const noexcept
    {
        size_t hash         = 0;
        const size_t length = std::strlen(key);

        for (size_t i = 0; i < length; i++)
        {
            hash ^= static_cast<size_t>(key[i]);
            hash *= FNV_PRIME;
        }
        return hash;
    }
};
