typedef struct MediaTypeHandler MediaTypeHandler;
typedef struct MediaTypeHandlerVtbl
{
	// IUnknown
	HRESULT (*QueryInterface) (MediaTypeHandler* this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (MediaTypeHandler* this);
	ULONG   (*Release)        (MediaTypeHandler* this);

	// IMFMediaTypeHandler
	HRESULT (*IsMediaTypeSupported) (MediaTypeHandler* this, IMFMediaType* pMediaType, IMFMediaType** ppMediaType);
	HRESULT (*GetMediaTypeCount)    (MediaTypeHandler* this, DWORD* pdwTypeCount);
	HRESULT (*GetMediaTypeByIndex)  (MediaTypeHandler* this, DWORD dwIndex, IMFMediaType** ppType);
	HRESULT (*SetCurrentMediaType)  (MediaTypeHandler* this, IMFMediaType* pMediaType);
	HRESULT (*GetCurrentMediaType)  (MediaTypeHandler* this, IMFMediaType** ppMediaType);
	HRESULT (*GetMajorType)         (MediaTypeHandler* this, GUID* pguidMajorType);
} MediaTypeHandlerVtbl;

typedef struct MediaTypeHandler
{
	MediaTypeHandlerVtbl* lpVtbl;
	u32 ref_count;
	IMFMediaTypeHandler* handler;
} MediaTypeHandler;

HRESULT
MediaTypeHandler__QueryInterface(MediaTypeHandler* this, REFIID riid, void** handle)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IMFMediaTypeHandler)) result = E_NOINTERFACE;
		else
		{
			*handle = this;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

ULONG
MediaTypeHandler__AddRef(MediaTypeHandler* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedIncrement(&this->ref_count);

	return ref_count;
}

ULONG
MediaTypeHandler__Release(MediaTypeHandler* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedDecrement(&this->ref_count);

	if (ref_count == 0)
	{
		if (this->handler != 0)
		{
			IMFMediaTypeHandler_Release(this->handler);
			this->handler = 0;
		}

		CoTaskMemFree(this);
	}

	return ref_count;
}

HRESULT
MediaTypeHandler__IsMediaTypeSupported(MediaTypeHandler* this, IMFMediaType* pMediaType, IMFMediaType** ppMediaType)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFMediaTypeHandler_IsMediaTypeSupported(this->handler, pMediaType, ppMediaType);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaTypeHandler__GetMediaTypeCount(MediaTypeHandler* this, DWORD* pdwTypeCount)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFMediaTypeHandler_GetMediaTypeCount(this->handler, pdwTypeCount);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaTypeHandler__GetMediaTypeByIndex(MediaTypeHandler* this, DWORD dwIndex, IMFMediaType** ppType)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFMediaTypeHandler_GetMediaTypeByIndex(this->handler, dwIndex, ppType);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaTypeHandler__SetCurrentMediaType(MediaTypeHandler* this, IMFMediaType* pMediaType)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFMediaTypeHandler_SetCurrentMediaType(this->handler, pMediaType);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaTypeHandler__GetCurrentMediaType(MediaTypeHandler* this, IMFMediaType** ppMediaType)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFMediaTypeHandler_GetCurrentMediaType(this->handler, ppMediaType);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaTypeHandler__GetMajorType(MediaTypeHandler* this, GUID* pguidMajorType)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFMediaTypeHandler_GetMajorType(this->handler, pguidMajorType);

	LOG_FUNCTION_RESULT(result);
	return result;
}

MediaTypeHandlerVtbl MediaTypeHandler_Vtbl = {
	.QueryInterface       = MediaTypeHandler__QueryInterface,
	.AddRef               = MediaTypeHandler__AddRef,
	.Release              = MediaTypeHandler__Release,
	.IsMediaTypeSupported = MediaTypeHandler__IsMediaTypeSupported,
	.GetMediaTypeCount    = MediaTypeHandler__GetMediaTypeCount,
	.GetMediaTypeByIndex  = MediaTypeHandler__GetMediaTypeByIndex,
	.SetCurrentMediaType  = MediaTypeHandler__SetCurrentMediaType,
	.GetCurrentMediaType  = MediaTypeHandler__GetCurrentMediaType,
	.GetMajorType         = MediaTypeHandler__GetMajorType,
};

HRESULT
MediaTypeHandler__CreateInstance(IMFMediaTypeHandler* mf_handler, IMFMediaTypeHandler** out)
{
	HRESULT result = E_FAIL;

	MediaTypeHandler* handler = CoTaskMemAlloc(sizeof(MediaTypeHandler));
	if (handler != 0)
	{
		*handler = (MediaTypeHandler){
			.lpVtbl    = &MediaTypeHandler_Vtbl,
			.ref_count = 1,
			.handler   = mf_handler,
		};

		*out = (IMFMediaTypeHandler*)handler;

		result = S_OK;
	}

	return result;
}
