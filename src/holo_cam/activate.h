// {F01E07F0-A701-42EF-BB88-953281685411}
DEFINE_GUID(IID_Activate, 0xf01e07f0, 0xa701, 0x42ef, 0xbb, 0x88, 0x95, 0x32, 0x81, 0x68, 0x54, 0x11);

// {03DBA7DB-F24C-4379-AA8F-79117D1AC20E}
DEFINE_GUID(IID_ActivateFactory, 0x3dba7db, 0xf24c, 0x4379, 0xaa, 0x8f, 0x79, 0x11, 0x7d, 0x1a, 0xc2, 0xe);

typedef struct Activate Activate;
typedef struct ActivateVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (Activate* this, REFIID riid, void** pp);
	ULONG   (*AddRef)         (Activate* this);
	ULONG   (*Release)        (Activate* this);

	// IMFAttributes
	HRESULT (*GetItem)            (Activate* this, REFGUID guidKey, PROPVARIANT* pValue);
	HRESULT (*GetItemType)        (Activate* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType);
	HRESULT (*CompareItem)        (Activate* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult);
	HRESULT (*Compare)            (Activate* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult);
	HRESULT (*GetUINT32)          (Activate* this, REFGUID guidKey, UINT32* punValue);
	HRESULT (*GetUINT64)          (Activate* this, REFGUID guidKey, UINT64* punValue);
	HRESULT (*GetDouble)          (Activate* this, REFGUID guidKey, double* pfValue);
	HRESULT (*GetGUID)            (Activate* this, REFGUID guidKey, GUID* pguidValue);
	HRESULT (*GetStringLength)    (Activate* this, REFGUID guidKey, UINT32* pcchLength);
	HRESULT (*GetString)          (Activate* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
	HRESULT (*GetAllocatedString) (Activate* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength);
	HRESULT (*GetBlobSize)        (Activate* this, REFGUID guidKey, UINT32* pcbBlobSize);
	HRESULT (*GetBlob)            (Activate* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize);
	HRESULT (*GetAllocatedBlob)   (Activate* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize);
	HRESULT (*GetUnknown)         (Activate* this, REFGUID guidKey, REFIID riid, LPVOID* ppv);
	HRESULT (*SetItem)            (Activate* this, REFGUID guidKey, REFPROPVARIANT Value);
	HRESULT (*DeleteItem)         (Activate* this, REFGUID guid);
	HRESULT (*DeleteAllItems)     (Activate* this);
	HRESULT (*SetUINT32)          (Activate* this, REFGUID guidKey, UINT32  unValue);
	HRESULT (*SetUINT64)          (Activate* this, REFGUID guidKey, UINT64  unValue);
	HRESULT (*SetDouble)          (Activate* this, REFGUID guidKey, double  fValue);
	HRESULT (*SetGUID)            (Activate* this, REFGUID guidKey, REFGUID guidValue);
	HRESULT (*SetString)          (Activate* this, REFGUID guidKey, LPCWSTR wszValue);
	HRESULT (*SetBlob)            (Activate* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize);
	HRESULT (*SetUnknown)         (Activate* this, REFGUID guidKey, IUnknown* pUnknown);
	HRESULT (*LockStore)          (Activate* this);
	HRESULT (*UnlockStore)        (Activate* this);
	HRESULT (*GetCount)           (Activate* this, UINT32* pcItems);
	HRESULT (*GetItemByIndex)     (Activate* this, INT32 unIndex, GUID* pGuidKey, PROPVARIANT* pValue);
	HRESULT (*CopyAllItems)       (Activate* this, IMFAttributes* pDest);

	// IMFActivate
	HRESULT (*ActivateObject) (Activate* this, REFIID riid, void** ppv);
	HRESULT (*ShutdownObject) (Activate* this);
	HRESULT (*DetachObject)   (Activate* this);
} ActivateVtbl;

typedef struct Activate
{
	ActivateVtbl* lpVtbl;
	u32 ref_count;
	IMFAttributes* attributes;
	MediaSource* media_source;
} Activate;

static Activate ActivatePool[HOLOCAM_MAX_CAMERA_COUNT] = {0};
static s32 ActivatePoolOccupancy = 0;

