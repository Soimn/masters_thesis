typedef struct IHoloCamMediaSource IHoloCamMediaSource;
typedef struct IHoloCamMediaSourceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (IHoloCamMediaSource* this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (IHoloCamMediaSource* this);
	ULONG   (*Release)        (IHoloCamMediaSource* this);

	// IMFMediaEventGenerator
	HRESULT (*BeginGetEvent)                (IHoloCamMediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)                  (IHoloCamMediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*GetEvent)                     (IHoloCamMediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)                   (IHoloCamMediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaSource
	HRESULT (*CreatePresentationDescriptor) (IHoloCamMediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor);
	HRESULT (*GetCharacteristics)           (IHoloCamMediaSource* this, DWORD* pdwCharacteristics);
	HRESULT (*Pause)                        (IHoloCamMediaSource* this);
	HRESULT (*Shutdown)                     (IHoloCamMediaSource* this);
	HRESULT (*Start)                        (IHoloCamMediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition);
	HRESULT (*Stop)                         (IHoloCamMediaSource* this);

	// IMFMediaSourceEx
	HRESULT (*GetSourceAttributes)          (IHoloCamMediaSource* this, IMFAttributes** ppAttributes);
	HRESULT (*GetStreamAttributes)          (IHoloCamMediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes);
	HRESULT (*SetD3DManager)                (IHoloCamMediaSource* this, IUnknown* pManager);
} IHoloCamMediaSourceVtbl;

typedef struct IHoloCamMediaSource
{
	IHoloCamMediaSourceVtbl* lpVtbl;
	u32 ref_count;
	IMFAttributes* attributes;
} IHoloCamMediaSource;

HRESULT
IHoloCamMediaSource__QueryInterface(IHoloCamMediaSource* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IHoloCamMediaSource)) result = CLASS_E_CLASSNOTAVAILABLE;
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
IHoloCamMediaSource__AddRef(IHoloCamMediaSource* this)
{
	this->ref_count += 1;
	return this->ref_count;
}

ULONG
IHoloCamMediaSource__Release(IHoloCamMediaSource* this)
{
	if (this->ref_count != 0) this->ref_count -= 1;
	return this->ref_count;
}

HRESULT
IHoloCamMediaSource__BeginGetEvent(IHoloCamMediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__EndGetEvent(IHoloCamMediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__GetEvent(IHoloCamMediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__QueueEvent(IHoloCamMediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__CreatePresentationDescriptor(IHoloCamMediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__GetCharacteristics(IHoloCamMediaSource* this, DWORD* pdwCharacteristics)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__Pause(IHoloCamMediaSource* this)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__Shutdown(IHoloCamMediaSource* this)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__Start(IHoloCamMediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__Stop(IHoloCamMediaSource* this)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__GetSourceAttributes(IHoloCamMediaSource* this, IMFAttributes** ppAttributes)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__GetStreamAttributes(IHoloCamMediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes)
{
	// TODO
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaSource__SetD3DManager(IHoloCamMediaSource* this, IUnknown* pManager)
{
	// TODO
	return E_NOTIMPL;
}

static IHoloCamMediaSourceVtbl IHoloCamMediaSource_Vtbl = {
	.QueryInterface               = IHoloCamMediaSource__QueryInterface,
	.AddRef                       = IHoloCamMediaSource__AddRef,
	.Release                      = IHoloCamMediaSource__Release,
	.BeginGetEvent                = IHoloCamMediaSource__BeginGetEvent,
	.EndGetEvent                  = IHoloCamMediaSource__EndGetEvent,
	.GetEvent                     = IHoloCamMediaSource__GetEvent,
	.QueueEvent                   = IHoloCamMediaSource__QueueEvent,
	.CreatePresentationDescriptor = IHoloCamMediaSource__CreatePresentationDescriptor,
	.GetCharacteristics           = IHoloCamMediaSource__GetCharacteristics,
	.Pause                        = IHoloCamMediaSource__Pause,
	.Shutdown                     = IHoloCamMediaSource__Shutdown,
	.Start                        = IHoloCamMediaSource__Start,
	.Stop                         = IHoloCamMediaSource__Stop,
	.GetSourceAttributes          = IHoloCamMediaSource__GetSourceAttributes,
	.GetStreamAttributes          = IHoloCamMediaSource__GetStreamAttributes,
	.SetD3DManager                = IHoloCamMediaSource__SetD3DManager,
};

HRESULT
IHoloCamMediaSource__Init(IHoloCamMediaSource* this, IMFAttributes* attributes)
{
	HRESULT result;

	if (this->ref_count != 1) result = E_FAIL;
	else
	{
		*this = (IHoloCamMediaSource){
			.lpVtbl    = &IHoloCamMediaSource_Vtbl,
			.ref_count = 1,
		};

		if (!SUCCEEDED(MFCreateAttributes(&this->attributes, 1)) || !SUCCEEDED(IMFAttributes_CopyAllItems(attributes, this->attributes))) result = E_FAIL;
		else
		{
			NOT_IMPLEMENTED;
		}
	}

	return result;
}

HRESULT
IHoloCamMediaSourceFactory__QueryInterface(IClassFactory* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else if (!IsEqualIID(riid, &IID_IHoloCamMediaSourceFactory) && !IsEqualIID(riid, &IID_IUnknown))
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
IHoloCamMediaSourceFactory__AddRef(IClassFactory* this)
{
	return 1;
}

HRESULT
IHoloCamMediaSourceFactory__Release(IClassFactory* this)
{
	return 1;
}

IHoloCamMediaSource VirtualCameraMediaSources[HOLO_MAX_CAMERA_COUNT];

HRESULT
IHoloCamMediaSourceFactory__CreateInstance(IClassFactory* this, IUnknown* outer, REFIID id, void** handle)
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
				if (InterlockedCompareExchange(&VirtualCameraMediaSources[i].ref_count, 1, 0) == 0) break;
			}

			if (i >= HOLO_MAX_CAMERA_COUNT) result = E_OUTOFMEMORY;
			else
			{
				IHoloCamMediaSource* src = &VirtualCameraMediaSources[i];
				result = src->lpVtbl->QueryInterface(src, id, handle);
				src->lpVtbl->Release(src);
			}
		}
	}

	return result;
}

HRESULT
IHoloCamMediaSourceFactory__LockServer(IClassFactory* this, BOOL flock)
{
	// TODO:
	return E_NOTIMPL;
}

static IClassFactoryVtbl IHoloCamMediaSourceFactoryVtbl = {
	.QueryInterface = IHoloCamMediaSourceFactory__QueryInterface,
	.AddRef         = IHoloCamMediaSourceFactory__AddRef,
	.Release        = IHoloCamMediaSourceFactory__Release,
	.CreateInstance = IHoloCamMediaSourceFactory__CreateInstance,
	.LockServer     = IHoloCamMediaSourceFactory__LockServer,
};

static IClassFactory HoloCamMediaSourceFactory = { &IHoloCamMediaSourceFactoryVtbl };
