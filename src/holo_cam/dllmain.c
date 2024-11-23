#include "../common.h"

#define NOT_IMPLEMENTED

#define COBJMACROS
#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#include <ks.h>

// NOTE: ksproxy.h for C is broken, this (definition from devicetopology.h) needs to be defined first to override the broken definition
#define _IKsControl_
DEFINE_GUID(IID_IKsControl, 0x28F54685L, 0x06FD, 0x11D2, 0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);
typedef struct IKsControl IKsControl;
typedef struct IKsControlVtbl
{
		HRESULT (*QueryInterface) (IKsControl* This, REFIID riid, void** ppvObject);
		ULONG   (*AddRef)         (IKsControl* This);
		ULONG   (*Release)        (IKsControl* This);
		HRESULT (*KsProperty)     (IKsControl* This, PKSPROPERTY Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned);
		HRESULT (*KsMethod)       (IKsControl* This, PKSMETHOD Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned);
		HRESULT (*KsEvent)        (IKsControl* This, PKSEVENT Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned);
} IKsControlVtbl;

struct IKsControl
{
	IKsControlVtbl* lpVtbl;
};

#include <ksproxy.h>
#include <ksguid.h>
#include <ksmedia.h>
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

#include "callback.h"
#include "media_stream.h"
#include "media_source.h"
#include "activate.h"

HINSTANCE Module = 0;

BOOL
DllMain(HINSTANCE module, DWORD reason, LPVOID _)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		for (u32 i = 0; i < ARRAY_LEN(HoloCamMediaSourcePool); ++i)
		{
			HoloCamMediaSourcePool[i] = (IHoloCamMediaSource){
				.ref_count = 0,
				.lock      = SRWLOCK_INIT,
			};
		}

		Module = module;
		DisableThreadLibraryCalls(module);
	}

	return TRUE;
}

HRESULT __stdcall
DllGetClassObject(REFCLSID clsid, REFIID riid, void** factory_handle)
{
	HRESULT result;
	*factory_handle = 0;

	OutputDebugStringA("DllGetClassObject\n");
	if (!IsEqualCLSID(clsid, &CLSID_IHoloCamActivate)) result = CLASS_E_CLASSNOTAVAILABLE;
	else                                               result = IHoloCamActivateFactoryVtbl.QueryInterface(&HoloCamActivateFactory, riid, factory_handle);
	
	return result;
}

HRESULT
DllCanUnloadNow()
{
	// TODO
	return S_OK;
}

HRESULT
DllRegisterServer()
{
	HRESULT result = E_FAIL;

	WCHAR dll_path[2*MAX_PATH];
	u32 dll_path_len = GetModuleFileNameW((HMODULE)Module, dll_path, ARRAY_LEN(dll_path));

	if (dll_path_len != 0)
	{
		HKEY key;
		if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\" CLSID_IHOLOCAMACTIVATE_STRING L"\\InProcServer32", 0, 0, 0, KEY_WRITE, 0, &key, 0) == ERROR_SUCCESS &&
				RegSetValueExW(key, 0, 0, REG_SZ, (BYTE*)dll_path, (dll_path_len + 1)*sizeof(WCHAR)) == ERROR_SUCCESS &&
				RegSetValueExW(key, L"ThreadingModel", 0, REG_SZ, (BYTE*)L"Both", sizeof(L"Both")) == ERROR_SUCCESS)
		{
			result = S_OK;
		}
	}

	return result;
}

HRESULT
DllUnregisterServer()
{
	return (RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\" CLSID_IHOLOCAMACTIVATE_STRING) == ERROR_SUCCESS ? S_OK : E_FAIL);
}
