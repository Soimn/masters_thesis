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
#include <ks.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <ksproxy.h>
#include <ksguid.h>
#include <ksmedia.h>
#include <evntprov.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

DEFINE_GUID(WHY_MICROSOFT_IID_IKsControl, 0x28F54685L, 0x06FD, 0x11D2, 0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

#include <stdio.h>

typedef signed __int8  s8;
typedef signed __int16 s16;
typedef signed __int32 s32;
typedef signed __int64 s64;

#define S8_MIN  ((s8)(1U << 7))
#define S16_MIN ((s16)(1U << 15))
#define S32_MIN ((s32)(1UL << 31))
#define S64_MIN ((s64)(1ULL << 63))

#define S8_MAX  ((s8)(~(u8)S8_MIN))
#define S16_MAX ((s16)(~(u16)S16_MIN))
#define S32_MAX ((s32)(~(u32)S32_MIN))
#define S64_MAX ((s64)(~(u64)S64_MIN))

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

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

#define BREAK_IF_FAILED(RESULT, CALL) { (RESULT) = (CALL); if (!SUCCEEDED((RESULT))) break; }

#define U64_HI_LO(HI, LO) (((u64)(HI) << 32) | (u64)(LO))

#ifdef HOLO_DEBUG
void
Log(char* format, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	buffer[sizeof(buffer)-1] = 0;

	OutputDebugStringA(buffer);
}

void
LogGUID(char* prefix, const GUID* guid)
{
	Log("%s{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", prefix, guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
			                                                        guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
																															guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

#else

void
Log(char* format, ...)
{
}

void
LogGUID(char* prefix, const GUID* guid)
{
}
#endif

#define LOG_FUNCTION_ENTRY() Log("[HOLO] -- " __FUNCTION__)
#define LOG_FUNCTION_RESULT(R) Log("[HOLO] :: " __FUNCTION__ " - %d", (R))

HMODULE Module = 0;

#define HOLOCAM_IDS
#include "../holo_cam.h"
#include "attributes.h"
#include "presentation_descriptor.h"
#include "media_type_handler.h"
#include "stream_descriptor.h"
#include "media_stream.h"
#include "media_source.h"
#include "activate.h"

BOOL
DllMain(HINSTANCE module, DWORD reason, LPVOID _)
{
	BOOL result = TRUE;

	if (reason == DLL_PROCESS_ATTACH)
	{
		Module = module;
		DisableThreadLibraryCalls(module);

		MediaStreamPoolOccupancy    = 0;
		MediaStreamPoolFreeList     = &MediaStreamPool[0];
		MediaStreamPoolFreeListLock = (SRWLOCK)SRWLOCK_INIT;
		for (umm i = 0; i < ARRAY_LEN(MediaStreamPool); ++i)
		{
			MediaStreamPool[i] = (Media_Stream){
				.lpVtbl          = &MediaStream_Vtbl,
				.lpKsControlVtbl = &MediaStream_KsControl_Vtbl,
				.ref_count       = 0,
				.lock            = SRWLOCK_INIT,
				.next_free       = &MediaStreamPool[i+1],
			};
		}
		MediaStreamPool[ARRAY_LEN(MediaStreamPool)-1].next_free = 0;

		MediaSourcePoolOccupancy    = 0;
		MediaSourcePoolFreeList     = &MediaSourcePool[0];
		MediaSourcePoolFreeListLock = (SRWLOCK)SRWLOCK_INIT;
		for (umm i = 0; i < ARRAY_LEN(MediaSourcePool); ++i)
		{
			MediaSourcePool[i] = (Media_Source){
				.lpVtbl                       = &MediaSource_Vtbl,
				.lpGetServiceVtbl             = &MediaSource_GetService_Vtbl,
				.lpSampleAllocatorControlVtbl = &MediaSource_SampleAllocatorControl_Vtbl,
				.lpKsControlVtbl              = &MediaSource_KsControl_Vtbl,
				.ref_count                    = 0,
				.lock                         = SRWLOCK_INIT,
				.next_free                    = &MediaSourcePool[i+1],
			};
		}
		MediaSourcePool[ARRAY_LEN(MediaSourcePool)-1].next_free = 0;

		ActivatePoolOccupancy    = 0;
		ActivatePoolFreeList     = &ActivatePool[0];
		ActivatePoolFreeListLock = (SRWLOCK)SRWLOCK_INIT;
		for (umm i = 0; i < ARRAY_LEN(ActivatePool); ++i)
		{
			ActivatePool[i] = (Activate){
				.lpVtbl    = &Activate_Vtbl,
				.ref_count = 0,
				.next_free = &ActivatePool[i+1],
			};
		}
		ActivatePool[ARRAY_LEN(ActivatePool)-1].next_free = 0;


		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) result = FALSE;
	}

	return result;
}

HRESULT __stdcall
DllGetClassObject(REFCLSID clsid, REFIID riid, void** factory_handle)
{
	HRESULT result;
	*factory_handle = 0;

	if (!IsEqualCLSID(clsid, &CLSID_HOLOCAM)) result = CLASS_E_CLASSNOTAVAILABLE;
	else                                      result = ActivateFactoryVtbl.QueryInterface(&ActivateFactory, riid, factory_handle);
	
	return result;
}

HRESULT
DllCanUnloadNow()
{
	LOG_FUNCTION_ENTRY();

	bool can_unload = true;

	AcquireSRWLockExclusive(&MediaStreamPoolFreeListLock);
	can_unload = (can_unload && MediaStreamPoolOccupancy == 0);
	ReleaseSRWLockExclusive(&MediaStreamPoolFreeListLock);

	AcquireSRWLockExclusive(&MediaSourcePoolFreeListLock);
	can_unload = (can_unload && MediaSourcePoolOccupancy == 0);
	ReleaseSRWLockExclusive(&MediaSourcePoolFreeListLock);

	AcquireSRWLockExclusive(&ActivatePoolFreeListLock);
	can_unload = (can_unload && ActivatePoolOccupancy == 0);
	ReleaseSRWLockExclusive(&ActivatePoolFreeListLock);

	LOG_FUNCTION_RESULT(can_unload);

	return (can_unload ? S_OK : S_FALSE);
}

#define REGISTRY_PATH L"Software\\Classes\\CLSID\\" CLSID_HOLOCAM_STRING

// TODO: Clean up registry entries on failure
HRESULT
DllRegisterServer()
{
	HRESULT result = E_FAIL;

	WCHAR dll_path[2*MAX_PATH];
	u32 dll_path_len = GetModuleFileNameW((HMODULE)Module, dll_path, ARRAY_LEN(dll_path));

		HKEY key;
	if (dll_path_len == 0) result = E_FAIL;
	else if (SUCCEEDED(RegCreateKeyExW(HKEY_LOCAL_MACHINE, REGISTRY_PATH L"\\InProcServer32", 0, 0, 0, KEY_WRITE, 0, &key, 0)) &&
				   SUCCEEDED(RegSetValueExW(key, 0, 0, REG_SZ, (BYTE*)dll_path, (dll_path_len + 1)*sizeof(WCHAR)))                   &&
				   SUCCEEDED(RegSetValueExW(key, L"ThreadingModel", 0, REG_SZ, (BYTE*)L"Both", sizeof(L"Both"))))
	{
		result = S_OK;
	}

	return result;
}

HRESULT
DllUnregisterServer()
{
	return RegDeleteTreeW(HKEY_LOCAL_MACHINE, REGISTRY_PATH);
}
