typedef struct IHoloCamActivate IHoloCamActivate;
typedef struct IHoloCamActivateVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (IHoloCamActivate* this, REFIID riid, void** pp);
	ULONG   (*AddRef)         (IHoloCamActivate* this);
	ULONG   (*Release)        (IHoloCamActivate* this);

	// IMFAttributes
	HRESULT (*GetItem)            (IHoloCamActivate* this, REFGUID guidKey, PROPVARIANT* pValue);
	HRESULT (*GetItemType)        (IHoloCamActivate* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType);
	HRESULT (*CompareItem)        (IHoloCamActivate* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult);
	HRESULT (*Compare)            (IHoloCamActivate* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult);
	HRESULT (*GetUINT32)          (IHoloCamActivate* this, REFGUID guidKey, UINT32* punValue);
	HRESULT (*GetUINT64)          (IHoloCamActivate* this, REFGUID guidKey, UINT64* punValue);
	HRESULT (*GetDouble)          (IHoloCamActivate* this, REFGUID guidKey, double* pfValue);
	HRESULT (*GetGUID)            (IHoloCamActivate* this, REFGUID guidKey, GUID* pguidValue);
	HRESULT (*GetStringLength)    (IHoloCamActivate* this, REFGUID guidKey, UINT32* pcchLength);
	HRESULT (*GetString)          (IHoloCamActivate* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
	HRESULT (*GetAllocatedString) (IHoloCamActivate* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength);
	HRESULT (*GetBlobSize)        (IHoloCamActivate* this, REFGUID guidKey, UINT32* pcbBlobSize);
	HRESULT (*GetBlob)            (IHoloCamActivate* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize);
	HRESULT (*GetAllocatedBlob)   (IHoloCamActivate* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize);
	HRESULT (*GetUnknown)         (IHoloCamActivate* this, REFGUID guidKey, REFIID riid, LPVOID* ppv);
	HRESULT (*SetItem)            (IHoloCamActivate* this, REFGUID guidKey, REFPROPVARIANT Value);
	HRESULT (*DeleteItem)         (IHoloCamActivate* this, REFGUID guid);
	HRESULT (*DeleteAllItems)     (IHoloCamActivate* this);
	HRESULT (*SetUINT32)          (IHoloCamActivate* this, REFGUID guidKey, UINT32  unValue);
	HRESULT (*SetUINT64)          (IHoloCamActivate* this, REFGUID guidKey, UINT64  unValue);
	HRESULT (*SetDouble)          (IHoloCamActivate* this, REFGUID guidKey, double  fValue);
	HRESULT (*SetGUID)            (IHoloCamActivate* this, REFGUID guidKey, REFGUID guidValue);
	HRESULT (*SetString)          (IHoloCamActivate* this, REFGUID guidKey, LPCWSTR wszValue);
	HRESULT (*SetBlob)            (IHoloCamActivate* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize);
	HRESULT (*SetUnknown)         (IHoloCamActivate* this, REFGUID guidKey, IUnknown* pUnknown);
	HRESULT (*LockStore)          (IHoloCamActivate* this);
	HRESULT (*UnlockStore)        (IHoloCamActivate* this);
	HRESULT (*GetCount)           (IHoloCamActivate* this, UINT32* pcItems);
	HRESULT (*GetItemByIndex)     (IHoloCamActivate* this, INT32 unIndex, GUID* pGuidKey, PROPVARIANT* pValue);
	HRESULT (*CopyAllItems)       (IHoloCamActivate* this, IMFAttributes* pDest);

	// IMFActivate
	HRESULT (*ActivateObject) (IHoloCamActivate* this, REFIID riid, void** ppv);
	HRESULT (*ShutdownObject) (IHoloCamActivate* this);
	HRESULT (*DetachObject)   (IHoloCamActivate* this);
} IHoloCamActivateVtbl;

typedef struct IHoloCamActivate
{
	IHoloCamActivateVtbl* lpVtbl;
	u32 ref_count;
	IMFAttributes* attributes;
	IHoloCamMediaSource* media_source;
} IHoloCamActivate;

HRESULT
IHoloCamActivate__QueryInterface(IHoloCamActivate* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IMFActivate) && !IsEqualIID(riid, &IID_IHoloCamActivate))
	{
		*handle = 0;
		result = E_NOINTERFACE;
	}
	else
	{
		*handle = this;
		this->lpVtbl->AddRef(this);
		result = S_OK;
	}

	return result;
}

