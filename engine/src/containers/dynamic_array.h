
#pragma once
#include "core/defines.h"
#include "core/memory.h"

namespace C3D
{
	// Default capacity of dynamic array if no capacity is provided in create method
	constexpr u32 DYNAMIC_ARRAY_DEFAULT_CAPACITY = 4;
	// Times how much the dynamic array is increase every time a resize is required
	constexpr u8 DYNAMIC_ARRAY_RESIZE_FACTOR = 2;

	template<class T>
	class __declspec(dllexport) DynamicArray
	{
	public:
		DynamicArray();

		/*
		 * @brief Creates the array with enough memory allocated for the provided initial capacity.
		 * The array will be dynamically resized if more space is necessary.
		 */
		bool Create(u64 initialCapacity = DYNAMIC_ARRAY_DEFAULT_CAPACITY);

		/* @brief Destroys the underlying memory allocated by this dynamic array. */
		void Destroy();

		/* @brief Gets a reference to the element at the provided index. */
		T& operator[](u64 index);

		/* @brief Gets the element at the provided index. */
		T Get(u64 index);

		/*
		 * @brief Adds the provided element to the back of the array.
		 * This will cause a resize if size + 1 >= capacity.
		 */
		void PushBack(const T& element);

		/*
		 * @brief Resizes the array to at least the provided new capacity.
		 * If the capacity >= newCapacity nothing will happen.
		 */
		void Resize(u64 newCapacity);

		/*
		 * @brief Zeroes out the elements in the array.
		 * Does not delete the memory and the capacity will remain the same.
		 */
		void Clear();

		[[nodiscard]] u64 Size() const { return m_size; }

		class Iterator
		{
		public:
			using iteratorCategory	= std::forward_iterator_tag;
			using differenceType	= std::ptrdiff_t;
			using valueType = T;
			using pointer	= T*;
			using reference = T&;

			explicit Iterator(pointer ptr) : m_ptr(ptr) {}

			reference operator*() const { return *m_ptr; }
			pointer operator->() { return m_ptr; }
			Iterator& operator++() { ++m_ptr; return *this; }
			Iterator operator++(int)
			{
				Iterator temp(*this);
				++*this;
				return temp;
			}

			friend bool operator==(const Iterator& lhs, const Iterator& rhs) { return lhs.m_ptr == rhs.m_ptr; }
			friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { return lhs.m_ptr != rhs.m_ptr; }

		private:
			pointer m_ptr;
		};

		[[nodiscard]] Iterator begin() const;
		[[nodiscard]] Iterator end() const;

	private:
		void Resize();

		u64 m_capacity;
		u64 m_size;
		T* m_elements;
	};

	template<class T>
	DynamicArray<T>::DynamicArray() : m_capacity(0), m_size(0), m_elements(nullptr) {}

	template<class T>
	bool DynamicArray<T>::Create(const u64 initialCapacity)
	{
		if (initialCapacity == 0) 
		{
			Logger::Error("[DYNAMIC_ARRAY] - Create() - Was called with initialCapacity of 0.");
			return false;
		}

		m_capacity = initialCapacity;
		m_elements = Memory.Allocate<T>(m_capacity, MemoryType::DynamicArray);

		return true;
	}

	template <class T>
	void DynamicArray<T>::Destroy()
	{
		Memory.Free(m_elements, sizeof(T) * m_capacity, MemoryType::DynamicArray);
		m_capacity = 0;
		m_elements = nullptr;
		m_size = 0;
	}

	template <class T>
	T& DynamicArray<T>::operator[](u64 index)
	{
#ifdef _DEBUG
		assert(index < m_size);
#endif
		return m_elements[index];
	}

	template <class T>
	T DynamicArray<T>::Get(u64 index)
	{
#ifdef _DEBUG
		assert(index < m_size);
#endif
		return m_elements[index];
	}

	template <class T>
	void DynamicArray<T>::PushBack(const T& element)
	{
		if (m_size >= m_capacity)
		{
			Resize();
		}
		m_elements[m_size] = element;
		m_size++;
	}

	template <class T>
	void DynamicArray<T>::Resize(const u64 newCapacity)
	{
		if (newCapacity > m_capacity)
		{
			// The requested new size is larger than our current capacity so we actually need to resize.
			// We always resize by our resize factor so the actual capacity might be larger than requested
			Resize();
		}
	}

	template <class T>
	void DynamicArray<T>::Clear()
	{
		Memory.Zero(m_elements, sizeof(T) * m_size, MemoryType::DynamicArray);
		m_size = 0;
	}

	template <class T>
	typename DynamicArray<T>::Iterator DynamicArray<T>::begin() const
	{
		return Iterator(m_elements);
	}

	template <class T>
	typename DynamicArray<T>::Iterator DynamicArray<T>::end() const
	{
		return Iterator(m_elements + m_size);
	}

	template <class T>
	void DynamicArray<T>::Resize()
	{
		// Increase our capacity by the resize factor
		m_capacity *= DYNAMIC_ARRAY_RESIZE_FACTOR;
		// Allocate memory for the new capacity
		T* newElements = Memory.Allocate<T>(m_capacity, MemoryType::DynamicArray);
		// Copy over old elements to the newly allocated block of memory
		for (u32 i = 0; i < m_size; i++)
		{
			newElements[i] = m_elements[i];
		}
		// Free the old memory
		Memory.Free(m_elements, m_size * sizeof(T), MemoryType::DynamicArray);
		// Set our element pointer to our newly allocated memory
		m_elements = newElements;
	}
}
