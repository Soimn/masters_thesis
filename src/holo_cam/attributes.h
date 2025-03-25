typedef struct Attributes Attributes;
typedef struct AttributesVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (Attributes* this, REFIID riid, void** pp);
	ULONG   (*AddRef)         (Attributes* this);
	ULONG   (*Release)        (Attributes* this);

	// IMFAttributes
	HRESULT (*GetItem)            (Attributes* this, REFGUID guidKey, PROPVARIANT* pValue);
	HRESULT (*GetItemType)        (Attributes* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType);
	HRESULT (*CompareItem)        (Attributes* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult);
	HRESULT (*Compare)            (Attributes* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult);
	HRESULT (*GetUINT32)          (Attributes* this, REFGUID guidKey, UINT32* punValue);
	HRESULT (*GetUINT64)          (Attributes* this, REFGUID guidKey, UINT64* punValue);
	HRESULT (*GetDouble)          (Attributes* this, REFGUID guidKey, double* pfValue);
	HRESULT (*GetGUID)            (Attributes* this, REFGUID guidKey, GUID* pguidValue);
	HRESULT (*GetStringLength)    (Attributes* this, REFGUID guidKey, UINT32* pcchLength);
	HRESULT (*GetString)          (Attributes* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
	HRESULT (*GetAllocatedString) (Attributes* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength);
	HRESULT (*GetBlobSize)        (Attributes* this, REFGUID guidKey, UINT32* pcbBlobSize);
	HRESULT (*GetBlob)            (Attributes* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize);
	HRESULT (*GetAllocatedBlob)   (Attributes* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize);
	HRESULT (*GetUnknown)         (Attributes* this, REFGUID guidKey, REFIID riid, LPVOID* ppv);
	HRESULT (*SetItem)            (Attributes* this, REFGUID guidKey, REFPROPVARIANT Value);
	HRESULT (*DeleteItem)         (Attributes* this, REFGUID guid);
	HRESULT (*DeleteAllItems)     (Attributes* this);
	HRESULT (*SetUINT32)          (Attributes* this, REFGUID guidKey, UINT32  unValue);
	HRESULT (*SetUINT64)          (Attributes* this, REFGUID guidKey, UINT64  unValue);
	HRESULT (*SetDouble)          (Attributes* this, REFGUID guidKey, double  fValue);
	HRESULT (*SetGUID)            (Attributes* this, REFGUID guidKey, REFGUID guidValue);
	HRESULT (*SetString)          (Attributes* this, REFGUID guidKey, LPCWSTR wszValue);
	HRESULT (*SetBlob)            (Attributes* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize);
	HRESULT (*SetUnknown)         (Attributes* this, REFGUID guidKey, IUnknown* pUnknown);
	HRESULT (*LockStore)          (Attributes* this);
	HRESULT (*UnlockStore)        (Attributes* this);
	HRESULT (*GetCount)           (Attributes* this, UINT32* pcItems);
	HRESULT (*GetItemByIndex)     (Attributes* this, INT32 unIndex, GUID* pGuidKey, PROPVARIANT* pValue);
	HRESULT (*CopyAllItems)       (Attributes* this, IMFAttributes* pDest);
} AttributesVtbl;

typedef struct Attributes
{
	AttributesVtbl* lpVtbl;
	u32 ref_count;
	IMFAttributes* attributes;
} Attributes;

HRESULT
Attributes__QueryInterface(Attributes* this, REFIID riid, void** handle)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IMFAttributes)) result = E_NOINTERFACE;
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
Attributes__AddRef(Attributes* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedIncrement(&this->ref_count);

	return ref_count;
}

ULONG
Attributes__Release(Attributes* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedDecrement(&this->ref_count);

	if (ref_count == 0)
	{
		if (this->attributes != 0)
		{
			IMFAttributes_Release(this->attributes);
			this->attributes = 0;
		}

		CoTaskMemFree(this);
	}

	return ref_count;
}