HRESULT
Activate__QueryInterface(Activate* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IMFActivate) && !IsEqualIID(riid, &IID_Activate)) result = E_NOINTERFACE;
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
Activate__AddRef(Activate* this)
{
	this->ref_count += 1;
	return this->ref_count;
}

ULONG
Activate__Release(Activate* this)
{
	if (this->ref_count > 0)
	{
		this->ref_count -= 1;

		if (this->ref_count == 0)
		{
			if (this->attributes != 0)
			{
				IMFAttributes_Release(this->attributes);
				this->attributes = 0;
			}

			if (this->media_source != 0)
			{
				this->media_source->lpVtbl->Release(this->media_source);
				this->media_source = 0;
			}

			InterlockedDecrement(&ActivatePoolOccupancy);
		}
	}

	return this->ref_count;
}

HRESULT
Activate__GetItem(Activate* this, REFGUID guidKey, PROPVARIANT* pValue)
{
	return IMFAttributes_GetItem(this->attributes, guidKey, pValue);
}

HRESULT
Activate__GetItemType(Activate* this, REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType)
{
	return IMFAttributes_GetItemType(this->attributes, guidKey, pType);
}

HRESULT
Activate__CompareItem(Activate* this, REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult)
{
	return IMFAttributes_CompareItem(this->attributes, guidKey, Value, pbResult);
}

HRESULT
Activate__Compare(Activate* this, IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult)
{
	return IMFAttributes_Compare(this->attributes, pTheirs, MatchType, pbResult);
}

HRESULT
Activate__GetUINT32(Activate* this, REFGUID guidKey, UINT32* punValue)
{
	return IMFAttributes_GetUINT32(this->attributes, guidKey, punValue);
}

HRESULT
Activate__GetUINT64(Activate* this, REFGUID guidKey, UINT64* punValue)
{
	return IMFAttributes_GetUINT64(this->attributes, guidKey, punValue);
}

HRESULT
Activate__GetDouble(Activate* this, REFGUID guidKey, double* pfValue)
{
	return IMFAttributes_GetDouble(this->attributes, guidKey, pfValue);
}

HRESULT
Activate__GetGUID(Activate* this, REFGUID guidKey, GUID* pguidValue)
{
	return IMFAttributes_GetGUID(this->attributes, guidKey, pguidValue);
}

HRESULT
Activate__GetStringLength(Activate* this, REFGUID guidKey, UINT32* pcchLength)
{
	return IMFAttributes_GetStringLength(this->attributes, guidKey, pcchLength);
}

