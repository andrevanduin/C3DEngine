
#pragma once
#include "core/defines.h"
#include "core/memory.h"
#include "services/services.h"

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
		explicit DynamicArray(u64 initialCapacity);

		DynamicArray(const T* elements, u64 count);

		/*
		 * @brief Reserves enough memory for the provided initial capacity.
		 * The array will still have the original size and no elements will be created or added
		 */
		bool Reserve(u64 initialCapacity = DYNAMIC_ARRAY_DEFAULT_CAPACITY);

		/*
		 * @brief Resizes the array to have enough memory for the requested size
		 * The array will default construct elements in all newly created empty slots up to size - 1
		 */
		void Resize(u64 size);

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
		 * @brief Copies over the elements at the provided pointer
		 * This is a destructive operation that will first delete all the memory in the
		 * dynamic array if there is any. This will resize the dynamic array to count.
		 */
		void Copy(const T* elements, u64 count);

		/*
		 * @brief Zeroes out the elements in the array.
		 * Does not delete the memory and the capacity will remain the same.
		 */
		void Clear();

		[[nodiscard]] T* GetData() const;

		[[nodiscard]] u64 Size() const { return m_size; }

		[[nodiscard]] u64 Capacity() const { return m_capacity; }

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

	template <class T>
	DynamicArray<T>::DynamicArray(const u64 initialCapacity)
		: m_capacity(0), m_size(0), m_elements(nullptr)
	{
		Reserve(initialCapacity);
	}

	template <class T>
	DynamicArray<T>::DynamicArray(const T* elements, const u64 count)
	{
		Copy(elements, count);
	}

	template<class T>
	bool DynamicArray<T>::Reserve(const u64 initialCapacity)
	{
		if (initialCapacity == 0) 
		{
			Logger::Error("[DYNAMIC_ARRAY] - Reserve() - Was called with initialCapacity of 0.");
			return false;
		}

		if (m_capacity >= initialCapacity)
		{
			// Reserve is not needed since our capacity is already as large or larger
			Logger::Trace("[DYNAMIC_ARRAY] - Reserve() - Was called with initialCapacity <= currentCapacity. Doing nothing");
			return true;
		}

		// Logger::Trace("[DYNAMIC_ARRAY] - Reserve({})", initialCapacity);

		// We allocate enough memory for the new capacity
		T* newElements = Memory.Allocate<T>(initialCapacity, MemoryType::DynamicArray);
		if (m_elements)
		{
			// We already have a pointer	
			if (m_size != 0)
			{
				// We have elements in our array already which we need to copy over
				Memory.Copy(newElements, m_elements, m_size * sizeof(T));
			}

			// We delete our old memory
			Memory.Free(m_elements, sizeof(T) * m_capacity, MemoryType::DynamicArray);
		}

		// We set our new capacity
		m_capacity = initialCapacity;
		// We set our elements pointer to the newly allocated memory
		m_elements = newElements;
		return true;
	}

	template <class T>
	void DynamicArray<T>::Resize(const u64 size)
	{
		// Reserve enough capacity
		Reserve(size);
		// All new empty slots up to capacity should be filled with default elements
		for (u64 i = m_size; i < m_capacity; i++)
		{
			m_elements[i] = T();
		}
		m_size = m_capacity;
	}

	template <class T>
	void DynamicArray<T>::Destroy()
	{
		if (m_elements)
		{
			Memory.Free(m_elements, sizeof(T) * m_capacity, MemoryType::DynamicArray);
		}
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
		if (!m_elements || m_size >= m_capacity)
		{
			// If we have not initialized our element array or if
			// we have reached our capacity we need to resize
			Resize();
		}
		
		m_elements[m_size] = element;
		m_size++;
	}

	template <class T>
	void DynamicArray<T>::Copy(const T* elements, const u64 count)
	{
#ifdef _DEBUG
		assert(elements && count > 0);
#endif

		if (m_elements)
		{
			// We already have a valid pointer to elements so we free this memory
			Memory.Free(m_elements, m_capacity * sizeof(T), MemoryType::DynamicArray);
		}

		// We allocate enough memory for the provided count
		m_elements = Memory.Allocate<T>(count, MemoryType::DynamicArray);
		// Then we copy over the elements from the provided pointer into our newly allocated memory
		Memory.Copy(m_elements, elements, sizeof(T) * count);

		m_size = count;
		m_capacity = count;
	}

	template <class T>
	void DynamicArray<T>::Clear()
	{
		Memory.Zero(m_elements, sizeof(T) * m_size);
		m_size = 0;
	}

	template <class T>
	T* DynamicArray<T>::GetData() const
	{
		return m_elements;
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
#ifdef _DEBUG
		//auto newSize = m_capacity * DYNAMIC_ARRAY_RESIZE_FACTOR;
		//if (newSize == 0) newSize = DYNAMIC_ARRAY_DEFAULT_CAPACITY;
		//Logger::Trace("[DYNAMIC_ARRAY] - Resize() called with Capacity: {} -> {}", m_capacity, newSize);
#endif

		// Increase our capacity by the resize factor
		m_capacity *= DYNAMIC_ARRAY_RESIZE_FACTOR;
		if (m_capacity == 0) m_capacity = DYNAMIC_ARRAY_DEFAULT_CAPACITY;

		// Allocate memory for the new capacity
		T* newElements = Memory.Allocate<T>(m_capacity, MemoryType::DynamicArray);
		// If there exists an element pointer
		if (m_elements)
		{
			// Copy over old elements to the newly allocated block of memory
			if (m_size != 0) Memory.Copy(newElements, m_elements, m_size * sizeof(T));
			// Free the old memory if it exists
			Memory.Free(m_elements, m_size * sizeof(T), MemoryType::DynamicArray);
		}
		
		// Set our element pointer to our newly allocated memory
		m_elements = newElements;
	}
}