HRESULT
Attributes__GetItem(Attributes* this, REFGUID guidKey, PROPVARIANT* pValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetItem guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetItem(this->attributes, guidKey, pValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetItemType(Attributes* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetItemType guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetItemType(this->attributes, guidKey, pType);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__CompareItem(Attributes* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__CompareItem guidKey is ", guidKey);
	HRESULT result = IMFAttributes_CompareItem(this->attributes, guidKey, Value, pbResult);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__Compare(Attributes* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFAttributes_Compare(this->attributes, pTheirs, MatchType, pbResult);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetUINT32(Attributes* this, REFGUID guidKey, UINT32* punValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetUINT32 guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetUINT32(this->attributes, guidKey, punValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetUINT64(Attributes* this, REFGUID guidKey, UINT64* punValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetUINT64 guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetUINT64(this->attributes, guidKey, punValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetDouble(Attributes* this, REFGUID guidKey, double* pfValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetDouble guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetDouble(this->attributes, guidKey, pfValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetGUID(Attributes* this, REFGUID guidKey, GUID* pguidValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetGUID guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetGUID(this->attributes, guidKey, pguidValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetStringLength(Attributes* this, REFGUID guidKey, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetStringLength guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetStringLength(this->attributes, guidKey, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetString(Attributes* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetString guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetString(this->attributes, guidKey, pwszValue, cchBufSize, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetAllocatedString(Attributes* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetAllocatedString guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetAllocatedString(this->attributes, guidKey, ppwszValue, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetBlobSize(Attributes* this, REFGUID guidKey, UINT32* pcbBlobSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetBlobSize guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetBlobSize(this->attributes, guidKey, pcbBlobSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetBlob(Attributes* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetBlob guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetBlob(this->attributes, guidKey, pBuf, cbBufSize, pcbBlobSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetAllocatedBlob(Attributes* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetAllocatedBlob guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetAllocatedBlob(this->attributes, guidKey, ppBuf, pcbSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetUnknown(Attributes* this, REFGUID guidKey, REFIID riid, LPVOID* ppv)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetUnknown guidKey is ", guidKey);
	HRESULT result = IMFAttributes_GetUnknown(this->attributes, guidKey, riid, ppv);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetItem(Attributes* this, REFGUID guidKey, REFPROPVARIANT Value)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetItem guidKey is ", guidKey);
	LogGUID("[Holo] --- Attributes__SetItem guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetItem(this->attributes, guidKey, Value);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__DeleteItem(Attributes* this, REFGUID guidKey)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__DeleteItem guidKey is ", guidKey);
	HRESULT result = IMFAttributes_DeleteItem(this->attributes, guidKey);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__DeleteAllItems(Attributes* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFAttributes_DeleteAllItems(this->attributes);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetUINT32(Attributes* this, REFGUID guidKey, UINT32  unValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetUINT32 guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetUINT32(this->attributes, guidKey, unValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetUINT64(Attributes* this, REFGUID guidKey, UINT64  unValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetUINT64 guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetUINT64(this->attributes, guidKey, unValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetDouble(Attributes* this, REFGUID guidKey, double  fValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetDouble guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetDouble(this->attributes, guidKey, fValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetGUID(Attributes* this, REFGUID guidKey, REFGUID guidValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetGUID guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetGUID(this->attributes, guidKey, guidValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetString(Attributes* this, REFGUID guidKey, LPCWSTR wszValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetString guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetString(this->attributes, guidKey, wszValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetBlob(Attributes* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetBlob guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetBlob(this->attributes, guidKey, pBuf, cbBufSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__SetUnknown(Attributes* this, REFGUID guidKey, IUnknown* pUnknown)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__SetUnknown guidKey is ", guidKey);
	LogGUID("[Holo] --- Attributes__SetUnknown guidKey is ", guidKey);
	HRESULT result = IMFAttributes_SetUnknown(this->attributes, guidKey, pUnknown);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__LockStore(Attributes* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFAttributes_LockStore(this->attributes);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__UnlockStore(Attributes* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFAttributes_UnlockStore(this->attributes);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetCount(Attributes* this, UINT32* pcItems)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFAttributes_GetCount(this->attributes, pcItems);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__GetItemByIndex(Attributes* this, INT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- Attributes__GetItemByIndex guidKey is ", pguidKey);
	HRESULT result = IMFAttributes_GetItemByIndex(this->attributes, unIndex, pguidKey, pValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
Attributes__CopyAllItems(Attributes* this, IMFAttributes* pDest)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFAttributes_CopyAllItems(this->attributes, pDest);
	LOG_FUNCTION_RESULT(result);
	return result;
}

AttributesVtbl Attributes_Vtbl = {
	// IUnknown
	.QueryInterface = Attributes__QueryInterface,
	.AddRef         = Attributes__AddRef,
	.Release        = Attributes__Release,

	// IMFAttributes
	.GetItem            = Attributes__GetItem,
	.GetItemType        = Attributes__GetItemType,
	.CompareItem        = Attributes__CompareItem,
	.Compare            = Attributes__Compare,
	.GetUINT32          = Attributes__GetUINT32,
	.GetUINT64          = Attributes__GetUINT64,
	.GetDouble          = Attributes__GetDouble,
	.GetGUID            = Attributes__GetGUID,
	.GetStringLength    = Attributes__GetStringLength,
	.GetString          = Attributes__GetString,
	.GetAllocatedString = Attributes__GetAllocatedString,
	.GetBlobSize        = Attributes__GetBlobSize,
	.GetBlob            = Attributes__GetBlob,
	.GetAllocatedBlob   = Attributes__GetAllocatedBlob,
	.GetUnknown         = Attributes__GetUnknown,
	.SetItem            = Attributes__SetItem,
	.DeleteItem         = Attributes__DeleteItem,
	.DeleteAllItems     = Attributes__DeleteAllItems,
	.SetUINT32          = Attributes__SetUINT32,
	.SetUINT64          = Attributes__SetUINT64,
	.SetDouble          = Attributes__SetDouble,
	.SetGUID            = Attributes__SetGUID,
	.SetString          = Attributes__SetString,
	.SetBlob            = Attributes__SetBlob,
	.SetUnknown         = Attributes__SetUnknown,
	.LockStore          = Attributes__LockStore,
	.UnlockStore        = Attributes__UnlockStore,
	.GetCount           = Attributes__GetCount,
	.GetItemByIndex     = Attributes__GetItemByIndex,
	.CopyAllItems       = Attributes__CopyAllItems,
};

HRESULT
Attributes__CreateInstance(IMFAttributes** out)
{
	HRESULT result = E_FAIL;

	if (out == 0) result = E_POINTER;
	else
	{
		*out = 0;

		Attributes* attrs = CoTaskMemAlloc(sizeof(Attributes));

		if (attrs != 0)
		{
			IMFAttributes* mf_attrs = 0;

			result = MFCreateAttributes(&mf_attrs, 0);
			if (!SUCCEEDED(result)) CoTaskMemFree(attrs);
			else
			{
				*attrs = (Attributes){
					.lpVtbl     = &Attributes_Vtbl,
					.ref_count  = 1,
					.attributes = mf_attrs,
				};

				*out = (IMFAttributes*)attrs;
			}
		}
	}

	return result;
}