HRESULT
Activate__GetString(Activate* this, REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
{
	return IMFAttributes_GetString(this->attributes, guidKey, pwszValue, cchBufSize, pcchLength);
}

HRESULT
Activate__GetAllocatedString(Activate* this, REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength)
{
	return IMFAttributes_GetAllocatedString(this->attributes, guidKey, ppwszValue, pcchLength);
}

HRESULT
Activate__GetBlobSize(Activate* this, REFGUID guidKey, UINT32* pcbBlobSize)
{
	return IMFAttributes_GetBlobSize(this->attributes, guidKey, pcbBlobSize);
}

HRESULT
Activate__GetBlob(Activate* this, REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize)
{
	return IMFAttributes_GetBlob(this->attributes, guidKey, pBuf, cbBufSize, pcbBlobSize);
}

HRESULT
Activate__GetAllocatedBlob(Activate* this, REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize)
{
	return IMFAttributes_GetAllocatedBlob(this->attributes, guidKey, ppBuf, pcbSize);
}

HRESULT
Activate__GetUnknown(Activate* this, REFGUID guidKey, REFIID riid, LPVOID* ppv)
{
	return IMFAttributes_GetUnknown(this->attributes, guidKey, riid, ppv);
}

HRESULT
Activate__SetItem(Activate* this, REFGUID guidKey, REFPROPVARIANT Value)
{
	return IMFAttributes_SetItem(this->attributes, guidKey, Value);
}

HRESULT
Activate__DeleteItem(Activate* this, REFGUID guidKey)
{
	return IMFAttributes_DeleteItem(this->attributes, guidKey);
}

HRESULT
Activate__DeleteAllItems(Activate* this)
{
	return IMFAttributes_DeleteAllItems(this->attributes);
}

HRESULT
Activate__SetUINT32(Activate* this, REFGUID guidKey, UINT32  unValue)
{
	return IMFAttributes_SetUINT32(this->attributes, guidKey, unValue);
}

HRESULT
Activate__SetUINT64(Activate* this, REFGUID guidKey, UINT64  unValue)
{
	return IMFAttributes_SetUINT64(this->attributes, guidKey, unValue);
}

HRESULT
Activate__SetDouble(Activate* this, REFGUID guidKey, double  fValue)
{
	return IMFAttributes_SetDouble(this->attributes, guidKey, fValue);
}

HRESULT
Activate__SetGUID(Activate* this, REFGUID guidKey, REFGUID guidValue)
{
	return IMFAttributes_SetGUID(this->attributes, guidKey, guidValue);
}

HRESULT
Activate__SetString(Activate* this, REFGUID guidKey, LPCWSTR wszValue)
{
	return IMFAttributes_SetString(this->attributes, guidKey, wszValue);
}

HRESULT
Activate__SetBlob(Activate* this, REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize)
{
	return IMFAttributes_SetBlob(this->attributes, guidKey, pBuf, cbBufSize);
}

HRESULT
Activate__SetUnknown(Activate* this, REFGUID guidKey, IUnknown* pUnknown)
{
	return IMFAttributes_SetUnknown(this->attributes, guidKey, pUnknown);
}

HRESULT
Activate__LockStore(Activate* this)
{
	return IMFAttributes_LockStore(this->attributes);
}

HRESULT
Activate__UnlockStore(Activate* this)
{
	return IMFAttributes_UnlockStore(this->attributes);
}

HRESULT
Activate__GetCount(Activate* this, UINT32* pcItems)
{
	return IMFAttributes_GetCount(this->attributes, pcItems);
}

HRESULT
Activate__GetItemByIndex(Activate* this, INT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue)
{
	return IMFAttributes_GetItemByIndex(this->attributes, unIndex, pguidKey, pValue);
}

HRESULT
Activate__CopyAllItems(Activate* this, IMFAttributes* pDest)
{
	return IMFAttributes_CopyAllItems(this->attributes, pDest);
}

HRESULT
Activate__ActivateObject(Activate* this, REFIID riid, void** ppv)
{
	HRESULT result;

	if (ppv == 0) result = E_POINTER;
	else
	{
		*ppv = 0;

		MediaSource* media_source = 0;
		for (umm i = 0; i < ARRAY_LEN(MediaSourcePool) && media_source == 0; ++i)
		{
			AcquireSRWLockExclusive(&MediaSourcePool[i].lock);
			if (MediaSourcePool[i].ref_count == 0)
			{
				media_source = &MediaSourcePool[i];
				media_source->ref_count = 1;
			}
			ReleaseSRWLockExclusive(&MediaSourcePool[i].lock);
		}

		if (media_source == 0) result = E_OUTOFMEMORY;
		else
		{
			InterlockedIncrement(&MediaSourcePoolOccupancy);

			if (this->media_source != 0) this->media_source->lpVtbl->Release(this->media_source);

			this->media_source = media_source;
			*this->media_source = (MediaSource){
				.lpVtbl                       = &MediaSource_Vtbl,
				.lpGetServiceVtbl             = &MediaSource_GetService_Vtbl,
				.lpSampleAllocatorControlVtbl = &MediaSource_SampleAllocatorControl_Vtbl,
				.lpKsControlVtbl              = &MediaSource_KsControl_Vtbl,
				.ref_count                    = 1,
				.lock                         = SRWLOCK_INIT,
			};

			result = MediaSource__Init(this->media_source);

			if (SUCCEEDED(result))
			{
				result = this->media_source->lpVtbl->QueryInterface(this->media_source, riid, ppv);
			}

			if (!SUCCEEDED(result))
			{
				this->media_source->lpVtbl->Release(this->media_source);
				this->media_source = 0;
			}
		}
	}

	return result;
}

HRESULT
Activate__ShutdownObject(Activate* this)
{
	return S_OK;
}

HRESULT
Activate__DetachObject(Activate* this)
{
	if (this->media_source != 0)
	{
		this->media_source->lpVtbl->Release(this->media_source);
		this->media_source = 0;
	}
	return S_OK;
}

ActivateVtbl Activate_Vtbl = {
	// IUnknown
	.QueryInterface = Activate__QueryInterface,
	.AddRef         = Activate__AddRef,
	.Release        = Activate__Release,

	// IMFAttributes
	.GetItem            = Activate__GetItem,
	.GetItemType        = Activate__GetItemType,
	.CompareItem        = Activate__CompareItem,
	.Compare            = Activate__Compare,
	.GetUINT32          = Activate__GetUINT32,
	.GetUINT64          = Activate__GetUINT64,
	.GetDouble          = Activate__GetDouble,
	.GetGUID            = Activate__GetGUID,
	.GetStringLength    = Activate__GetStringLength,
	.GetString          = Activate__GetString,
	.GetAllocatedString = Activate__GetAllocatedString,
	.GetBlobSize        = Activate__GetBlobSize,
	.GetBlob            = Activate__GetBlob,
	.GetAllocatedBlob   = Activate__GetAllocatedBlob,
	.GetUnknown         = Activate__GetUnknown,
	.SetItem            = Activate__SetItem,
	.DeleteItem         = Activate__DeleteItem,
	.DeleteAllItems     = Activate__DeleteAllItems,
	.SetUINT32          = Activate__SetUINT32,
	.SetUINT64          = Activate__SetUINT64,
	.SetDouble          = Activate__SetDouble,
	.SetGUID            = Activate__SetGUID,
	.SetString          = Activate__SetString,
	.SetBlob            = Activate__SetBlob,
	.SetUnknown         = Activate__SetUnknown,
	.LockStore          = Activate__LockStore,
	.UnlockStore        = Activate__UnlockStore,
	.GetCount           = Activate__GetCount,
	.GetItemByIndex     = Activate__GetItemByIndex,
	.CopyAllItems       = Activate__CopyAllItems,

	// IMFActivate
	.ActivateObject = Activate__ActivateObject,
	.ShutdownObject = Activate__ShutdownObject,
	.DetachObject   = Activate__DetachObject,
};

HRESULT
ActivateFactory__QueryInterface(IClassFactory* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IClassFactory) && !IsEqualIID(riid, &IID_ActivateFactory)) result = E_NOINTERFACE;
		else
		{
			*handle = this;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
	}

	return result;
}

