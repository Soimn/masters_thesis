typedef struct Media_Stream Media_Stream;
typedef struct MediaStreamVtbl
{
	// IUnknown
	HRESULT (*QueryInterface) (Media_Stream* this, REFIID riid, void** ppvObject);
	ULONG (*AddRef)           (Media_Stream* this);
	ULONG (*Release)          (Media_Stream* this);

	// IMFMediaEventGenerator
	HRESULT (*GetEvent)      (Media_Stream* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*BeginGetEvent) (Media_Stream* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)   (Media_Stream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)    (Media_Stream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaStream
	HRESULT (*GetMediaSource)      (Media_Stream* this, IMFMediaSource** ppMediaSource);
	HRESULT (*GetStreamDescriptor) (Media_Stream* this, IMFStreamDescriptor** ppStreamDescriptor);
	HRESULT (*RequestSample)       (Media_Stream* this, IUnknown* pToken);

	// IMFMediaStream2
	HRESULT (*SetStreamState) (Media_Stream* this, MF_STREAM_STATE value);
	HRESULT (*GetStreamState) (Media_Stream* this, MF_STREAM_STATE* value);
} MediaStreamVtbl;

typedef struct MediaStream_KsControlVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (void* raw_this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (void* raw_this);
	ULONG   (*Release)        (void* raw_this);

	// IKsControl
	NTSTATUS (*KsProperty) (void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned);
	NTSTATUS (*KsMethod)   (void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned);
	NTSTATUS (*KsEvent)    (void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned);
} MediaStream_KsControlVtbl;

typedef struct Media_Stream_Dynamic_State
{
	IMFAttributes* attributes;
	IMFMediaEventQueue* event_queue;
	IMFStreamDescriptor* stream_descriptor;
	MF_STREAM_STATE stream_state;
} Media_Stream_Dynamic_State;

typedef struct Media_Stream
{
	// NOTE: Initialized by DllMain
	MediaStreamVtbl* lpVtbl;
	MediaStream_KsControlVtbl* lpKsControlVtbl;
	u32 ref_count;
	SRWLOCK lock;

	union
	{
		// NOTE: free list link
		struct Media_Stream* next_free;

		// NOTE: Initialized by MediaStream__Init
		Media_Stream_Dynamic_State dynamic_state;
		struct Media_Stream_Dynamic_State;
	};
} Media_Stream;

HRESULT MediaStream__Stop(Media_Stream* this);
HRESULT MediaStream__Start(Media_Stream* this, IMFMediaType* media_type);

// NOTE: Initialized by DllMain
static Media_Stream MediaStreamPool[HOLOCAM_MAX_CAMERA_COUNT*HOLOCAM_MAX_CAMERA_STREAM_COUNT];
static Media_Stream* MediaStreamPoolFreeList = 0;
static s32 MediaStreamPoolOccupancy          = 0;
static SRWLOCK MediaStreamPoolFreeListLock   = SRWLOCK_INIT;

#define MEDIASTREAM_ADJ_THIS(RAW_THIS, INTERFACE) (Media_Stream*)((u8*)(RAW_THIS) - (u8*)(&((Media_Stream*)0)->lp##INTERFACE##Vtbl))

HRESULT
MediaStream__QueryInterface(Media_Stream* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (IsEqualIID(riid, &IID_IUnknown)               ||
				IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
				IsEqualIID(riid, &IID_IMFMediaStream)         ||
				IsEqualIID(riid, &IID_IMFMediaStream2))
		{
			*handle = this;
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
MediaStream_KsControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	Media_Stream* this = MEDIASTREAM_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

ULONG
MediaStream__AddRef(Media_Stream* this)
{
	AcquireSRWLockExclusive(&this->lock);
	this->ref_count += 1;
	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

HRESULT
MediaStream_KsControl__AddRef(void* raw_this)
{
	Media_Stream* this = MEDIASTREAM_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->AddRef(this);
}

// NOTE: Requires lock held
void
MediaStream__ReleaseChildren(Media_Stream* this)
{
	if (this->attributes != 0) IMFAttributes_Release(this->attributes);
	if (this->event_queue != 0) IMFMediaEventQueue_Release(this->event_queue);
	if (this->stream_descriptor != 0) IMFStreamDescriptor_Release(this->stream_descriptor);
}

ULONG
MediaStream__Release(Media_Stream* this)
{
	AcquireSRWLockExclusive(&this->lock);

	if (this->ref_count > 0)
	{
		this->ref_count -= 1;

		if (this->ref_count == 0)
		{
			MediaStream__ReleaseChildren(this);

			AcquireSRWLockExclusive(&MediaStreamPoolFreeListLock);
			{
				this->next_free = MediaStreamPoolFreeList;
				MediaStreamPoolFreeList = this;
				MediaStreamPoolOccupancy -= 1;
			}
			ReleaseSRWLockExclusive(&MediaStreamPoolFreeListLock);
		}
	}

	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

ULONG
MediaStream_KsControl__Release(void* raw_this)
{
	Media_Stream* this = MEDIASTREAM_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaStream__GetEvent(Media_Stream* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__BeginGetEvent(Media_Stream* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__EndGetEvent(Media_Stream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__QueueEvent(Media_Stream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__GetMediaSource(Media_Stream* this, IMFMediaSource** ppMediaSource)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__GetStreamDescriptor(Media_Stream* this, IMFStreamDescriptor** ppStreamDescriptor)
{
	HRESULT result;

	if (ppStreamDescriptor == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);
		
		if (this->stream_descriptor == 0) result = MF_E_SHUTDOWN;
		else                              result = IMFStreamDescriptor_QueryInterface(this->stream_descriptor, &IID_IMFStreamDescriptor, ppStreamDescriptor);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaStream__RequestSample(Media_Stream* this, IUnknown* pToken)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__SetStreamState(Media_Stream* this, MF_STREAM_STATE value)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->stream_state == value) result = S_OK;
	else
	{
		if (value == MF_STREAM_STATE_PAUSED)
		{
			if (this->stream_state != MF_STREAM_STATE_RUNNING) result = MF_E_INVALID_STATE_TRANSITION;
			else
			{
				this->stream_state = value;
				result = S_OK;
			}
		}
		else if (value == MF_STREAM_STATE_RUNNING)
		{
			result = MediaStream__Start(this, 0);
		}
		else if (value == MF_STREAM_STATE_STOPPED)
		{
			result = MediaStream__Stop(this);
		}
		else result = MF_E_INVALID_STATE_TRANSITION;
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
MediaStream__GetStreamState(Media_Stream* this, MF_STREAM_STATE* value)
{
	HRESULT result;

	if (value == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		*value = this->stream_state;

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaStream_KsControl__KsEvent(void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;

	if (BytesReturned == 0) result = E_POINTER;
	else                   	result = ERROR_SET_NOT_FOUND;

	return result;
}

HRESULT
MediaStream_KsControl__KsMethod(void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;

	if (Method == 0 || BytesReturned == 0) result = E_POINTER;
	else                                   result = ERROR_SET_NOT_FOUND;

	return result;
}

HRESULT
MediaStream_KsControl__KsProperty(void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
	HRESULT result;

	if (Property == 0 || BytesReturned == 0) result = E_POINTER;
	else                                     result = ERROR_SET_NOT_FOUND;

	return result;
}

static MediaStreamVtbl MediaStream_Vtbl = {
	.QueryInterface      = MediaStream__QueryInterface,
	.AddRef              = MediaStream__AddRef,
	.Release             = MediaStream__Release,
	.GetEvent            = MediaStream__GetEvent,
	.BeginGetEvent       = MediaStream__BeginGetEvent,
	.EndGetEvent         = MediaStream__EndGetEvent,
	.QueueEvent          = MediaStream__QueueEvent,
	.GetMediaSource      = MediaStream__GetMediaSource,
	.GetStreamDescriptor = MediaStream__GetStreamDescriptor,
	.RequestSample       = MediaStream__RequestSample,
	.SetStreamState      = MediaStream__SetStreamState,
	.GetStreamState      = MediaStream__GetStreamState,
};

static MediaStream_KsControlVtbl MediaStream_KsControl_Vtbl = {
	.QueryInterface = MediaStream_KsControl__QueryInterface,
	.AddRef         = MediaStream_KsControl__AddRef,
	.Release        = MediaStream_KsControl__Release,
	.KsProperty     = MediaStream_KsControl__KsProperty,
	.KsMethod       = MediaStream_KsControl__KsMethod,
	.KsEvent        = MediaStream_KsControl__KsEvent,
};

// NOTE: Must only be called from MediaSource__Init
HRESULT
MediaStream__Init(Media_Stream* this, u32 index)
{
	HRESULT result;

	// TODO
	u32 width     = 1280;
	u32 height    = 960;
	u32 stride    = width*sizeof(u32);
	u32 framerate = 30;
	u32 bitrate   = stride*height*framerate*8;

	IMFMediaType* media_type     = 0;
	IMFMediaTypeHandler* handler = 0;
	do
	{
		BREAK_IF_FAILED(result, MFCreateAttributes(&this->attributes, 0));
		BREAK_IF_FAILED(result, IMFAttributes_SetGUID(this->attributes, &MF_DEVICESTREAM_STREAM_CATEGORY, &PINNAME_VIDEO_CAPTURE));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_STREAM_ID, index));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes_Color));

		BREAK_IF_FAILED(result, MFCreateEventQueue(&this->event_queue));

		BREAK_IF_FAILED(result, MFCreateMediaType(&media_type));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((u64)width << 32) | height));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, stride));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_INTERLACE_MODE,	MFVideoInterlace_Progressive));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, (u64)framerate << 32));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_AVG_BITRATE, bitrate));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, (1ULL << 32) | 1));

		BREAK_IF_FAILED(result, MFCreateStreamDescriptor(index, 1, &media_type, &this->stream_descriptor));

		BREAK_IF_FAILED(result, IMFStreamDescriptor_GetMediaTypeHandler(this->stream_descriptor, &handler));
		BREAK_IF_FAILED(result, IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type));
	} while (0);

	if (media_type != 0) IMFMediaType_Release(media_type);
	if (handler != 0) IMFMediaTypeHandler_Release(handler);

	return result;
}

HRESULT
MediaStream__Start(Media_Stream* this, IMFMediaType* media_type)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
MediaStream__Stop(Media_Stream* this)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

void
MediaStream__Shutdown(Media_Stream* this)
{
	// NOTE: Consider logging result
	if (this->event_queue != 0) IMFMediaEventQueue_Shutdown(this->event_queue);

	MediaStream__ReleaseChildren(this);
}

HRESULT
MediaStream__SetD3DManager(Media_Stream* this, IUnknown* manager)
{
	HRESULT result;

	if (manager == 0) result = E_POINTER;
	else
	{
		result = E_NOTIMPL;
	}

	return result;
}

HRESULT
MediaStream__SetAllocator(Media_Stream* this, IUnknown* allocator)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}
