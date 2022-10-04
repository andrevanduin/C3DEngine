
#pragma once
#include "core/defines.h"

namespace C3D
{
	template <typename Type, u64 ElementCount>
	class Array
	{
	public:
		Array() = default;
		Array(std::initializer_list<Type> values);

		Type& operator[](u64 index);
		const Type& operator[](u64 index) const;

		Type& At(u64 index);
		[[nodiscard]] const Type& At(u64 index) const;

		[[nodiscard]] static u64 Size() { return ElementCount; }

		[[nodiscard]] constexpr Type* Data() { return static_cast<Type*>(m_elements); }
		[[nodiscard]] constexpr const Type* Data() const { return const_cast<Type*>(m_elements); }

		constexpr Type* begin() noexcept { return Data(); }
		[[nodiscard]] constexpr const Type* begin() const noexcept { return Data(); }

		constexpr Type* end() noexcept { return &m_elements[ElementCount]; }
		[[nodiscard]] constexpr const Type* end() const noexcept { return const_cast<Type*>(&m_elements[ElementCount]); }
	private:
		Type m_elements[ElementCount];
	};


	template <typename Type, u64 ElementCount>
	Array<Type, ElementCount>::Array(std::initializer_list<Type> values)
	{
		assert(values.size() <= ElementCount);
		std::copy(values.begin(), values.end(), m_elements);
	}

	template<typename Type, u64 ElementCount>
	Type& Array<Type, ElementCount>::operator[](u64 index)
	{
		return m_elements[index];
	}

	template<typename Type, u64 ElementCount>
	const Type& Array<Type, ElementCount>::operator[](u64 index) const
	{
		return m_elements[index];
	}

	template<typename Type, u64 ElementCount>
	Type& Array<Type, ElementCount>::At(u64 index)
	{
		assert(index > 0 && index < ElementCount);
		return m_elements[index];;
	}

	template<typename Type, u64 ElementCount>
	const Type& Array<Type, ElementCount>::At(u64 index) const
	{
		assert(index > 0 && index < ElementCount);
		return m_elements[index];
	}
}
