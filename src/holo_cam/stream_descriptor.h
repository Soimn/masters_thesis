typedef struct StreamDescriptor StreamDescriptor;
typedef struct StreamDescriptorVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (StreamDescriptor* this, REFIID riid, void** pp);
	ULONG   (*AddRef)         (StreamDescriptor* this);
	ULONG   (*Release)        (StreamDescriptor* this);

	// IMFAttributes
	HRESULT (*GetItem)            (StreamDescriptor* this, REFGUID guidKey, PROPVARIANT* pValue);
	HRESULT (*GetItemType)        (StreamDescriptor* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType);
	HRESULT (*CompareItem)        (StreamDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult);
	HRESULT (*Compare)            (StreamDescriptor* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult);
	HRESULT (*GetUINT32)          (StreamDescriptor* this, REFGUID guidKey, UINT32* punValue);
	HRESULT (*GetUINT64)          (StreamDescriptor* this, REFGUID guidKey, UINT64* punValue);
	HRESULT (*GetDouble)          (StreamDescriptor* this, REFGUID guidKey, double* pfValue);
	HRESULT (*GetGUID)            (StreamDescriptor* this, REFGUID guidKey, GUID* pguidValue);
	HRESULT (*GetStringLength)    (StreamDescriptor* this, REFGUID guidKey, UINT32* pcchLength);
	HRESULT (*GetString)          (StreamDescriptor* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
	HRESULT (*GetAllocatedString) (StreamDescriptor* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength);
	HRESULT (*GetBlobSize)        (StreamDescriptor* this, REFGUID guidKey, UINT32* pcbBlobSize);
	HRESULT (*GetBlob)            (StreamDescriptor* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize);
	HRESULT (*GetAllocatedBlob)   (StreamDescriptor* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize);
	HRESULT (*GetUnknown)         (StreamDescriptor* this, REFGUID guidKey, REFIID riid, LPVOID* ppv);
	HRESULT (*SetItem)            (StreamDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value);
	HRESULT (*DeleteItem)         (StreamDescriptor* this, REFGUID guid);
	HRESULT (*DeleteAllItems)     (StreamDescriptor* this);
	HRESULT (*SetUINT32)          (StreamDescriptor* this, REFGUID guidKey, UINT32  unValue);
	HRESULT (*SetUINT64)          (StreamDescriptor* this, REFGUID guidKey, UINT64  unValue);
	HRESULT (*SetDouble)          (StreamDescriptor* this, REFGUID guidKey, double  fValue);
	HRESULT (*SetGUID)            (StreamDescriptor* this, REFGUID guidKey, REFGUID guidValue);
	HRESULT (*SetString)          (StreamDescriptor* this, REFGUID guidKey, LPCWSTR wszValue);
	HRESULT (*SetBlob)            (StreamDescriptor* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize);
	HRESULT (*SetUnknown)         (StreamDescriptor* this, REFGUID guidKey, IUnknown* pUnknown);
	HRESULT (*LockStore)          (StreamDescriptor* this);
	HRESULT (*UnlockStore)        (StreamDescriptor* this);
	HRESULT (*GetCount)           (StreamDescriptor* this, UINT32* pcItems);
	HRESULT (*GetItemByIndex)     (StreamDescriptor* this, INT32 unIndex, GUID* pGuidKey, PROPVARIANT* pValue);
	HRESULT (*CopyAllItems)       (StreamDescriptor* this, IMFAttributes* pDest);

	// IMFStreamDescriptor
	HRESULT (*GetStreamIdentifier) (StreamDescriptor* this, DWORD* pdwStreamIdentifier);
	HRESULT (*GetMediaTypeHandler) (StreamDescriptor* this, IMFMediaTypeHandler** ppMediaTypeHandler);
} StreamDescriptorVtbl;

typedef struct StreamDescriptor
{
	StreamDescriptorVtbl* lpVtbl;
	u32 ref_count;
	IMFStreamDescriptor* desc;
} StreamDescriptor;

HRESULT
StreamDescriptor__QueryInterface(StreamDescriptor* this, REFIID riid, void** handle)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IMFAttributes) && !IsEqualIID(riid, &IID_IMFStreamDescriptor)) result = E_NOINTERFACE;
		else
		{
			*handle = this;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
	}

	return result;
}

ULONG
StreamDescriptor__AddRef(StreamDescriptor* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedIncrement(&this->ref_count);

	return ref_count;
}

