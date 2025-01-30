#define TEMP_WIDTH 1280
#define TEMP_HEIGHT 960
#define TEMP_FPS 30

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

typedef struct Media_Source Media_Source;
HRESULT MediaSource__QueryInterface(Media_Source* this, REFIID riid, void** ppvObject);
ULONG MediaSource__Release(Media_Source* this);

typedef struct Media_Stream_Dynamic_State
{
	IMFAttributes* attributes;
	IMFMediaEventQueue* event_queue;
	IMFStreamDescriptor* stream_descriptor;
	MF_STREAM_STATE stream_state;
	Media_Source* parent;
	IMFVideoSampleAllocatorEx* sample_allocator;
	GUID format;
	Frame_Generator frame_generator;
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

HRESULT MediaStream__StartInternal(Media_Stream* this, IMFMediaType* media_type, bool send_event);
HRESULT MediaStream__StopInternal(Media_Stream* this, bool send_event);

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

	if (this->stream_descriptor != 0)
	{
		IMFStreamDescriptor_Release(this->stream_descriptor);
		this->stream_descriptor = 0;
	}

	if (this->parent != 0)
	{
		MediaSource__Release(this->parent);
		this->parent = 0;
	}

	if (this->sample_allocator != 0)
	{
		IMFVideoSampleAllocatorEx_Release(this->sample_allocator);
		this->sample_allocator = 0;
	}

	FrameGenerator_Teardown(&this->frame_generator);
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

	if (ppEvent == 0) result = E_POINTER;
	else
	{
		*ppEvent = 0;

		IMFMediaEventQueue* event_queue = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->event_queue == 0) result = MF_E_SHUTDOWN;
		else
		{
			result = IMFMediaEventQueue_QueryInterface(this->event_queue, &IID_IMFMediaEventQueue, &event_queue);
		}

		ReleaseSRWLockExclusive(&this->lock);

		if (SUCCEEDED(result))
		{
			result = IMFMediaEventQueue_GetEvent(event_queue, dwFlags, ppEvent);
		}

		if (event_queue != 0) IMFMediaEventQueue_Release(event_queue);
	}

	return result;
}

HRESULT
MediaStream__BeginGetEvent(Media_Stream* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0) result = MF_E_SHUTDOWN;
	else                        result = IMFMediaEventQueue_BeginGetEvent(this->event_queue, pCallback, punkState);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
MediaStream__EndGetEvent(Media_Stream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	if (ppEvent == 0) result = E_POINTER;
	else
	{
		*ppEvent = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->event_queue == 0) result = MF_E_SHUTDOWN;
		else                        result = IMFMediaEventQueue_EndGetEvent(this->event_queue, pResult, ppEvent);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaStream__QueueEvent(Media_Stream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0) result = MF_E_SHUTDOWN;
	else                        result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, met, guidExtendedType, hrStatus, pvValue);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
