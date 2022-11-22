
#pragma once
#include "core/defines.h"
#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	// Default capacity of dynamic array if no capacity is provided in create method
	constexpr u32 DYNAMIC_ARRAY_DEFAULT_CAPACITY = 4;
	// Times how much the dynamic array is increased every time a resize is required
	constexpr f32 DYNAMIC_ARRAY_RESIZE_FACTOR = 1.5f;

	template<class T>
	class __declspec(dllexport) DynamicArray
	{
	public:
		DynamicArray();

		DynamicArray(const DynamicArray<T>& other);
		DynamicArray(DynamicArray<T>&& other) noexcept;

		DynamicArray<T>& operator=(const DynamicArray<T>& other);
		DynamicArray<T>& operator=(DynamicArray<T>&& other) noexcept;

		~DynamicArray();

		/*
		 * @brief Creates the array with enough memory allocated for the provided initial capacity.
		 * The array will be dynamically resized if more space is necessary.
		 */
		explicit DynamicArray(u64 initialCapacity);

		DynamicArray(std::initializer_list<T> list);
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
		const T& operator[](u64 index) const;

		/* @brief Gets a reference to the element at the provided index. Includes out-of-range checks */
		T& At(u64 index);
		[[nodiscard]] const T& At(u64 index) const;

		/* @brief Returns a reference to the first element in the array. */
		T& Front();
		[[nodiscard]] const T& Front() const;

		/* @brief Returns a reference to the last element in the array. */
		T& Back();
		[[nodiscard]] const T& Back() const;

		/*
		 * @brief Adds the provided element to the back of the array.
		 * This will cause a resize if size + 1 >= capacity.
		 */
		void PushBack(const T& element);

		template <class... Args>
		T& EmplaceBack(Args&& ... args);

		/*
		 * @brief Copies over the elements at the provided pointer
		 * This is a destructive operation that will first delete all the memory in the
		 * dynamic array if there is any. This will resize the dynamic array to count.
		 */
		void Copy(const T* elements, u64 count);
		void Copy(const DynamicArray<T>& other);

		/*
		 * @brief Clears all elements in the array (calling the destructor for every element)
		 * Does not delete the memory and the capacity will remain the same.
		 */
		void Clear();

		[[nodiscard]] T* GetData() const { return m_elements; }

		[[nodiscard]] u64 Size() const noexcept { return m_size; }

		[[nodiscard]] bool Empty() const noexcept { return m_size == 0; }

		[[nodiscard]] u64 Capacity() const noexcept { return m_capacity; }

		[[nodiscard]] T* begin() const noexcept;
		[[nodiscard]] T* end() const noexcept;
	private:
		void Resize();
		void Free();

		u64 m_capacity;
		u64 m_size;
		T* m_elements;
	};

	template<class T>
	DynamicArray<T>::DynamicArray() : m_capacity(0), m_size(0), m_elements(nullptr) {}

	template <class T>
	DynamicArray<T>::DynamicArray(const DynamicArray<T>& other)
		: m_capacity(0), m_size(0), m_elements(nullptr)
	{
		Copy(other);
	}

	template <class T>
	DynamicArray<T>& DynamicArray<T>::operator=(const DynamicArray<T>& other)
	{
		if (this == &other) return *this;

		Copy(other);
		return *this;
	}

	template <class T>
	DynamicArray<T>& DynamicArray<T>::operator=(DynamicArray<T>&& other) noexcept
	{
		if (this == &other) return *this;

		m_elements = other.GetData();
		m_size = other.Size();
		m_capacity = other.Capacity();

		// Null out "other" to ensure we don't double free
		other.m_capacity = 0;
		other.m_size = 0;
		other.m_elements = nullptr;

		return *this;
	}

	template <class T>
	DynamicArray<T>::DynamicArray(DynamicArray<T>&& other) noexcept
		: m_capacity(other.Capacity()), m_size(other.Size()), m_elements(other.GetData())
	{
		other.m_capacity = 0;
		// Take all data from other and null out other so it does not call Free() twice on the sa
		other.m_size = 0;
		other.m_elements = nullptr;
	}

	template <class T>
	DynamicArray<T>::~DynamicArray()
	{
		Destroy();
	}

	template <class T>
	DynamicArray<T>::DynamicArray(const u64 initialCapacity)
		: m_capacity(0), m_size(0), m_elements(nullptr)
	{
		Reserve(initialCapacity);
	}

	template <class T>
	DynamicArray<T>::DynamicArray(std::initializer_list<T> list)
	{
		Copy(list.begin(), list.size());
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
			//Logger::Trace("[DYNAMIC_ARRAY] - Reserve() - Was called with initialCapacity <= currentCapacity. Doing nothing");
			return true;
		}

		//Logger::Trace("[DYNAMIC_ARRAY] - Reserve({})", initialCapacity);

		// We allocate enough memory for the new capacity
		T* newElements = Memory.Allocate<T>(initialCapacity, MemoryType::DynamicArray);
		u64 newSize = 0;

		if (m_elements)
		{
			// We already have a pointer	
			if (m_size != 0)
			{
				// We have elements in our array already which we need to copy over
				// NOTE: We may not use std::memcpy here since we don't know the type of T
				// And T could have an arbitrarily complex copy constructor
				for (u64 i = 0; i < m_size; i++)
				{
					newElements[i] = m_elements[i];
				}

				newSize = m_size;
			}

			// We delete our old memory
			Free();
		}

		// We set our new capacity
		m_capacity = initialCapacity;
		// We copy over our size (since it might have been cleared by our Free() call
		m_size = newSize;
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
		for (size_t i = m_size; i < m_capacity; i++)
		{
			m_elements[i] = T();
		}
		m_size = m_capacity;
	}

	template <class T>
	void DynamicArray<T>::Destroy()
	{
		// Call destructor for all elements
		for (size_t i = 0; i < m_size; i++)
		{
			m_elements[i].~T();
		}
		Free();
	}

	template <class T>
	T& DynamicArray<T>::operator[](u64 index)
	{
		return m_elements[index];
	}

	template <class T>
	const T& DynamicArray<T>::operator[](u64 index) const
	{
		return m_elements[index];
	}

	template <class T>
	T& DynamicArray<T>::At(u64 index)
	{
		if (index >= m_size) throw std::out_of_range("Index >= Size()");
		return m_elements[index];
	}

	template <class T>
	const T& DynamicArray<T>::At(u64 index) const
	{
		if (index >= m_size) throw std::out_of_range("Index >= Size()");
		return m_elements[index];
	}

	template <class T>
	T& DynamicArray<T>::Front()
	{
		return m_elements[0];
	}

	template <class T>
	const T& DynamicArray<T>::Front() const
	{
		return m_elements[0];
	}

	template <class T>
	T& DynamicArray<T>::Back()
	{
		return m_elements[m_size - 1];
	}

	template <class T>
	const T& DynamicArray<T>::Back() const
	{
		return m_elements[m_size - 1];
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

	template<class T>
	template<class ...Args>
	T& DynamicArray<T>::EmplaceBack(Args&& ...args)
	{
		if (!m_elements || m_size >= m_capacity)
		{
			// If we have not initialized our element array or if
			// we have reached our capacity we need to resize
			Resize();
		}

		// Construct element T (in-place) with the provided arguments
		new(&m_elements[m_size]) T(std::forward<Args>(args)...);
		// Return a reference to the newly added element and then increment size
		return m_elements[m_size++];
	}

	template <class T>
	void DynamicArray<T>::Copy(const T* elements, const u64 count)
	{
		if (count == 0) return; // No need to do anything
		
#ifdef _DEBUG
		assert(elements);
#endif

		// If we have any memory allocated we have to free it first
		Free();

		// We allocate enough memory for the provided count
		m_elements = Memory.Allocate<T>(count, MemoryType::DynamicArray);
		// Then we copy over the elements from the provided pointer into our newly allocated memory
		// Note: Again we may not use std::memcpy here since T could have an arbitrarily complex copy constructor
		for (u64 i = 0; i < count; i++)
		{
			m_elements[i] = elements[i];
		}

		m_size = count;
		m_capacity = count;
	}

	template <class T>
	void DynamicArray<T>::Copy(const DynamicArray<T>& other)
	{
		Copy(other.GetData(), other.Size());
	}

	template <class T>
	void DynamicArray<T>::Clear()
	{
		for (size_t i = 0; i < m_size; i++)
		{
			m_elements[i].~T();
		}

		Memory.Zero(m_elements, sizeof(T) * m_size);
		m_size = 0;
	}

	template <class T>
	T* DynamicArray<T>::begin() const noexcept
	{
		return m_elements;
	}

	template <class T>
	T* DynamicArray<T>::end() const noexcept
	{
		return &m_elements[m_size];
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
		auto newCapacity = static_cast<u64>(static_cast<f32>(m_capacity) * DYNAMIC_ARRAY_RESIZE_FACTOR);
		if (newCapacity == 0) newCapacity = DYNAMIC_ARRAY_DEFAULT_CAPACITY;

		// Allocate memory for the new capacity
		T* newElements = Memory.Allocate<T>(newCapacity, MemoryType::DynamicArray);
		u64 newSize = 0;

		// If there exists an element pointer
		if (m_elements && m_size > 0)
		{
			// Copy over old elements to the newly allocated block of memory
			// Note: we may not use std::memcpy here since T could have an arbitrarily complex copy constructor
			for (u64 i = 0; i < m_size; i++)
			{
				newElements[i] = m_elements[i];
			}
			newSize = m_size;

			// Free the old memory if it exists
			Free();
		}

		// Set our new element pointer, capacity and size
		m_elements = newElements;
		m_capacity = newCapacity;
		m_size = newSize;
	}

	template <class T>
	void DynamicArray<T>::Free()
	{
		if (m_elements && m_capacity != 0)
		{
			Memory.Free(m_elements, m_capacity * sizeof(T), MemoryType::DynamicArray);
			m_elements = nullptr;
			m_capacity = 0;
			m_size = 0;
		}
	}
}
