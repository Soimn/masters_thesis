typedef struct PresentationDescriptor PresentationDescriptor;
typedef struct PresentationDescriptorVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (PresentationDescriptor* this, REFIID riid, void** pp);
	ULONG   (*AddRef)         (PresentationDescriptor* this);
	ULONG   (*Release)        (PresentationDescriptor* this);

	// IMFAttributes
	HRESULT (*GetItem)            (PresentationDescriptor* this, REFGUID guidKey, PROPVARIANT* pValue);
	HRESULT (*GetItemType)        (PresentationDescriptor* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType);
	HRESULT (*CompareItem)        (PresentationDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult);
	HRESULT (*Compare)            (PresentationDescriptor* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult);
	HRESULT (*GetUINT32)          (PresentationDescriptor* this, REFGUID guidKey, UINT32* punValue);
	HRESULT (*GetUINT64)          (PresentationDescriptor* this, REFGUID guidKey, UINT64* punValue);
	HRESULT (*GetDouble)          (PresentationDescriptor* this, REFGUID guidKey, double* pfValue);
	HRESULT (*GetGUID)            (PresentationDescriptor* this, REFGUID guidKey, GUID* pguidValue);
	HRESULT (*GetStringLength)    (PresentationDescriptor* this, REFGUID guidKey, UINT32* pcchLength);
	HRESULT (*GetString)          (PresentationDescriptor* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
	HRESULT (*GetAllocatedString) (PresentationDescriptor* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength);
	HRESULT (*GetBlobSize)        (PresentationDescriptor* this, REFGUID guidKey, UINT32* pcbBlobSize);
	HRESULT (*GetBlob)            (PresentationDescriptor* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize);
	HRESULT (*GetAllocatedBlob)   (PresentationDescriptor* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize);
	HRESULT (*GetUnknown)         (PresentationDescriptor* this, REFGUID guidKey, REFIID riid, LPVOID* ppv);
	HRESULT (*SetItem)            (PresentationDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value);
	HRESULT (*DeleteItem)         (PresentationDescriptor* this, REFGUID guid);
	HRESULT (*DeleteAllItems)     (PresentationDescriptor* this);
	HRESULT (*SetUINT32)          (PresentationDescriptor* this, REFGUID guidKey, UINT32  unValue);
	HRESULT (*SetUINT64)          (PresentationDescriptor* this, REFGUID guidKey, UINT64  unValue);
	HRESULT (*SetDouble)          (PresentationDescriptor* this, REFGUID guidKey, double  fValue);
	HRESULT (*SetGUID)            (PresentationDescriptor* this, REFGUID guidKey, REFGUID guidValue);
	HRESULT (*SetString)          (PresentationDescriptor* this, REFGUID guidKey, LPCWSTR wszValue);
	HRESULT (*SetBlob)            (PresentationDescriptor* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize);
	HRESULT (*SetUnknown)         (PresentationDescriptor* this, REFGUID guidKey, IUnknown* pUnknown);
	HRESULT (*LockStore)          (PresentationDescriptor* this);
	HRESULT (*UnlockStore)        (PresentationDescriptor* this);
	HRESULT (*GetCount)           (PresentationDescriptor* this, UINT32* pcItems);
	HRESULT (*GetItemByIndex)     (PresentationDescriptor* this, INT32 unIndex, GUID* pGuidKey, PROPVARIANT* pValue);
	HRESULT (*CopyAllItems)       (PresentationDescriptor* this, IMFAttributes* pDest);

	// IMFPresentationDescriptor
	HRESULT (*GetStreamDescriptorCount)   (PresentationDescriptor* this, DWORD* pdwDescriptorCount);
	HRESULT (*GetStreamDescriptorByIndex) (PresentationDescriptor* this, DWORD dwIndex, BOOL* pfSelected, IMFStreamDescriptor** ppDescriptor);
	HRESULT (*SelectStream)               (PresentationDescriptor* this, DWORD dwDescriptorIndex);
	HRESULT (*DeselectStream)             (PresentationDescriptor* this, DWORD dwDescriptorIndex);
	HRESULT (*Clone)                      (PresentationDescriptor* this, IMFPresentationDescriptor** ppPresentationDescriptor);
} PresentationDescriptorVtbl;