ULONG
StreamDescriptor__Release(StreamDescriptor* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedDecrement(&this->ref_count);

	if (ref_count == 0)
	{
		if (this->desc != 0)
		{
			IMFStreamDescriptor_Release(this->desc);
			this->desc = 0;
		}

		CoTaskMemFree(this);
	}

	return ref_count;
}

HRESULT
StreamDescriptor__GetItem(StreamDescriptor* this, REFGUID guidKey, PROPVARIANT* pValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetItem guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetItem(this->desc, guidKey, pValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetItemType(StreamDescriptor* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetItemType guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetItemType(this->desc, guidKey, pType);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__CompareItem(StreamDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__CompareItem guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_CompareItem(this->desc, guidKey, Value, pbResult);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__Compare(StreamDescriptor* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFStreamDescriptor_Compare(this->desc, pTheirs, MatchType, pbResult);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetUINT32(StreamDescriptor* this, REFGUID guidKey, UINT32* punValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetUINT32 guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetUINT32(this->desc, guidKey, punValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetUINT64(StreamDescriptor* this, REFGUID guidKey, UINT64* punValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetUINT64 guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetUINT64(this->desc, guidKey, punValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetDouble(StreamDescriptor* this, REFGUID guidKey, double* pfValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetDouble guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetDouble(this->desc, guidKey, pfValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetGUID(StreamDescriptor* this, REFGUID guidKey, GUID* pguidValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetGUID guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetGUID(this->desc, guidKey, pguidValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetStringLength(StreamDescriptor* this, REFGUID guidKey, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetStringLength guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetStringLength(this->desc, guidKey, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetString(StreamDescriptor* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetString guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetString(this->desc, guidKey, pwszValue, cchBufSize, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetAllocatedString(StreamDescriptor* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetAllocatedString guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetAllocatedString(this->desc, guidKey, ppwszValue, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetBlobSize(StreamDescriptor* this, REFGUID guidKey, UINT32* pcbBlobSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetBlobSize guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetBlobSize(this->desc, guidKey, pcbBlobSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetBlob(StreamDescriptor* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetBlob guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetBlob(this->desc, guidKey, pBuf, cbBufSize, pcbBlobSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetAllocatedBlob(StreamDescriptor* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetAllocatedBlob guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetAllocatedBlob(this->desc, guidKey, ppBuf, pcbSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetUnknown(StreamDescriptor* this, REFGUID guidKey, REFIID riid, LPVOID* ppv)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetUnknown guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_GetUnknown(this->desc, guidKey, riid, ppv);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetItem(StreamDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetItem guidKey is ", guidKey);
	LogGUID("[Holo] --- StreamDescriptor__SetItem guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetItem(this->desc, guidKey, Value);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__DeleteItem(StreamDescriptor* this, REFGUID guidKey)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__DeleteItem guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_DeleteItem(this->desc, guidKey);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__DeleteAllItems(StreamDescriptor* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFStreamDescriptor_DeleteAllItems(this->desc);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetUINT32(StreamDescriptor* this, REFGUID guidKey, UINT32  unValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetUINT32 guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetUINT32(this->desc, guidKey, unValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetUINT64(StreamDescriptor* this, REFGUID guidKey, UINT64  unValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetUINT64 guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetUINT64(this->desc, guidKey, unValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetDouble(StreamDescriptor* this, REFGUID guidKey, double  fValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetDouble guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetDouble(this->desc, guidKey, fValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetGUID(StreamDescriptor* this, REFGUID guidKey, REFGUID guidValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetGUID guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetGUID(this->desc, guidKey, guidValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetString(StreamDescriptor* this, REFGUID guidKey, LPCWSTR wszValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetString guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetString(this->desc, guidKey, wszValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetBlob(StreamDescriptor* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetBlob guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetBlob(this->desc, guidKey, pBuf, cbBufSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__SetUnknown(StreamDescriptor* this, REFGUID guidKey, IUnknown* pUnknown)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__SetUnknown guidKey is ", guidKey);
	LogGUID("[Holo] --- StreamDescriptor__SetUnknown guidKey is ", guidKey);
	HRESULT result = IMFStreamDescriptor_SetUnknown(this->desc, guidKey, pUnknown);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__LockStore(StreamDescriptor* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFStreamDescriptor_LockStore(this->desc);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__UnlockStore(StreamDescriptor* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFStreamDescriptor_UnlockStore(this->desc);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetCount(StreamDescriptor* this, UINT32* pcItems)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFStreamDescriptor_GetCount(this->desc, pcItems);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetItemByIndex(StreamDescriptor* this, INT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- StreamDescriptor__GetItemByIndex guidKey is ", pguidKey);
	HRESULT result = IMFStreamDescriptor_GetItemByIndex(this->desc, unIndex, pguidKey, pValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__CopyAllItems(StreamDescriptor* this, IMFAttributes* pDest)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFStreamDescriptor_CopyAllItems(this->desc, pDest);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetStreamIdentifier(StreamDescriptor* this, DWORD* pdwStreamIdentifier)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFStreamDescriptor_GetStreamIdentifier(this->desc, pdwStreamIdentifier);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
StreamDescriptor__GetMediaTypeHandler(StreamDescriptor* this, IMFMediaTypeHandler** ppMediaTypeHandler)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (ppMediaTypeHandler == 0) result = E_POINTER;
	else
	{
		*ppMediaTypeHandler = 0;

		IMFMediaTypeHandler* handler = 0;
		result = IMFStreamDescriptor_GetMediaTypeHandler(this->desc, &handler);
		if (SUCCEEDED(result))
		{
			result = MediaTypeHandler__CreateInstance(handler, ppMediaTypeHandler);
		}

		if (!SUCCEEDED(result)) IMFMediaTypeHandler_Release(handler);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

StreamDescriptorVtbl StreamDescriptor_Vtbl = {
	// IUnknown
	.QueryInterface = StreamDescriptor__QueryInterface,
	.AddRef         = StreamDescriptor__AddRef,
	.Release        = StreamDescriptor__Release,

	// IMFAttributes
	.GetItem            = StreamDescriptor__GetItem,
	.GetItemType        = StreamDescriptor__GetItemType,
	.CompareItem        = StreamDescriptor__CompareItem,
	.Compare            = StreamDescriptor__Compare,
	.GetUINT32          = StreamDescriptor__GetUINT32,
	.GetUINT64          = StreamDescriptor__GetUINT64,
	.GetDouble          = StreamDescriptor__GetDouble,
	.GetGUID            = StreamDescriptor__GetGUID,
	.GetStringLength    = StreamDescriptor__GetStringLength,
	.GetString          = StreamDescriptor__GetString,
	.GetAllocatedString = StreamDescriptor__GetAllocatedString,
	.GetBlobSize        = StreamDescriptor__GetBlobSize,
	.GetBlob            = StreamDescriptor__GetBlob,
	.GetAllocatedBlob   = StreamDescriptor__GetAllocatedBlob,
	.GetUnknown         = StreamDescriptor__GetUnknown,
	.SetItem            = StreamDescriptor__SetItem,
	.DeleteItem         = StreamDescriptor__DeleteItem,
	.DeleteAllItems     = StreamDescriptor__DeleteAllItems,
	.SetUINT32          = StreamDescriptor__SetUINT32,
	.SetUINT64          = StreamDescriptor__SetUINT64,
	.SetDouble          = StreamDescriptor__SetDouble,
	.SetGUID            = StreamDescriptor__SetGUID,
	.SetString          = StreamDescriptor__SetString,
	.SetBlob            = StreamDescriptor__SetBlob,
	.SetUnknown         = StreamDescriptor__SetUnknown,
	.LockStore          = StreamDescriptor__LockStore,
	.UnlockStore        = StreamDescriptor__UnlockStore,
	.GetCount           = StreamDescriptor__GetCount,
	.GetItemByIndex     = StreamDescriptor__GetItemByIndex,
	.CopyAllItems       = StreamDescriptor__CopyAllItems,

	// IMFStreamDescriptor
	.GetStreamIdentifier = StreamDescriptor__GetStreamIdentifier,
	.GetMediaTypeHandler = StreamDescriptor__GetMediaTypeHandler,
};

HRESULT
StreamDescriptor__CreateInstance(DWORD stream_ident, DWORD media_types_len, IMFMediaType** media_types, IMFStreamDescriptor** out)
{
	HRESULT result = E_FAIL;

	if (out == 0) result = E_POINTER;
	else
	{
		*out = 0;

		StreamDescriptor* desc = CoTaskMemAlloc(sizeof(StreamDescriptor));

		if (desc != 0)
		{
			IMFStreamDescriptor* mf_desc = 0;

			result = MFCreateStreamDescriptor(stream_ident, media_types_len, media_types, &mf_desc);
			if (!SUCCEEDED(result)) CoTaskMemFree(desc);
			else
			{
				*desc = (StreamDescriptor){
					.lpVtbl    = &StreamDescriptor_Vtbl,
					.ref_count = 1,
					.desc      = mf_desc,
				};

				*out = (IMFStreamDescriptor*)desc;
			}
		}
	}

	return result;
}
