#include "../common.h"

#define NOT_IMPLEMENTED

#include "media_source.h"
#include "activate.h"

HRESULT
DllGetClassObject(REFCLSID clsid, REFIID riid, void** factory_handle)
{
	HRESULT result;
	*factory_handle = 0;

	if (!IsEqualCLSID(clsid, &CLSID_IHoloCamActivate)) result = CLASS_E_CLASSNOTAVAILABLE;
	else                                       result = IHoloCamActivateFactory__QueryInterface(&HoloCamActivateFactory, riid, factory_handle);
	
	return result;
}