ULONG
IHoloCamActivate__AddRef(IHoloCamActivate* this)
{
	this->ref_count += 1;
	return this->ref_count;
}

ULONG
IHoloCamActivate__Release(IHoloCamActivate* this)
{
	if (this->ref_count != 0) this->ref_count -= 1;
	return this->ref_count;
}

HRESULT
IHoloCamActivate__GetItem(IHoloCamActivate* this, REFGUID guidKey, PROPVARIANT* pValue)
{
	return IMFAttributes_GetItem(this->attributes, guidKey, pValue);
}

HRESULT
IHoloCamActivate__GetItemType(IHoloCamActivate* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType)
{
	return IMFAttributes_GetItemType(this->attributes, guidKey, pType);
}

HRESULT
IHoloCamActivate__CompareItem(IHoloCamActivate* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult)
{
	return IMFAttributes_CompareItem(this->attributes, guidKey, Value, pbResult);
}

HRESULT
IHoloCamActivate__Compare(IHoloCamActivate* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult)
{
	return IMFAttributes_Compare(this->attributes, pTheirs, MatchType, pbResult);
}

HRESULT
IHoloCamActivate__GetUINT32(IHoloCamActivate* this, REFGUID guidKey, UINT32* punValue)
{
	return IMFAttributes_GetUINT32(this->attributes, guidKey, punValue);
}

HRESULT
IHoloCamActivate__GetUINT64(IHoloCamActivate* this, REFGUID guidKey, UINT64* punValue)
{
	return IMFAttributes_GetUINT64(this->attributes, guidKey, punValue);
}

HRESULT
IHoloCamActivate__GetDouble(IHoloCamActivate* this, REFGUID guidKey, double* pfValue)
{
	return IMFAttributes_GetDouble(this->attributes, guidKey, pfValue);
}

HRESULT
IHoloCamActivate__GetGUID(IHoloCamActivate* this, REFGUID guidKey, GUID* pguidValue)
{
	return IMFAttributes_GetGUID(this->attributes, guidKey, pguidValue);
}

HRESULT
IHoloCamActivate__GetStringLength(IHoloCamActivate* this, REFGUID guidKey, UINT32* pcchLength)
{
	return IMFAttributes_GetStringLength(this->attributes, guidKey, pcchLength);
}