MediaStream__GetMediaSource(Media_Stream* this, IMFMediaSource** ppMediaSource)
{
	HRESULT result;

	if (ppMediaSource == 0) result = E_POINTER;
	else
	{
		*ppMediaSource = 0;

		if (this->parent == 0) result = MF_E_SHUTDOWN;
		else              		 result = MediaSource__QueryInterface(this->parent, &IID_IMFMediaSource, ppMediaSource);
	}

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

	AcquireSRWLockExclusive(&this->lock);

	if (this->sample_allocator == 0 || this->event_queue == 0) result = MF_E_SHUTDOWN;
	else
	{
		IMFSample* sample           = 0;
		IMFSample* generated_sample = 0;

		do
		{
			BREAK_IF_FAILED(result, IMFVideoSampleAllocatorEx_AllocateSample(this->sample_allocator, &sample));
			BREAK_IF_FAILED(result, IMFSample_SetSampleTime(sample, MFGetSystemTime()));
			BREAK_IF_FAILED(result, IMFSample_SetSampleDuration(sample, 10000000LL/TEMP_FPS));

			BREAK_IF_FAILED(result, FrameGenerator_Generate(&this->frame_generator, sample, &this->format, &generated_sample));

			if (pToken != 0)
			{
				BREAK_IF_FAILED(result, IMFSample_SetUnknown(generated_sample, &MFSampleExtension_Token, pToken));
			}

			BREAK_IF_FAILED(result, IMFMediaEventQueue_QueueEventParamUnk(this->event_queue, MEMediaSample, 0, S_OK, (IUnknown*)generated_sample));
		} while (0);

		if (sample           != 0) IMFSample_Release(sample);
		if (generated_sample != 0) IMFSample_Release(generated_sample);
	}

	ReleaseSRWLockExclusive(&this->lock);

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
			result = MediaStream__StartInternal(this, 0, false);
		}
		else if (value == MF_STREAM_STATE_STOPPED)
		{
			result = MediaStream__StopInternal(this, false);
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
MediaStream__Init(Media_Stream* this, u32 index, Media_Source* parent)
{
	HRESULT result;

	// TODO
	u32 width     = TEMP_WIDTH;
	u32 height    = TEMP_HEIGHT;
	u32 framerate = TEMP_FPS;

	IMFMediaType* media_types[2] = {0};
	IMFMediaTypeHandler* handler = 0;
	do
	{
		FrameGenerator_Init(&this->frame_generator);

		BREAK_IF_FAILED(result, MediaSource__QueryInterface(parent, &IID_IMFMediaSource, &this->parent));

		BREAK_IF_FAILED(result, MFCreateAttributes(&this->attributes, 0));
		BREAK_IF_FAILED(result, IMFAttributes_SetGUID(this->attributes, &MF_DEVICESTREAM_STREAM_CATEGORY, &PINNAME_VIDEO_CAPTURE));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_STREAM_ID, index));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes_Color));

		BREAK_IF_FAILED(result, MFCreateEventQueue(&this->event_queue));

		BREAK_IF_FAILED(result, MFCreateMediaType(&media_types[0]));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_types[0],   &MF_MT_MAJOR_TYPE,              &MFMediaType_Video));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_types[0],   &MF_MT_SUBTYPE,                 &MFVideoFormat_RGB32));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[0], &MF_MT_INTERLACE_MODE,	         MFVideoInterlace_Progressive));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[0], &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_types[0], &MF_MT_FRAME_SIZE,              U64_HI_LO(width, height)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[0], &MF_MT_DEFAULT_STRIDE,          width*4));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_types[0], &MF_MT_FRAME_RATE,              U64_HI_LO(framerate, 1)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[0], &MF_MT_AVG_BITRATE,             width*height*4*8*framerate));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_types[0], &MF_MT_PIXEL_ASPECT_RATIO,      U64_HI_LO(1, 1)));

		BREAK_IF_FAILED(result, MFCreateMediaType(&media_types[1]));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_types[1],   &MF_MT_MAJOR_TYPE,              &MFMediaType_Video));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_types[1],   &MF_MT_SUBTYPE,                 &MFVideoFormat_NV12));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[1], &MF_MT_INTERLACE_MODE,          MFVideoInterlace_Progressive));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[1], &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_types[1], &MF_MT_FRAME_SIZE,              U64_HI_LO(width, height)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[1], &MF_MT_DEFAULT_STRIDE,          (u32)(width*1.5)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_types[1], &MF_MT_FRAME_RATE,              U64_HI_LO(framerate, 1)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_types[1], &MF_MT_AVG_BITRATE,             width*height*12*framerate));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_types[1], &MF_MT_PIXEL_ASPECT_RATIO,      U64_HI_LO(1, 1)));

		BREAK_IF_FAILED(result, MFCreateStreamDescriptor(index, ARRAY_LEN(media_types), &media_types[0], &this->stream_descriptor));

		BREAK_IF_FAILED(result, IMFStreamDescriptor_GetMediaTypeHandler(this->stream_descriptor, &handler));
		BREAK_IF_FAILED(result, IMFMediaTypeHandler_SetCurrentMediaType(handler, media_types[0]));
	} while (0);

	for (umm i = 0; i < ARRAY_LEN(media_types); ++i)
	{
		if (media_types[i] != 0) IMFMediaType_Release(media_types[i]);
	}

	if (handler != 0) IMFMediaTypeHandler_Release(handler);

	return result;
}

HRESULT
MediaStream__Start(Media_Stream* this, IMFMediaType* media_type)
{
	HRESULT result;

	if (media_type == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		result = MediaStream__StartInternal(this, media_type, true);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaStream__StartInternal(Media_Stream* this, IMFMediaType* media_type, bool send_event)
{
	HRESULT result;

	if (this->event_queue || this->sample_allocator == 0) result = MF_E_SHUTDOWN;
	else
	{
		do
		{
			if (media_type != 0)
			{
				BREAK_IF_FAILED(result, IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &this->format));
			}

			BREAK_IF_FAILED(result, FrameGenerator_EnsureRenderTarget(&this->frame_generator, TEMP_WIDTH, TEMP_HEIGHT));

			BREAK_IF_FAILED(result, IMFVideoSampleAllocatorEx_InitializeSampleAllocator(this->sample_allocator, 10, media_type));

			BREAK_IF_FAILED(result, IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MEStreamStarted, 0, S_OK, 0));

			this->stream_state = MF_STREAM_STATE_RUNNING;
		} while (0);
	}

	return result;
}

HRESULT
MediaStream__Stop(Media_Stream* this, bool send_event)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0 || this->sample_allocator == 0) result = MF_E_SHUTDOWN;
	else
	{
		result = MediaStream__StopInternal(this, send_event);

		if (SUCCEEDED(result))
		{
			if (this->sample_allocator != 0)
			{
				result = IMFVideoSampleAllocatorEx_UninitializeSampleAllocator(this->sample_allocator);
			}
		}
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
MediaStream__StopInternal(Media_Stream* this, bool send_event)
{
	HRESULT result;

	this->stream_state = MF_STREAM_STATE_STOPPED;
		
	if (send_event)
	{
		result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MEStreamStopped, 0, S_OK, 0);
	}

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
		result = IMFVideoSampleAllocatorEx_SetDirectXManager(this->sample_allocator, manager);
		if (SUCCEEDED(result))
		{
			FrameGenerator_SetD3DManager(&this->frame_generator, manager, TEMP_WIDTH, TEMP_HEIGHT);
		}
	}

	return result;
}

HRESULT
MediaStream__SetAllocator(Media_Stream* this, IUnknown* allocator)
{
	HRESULT result;

	if (allocator == 0) result = E_POINTER;
	else
	{
		if (this->sample_allocator != 0) IMFVideoSampleAllocatorEx_Release(this->sample_allocator);
		result = IMFVideoSampleAllocatorEx_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorEx, &this->sample_allocator);
	}

	return result;
}
