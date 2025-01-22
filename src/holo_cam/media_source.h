// {26335E9E-9CFA-4CF4-9767-E733597E148B}
DEFINE_GUID(IID_MediaSource, 0x26335e9e, 0x9cfa, 0x4cf4, 0x97, 0x67, 0xe7, 0x33, 0x59, 0x7e, 0x14, 0x8b);

typedef struct MediaSource MediaSource;
typedef struct MediaSourceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (MediaSource* this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (MediaSource* this);
	ULONG   (*Release)        (MediaSource* this);

	// IMFMediaEventGenerator
	HRESULT (*GetEvent)      (MediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*BeginGetEvent) (MediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)   (MediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)    (MediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaSource
	HRESULT (*GetCharacteristics)           (MediaSource* this, DWORD* pdwCharacteristics);
	HRESULT (*CreatePresentationDescriptor) (MediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor);
	HRESULT (*Start)                        (MediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition);
	HRESULT (*Stop)                         (MediaSource* this);
	HRESULT (*Pause)                        (MediaSource* this);
	HRESULT (*Shutdown)                     (MediaSource* this);

	// IMFMediaSourceEx
	HRESULT (*GetSourceAttributes) (MediaSource* this, IMFAttributes** ppAttributes);
	HRESULT (*GetStreamAttributes) (MediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes);
	HRESULT (*SetD3DManager)       (MediaSource* this, IUnknown* pManager);
} MediaSourceVtbl;

typedef struct MediaSource_GetServiceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IMFGetService
	HRESULT (*GetService) (void* raw_this, REFGUID guidService, REFIID riid, void** ppvObject);
} MediaSource_GetServiceVtbl;

typedef struct MediaSource_SampleAllocatorControlVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IMFSampleAllocatorControl
	HRESULT (*SetDefaultAllocator) (void* raw_this, DWORD dwOutputStreamID, IUnknown* pAllocator);
	HRESULT (*GetAllocatorUsage)   (void* raw_this, DWORD dwOutputStreamID, DWORD* pdwInputStreamID, MFSampleAllocatorUsage* peUsage);
} MediaSource_SampleAllocatorControlVtbl;

typedef struct MediaSource_KsControlVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IKsControl
	NTSTATUS (*KsProperty) (void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned);
	NTSTATUS (*KsMethod)   (void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned);
	NTSTATUS (*KsEvent)    (void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned);
} MediaSource_KsControlVtbl;

typedef struct MediaSource
{
	MediaSourceVtbl* lpVtbl;
	MediaSource_GetServiceVtbl* lpGetServiceVtbl;
	MediaSource_SampleAllocatorControlVtbl* lpSampleAllocatorControlVtbl;
	MediaSource_KsControlVtbl* lpKsControlVtbl;
	u32 ref_count;
	SRWLOCK lock;
	IMFAttributes* attributes;
	IMFPresentationDescriptor* presentation_descriptor;
} MediaSource;

// NOTE: Initialized in DllMain
static MediaSource MediaSourcePool[HOLOCAM_MAX_CAMERA_COUNT];
static s32 MediaSourcePoolOccupancy = 0;

#define MEDIASOURCE_ADJ_THIS(RAW_THIS, INTERFACE) (MediaSource*)((u8*)raw_this - (u8*)&((MediaSource*)0)->lp##INTERFACE##Vtbl)

HRESULT
MediaSource__QueryInterface(MediaSource* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (IsEqualIID(riid, &IID_IUnknown)               ||
				IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
				IsEqualIID(riid, &IID_IMFMediaSource)         ||
				IsEqualIID(riid, &IID_IMFMediaSourceEx)       ||
				IsEqualIID(riid, &IID_MediaSource))
		{
			*handle = this;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
		else if (IsEqualIID(riid, &IID_IMFGetService))
		{
			*handle = &this->lpGetServiceVtbl;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
		else if (IsEqualIID(riid, &IID_IMFSampleAllocatorControl))
		{
			*handle = &this->lpSampleAllocatorControlVtbl;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
		else if (IsEqualIID(riid, &IID_IKsControl))
		{
			*handle = &this->lpKsControlVtbl;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
		else result = E_NOINTERFACE;
	}

	return result;
}

HRESULT
MediaSource_GetService__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
MediaSource_SampleAllocatorControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
MediaSource_KsControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

ULONG
MediaSource__AddRef(MediaSource* this)
{
	return InterlockedIncrement(&this->ref_count);
}

HRESULT
MediaSource_GetService__AddRef(void* raw_this)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->AddRef(this);
}

HRESULT
MediaSource_SampleAllocatorControl__AddRef(void* raw_this)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->AddRef(this);
}

HRESULT
MediaSource_KsControl__AddRef(void* raw_this)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->AddRef(this);
}

