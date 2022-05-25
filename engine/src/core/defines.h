
#pragma once
#include <string>
#include <cstdint>

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