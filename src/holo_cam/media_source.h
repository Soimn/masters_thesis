typedef struct Media_Source Media_Source;
typedef struct MediaSourceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (Media_Source* this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (Media_Source* this);
	ULONG   (*Release)        (Media_Source* this);

	// IMFMediaEventGenerator
	HRESULT (*GetEvent)      (Media_Source* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*BeginGetEvent) (Media_Source* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)   (Media_Source* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)    (Media_Source* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaSource
	HRESULT (*GetCharacteristics)           (Media_Source* this, DWORD* pdwCharacteristics);
	HRESULT (*CreatePresentationDescriptor) (Media_Source* this, IMFPresentationDescriptor** ppPresentationDescriptor);
	HRESULT (*Start)                        (Media_Source* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition);
	HRESULT (*Stop)                         (Media_Source* this);
	HRESULT (*Pause)                        (Media_Source* this);
	HRESULT (*Shutdown)                     (Media_Source* this);

	// IMFMediaSourceEx
	HRESULT (*GetSourceAttributes) (Media_Source* this, IMFAttributes** ppAttributes);
	HRESULT (*GetStreamAttributes) (Media_Source* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes);
	HRESULT (*SetD3DManager)       (Media_Source* this, IUnknown* pManager);
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


typedef struct Media_Source_Dynamic_State
{
	IMFAttributes* attributes;
	IMFPresentationDescriptor* presentation_descriptor;
	IMFMediaEventQueue* event_queue;
	u32 streams_len;
	Media_Stream* streams[HOLOCAM_MAX_CAMERA_STREAM_COUNT];
} Media_Source_Dynamic_State;

typedef struct Media_Source
{
	// NOTE: Initialized by DllMain
	MediaSourceVtbl* lpVtbl;
	MediaSource_GetServiceVtbl* lpGetServiceVtbl;
	MediaSource_SampleAllocatorControlVtbl* lpSampleAllocatorControlVtbl;
	MediaSource_KsControlVtbl* lpKsControlVtbl;
	u32 ref_count;
	SRWLOCK lock;

	union
	{
		// NOTE: free list link
		struct Media_Source* next_free;

		// NOTE: Initialized by MediaSource__Init
		Media_Source_Dynamic_State dynamic_state;
		struct Media_Source_Dynamic_State;
	};
} Media_Source;

// NOTE: Initialized by DllMain
static Media_Source MediaSourcePool[HOLOCAM_MAX_CAMERA_COUNT];
static Media_Source* MediaSourcePoolFreeList = 0;
static s32 MediaSourcePoolOccupancy          = 0;
static SRWLOCK MediaSourcePoolFreeListLock   = SRWLOCK_INIT;

#define MEDIASOURCE_ADJ_THIS(RAW_THIS, INTERFACE) (Media_Source*)((u8*)raw_this - (u8*)&((Media_Source*)0)->lp##INTERFACE##Vtbl)

HRESULT
MediaSource__QueryInterface(Media_Source* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (IsEqualIID(riid, &IID_IUnknown)               ||
				IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
				IsEqualIID(riid, &IID_IMFMediaSource)         ||
				IsEqualIID(riid, &IID_IMFMediaSourceEx))
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
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
MediaSource_SampleAllocatorControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
MediaSource_KsControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

ULONG
MediaSource__AddRef(Media_Source* this)
{
	AcquireSRWLockExclusive(&this->lock);
	this->ref_count += 1;
	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

HRESULT
MediaSource_GetService__AddRef(void* raw_this)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->AddRef(this);
}

HRESULT
MediaSource_SampleAllocatorControl__AddRef(void* raw_this)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->AddRef(this);
}

HRESULT
MediaSource_KsControl__AddRef(void* raw_this)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->AddRef(this);
}

// NOTE: Requires lock held
void
MediaSource__ReleaseChildren(Media_Source* this)
{
	if (this->attributes != 0)
	{
		IMFAttributes_Release(this->attributes);
		this->attributes = 0;
	}

	if (this->presentation_descriptor != 0)
	{
		IMFPresentationDescriptor_Release(this->presentation_descriptor);
		this->presentation_descriptor = 0;
	}

	if (this->event_queue != 0)
	{
		IMFMediaEventQueue_Release(this->event_queue);
		this->event_queue = 0;
	}

	for (umm i = 0; i < this->streams_len; ++i)
	{
		this->streams[i]->lpVtbl->Release(this->streams[i]);
		this->streams[i] = 0;
	}
}

ULONG
MediaSource__Release(Media_Source* this)
{
	AcquireSRWLockExclusive(&this->lock);

	if (this->ref_count > 0)
	{
		this->ref_count -= 1;

		if (this->ref_count == 0)
		{
			MediaSource__ReleaseChildren(this);

			AcquireSRWLockExclusive(&MediaSourcePoolFreeListLock);
			{
				this->next_free = MediaSourcePoolFreeList;
				MediaSourcePoolFreeList = this;
				MediaSourcePoolOccupancy -= 1;
			}
			ReleaseSRWLockExclusive(&MediaSourcePoolFreeListLock);
		}
	}

	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

HRESULT
MediaSource_GetService__Release(void* raw_this)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource_SampleAllocatorControl__Release(void* raw_this)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource_KsControl__Release(void* raw_this)
{
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource__BeginGetEvent(Media_Source* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__EndGetEvent(Media_Source* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__GetEvent(Media_Source* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__QueueEvent(Media_Source* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__CreatePresentationDescriptor(Media_Source* this, IMFPresentationDescriptor** ppPresentationDescriptor)
{
	HRESULT result;
	
	if (ppPresentationDescriptor == 0) result = E_POINTER;
	else
	{
		*ppPresentationDescriptor = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->presentation_descriptor == 0) result = MF_E_SHUTDOWN;
		else                                    result = IMFPresentationDescriptor_Clone(this->presentation_descriptor, ppPresentationDescriptor);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaSource__GetCharacteristics(Media_Source* this, DWORD* pdwCharacteristics)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__Pause(Media_Source* this)
{
	return MF_E_INVALID_STATE_TRANSITION;
}

HRESULT
MediaSource__Shutdown(Media_Source* this)
{
	return E_NOTIMPL;
}

HRESULT
MediaSource__Start(Media_Source* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__Stop(Media_Source* this)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__GetSourceAttributes(Media_Source* this, IMFAttributes** ppAttributes)
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
MediaSource__GetStreamAttributes(Media_Source* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaSource__SetD3DManager(Media_Source* this, IUnknown* pManager)
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
MediaSource__Init(Media_Source* this, IMFAttributes* parent_attributes)
{
	HRESULT result;

	IMFSensorProfileCollection* sensor_collection = 0;
	IMFSensorProfile* legacy_profile              = 0;
	IMFSensorProfile* profile                     = 0;
	IMFStreamDescriptor* stream_descriptors[ARRAY_LEN(this->streams)] = {0};
	do
	{
		BREAK_IF_FAILED(result, MFCreateAttributes(&this->attributes, 0));
		BREAK_IF_FAILED(result, IMFAttributes_CopyAllItems(parent_attributes, this->attributes));

		{ /// Stream init
			this->streams_len = 1; // TODO

			AcquireSRWLockExclusive(&MediaStreamPoolFreeListLock);
			{
				if ((umm)MediaStreamPoolOccupancy + (umm)this->streams_len >= ARRAY_LEN(MediaStreamPool)) result = E_OUTOFMEMORY;
				else
				{
					for (umm i = 0; i < this->streams_len; ++i)
					{
						this->streams[i] = MediaStreamPoolFreeList;
						MediaStreamPoolFreeList = MediaStreamPoolFreeList->next_free;
						MediaStreamPoolOccupancy += 1;

						this->streams[i]->ref_count     = 1;
						this->streams[i]->dynamic_state = (Media_Stream_Dynamic_State){0};
					}
				}
			}
			ReleaseSRWLockExclusive(&MediaStreamPoolFreeListLock);

			for (u32 i = 0; i < this->streams_len && SUCCEEDED(result); ++i)
			{
				result = MediaStream__Init(this->streams[i], i);
			}

			if (!SUCCEEDED(result)) break;
		}

		// TODO: This might need to change for multiple streams
		{ /// Sensor profile
			BREAK_IF_FAILED(result, MFCreateSensorProfileCollection(&sensor_collection));

			u32 stream_id = 0;
			BREAK_IF_FAILED(result, MFCreateSensorProfile(&KSCAMERAPROFILE_Legacy, 0, 0, &legacy_profile));
			BREAK_IF_FAILED(result, IMFSensorProfile_AddProfileFilter(legacy_profile, stream_id, L"((RES==;FRT<=30,1;SUT==))"));
			BREAK_IF_FAILED(result, IMFSensorProfileCollection_AddProfile(sensor_collection, legacy_profile));

			BREAK_IF_FAILED(result, MFCreateSensorProfile(&KSCAMERAPROFILE_HighFrameRate, 0, 0, &profile));
			BREAK_IF_FAILED(result, IMFSensorProfile_AddProfileFilter(profile, stream_id, L"((RES==;FRT>=60,1;SUT==))"));
			BREAK_IF_FAILED(result, IMFSensorProfileCollection_AddProfile(sensor_collection, profile));

			BREAK_IF_FAILED(result, IMFAttributes_SetUnknown(this->attributes, &MF_DEVICEMFT_SENSORPROFILE_COLLECTION, (IUnknown*)sensor_collection));
		}

		for (u32 i = 0; i < this->streams_len && SUCCEEDED(result); ++i)
		{
			result = this->streams[i]->lpVtbl->GetStreamDescriptor(this->streams[i], &stream_descriptors[i]);
		}
		if (!SUCCEEDED(result)) break;

		BREAK_IF_FAILED(result, MFCreatePresentationDescriptor(this->streams_len, &stream_descriptors[0], &this->presentation_descriptor));
		BREAK_IF_FAILED(result, MFCreateEventQueue(&this->event_queue));
	} while (0);

	if (sensor_collection != 0) IMFSensorProfileCollection_Release(sensor_collection);
	if (legacy_profile != 0) IMFSensorProfile_Release(legacy_profile);
	if (profile != 0) IMFSensorProfile_Release(profile);

	for (u32 i = 0; i < ARRAY_LEN(stream_descriptors); ++i)
	{
		if (stream_descriptors[i] != 0) IMFStreamDescriptor_Release(stream_descriptors[i]);
	}

	return result;
}
