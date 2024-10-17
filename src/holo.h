#pragma once

#include <stdint.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define S8_MIN  ((s8)(1U << 7))
#define S16_MIN ((s16)(1U << 15))
#define S32_MIN ((s32)(1UL << 31))
#define S64_MIN ((s64)(1ULL << 63))

#define S8_MAX  ((s8)(~(u8)S8_MIN))
#define S16_MAX ((s16)(~(u16)S16_MIN))
#define S32_MAX ((s32)(~(u32)S32_MIN))
#define S64_MAX ((s64)(~(u64)S64_MIN))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define U8_MAX  (~(u8)0)
#define U16_MAX (~(u16)0)
#define U32_MAX (~(u32)0)
#define U64_MAX (~(u64)0)

typedef s64 smm;
typedef u64 umm;

typedef float f32;
typedef double f64;

typedef u8 bool;
#define true 1
#define false 0

bool IsRunInDebugger();
void AssertHandler(const char* file, int line, const char* ex);
#define ASSERT(EX) ((EX) ? 1 : ((IsRunInDebugger() ? *(volatile int*)0 = 0 : AssertHandler(__FILE__, __LINE__, #EX)), 0))
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

#define ARRAY_LEN(A) (sizeof(A)/sizeof(0[A]))

#ifdef _MSC_VER
#  define ALIGNOF(T) _alignof(T)
#else
#  error NOT IMPLEMENTED
#endif

#define ALIGN(N, A) (((umm)(N) + ((umm)(A) + 1)) & (umm)-(smm)(A))

void*
Copy(void* dst, void* src, u64 size)
{
	return memcpy(dst, src, size);
}

void*
Move(void* dst, void* src, u64 size)
{
	return memmove(dst, src, size);
}

void
Zero(void* ptr, u64 size)
{
	memset(ptr, 0, size);
}

typedef struct Arena
{
	u64 reserved;
	u64 cursor;
	u64 committed;
	u8* memory;
} Arena;

typedef struct Arena_Marker
{
	u64 _value;
} Arena_Marker;

Arena Arena_Create(umm reserve_size);
void Arena_Destroy(Arena* arena);
void* Arena_Push(Arena* arena, u64 size, u8 alignment);

#define Arena_PushArray(ARENA, T, LEN) (T*)Arena_Push((ARENA), sizeof(T)*(LEN), ALIGNOF(T))

wchar_t*
Arena_PushWideString(Arena* arena, wchar_t* string)
{
	wchar_t* scan = string;
	while (*scan != 0) ++scan;
	u64 len = scan - string;

	wchar_t* result = Arena_PushArray(arena, wchar_t, len + 1);
	Copy(result, string, len);

	return result;
}

void
Arena_Clear(Arena* arena)
{
	arena->cursor = 0;
}

Arena_Marker
Arena_GetMarker(Arena* arena)
{
	return (Arena_Marker){ ._value = arena->cursor };
}

Arena_Marker
ArenaMarker_Advance(Arena_Marker marker, u32 amount)
{
	return (Arena_Marker){ ._value = marker._value + amount };
}

void
Arena_PopToMarker(Arena* arena, Arena_Marker marker)
{
	ASSERT(arena->cursor >= marker._value);
	arena->cursor = marker._value;
}

#define ARENA_SCOPED_TEMP(ARENA) for (Arena_Marker arena__scoped_temp_marker = Arena_GetMarker(ARENA); arena__scoped_temp_marker._value != -1; Arena_PopToMarker((ARENA), arena__scoped_temp_marker), arena__scoped_temp_marker._value = -1)