HRESULT
ActivateFactory__AddRef(IClassFactory* this)
{
	return 1;
}

HRESULT
ActivateFactory__Release(IClassFactory* this)
{
	return 1;
}

HRESULT
ActivateFactory__CreateInstance(IClassFactory* this, IUnknown* outer, REFIID id, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	{
		*handle = 0;

		if (outer != 0) result = CLASS_E_NOAGGREGATION;
		else
		{
			u32 i = 0;
			for (; i < ARRAY_LEN(ActivatePool); ++i)
			{
				if (InterlockedCompareExchange(&ActivatePool[i].ref_count, 1, 0) == 0) break;
			}

			if (i >= ARRAY_LEN(ActivatePool)) result = E_OUTOFMEMORY;
			else
			{
				InterlockedIncrement(&ActivatePoolOccupancy);

				Activate* activate = &ActivatePool[i];

				activate->lpVtbl = &Activate_Vtbl;

				if (!SUCCEEDED(MFCreateAttributes(&activate->attributes, 0)))
				{
					result = E_FAIL;
					activate->lpVtbl->Release(activate);
				}
				else
				{
					result = activate->lpVtbl->QueryInterface(activate, id, handle);
					activate->lpVtbl->Release(activate);
				}
			}
		}
	}

	return result;
}

HRESULT
ActivateFactory__LockServer(IClassFactory* this, BOOL flock)
{
	// TODO:
	return E_NOTIMPL;
}

static IClassFactoryVtbl ActivateFactoryVtbl = {
	.QueryInterface = ActivateFactory__QueryInterface,
	.AddRef         = ActivateFactory__AddRef,
	.Release        = ActivateFactory__Release,
	.CreateInstance = ActivateFactory__CreateInstance,
	.LockServer     = ActivateFactory__LockServer,
};

static IClassFactory ActivateFactory = { &ActivateFactoryVtbl };
