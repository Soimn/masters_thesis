// {0E7FE760-A1C9-4793-A237-F951743F994D}
DEFINE_GUID(IID_IHoloCamMediaSource, 0xe7fe760, 0xa1c9, 0x4793, 0xa2, 0x37, 0xf9, 0x51, 0x74, 0x3f, 0x99, 0x4d);

typedef struct IHoloCamMediaSource IHoloCamMediaSource;
typedef struct IHoloCamMediaSourceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (IHoloCamMediaSource* this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (IHoloCamMediaSource* this);
	ULONG   (*Release)        (IHoloCamMediaSource* this);

	// IMFMediaEventGenerator
	HRESULT (*GetEvent)      (IHoloCamMediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*BeginGetEvent) (IHoloCamMediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)   (IHoloCamMediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)    (IHoloCamMediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaSource
	HRESULT (*GetCharacteristics)           (IHoloCamMediaSource* this, DWORD* pdwCharacteristics);
	HRESULT (*CreatePresentationDescriptor) (IHoloCamMediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor);
	HRESULT (*Start)                        (IHoloCamMediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition);
	HRESULT (*Stop)                         (IHoloCamMediaSource* this);
	HRESULT (*Pause)                        (IHoloCamMediaSource* this);
	HRESULT (*Shutdown)                     (IHoloCamMediaSource* this);

	// IMFMediaSourceEx
	HRESULT (*GetSourceAttributes) (IHoloCamMediaSource* this, IMFAttributes** ppAttributes);
	HRESULT (*GetStreamAttributes) (IHoloCamMediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes);
	HRESULT (*SetD3DManager)       (IHoloCamMediaSource* this, IUnknown* pManager);
} IHoloCamMediaSourceVtbl;

typedef struct IHoloCamMediaSource_GetServiceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IMFGetService
	HRESULT (*GetService) (void* raw_this, REFGUID guidService, REFIID riid, void** ppvObject);
} IHoloCamMediaSource_GetServiceVtbl;

typedef struct IHoloCamMediaSource_SampleAllocatorControlVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IMFSampleAllocatorControl
	HRESULT (*SetDefaultAllocator) (void* raw_this, DWORD dwOutputStreamID, IUnknown* pAllocator);
	HRESULT (*GetAllocatorUsage)   (void* raw_this, DWORD dwOutputStreamID, DWORD* pdwInputStreamID, MFSampleAllocatorUsage* peUsage);
} IHoloCamMediaSource_SampleAllocatorControlVtbl;

typedef struct IHoloCamMediaSource_KsControlVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IKsControl
	NTSTATUS (*KsProperty) (void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned);
	NTSTATUS (*KsMethod)   (void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned);
	NTSTATUS (*KsEvent)    (void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned);
} IHoloCamMediaSource_KsControlVtbl;

typedef enum IHoloCamMediaSource_State
{
	IHoloCamMediaSourceState_Shutdown = 0,
	IHoloCamMediaSourceState_Stopped  = 1,
	IHoloCamMediaSourceState_Started  = 2,
} IHoloCamMediaSource_State;

typedef struct IHoloCamMediaSource
{
	IHoloCamMediaSourceVtbl* lpVtbl;
	IHoloCamMediaSource_GetServiceVtbl* lpGetServiceVtbl;
	IHoloCamMediaSource_SampleAllocatorControlVtbl* lpSampleAllocatorControlVtbl;
	IHoloCamMediaSource_KsControlVtbl* lpKsControlVtbl;
	u32 ref_count;
	SRWLOCK lock;
	IHoloCamMediaSource_State state;
	IMFMediaSource* device_media_source;
	IMFAttributes* attributes;
	IMFMediaEventQueue* event_queue;
	u32 stream_count;
	IHoloCamMediaStream streams[HOLO_MAX_PER_CAMERA_STREAM_COUNT];
	u32 queue_id;
	IHoloCamCallback callback;
} IHoloCamMediaSource;