ULONG
MediaSource__Release(MediaSource* this)
{
	AcquireSRWLockExclusive(&this->lock);

	if (this->ref_count > 0)
	{
		this->ref_count -= 1;

		if (this->ref_count == 0)
		{
			if (this->attributes != 0) IMFAttributes_Release(this->attributes);
			if (this->presentation_descriptor != 0) IMFPresentationDescriptor_Release(this->presentation_descriptor);

			InterlockedDecrement(&MediaSourcePoolOccupancy);
		}
	}

	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

HRESULT
MediaSource_GetService__Release(void* raw_this)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource_SampleAllocatorControl__Release(void* raw_this)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource_KsControl__Release(void* raw_this)
{
	MediaSource* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource__BeginGetEvent(MediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__EndGetEvent(MediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__GetEvent(MediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__QueueEvent(MediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__CreatePresentationDescriptor(MediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor)
{
	HRESULT result;
	
	if (ppPresentationDescriptor == 0) result = E_POINTER;
	else
	{
		*ppPresentationDescriptor = 0;

		AcquireSRWLockExclusive(&this->lock);

		// TODO
		result = E_NOTIMPL;

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaSource__GetCharacteristics(MediaSource* this, DWORD* pdwCharacteristics)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__Pause(MediaSource* this)
{
	return MF_E_INVALID_STATE_TRANSITION;
}

HRESULT
MediaSource__Shutdown(MediaSource* this)
{
	return E_NOTIMPL;
}

HRESULT
MediaSource__Start(MediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__Stop(MediaSource* this)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__GetSourceAttributes(MediaSource* this, IMFAttributes** ppAttributes)
{
	HRESULT result;

	if (ppAttributes == 0) result = E_POINTER;
	else
	{
		*ppAttributes = 0;

		AcquireSRWLockExclusive(&this->lock);

		result = IMFAttributes_QueryInterface(this->attributes, &IID_IMFAttributes, ppAttributes);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaSource__GetStreamAttributes(MediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__SetD3DManager(MediaSource* this, IUnknown* pManager)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource_GetService__GetService(void* raw_this, REFGUID guidService, REFIID riid, void** ppvObject)
{
	return MF_E_UNSUPPORTED_SERVICE;
}

HRESULT
MediaSource_SampleAllocatorControl__GetAllocatorUsage(void* raw_this, DWORD dwOutputStreamID, DWORD* pdwInputStreamID, MFSampleAllocatorUsage* peUsage)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource_SampleAllocatorControl__SetDefaultAllocator(void* raw_this, DWORD dwOutputStreamID, IUnknown* pAllocator)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource_KsControl__KsEvent(void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource_KsControl__KsMethod(void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource_KsControl__KsProperty(void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

static MediaSourceVtbl MediaSource_Vtbl = {
	.QueryInterface               = MediaSource__QueryInterface,
	.AddRef                       = MediaSource__AddRef,
	.Release                      = MediaSource__Release,
	.BeginGetEvent                = MediaSource__BeginGetEvent,
	.EndGetEvent                  = MediaSource__EndGetEvent,
	.GetEvent                     = MediaSource__GetEvent,
	.QueueEvent                   = MediaSource__QueueEvent,
	.CreatePresentationDescriptor = MediaSource__CreatePresentationDescriptor,
	.GetCharacteristics           = MediaSource__GetCharacteristics,
	.Pause                        = MediaSource__Pause,
	.Shutdown                     = MediaSource__Shutdown,
	.Start                        = MediaSource__Start,
	.Stop                         = MediaSource__Stop,
	.GetSourceAttributes          = MediaSource__GetSourceAttributes,
	.GetStreamAttributes          = MediaSource__GetStreamAttributes,
	.SetD3DManager                = MediaSource__SetD3DManager,
};

static MediaSource_GetServiceVtbl MediaSource_GetService_Vtbl = {
	.QueryInterface = MediaSource_GetService__QueryInterface,
	.AddRef         = MediaSource_GetService__AddRef,
	.Release        = MediaSource_GetService__Release,
	.GetService     = MediaSource_GetService__GetService,
};

static MediaSource_SampleAllocatorControlVtbl MediaSource_SampleAllocatorControl_Vtbl = {
	.QueryInterface      = MediaSource_SampleAllocatorControl__QueryInterface,
	.AddRef              = MediaSource_SampleAllocatorControl__AddRef,
	.Release             = MediaSource_SampleAllocatorControl__Release,
	.SetDefaultAllocator = MediaSource_SampleAllocatorControl__SetDefaultAllocator,
	.GetAllocatorUsage   = MediaSource_SampleAllocatorControl__GetAllocatorUsage,
};

static MediaSource_KsControlVtbl MediaSource_KsControl_Vtbl = {
	.QueryInterface = MediaSource_KsControl__QueryInterface,
	.AddRef         = MediaSource_KsControl__AddRef,
	.Release        = MediaSource_KsControl__Release,
	.KsProperty     = MediaSource_KsControl__KsProperty,
	.KsMethod       = MediaSource_KsControl__KsMethod,
	.KsEvent        = MediaSource_KsControl__KsEvent,
};

// NOTE: Must only be called from Activate__ActivateObject
HRESULT
MediaSource__Init(MediaSource* this)
{
	HRESULT result;

	result = MFCreateAttributes(&this->attributes, 0);

	result = MFCreatePresentationDescriptor(streams_len, streams, &this->presentation_descriptor);

	return result;
}