HRESULT
IHoloCamActivate__GetString(IHoloCamActivate* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
{
	return IMFAttributes_GetString(this->attributes, guidKey, pwszValue, cchBufSize, pcchLength);
}

HRESULT
IHoloCamActivate__GetAllocatedString(IHoloCamActivate* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength)
{
	return IMFAttributes_GetAllocatedString(this->attributes, guidKey, ppwszValue, pcchLength);
}

HRESULT
IHoloCamActivate__GetBlobSize(IHoloCamActivate* this, REFGUID guidKey, UINT32* pcbBlobSize)
{
	return IMFAttributes_GetBlobSize(this->attributes, guidKey, pcbBlobSize);
}

HRESULT
IHoloCamActivate__GetBlob(IHoloCamActivate* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize)
{
	return IMFAttributes_GetBlob(this->attributes, guidKey, pBuf, cbBufSize, pcbBlobSize);
}

HRESULT
IHoloCamActivate__GetAllocatedBlob(IHoloCamActivate* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize)
{
	return IMFAttributes_GetAllocatedBlob(this->attributes, guidKey, ppBuf, pcbSize);
}

HRESULT
IHoloCamActivate__GetUnknown(IHoloCamActivate* this, REFGUID guidKey, REFIID riid, LPVOID* ppv)
{
	return IMFAttributes_GetUnknown(this->attributes, guidKey, riid, ppv);
}

HRESULT
IHoloCamActivate__SetItem(IHoloCamActivate* this, REFGUID guidKey, REFPROPVARIANT Value)
{
	return IMFAttributes_SetItem(this->attributes, guidKey, Value);
}

HRESULT
IHoloCamActivate__DeleteItem(IHoloCamActivate* this, REFGUID guidKey)
{
	return IMFAttributes_DeleteItem(this->attributes, guidKey);
}

HRESULT
IHoloCamActivate__DeleteAllItems(IHoloCamActivate* this)
{
	return IMFAttributes_DeleteAllItems(this->attributes);
}

HRESULT
IHoloCamActivate__SetUINT32(IHoloCamActivate* this, REFGUID guidKey, UINT32  unValue)
{
	return IMFAttributes_SetUINT32(this->attributes, guidKey, unValue);
}

HRESULT
IHoloCamActivate__SetUINT64(IHoloCamActivate* this, REFGUID guidKey, UINT64  unValue)
{
	return IMFAttributes_SetUINT64(this->attributes, guidKey, unValue);
}

HRESULT
IHoloCamActivate__SetDouble(IHoloCamActivate* this, REFGUID guidKey, double  fValue)
{
	return IMFAttributes_SetDouble(this->attributes, guidKey, fValue);
}

HRESULT
IHoloCamActivate__SetGUID(IHoloCamActivate* this, REFGUID guidKey, REFGUID guidValue)
{
	return IMFAttributes_SetGUID(this->attributes, guidKey, guidValue);
}

HRESULT
IHoloCamActivate__SetString(IHoloCamActivate* this, REFGUID guidKey, LPCWSTR wszValue)
{
	return IMFAttributes_SetString(this->attributes, guidKey, wszValue);
}

HRESULT
IHoloCamActivate__SetBlob(IHoloCamActivate* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize)
{
	return IMFAttributes_SetBlob(this->attributes, guidKey, pBuf, cbBufSize);
}

HRESULT
IHoloCamActivate__SetUnknown(IHoloCamActivate* this, REFGUID guidKey, IUnknown* pUnknown)
{
	return IMFAttributes_SetUnknown(this->attributes, guidKey, pUnknown);
}

HRESULT
IHoloCamActivate__LockStore(IHoloCamActivate* this)
{
	return IMFAttributes_LockStore(this->attributes);
}

HRESULT
IHoloCamActivate__UnlockStore(IHoloCamActivate* this)
{
	return IMFAttributes_UnlockStore(this->attributes);
}

HRESULT
IHoloCamActivate__GetCount(IHoloCamActivate* this, UINT32* pcItems)
{
	return IMFAttributes_GetCount(this->attributes, pcItems);
}

HRESULT
IHoloCamActivate__GetItemByIndex(IHoloCamActivate* this, INT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue)
{
	return IMFAttributes_GetItemByIndex(this->attributes, unIndex, pguidKey, pValue);
}

HRESULT
IHoloCamActivate__CopyAllItems(IHoloCamActivate* this, IMFAttributes* pDest)
{
	return IMFAttributes_CopyAllItems(this->attributes, pDest);
}

HRESULT
IHoloCamActivate__ActivateObject(IHoloCamActivate* this, REFIID riid, void** ppv)
{
	HRESULT result;

	if (ppv == 0) result = E_POINTER;
	else
	{
		*ppv = 0;

		// TODO
		if      (!SUCCEEDED(IHoloCamMediaSourceFactoryVtbl.CreateInstance(&HoloCamMediaSourceFactory, 0, &IID_IHoloCamMediaSource, &this->media_source))) result = E_FAIL;
		else if (!SUCCEEDED(IHoloCamMediaSource__Init(this->media_source, this->attributes)))                                                             result = E_FAIL;
		else
		{
			*ppv = this->media_source;
			result = S_OK;
		}
	}

	return result;
}

HRESULT
IHoloCamActivate__ShutdownObject(IHoloCamActivate* this)
{
	return S_OK;
}

HRESULT
IHoloCamActivate__DetachObject(IHoloCamActivate* this)
{
	this->media_source = 0;
	return S_OK;
}

IHoloCamActivateVtbl IHoloCamActivate_Vtbl = {
	// IUnknown
	.QueryInterface = IHoloCamActivate__QueryInterface,
	.AddRef         = IHoloCamActivate__AddRef,
	.Release        = IHoloCamActivate__Release,

	// IMFAttributes
	.GetItem            = IHoloCamActivate__GetItem,
	.GetItemType        = IHoloCamActivate__GetItemType,
	.CompareItem        = IHoloCamActivate__CompareItem,
	.Compare            = IHoloCamActivate__Compare,
	.GetUINT32          = IHoloCamActivate__GetUINT32,
	.GetUINT64          = IHoloCamActivate__GetUINT64,
	.GetDouble          = IHoloCamActivate__GetDouble,
	.GetGUID            = IHoloCamActivate__GetGUID,
	.GetStringLength    = IHoloCamActivate__GetStringLength,
	.GetString          = IHoloCamActivate__GetString,
	.GetAllocatedString = IHoloCamActivate__GetAllocatedString,
	.GetBlobSize        = IHoloCamActivate__GetBlobSize,
	.GetBlob            = IHoloCamActivate__GetBlob,
	.GetAllocatedBlob   = IHoloCamActivate__GetAllocatedBlob,
	.GetUnknown         = IHoloCamActivate__GetUnknown,
	.SetItem            = IHoloCamActivate__SetItem,
	.DeleteItem         = IHoloCamActivate__DeleteItem,
	.DeleteAllItems     = IHoloCamActivate__DeleteAllItems,
	.SetUINT32          = IHoloCamActivate__SetUINT32,
	.SetUINT64          = IHoloCamActivate__SetUINT64,
	.SetDouble          = IHoloCamActivate__SetDouble,
	.SetGUID            = IHoloCamActivate__SetGUID,
	.SetString          = IHoloCamActivate__SetString,
	.SetBlob            = IHoloCamActivate__SetBlob,
	.SetUnknown         = IHoloCamActivate__SetUnknown,
	.LockStore          = IHoloCamActivate__LockStore,
	.UnlockStore        = IHoloCamActivate__UnlockStore,
	.GetCount           = IHoloCamActivate__GetCount,
	.GetItemByIndex     = IHoloCamActivate__GetItemByIndex,
	.CopyAllItems       = IHoloCamActivate__CopyAllItems,

	// IMFActivate
	.ActivateObject = IHoloCamActivate__ActivateObject,
	.ShutdownObject = IHoloCamActivate__ShutdownObject,
	.DetachObject   = IHoloCamActivate__DetachObject,
};

HRESULT
IHoloCamActivate__Init(IHoloCamActivate* this)
{
	HRESULT result;

	if (this->ref_count != 1) result = E_FAIL;
	else
	{
		*this = (IHoloCamActivate){
			.lpVtbl    = &IHoloCamActivate_Vtbl,
			.ref_count = 1,
		};

		result = MFCreateAttributes(&this->attributes, 1);
	}

	return result;
}

HRESULT
IHoloCamActivateFactory__QueryInterface(IClassFactory* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IClassFactory) && !IsEqualIID(riid, &IID_IHoloCamActivateFactory))
	{
		*handle = 0;
		result  = E_NOINTERFACE;
	}
	else
	{
		*handle = this;
		this->lpVtbl->AddRef(this);
		result = S_OK;
	}

	return result;
}