#define IHOLOCAMMEDIASOURCE_ADJ_THIS(RAW_THIS, INTERFACE) (IHoloCamMediaSource*)((u8*)raw_this - (u8*)&((IHoloCamMediaSource*)0)->lp##INTERFACE##Vtbl)

HRESULT
IHoloCamMediaSource__QueryInterface(IHoloCamMediaSource* this, REFIID riid, void** handle)
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
				IsEqualIID(riid, &IID_IHoloCamMediaSource))
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
IHoloCamMediaSource_GetService__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
IHoloCamMediaSource_SampleAllocatorControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
IHoloCamMediaSource_KsControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

ULONG
IHoloCamMediaSource__AddRef(IHoloCamMediaSource* this)
{
	return InterlockedIncrement(&this->ref_count);
}

HRESULT
IHoloCamMediaSource_GetService__AddRef(void* raw_this)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->AddRef(this);
}

HRESULT
IHoloCamMediaSource_SampleAllocatorControl__AddRef(void* raw_this)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->AddRef(this);
}

HRESULT
IHoloCamMediaSource_KsControl__AddRef(void* raw_this)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->AddRef(this);
}

// NOTE: Requires lock held
void
IHoloCamMediaSource__ReleaseChildren(IHoloCamMediaSource* this)
{
	if (this->attributes != 0)
	{
		IMFAttributes_Release(this->attributes);
		this->attributes = 0;
	}

	if (this->event_queue != 0)
	{
		IMFMediaEventQueue_Release(this->event_queue);
		this->event_queue = 0;
	}

	for (u32 i = 0; i < this->stream_count; ++i) this->streams[i].lpVtbl->Release(&this->streams[i]);
}

ULONG
IHoloCamMediaSource__Release(IHoloCamMediaSource* this)
{
	AcquireSRWLockExclusive(&this->lock);

	if (this->ref_count != 0)
	{
		this->ref_count -= 1;
		if(this->ref_count == 0) IHoloCamMediaSource__ReleaseChildren(this);
	}

	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

HRESULT
IHoloCamMediaSource_GetService__Release(void* raw_this)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->Release(this);
}

HRESULT
IHoloCamMediaSource_SampleAllocatorControl__Release(void* raw_this)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->Release(this);
}

HRESULT
IHoloCamMediaSource_KsControl__Release(void* raw_this)
{
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->Release(this);
}

