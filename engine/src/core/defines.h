
#pragma once
#include <string>
#include <cstdint>
#include <type_traits>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef float		f32;
typedef double		f64;

typedef std::string string;

static_assert(sizeof(u8)  == 1, "Expected u8  to be 1 byte");
static_assert(sizeof(u16) == 2, "Expected u16 to be 2 bytes");
static_assert(sizeof(u32) == 4, "Expected u32 to be 4 bytes");
static_assert(sizeof(u64) == 8, "Expected u64 to be 8 bytes");

static_assert(sizeof(i8)  == 1, "Expected i8  to be 1 byte");
static_assert(sizeof(i16) == 2, "Expected i16 to be 2 bytes");
static_assert(sizeof(i32) == 4, "Expected i32 to be 4 bytes");
static_assert(sizeof(i64) == 8, "Expected i64 to be 8 bytes");

template <typename E>
constexpr auto ToUnderlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

/**
 * @brief Any id set to this value should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID UINT32_MAX

#ifdef C3D_EXPORT
#ifdef _MSC_VER
#define C3D_API __declspec(dllexport)
#else
#define C3D_API __attribute__((visibility("default")))
#endif
#else
#ifdef _MSC_VER
#define C3D_API __declspec(dllimport)
#else
#define C3D_API
#endif
#endif

#define C3D_CLAMP(value, min, max) ((value) <= (min)) ? (min) : ((value) >= (max)) ? (max) : (value);

#ifdef _MSC_VER
#define C3D_INLINE __forceinline
#define C3D_NO_INLINE __declspec(noinline)
#else
#define C3D_INLINE static inline
#define C3D_NO_INLINE
#endif

 /** @brief Gets the number of bytes from amount of gibibytes (GiB) (1024*1024*1024) */
#define GIBIBYTES(amount) amount * 1024 * 1024 * 1024
/** @brief Gets the number of bytes from amount of mebibytes (MiB) (1024*1024) */
#define MEBIBYTES(amount) amount * 1024 * 1024
/** @brief Gets the number of bytes from amount of kibibytes (KiB) (1024) */
#define KIBIBYTES(amount) amount * 1024

/** @brief Gets the number of bytes from amount of gigabsytes (GB) (1000*1000*1000) */
#define GIGABYTES(amount) amount * 1000 * 1000 * 1000
/** @brief Gets the number of bytes from amount of megabytes (MB) (1000*1000) */
#define MEGABYTES(amount) amount * 1000 * 1000
/** @brief Gets the number of bytes from amount of kilobytes (KB) (1000) */
#define KILOBYTES(amount) amount * 1000