HRESULT
IHoloCamActivateFactory__AddRef(IClassFactory* this)
{
	return 1;
}

HRESULT
IHoloCamActivateFactory__Release(IClassFactory* this)
{
	return 1;
}

IHoloCamActivate VirtualCameras[HOLO_MAX_CAMERA_COUNT];

HRESULT
IHoloCamActivateFactory__CreateInstance(IClassFactory* this, IUnknown* outer, REFIID id, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	{
		*handle = 0;

		if (outer != 0) result = CLASS_E_NOAGGREGATION;
		else
		{
			u32 i = 0;
			for (; i < HOLO_MAX_CAMERA_COUNT; ++i)
			{
				if (InterlockedCompareExchange(&VirtualCameras[i].ref_count, 1, 0) == 0) break;
			}

			if (i >= HOLO_MAX_CAMERA_COUNT) result = E_OUTOFMEMORY;
			else
			{
				IHoloCamActivate* cam = &VirtualCameras[i];
				if (!SUCCEEDED(IHoloCamActivate__Init(cam))) result = E_FAIL;
				else
				{
					result = cam->lpVtbl->QueryInterface(cam, id, handle);
					cam->lpVtbl->Release(cam);
				}
			}
		}
	}

	return result;
}

HRESULT
IHoloCamActivateFactory__LockServer(IClassFactory* this, BOOL flock)
{
	// TODO:
	return E_NOTIMPL;
}

static IClassFactoryVtbl IHoloCamActivateFactoryVtbl = {
	.QueryInterface = IHoloCamActivateFactory__QueryInterface,
	.AddRef         = IHoloCamActivateFactory__AddRef,
	.Release        = IHoloCamActivateFactory__Release,
	.CreateInstance = IHoloCamActivateFactory__CreateInstance,
	.LockServer     = IHoloCamActivateFactory__LockServer,
};

static IClassFactory HoloCamActivateFactory = { &IHoloCamActivateFactoryVtbl };
