#pragma once

#define COBJMACROS
#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <initguid.h>
#include <propvarutil.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfvirtualcamera.h>
#include <mferror.h>
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

#include <stdint.h>
#include <stdio.h>

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
	u8* bdst = dst;
	u8* bsrc = src;
	
	for (u64 i = 0; i < size; ++i) bdst[i] = bsrc[i];

	return dst;
}

void*
Move(void* dst, void* src, u64 size)
{
	u8* bdst = dst;
	u8* bsrc = src;

	if (bdst > bsrc) for (u64 i = 0;      i < size; ++i) bdst[i] = bsrc[i];
	else             for (u64 i = size-1; i < size; --i) bdst[i] = bsrc[i];

	return dst;
}

void
Zero(void* ptr, u64 size)
{
	for (u64 i = 0; i < size; ++i) ((u8*)ptr)[i] = 0;
}

#define HOLO_MAX_CAMERA_COUNT 256
#define HOLO_MAX_PER_CAMERA_STREAM_COUNT 32

// {AAEB72CB-0C36-4633-BB16-EC69B3F32365}
DEFINE_GUID(IID_IHoloCamActivateFactory, 0xaaeb72cb, 0xc36, 0x4633, 0xbb, 0x16, 0xec, 0x69, 0xb3, 0xf3, 0x23, 0x65);

// {6082E58D-228A-44FC-B29C-1CA877E77A0F}
DEFINE_GUID(IID_IHoloCamActivate, 0x6082e58d, 0x228a, 0x44fc, 0xb2, 0x9c, 0x1c, 0xa8, 0x77, 0xe7, 0x7a, 0xf);

// {244DF9E8-EF36-4330-850D-6324326F9446}
DEFINE_GUID(CLSID_IHoloCamActivate, 0x244df9e8, 0xef36, 0x4330, 0x85, 0xd, 0x63, 0x24, 0x32, 0x6f, 0x94, 0x46);
#define CLSID_IHOLOCAMACTIVATE_STRING L"{244DF9E8-EF36-4330-850D-6324326F9446}"

// {1E5D57F3-AEA2-45B3-9D21-E9A79F43CE64}
DEFINE_GUID(HOLO_CAM_PHYSICAL_DEVICE_SYMLINK, 0x1e5d57f3, 0xaea2, 0x45b3, 0x9d, 0x21, 0xe9, 0xa7, 0x9f, 0x43, 0xce, 0x64);

#define RET_IF_FAIL(E) do { HRESULT RET_result = (E); if (FAILED(RET_result)) return RET_result; } while (0);