typedef struct PresentationDescriptor
{
	PresentationDescriptorVtbl* lpVtbl;
	u32 ref_count;
	IMFPresentationDescriptor* desc;
} PresentationDescriptor;

HRESULT
PresentationDescriptor__QueryInterface(PresentationDescriptor* this, REFIID riid, void** handle)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IMFAttributes) && !IsEqualIID(riid, &IID_IMFPresentationDescriptor)) result = E_NOINTERFACE;
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
PresentationDescriptor__AddRef(PresentationDescriptor* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedIncrement(&this->ref_count);

	return ref_count;
}

ULONG
PresentationDescriptor__Release(PresentationDescriptor* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedDecrement(&this->ref_count);

	if (ref_count == 0)
	{
		if (this->desc != 0)
		{
			IMFPresentationDescriptor_Release(this->desc);
			this->desc = 0;
		}

		CoTaskMemFree(this);
	}

	return ref_count;
}

HRESULT
PresentationDescriptor__GetItem(PresentationDescriptor* this, REFGUID guidKey, PROPVARIANT* pValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetItem guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetItem(this->desc, guidKey, pValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetItemType(PresentationDescriptor* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetItemType guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetItemType(this->desc, guidKey, pType);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__CompareItem(PresentationDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__CompareItem guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_CompareItem(this->desc, guidKey, Value, pbResult);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__Compare(PresentationDescriptor* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFPresentationDescriptor_Compare(this->desc, pTheirs, MatchType, pbResult);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetUINT32(PresentationDescriptor* this, REFGUID guidKey, UINT32* punValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetUINT32 guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetUINT32(this->desc, guidKey, punValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetUINT64(PresentationDescriptor* this, REFGUID guidKey, UINT64* punValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetUINT64 guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetUINT64(this->desc, guidKey, punValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetDouble(PresentationDescriptor* this, REFGUID guidKey, double* pfValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetDouble guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetDouble(this->desc, guidKey, pfValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetGUID(PresentationDescriptor* this, REFGUID guidKey, GUID* pguidValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetGUID guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetGUID(this->desc, guidKey, pguidValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetStringLength(PresentationDescriptor* this, REFGUID guidKey, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetStringLength guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetStringLength(this->desc, guidKey, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetString(PresentationDescriptor* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetString guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetString(this->desc, guidKey, pwszValue, cchBufSize, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetAllocatedString(PresentationDescriptor* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetAllocatedString guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetAllocatedString(this->desc, guidKey, ppwszValue, pcchLength);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetBlobSize(PresentationDescriptor* this, REFGUID guidKey, UINT32* pcbBlobSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetBlobSize guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetBlobSize(this->desc, guidKey, pcbBlobSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetBlob(PresentationDescriptor* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetBlob guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetBlob(this->desc, guidKey, pBuf, cbBufSize, pcbBlobSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetAllocatedBlob(PresentationDescriptor* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetAllocatedBlob guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetAllocatedBlob(this->desc, guidKey, ppBuf, pcbSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetUnknown(PresentationDescriptor* this, REFGUID guidKey, REFIID riid, LPVOID* ppv)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetUnknown guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_GetUnknown(this->desc, guidKey, riid, ppv);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetItem(PresentationDescriptor* this, REFGUID guidKey, REFPROPVARIANT Value)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetItem guidKey is ", guidKey);
	LogGUID("[Holo] --- PresentationDescriptor__SetItem guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetItem(this->desc, guidKey, Value);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__DeleteItem(PresentationDescriptor* this, REFGUID guidKey)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__DeleteItem guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_DeleteItem(this->desc, guidKey);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__DeleteAllItems(PresentationDescriptor* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFPresentationDescriptor_DeleteAllItems(this->desc);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetUINT32(PresentationDescriptor* this, REFGUID guidKey, UINT32  unValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetUINT32 guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetUINT32(this->desc, guidKey, unValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetUINT64(PresentationDescriptor* this, REFGUID guidKey, UINT64  unValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetUINT64 guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetUINT64(this->desc, guidKey, unValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetDouble(PresentationDescriptor* this, REFGUID guidKey, double  fValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetDouble guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetDouble(this->desc, guidKey, fValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetGUID(PresentationDescriptor* this, REFGUID guidKey, REFGUID guidValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetGUID guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetGUID(this->desc, guidKey, guidValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetString(PresentationDescriptor* this, REFGUID guidKey, LPCWSTR wszValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetString guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetString(this->desc, guidKey, wszValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetBlob(PresentationDescriptor* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetBlob guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetBlob(this->desc, guidKey, pBuf, cbBufSize);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SetUnknown(PresentationDescriptor* this, REFGUID guidKey, IUnknown* pUnknown)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__SetUnknown guidKey is ", guidKey);
	LogGUID("[Holo] --- PresentationDescriptor__SetUnknown guidKey is ", guidKey);
	HRESULT result = IMFPresentationDescriptor_SetUnknown(this->desc, guidKey, pUnknown);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__LockStore(PresentationDescriptor* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFPresentationDescriptor_LockStore(this->desc);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__UnlockStore(PresentationDescriptor* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFPresentationDescriptor_UnlockStore(this->desc);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetCount(PresentationDescriptor* this, UINT32* pcItems)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFPresentationDescriptor_GetCount(this->desc, pcItems);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetItemByIndex(PresentationDescriptor* this, INT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue)
{
	LOG_FUNCTION_ENTRY();
	LogGUID("[HOLOO] --- PresentationDescriptor__GetItemByIndex guidKey is ", pguidKey);
	HRESULT result = IMFPresentationDescriptor_GetItemByIndex(this->desc, unIndex, pguidKey, pValue);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__CopyAllItems(PresentationDescriptor* this, IMFAttributes* pDest)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = IMFPresentationDescriptor_CopyAllItems(this->desc, pDest);
	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetStreamDescriptorCount(PresentationDescriptor* this, DWORD* pdwDescriptorCount)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFPresentationDescriptor_GetStreamDescriptorCount(this->desc, pdwDescriptorCount);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__GetStreamDescriptorByIndex(PresentationDescriptor* this, DWORD dwIndex, BOOL* pfSelected, IMFStreamDescriptor** ppDescriptor)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFPresentationDescriptor_GetStreamDescriptorByIndex(this->desc, dwIndex, pfSelected, ppDescriptor);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__SelectStream(PresentationDescriptor* this, DWORD dwDescriptorIndex)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFPresentationDescriptor_SelectStream(this->desc, dwDescriptorIndex);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
PresentationDescriptor__DeselectStream(PresentationDescriptor* this, DWORD dwDescriptorIndex)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	result = IMFPresentationDescriptor_DeselectStream(this->desc, dwDescriptorIndex);

	LOG_FUNCTION_RESULT(result);
	return result;
}

void PresentationDescriptor__Init(PresentationDescriptor* desc, IMFPresentationDescriptor* mf_desc);

HRESULT
PresentationDescriptor__Clone(PresentationDescriptor* this, IMFPresentationDescriptor** ppPresentationDescriptor)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (ppPresentationDescriptor == 0) result = E_POINTER;
	else
	{
		*ppPresentationDescriptor = 0;

		PresentationDescriptor* desc = CoTaskMemAlloc(sizeof(PresentationDescriptor));

		IMFPresentationDescriptor* clone = 0;
		result = IMFPresentationDescriptor_Clone(this->desc, &clone);

		if (desc != 0 && SUCCEEDED(result))
		{
			PresentationDescriptor__Init(desc, clone);

			*ppPresentationDescriptor = (IMFPresentationDescriptor*)desc;
		}

		if (!SUCCEEDED(result))
		{
			if (desc != 0) CoTaskMemFree(desc);
			if (clone != 0) IMFPresentationDescriptor_Release(clone);
		}
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

PresentationDescriptorVtbl PresentationDescriptor_Vtbl = {
	// IUnknown
	.QueryInterface = PresentationDescriptor__QueryInterface,
	.AddRef         = PresentationDescriptor__AddRef,
	.Release        = PresentationDescriptor__Release,

	// IMFAttributes
	.GetItem            = PresentationDescriptor__GetItem,
	.GetItemType        = PresentationDescriptor__GetItemType,
	.CompareItem        = PresentationDescriptor__CompareItem,
	.Compare            = PresentationDescriptor__Compare,
	.GetUINT32          = PresentationDescriptor__GetUINT32,
	.GetUINT64          = PresentationDescriptor__GetUINT64,
	.GetDouble          = PresentationDescriptor__GetDouble,
	.GetGUID            = PresentationDescriptor__GetGUID,
	.GetStringLength    = PresentationDescriptor__GetStringLength,
	.GetString          = PresentationDescriptor__GetString,
	.GetAllocatedString = PresentationDescriptor__GetAllocatedString,
	.GetBlobSize        = PresentationDescriptor__GetBlobSize,
	.GetBlob            = PresentationDescriptor__GetBlob,
	.GetAllocatedBlob   = PresentationDescriptor__GetAllocatedBlob,
	.GetUnknown         = PresentationDescriptor__GetUnknown,
	.SetItem            = PresentationDescriptor__SetItem,
	.DeleteItem         = PresentationDescriptor__DeleteItem,
	.DeleteAllItems     = PresentationDescriptor__DeleteAllItems,
	.SetUINT32          = PresentationDescriptor__SetUINT32,
	.SetUINT64          = PresentationDescriptor__SetUINT64,
	.SetDouble          = PresentationDescriptor__SetDouble,
	.SetGUID            = PresentationDescriptor__SetGUID,
	.SetString          = PresentationDescriptor__SetString,
	.SetBlob            = PresentationDescriptor__SetBlob,
	.SetUnknown         = PresentationDescriptor__SetUnknown,
	.LockStore          = PresentationDescriptor__LockStore,
	.UnlockStore        = PresentationDescriptor__UnlockStore,
	.GetCount           = PresentationDescriptor__GetCount,
	.GetItemByIndex     = PresentationDescriptor__GetItemByIndex,
	.CopyAllItems       = PresentationDescriptor__CopyAllItems,

	// IMFPresentationDescriptor
	.GetStreamDescriptorCount   = PresentationDescriptor__GetStreamDescriptorCount,
	.GetStreamDescriptorByIndex = PresentationDescriptor__GetStreamDescriptorByIndex,
	.SelectStream               = PresentationDescriptor__SelectStream,
	.DeselectStream             = PresentationDescriptor__DeselectStream,
	.Clone                      = PresentationDescriptor__Clone,
};

HRESULT
PresentationDescriptor__CreateInstance(u32 stream_descs_len, IMFStreamDescriptor** stream_descs, IMFPresentationDescriptor** out)
{
	HRESULT result = E_FAIL;

	if (out == 0) result = E_POINTER;
	else
	{
		*out = 0;

		PresentationDescriptor* desc = CoTaskMemAlloc(sizeof(PresentationDescriptor));

		if (desc != 0)
		{
			IMFPresentationDescriptor* mf_desc = 0;

			result = MFCreatePresentationDescriptor(stream_descs_len, stream_descs, &mf_desc);
			if (!SUCCEEDED(result)) CoTaskMemFree(desc);
			else
			{
				PresentationDescriptor__Init(desc, mf_desc);

				*out = (IMFPresentationDescriptor*)desc;
			}
		}
	}

	return result;
}

void
PresentationDescriptor__Init(PresentationDescriptor* desc, IMFPresentationDescriptor* mf_desc)
{
	*desc = (PresentationDescriptor){
		.lpVtbl    = &PresentationDescriptor_Vtbl,
		.ref_count = 1,
		.desc      = mf_desc,
	};
}
