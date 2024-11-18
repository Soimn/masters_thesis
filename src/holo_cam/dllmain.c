#include "../common.h"

#define NOT_IMPLEMENTED

#include "media_source.h"
#include "activate.h"

HINSTANCE Module = 0;

BOOL
DllMain(HINSTANCE module, DWORD reason, LPVOID _)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
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