HRESULT
IHoloCamMediaSource__BeginGetEvent(IHoloCamMediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else                                                  result = IMFMediaEventQueue_BeginGetEvent(this->event_queue, pCallback, punkState);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource__EndGetEvent(IHoloCamMediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else                                                  result = IMFMediaEventQueue_EndGetEvent(this->event_queue, pResult, ppEvent);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource__GetEvent(IHoloCamMediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	IMFMediaEventQueue* queue;
	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else                                                  result = IMFMediaEventQueue_QueryInterface(this->event_queue, &IID_IMFMediaEventQueue, &queue);

	ReleaseSRWLockExclusive(&this->lock);

	if (result == S_OK)
	{
		result = IMFMediaEventQueue_GetEvent(queue, dwFlags, ppEvent);
		IMFMediaEventQueue_Release(queue);
	}

	return result;
}

HRESULT
IHoloCamMediaSource__QueueEvent(IHoloCamMediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else                                                  result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, met, guidExtendedType, hrStatus, pvValue);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource__CreatePresentationDescriptor(IHoloCamMediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor)
{
	HRESULT result;
	
	if (ppPresentationDescriptor == 0) result = E_POINTER;
	else
	{
		*ppPresentationDescriptor = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
		else                                                  result = IMFMediaSource_CreatePresentationDescriptor(this->device_media_source, ppPresentationDescriptor);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource__GetCharacteristics(IHoloCamMediaSource* this, DWORD* pdwCharacteristics)
{
	HRESULT result;

	if (pdwCharacteristics == 0) result = E_POINTER;
	else
	{
		*pdwCharacteristics = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
		else                                                  result = IMFMediaSource_GetCharacteristics(this->device_media_source, pdwCharacteristics);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource__Pause(IHoloCamMediaSource* this)
{
	return MF_E_INVALID_STATE_TRANSITION;
}

HRESULT
IHoloCamMediaSource__Shutdown(IHoloCamMediaSource* this)
{
	AcquireSRWLockExclusive(&this->lock);

	if (this->state != IHoloCamMediaSourceState_Shutdown)
	{
		IMFMediaEventQueue_Shutdown(this->event_queue);

		for (u32 i = 0; i < this->stream_count; ++i) IHoloCamMediaStream_Shutdown(&this->streams[i]);

		IMFMediaSource_Shutdown(this->device_media_source);

		IHoloCamMediaSource__ReleaseChildren(this);

		if (this->queue_id != 0)
		{
			MFUnlockWorkQueue(this->queue_id);
			this->queue_id = 0;
		}

		this->state = IHoloCamMediaSourceState_Shutdown;
	}

	ReleaseSRWLockExclusive(&this->lock);

	return S_OK;
}

HRESULT
IHoloCamMediaSource__Start(IHoloCamMediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
	HRESULT result;

	if      (pPresentationDescriptor == 0 || pvarStartPosition == 0)           result = E_POINTER;
	else if (pguidTimeFormat != 0 && !IsEqualIID(pguidTimeFormat, &GUID_NULL)) result = MF_E_UNSUPPORTED_TIME_FORMAT;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
		else                                                  result = IMFMediaSource_Start(this->device_media_source, pPresentationDescriptor, pguidTimeFormat, pvarStartPosition);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource__Stop(IHoloCamMediaSource* this)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if      (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else if (this->state != IHoloCamMediaSourceState_Started)  result = MF_E_INVALID_STATE_TRANSITION;
	else
	{
		this->state = IHoloCamMediaSourceState_Stopped;
		result = IMFMediaSource_Stop(this->device_media_source);
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource__GetSourceAttributes(IHoloCamMediaSource* this, IMFAttributes** ppAttributes)
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
IHoloCamMediaSource__GetStreamAttributes(IHoloCamMediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes)
{
	HRESULT result;

	if (ppAttributes == 0) result = E_POINTER;
	else
	{
		*ppAttributes = 0;

		AcquireSRWLockExclusive(&this->lock);

		IMFMediaSourceEx* media_source_ex;
		result = IMFMediaSource_QueryInterface(this->device_media_source, &IID_IMFMediaSourceEx, &media_source_ex);
		if (SUCCEEDED(result)) result = IMFMediaSourceEx_GetStreamAttributes(media_source_ex, dwStreamIdentifier, ppAttributes);
		IMFMediaSourceEx_Release(media_source_ex);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource__SetD3DManager(IHoloCamMediaSource* this, IUnknown* pManager)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	IMFMediaSourceEx* media_source_ex;
	result = IMFMediaSource_QueryInterface(this->device_media_source, &IID_IMFMediaSourceEx, &media_source_ex);
	if (SUCCEEDED(result)) result = IMFMediaSourceEx_SetD3DManager(media_source_ex, pManager);
	IMFMediaSourceEx_Release(media_source_ex);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource_GetService__GetService(void* raw_this, REFGUID guidService, REFIID riid, void** ppvObject)
{
	HRESULT result;
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, GetService);
	
	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else
	{
		IMFGetService* get_service;
		if (!SUCCEEDED(IMFMediaSource_QueryInterface(this->device_media_source, &IID_IMFGetService, &get_service))) result = MF_E_UNSUPPORTED_SERVICE;
		else
		{
			result = IMFGetService_GetService(get_service, guidService, riid, ppvObject);

			IMFGetService_Release(get_service);
		}
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource_SampleAllocatorControl__GetAllocatorUsage(void* raw_this, DWORD dwOutputStreamID, DWORD* pdwInputStreamID, MFSampleAllocatorUsage* peUsage)
{
	HRESULT result;
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	
	if (pdwInputStreamID == 0 || peUsage == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
		else
		{
			*pdwInputStreamID = dwOutputStreamID;
			*peUsage          = MFSampleAllocatorUsage_UsesCustomAllocator;

			IMFSampleAllocatorControl* alloc_control;
			if (SUCCEEDED(IMFMediaSource_QueryInterface(this->device_media_source, &IID_IMFSampleAllocatorControl, &alloc_control)))
			{
				result = IMFSampleAllocatorControl_GetAllocatorUsage(alloc_control, dwOutputStreamID, pdwInputStreamID, peUsage);
				IMFSampleAllocatorControl_Release(alloc_control);
			}
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource_SampleAllocatorControl__SetDefaultAllocator(void* raw_this, DWORD dwOutputStreamID, IUnknown* pAllocator)
{
	HRESULT result;
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	
	if (pAllocator == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
		else
		{
			IMFSampleAllocatorControl* alloc_control;
			result = IMFMediaSource_QueryInterface(this->device_media_source, &IID_IMFSampleAllocatorControl, &alloc_control);
			if (SUCCEEDED(result))
			{
				result = IMFSampleAllocatorControl_SetDefaultAllocator(alloc_control, dwOutputStreamID, pAllocator);
				IMFSampleAllocatorControl_Release(alloc_control);
			}
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource_KsControl__KsEvent(void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, KsControl);

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else
	{
		/*
		IKsControl* ks_control;
		if (!SUCCEEDED(IMFMediaSource_QueryInterface(this->device_media_source, &IID_IKsControl, &ks_control))) result = HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
		else
		{
			result = ks_control->lpVtbl->KsEvent(ks_control, Event, EventLength, EventData, DataLength, BytesReturned);
			ks_control->lpVtbl->Release(ks_control);
		}
		*/
		result = HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource_KsControl__KsMethod(void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, KsControl);

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else
	{
		/*
		IKsControl* ks_control;
		if (!SUCCEEDED(IMFMediaSource_QueryInterface(this->device_media_source, &IID_IKsControl, &ks_control))) result = HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
		else
		{
			result = ks_control->lpVtbl->KsMethod(ks_control, Method, MethodLength, MethodData, DataLength, BytesReturned);
			ks_control->lpVtbl->Release(ks_control);
		}
		*/
		result = HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource_KsControl__KsProperty(void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;
	IHoloCamMediaSource* this = IHOLOCAMMEDIASOURCE_ADJ_THIS(raw_this, KsControl);

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
	else
	{
		/*IKsControl* ks_control;
		if (!SUCCEEDED(IMFMediaSource_QueryInterface(this->device_media_source, &IID_IKsControl, &ks_control))) result = HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
		else
		{
			result = ks_control->lpVtbl->KsProperty(ks_control, Property, PropertyLength, PropertyData, DataLength, BytesReturned);
			ks_control->lpVtbl->Release(ks_control);
		}*/

		result = HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
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

static IHoloCamMediaSource_GetServiceVtbl IHoloCamMediaSource_GetService_Vtbl = {
	.QueryInterface = IHoloCamMediaSource_GetService__QueryInterface,
	.AddRef         = IHoloCamMediaSource_GetService__AddRef,
	.Release        = IHoloCamMediaSource_GetService__Release,
	.GetService     = IHoloCamMediaSource_GetService__GetService,
};

static IHoloCamMediaSource_SampleAllocatorControlVtbl IHoloCamMediaSource_SampleAllocatorControl_Vtbl = {
	.QueryInterface      = IHoloCamMediaSource_SampleAllocatorControl__QueryInterface,
	.AddRef              = IHoloCamMediaSource_SampleAllocatorControl__AddRef,
	.Release             = IHoloCamMediaSource_SampleAllocatorControl__Release,
	.SetDefaultAllocator = IHoloCamMediaSource_SampleAllocatorControl__SetDefaultAllocator,
	.GetAllocatorUsage   = IHoloCamMediaSource_SampleAllocatorControl__GetAllocatorUsage,
};

static IHoloCamMediaSource_KsControlVtbl IHoloCamMediaSource_KsControl_Vtbl = {
	.QueryInterface = IHoloCamMediaSource_KsControl__QueryInterface,
	.AddRef         = IHoloCamMediaSource_KsControl__AddRef,
	.Release        = IHoloCamMediaSource_KsControl__Release,
	.KsProperty     = IHoloCamMediaSource_KsControl__KsProperty,
	.KsMethod       = IHoloCamMediaSource_KsControl__KsMethod,
	.KsEvent        = IHoloCamMediaSource_KsControl__KsEvent,
};

void
IHoloCamMediaSource__OnMediaSourceEvent(IHoloCamMediaSource* this, IMFAsyncResult* pResult)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	IUnknown* unknown_result;
	result = IMFAsyncResult_GetState(pResult, &unknown_result);
	if (SUCCEEDED(result))
	{
		IMFMediaSource* media_source;
		result = IUnknown_QueryInterface(unknown_result, &IID_IMFMediaSource, &media_source);
		IUnknown_Release(unknown_result);

		if (SUCCEEDED(result))
		{
			IMFMediaEvent* event;
			result = IMFMediaSource_EndGetEvent(media_source, pResult, &event);
			IMFMediaSource_Release(media_source);

			if (SUCCEEDED(result))
			{
				MediaEventType event_type = MEUnknown;
				result = IMFMediaEvent_GetType(event, &event_type);

				if (SUCCEEDED(result))
				{
					if (event_type == MENewStream || event_type == MEUpdatedStream)
					{
						PROPVARIANT prop_value;
						PropVariantInit(&prop_value);
						if (SUCCEEDED(IMFMediaEvent_GetValue(event, &prop_value)) && prop_value.vt == VT_UNKNOWN && prop_value.punkVal != 0)
						{
							IMFMediaStream* media_stream;
							if (SUCCEEDED(IUnknown_QueryInterface(prop_value.punkVal, &IID_IMFMediaStream, &media_stream)))
							{
								IMFStreamDescriptor* stream_descriptor;
								if (SUCCEEDED(IMFMediaStream_GetStreamDescriptor(media_stream, &stream_descriptor)))
								{
									DWORD stream_id;
									if (SUCCEEDED(IMFStreamDescriptor_GetStreamIdentifier(stream_descriptor, &stream_id)))
									{
										u32 i = 0;
										for (; i < this->stream_count && this->streams[i].stream_id != stream_id; ++i);

										if (i < this->stream_count)
										{
											if (SUCCEEDED(IHoloCamMediaStream_SetMediaStream(&this->streams[i], media_stream)))
											{
												IUnknown* unknown_stream;
												if (SUCCEEDED(this->streams[i].lpVtbl->QueryInterface(&this->streams[i], &IID_IUnknown, &unknown_stream)))
												{
													IMFMediaEventQueue_QueueEventParamUnk(this->event_queue, event_type, &GUID_NULL, S_OK, unknown_stream);
													IUnknown_Release(unknown_stream);
												}
											}
										}
									}

									IMFStreamDescriptor_Release(stream_descriptor);
								}

								IMFMediaStream_Release(media_stream);
							}
						}
					}
					else if (event_type == MESourceStarted)
					{
						this->state = IHoloCamMediaSourceState_Started;
						IMFMediaEventQueue_QueueEvent(this->event_queue, event);
					}
					else if (event_type == MESourceStopped)
					{
						for (u32 i = 0; i < this->stream_count; ++i)
						{
							this->streams[i].lpVtbl->SetStreamState(&this->streams[i], MF_STREAM_STATE_STOPPED);
						}

						IMFMediaEventQueue_QueueEvent(this->event_queue, event);

						this->state = IHoloCamMediaSourceState_Stopped;
					}
					else IMFMediaEventQueue_QueueEvent(this->event_queue, event);
				}

				IMFMediaEvent_Release(event);
			}
		}
	}

	if (this->state != IHoloCamMediaSourceState_Shutdown)
	{
		IMFMediaSource_BeginGetEvent(this->device_media_source, (IMFAsyncCallback*)&this->callback, (IUnknown*)this->device_media_source);
	}

	ReleaseSRWLockExclusive(&this->lock);
}

HRESULT
IHoloCamMediaSource__Init(IHoloCamMediaSource* this, IMFAttributes* attributes, IMFMediaSource* device_media_source)
{
	if (this->ref_count != 0) return E_UNEXPECTED;

	*this = (IHoloCamMediaSource){
		.lpVtbl                       = &IHoloCamMediaSource_Vtbl,
		.lpGetServiceVtbl             = &IHoloCamMediaSource_GetService_Vtbl,
		.lpSampleAllocatorControlVtbl = &IHoloCamMediaSource_SampleAllocatorControl_Vtbl,
		.lpKsControlVtbl              = &IHoloCamMediaSource_KsControl_Vtbl,
		.ref_count                    = 1,
		.lock                         = SRWLOCK_INIT,
		.state                        = IHoloCamMediaSourceState_Stopped,
	};

	RET_IF_FAIL(IMFMediaSource_QueryInterface(device_media_source, &IID_IMFMediaSource, &this->device_media_source));

	{ // attributes
		RET_IF_FAIL(MFCreateAttributes(&this->attributes, 3));

		IMFAttributes* device_attributes;
		IMFMediaSourceEx* device_media_source_ex;
		HRESULT result = IMFMediaSource_QueryInterface(this->device_media_source, &IID_IMFMediaSourceEx, &device_media_source_ex);
		if (SUCCEEDED(result)) result = IMFMediaSourceEx_GetSourceAttributes(device_media_source_ex, &device_attributes);
		if (SUCCEEDED(result)) result = IMFAttributes_CopyAllItems(device_attributes, this->attributes);
		IMFAttributes_Release(device_attributes);
		IMFMediaSourceEx_Release(device_media_source_ex);

		if (attributes != 0)
		{
			u32 attribute_count;
			RET_IF_FAIL(IMFAttributes_GetCount(attributes, &attribute_count));

			for (u32 i = 0; i < attribute_count; ++i)
			{
				GUID guid;
				PROPVARIANT prop_var;
				RET_IF_FAIL(IMFAttributes_GetItemByIndex(attributes, i, &guid, &prop_var));
				RET_IF_FAIL(IMFAttributes_SetItem(this->attributes, &guid, &prop_var));
			}
		}
	}

	{ // media streams
		IMFPresentationDescriptor* presentation_descriptor;
		RET_IF_FAIL(IMFMediaSource_CreatePresentationDescriptor(this->device_media_source, &presentation_descriptor));

		HRESULT result = IMFPresentationDescriptor_GetStreamDescriptorCount(presentation_descriptor, &this->stream_count);
		if (SUCCEEDED(result) && this->stream_count > ARRAY_LEN(this->streams)) result = E_OUTOFMEMORY;

		for (u32 i = 0; SUCCEEDED(result) && i < this->stream_count; ++i)
		{
			IMFStreamDescriptor* stream_descriptor;
			BOOL is_selected = FALSE;
			result = IMFPresentationDescriptor_GetStreamDescriptorByIndex(presentation_descriptor, i, &is_selected, &stream_descriptor);
			if (SUCCEEDED(result))
			{
				if (SUCCEEDED(result)) result = IHoloCamMediaStream__Init(&this->streams[i], (IMFMediaSource*)this, stream_descriptor, this->queue_id);

				IMFStreamDescriptor_Release(stream_descriptor);
			}
		}

	RET_IF_FAIL(MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &this->queue_id));
	IHoloCamCallback__Init(&this->callback, this, (void (*)(void*, IMFAsyncResult*))IHoloCamMediaSource__OnMediaSourceEvent, this->queue_id);
	RET_IF_FAIL(IMFMediaSource_BeginGetEvent(this->device_media_source, (IMFAsyncCallback*)&this->callback, (IUnknown*)this->device_media_source));

		IMFPresentationDescriptor_Release(presentation_descriptor);
		if (!SUCCEEDED(result)) return result;
	}
	
	RET_IF_FAIL(MFCreateEventQueue(&this->event_queue));

	return S_OK;
}
