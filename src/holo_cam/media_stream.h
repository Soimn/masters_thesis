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
//ULONG MediaSource__Release(Media_Source* this);

typedef struct Media_Stream_Dynamic_State
{
	IMFAttributes* attributes;
	IMFMediaEventQueue* event_queue;
	IMFStreamDescriptor* stream_descriptor;
	MF_STREAM_STATE stream_state;
	Media_Source* parent;
	IMFVideoSampleAllocatorEx* sample_allocator;
	u32 width;
	u32 height;
	u32 fps;
	u32 port;
	SOCKET socket;
	SOCKET listen_socket;
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
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

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
		else if (IsEqualIID(riid, &WHY_MICROSOFT_IID_IKsControl))
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
	LOG_FUNCTION_ENTRY();
	Media_Stream* this = MEDIASTREAM_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

ULONG
MediaStream__AddRef(Media_Stream* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedIncrement(&this->ref_count);
	if (ref_count == 1) Log("[Holo] --- MediaStream was ressurected");

	return ref_count;
}

HRESULT
MediaStream_KsControl__AddRef(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Stream* this = MEDIASTREAM_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->AddRef(this);
}

// NOTE: Requires lock held
void
MediaStream__ReleaseChildren(Media_Stream* this)
{
	LOG_FUNCTION_ENTRY();
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

	if (this->sample_allocator != 0)
	{
		IMFVideoSampleAllocatorEx_Release(this->sample_allocator);
		this->sample_allocator = 0;
	}

	shutdown(this->listen_socket, SD_SEND);
	closesocket(this->listen_socket);
	this->listen_socket = INVALID_SOCKET;

	closesocket(this->socket);
	this->socket = INVALID_SOCKET;
}

ULONG
MediaStream__Release(Media_Stream* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedDecrement(&this->ref_count);

	if (this->ref_count == 0)
	{
		AcquireSRWLockExclusive(&this->lock);
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
		ReleaseSRWLockExclusive(&this->lock);
	}

	return ref_count;
}

ULONG
MediaStream_KsControl__Release(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Stream* this = MEDIASTREAM_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaStream__GetEvent(Media_Stream* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

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

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__BeginGetEvent(Media_Stream* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0) result = MF_E_SHUTDOWN;
	else                        result = IMFMediaEventQueue_BeginGetEvent(this->event_queue, pCallback, punkState);

	ReleaseSRWLockExclusive(&this->lock);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__EndGetEvent(Media_Stream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (ppEvent == 0) result = E_POINTER;
	else
	{
		*ppEvent = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->event_queue == 0) result = MF_E_SHUTDOWN;
		else                        result = IMFMediaEventQueue_EndGetEvent(this->event_queue, pResult, ppEvent);

		ReleaseSRWLockExclusive(&this->lock);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__QueueEvent(Media_Stream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0) result = MF_E_SHUTDOWN;
	else                        result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, met, guidExtendedType, hrStatus, pvValue);

	ReleaseSRWLockExclusive(&this->lock);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__GetMediaSource(Media_Stream* this, IMFMediaSource** ppMediaSource)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (ppMediaSource == 0) result = E_POINTER;
	else
	{
		*ppMediaSource = 0;

		if (this->parent == 0) result = MF_E_SHUTDOWN;
		else              		 result = MediaSource__QueryInterface(this->parent, &IID_IMFMediaSource, ppMediaSource);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__GetStreamDescriptor(Media_Stream* this, IMFStreamDescriptor** ppStreamDescriptor)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (ppStreamDescriptor == 0) result = E_POINTER;
	else
	{
		*ppStreamDescriptor = 0;

		AcquireSRWLockExclusive(&this->lock);
		
		if (this->stream_descriptor == 0) result = MF_E_SHUTDOWN;
		else                              result = IMFStreamDescriptor_QueryInterface(this->stream_descriptor, &IID_IMFStreamDescriptor, ppStreamDescriptor);

		ReleaseSRWLockExclusive(&this->lock);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__RequestSample(Media_Stream* this, IUnknown* pToken)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	AcquireSRWLockExclusive(&this->lock);

	if      (this->sample_allocator == 0 || this->event_queue == 0) result = MF_E_SHUTDOWN;
	else if (this->stream_state != MF_STREAM_STATE_RUNNING)         result = MF_E_INVALIDREQUEST;
	else
	{
		IMFSample* sample            = 0;
		IMFMediaBuffer* media_buffer = 0;
		IMF2DBuffer2* buffer         = 0;

		do
		{
			BREAK_IF_FAILED(result, IMFVideoSampleAllocatorEx_AllocateSample(this->sample_allocator, &sample));

			BREAK_IF_FAILED(result, IMFSample_GetBufferByIndex(sample, 0, &media_buffer));
			BREAK_IF_FAILED(result, IMFMediaBuffer_QueryInterface(media_buffer, &IID_IMF2DBuffer2, &buffer));

			BYTE* data       = 0;
			LONG pitch       = 0;
			BYTE* data_start = 0;
			DWORD data_len   = 0;
			BREAK_IF_FAILED(result, IMF2DBuffer2_Lock2DSize(buffer, MF2DBuffer_LockFlags_Write, &data, &pitch, &data_start, &data_len));

			recv(this->listen_socket, data, data_len, 0);
			send(this->listen_socket, &(char){0}, 1, 0);

			BREAK_IF_FAILED(result, IMF2DBuffer2_Unlock2D(buffer));

			BREAK_IF_FAILED(result, IMFSample_SetSampleTime(sample, MFGetSystemTime()));
			BREAK_IF_FAILED(result, IMFSample_SetSampleDuration(sample, 10000000LL/this->fps));

			if (pToken != 0)
			{
				BREAK_IF_FAILED(result, IMFSample_SetUnknown(sample, &MFSampleExtension_Token, pToken));
			}

			BREAK_IF_FAILED(result, IMFMediaEventQueue_QueueEventParamUnk(this->event_queue, MEMediaSample, &GUID_NULL, S_OK, (IUnknown*)sample));
		} while (0);

		if (buffer       != 0) IMF2DBuffer2_Release(buffer);
		if (media_buffer != 0) IMFMediaBuffer_Release(media_buffer);
		if (sample       != 0) IMFSample_Release(sample);
	}

	ReleaseSRWLockExclusive(&this->lock);

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__SetStreamState(Media_Stream* this, MF_STREAM_STATE value)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

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

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__GetStreamState(Media_Stream* this, MF_STREAM_STATE* value)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (value == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		*value = this->stream_state;

		ReleaseSRWLockExclusive(&this->lock);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream_KsControl__KsEvent(void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned)
{
	LOG_FUNCTION_ENTRY();
	return ERROR_SET_NOT_FOUND;
}

HRESULT
MediaStream_KsControl__KsMethod(void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned)
{
	LOG_FUNCTION_ENTRY();
	return ERROR_SET_NOT_FOUND;
}

HRESULT
MediaStream_KsControl__KsProperty(void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
	LOG_FUNCTION_ENTRY();
	return ERROR_SET_NOT_FOUND;
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
MediaStream__Init(Media_Stream* this, u32 index, Media_Source* parent, IMFAttributes* parent_attributes)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	IMFMediaType* media_type     = 0;
	IMFMediaTypeHandler* handler = 0;
	do
	{
		this->parent = parent;

		this->socket        = INVALID_SOCKET;
		this->listen_socket = INVALID_SOCKET;

		BREAK_IF_FAILED(result, Attributes__CreateInstance(&this->attributes));
		BREAK_IF_FAILED(result, IMFAttributes_SetGUID(this->attributes, &MF_DEVICESTREAM_STREAM_CATEGORY, &PINNAME_VIDEO_CAPTURE));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_STREAM_ID, index));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
		BREAK_IF_FAILED(result, IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes_Color));

		u64 frame_size = 0;
		BREAK_IF_FAILED(result, IMFAttributes_GetUINT64(parent_attributes, &GUID_HOLOCAM_FRAME_SIZE, &frame_size));

		this->width  = (u32)(frame_size >> 32);
		this->height = (u32)frame_size;

		BREAK_IF_FAILED(result, IMFAttributes_GetUINT32(parent_attributes, &GUID_HOLOCAM_FPS, &this->fps));
		BREAK_IF_FAILED(result, IMFAttributes_GetUINT32(parent_attributes, &GUID_HOLOCAM_PORT, &this->port));

		BREAK_IF_FAILED(result, MFCreateEventQueue(&this->event_queue));

		BREAK_IF_FAILED(result, MFCreateMediaType(&media_type));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_type,   &MF_MT_MAJOR_TYPE,              &MFMediaType_Video));
		BREAK_IF_FAILED(result, IMFMediaType_SetGUID(media_type,   &MF_MT_SUBTYPE,                 &MFVideoFormat_RGB32));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_INTERLACE_MODE,	         MFVideoInterlace_Progressive));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE,              U64_HI_LO(this->width, this->height)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE,          this->width*4));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE,              U64_HI_LO(this->fps, 1)));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT32(media_type, &MF_MT_AVG_BITRATE,             this->width*this->height*4*8*this->fps));
		BREAK_IF_FAILED(result, IMFMediaType_SetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO,      U64_HI_LO(1, 1)));

		//BREAK_IF_FAILED(result, MFCreateStreamDescriptor(index, 1, &media_type, &this->stream_descriptor));
		BREAK_IF_FAILED(result, StreamDescriptor__CreateInstance(index, 1, &media_type, &this->stream_descriptor));

		BREAK_IF_FAILED(result, IMFStreamDescriptor_GetMediaTypeHandler(this->stream_descriptor, &handler));
		BREAK_IF_FAILED(result, IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type));

		/*
		BREAK_IF_FAILED(result, IMFStreamDescriptor_SetGUID(this->stream_descriptor, &MF_DEVICESTREAM_STREAM_CATEGORY, &PINNAME_VIDEO_CAPTURE));
		BREAK_IF_FAILED(result, IMFStreamDescriptor_SetUINT32(this->stream_descriptor, &MF_DEVICESTREAM_STREAM_ID, index));
		BREAK_IF_FAILED(result, IMFStreamDescriptor_SetUINT32(this->stream_descriptor, &MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
		BREAK_IF_FAILED(result, IMFStreamDescriptor_SetUINT32(this->stream_descriptor, &MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes_Color));
		*/
	} while (0);

	if (media_type != 0) IMFMediaType_Release(media_type);
	if (handler != 0) IMFMediaTypeHandler_Release(handler);

	return result;
}

HRESULT
MediaStream__Start(Media_Stream* this, IMFMediaType* media_type)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (media_type == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		result = MediaStream__StartInternal(this, media_type, true);

		ReleaseSRWLockExclusive(&this->lock);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__StartInternal(Media_Stream* this, IMFMediaType* media_type, bool send_event)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (this->event_queue == 0 || this->sample_allocator == 0) result = MF_E_SHUTDOWN;
	else
	{
		do
		{
			if (media_type != 0)
			{
				GUID format;
				BREAK_IF_FAILED(result, IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &format));

				u64 frame_size;
				BREAK_IF_FAILED(result, IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &frame_size));

				BREAK_IF_FAILED(result, IMFVideoSampleAllocatorEx_InitializeSampleAllocator(this->sample_allocator, 10, media_type));

				if (!IsEqualGUID(&format, &MFVideoFormat_RGB32) || (u32)(frame_size >> 32) != this->width || (u32)frame_size != this->height)
				{
					result = MF_E_UNSUPPORTED_FORMAT;
					break;
				}
			}

			if (this->listen_socket == INVALID_SOCKET)
			{
				char port_string[6] = {
					'0' + (this->port/10000) % 10,
					'0' + (this->port/1000) % 10,
					'0' + (this->port/100) % 10,
					'0' + (this->port/10) % 10,
					'0' + (this->port/1) % 10,
					0,
				};

				struct addrinfo* info;
				struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_TCP, .ai_flags = AI_PASSIVE };
				if (getaddrinfo(0, port_string, &hints, &info) != 0) result = E_FAIL;
				else
				{
					this->socket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
					if (this->socket == INVALID_SOCKET) result = E_FAIL;
					else
					{
						if (bind(this->socket, info->ai_addr, (int)info->ai_addrlen) == SOCKET_ERROR) result = E_FAIL;
						else
						{
							Log("bound");
							if (listen(this->socket, 1) == SOCKET_ERROR) result = E_FAIL;
							else
							{
								Log("listen");
							}
						}

						if (!SUCCEEDED(result))
						{
							closesocket(this->socket);
							this->socket = INVALID_SOCKET;
						}
					}

					freeaddrinfo(info);
				}
				if (!SUCCEEDED(result)) break;
				this->listen_socket = accept(this->socket, 0, 0);

				if (this->listen_socket == INVALID_SOCKET)
				{
					closesocket(this->socket);
					result = E_FAIL;
					break;
				}
			}

			BREAK_IF_FAILED(result, IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MEStreamStarted, &GUID_NULL, S_OK, 0));

			this->stream_state = MF_STREAM_STATE_RUNNING;
		} while (0);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__Stop(Media_Stream* this, bool send_event)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

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

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__StopInternal(Media_Stream* this, bool send_event)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = S_OK;

	this->stream_state = MF_STREAM_STATE_STOPPED;
		
	if (send_event)
	{
		result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MEStreamStopped, &GUID_NULL, S_OK, 0);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

void
MediaStream__Shutdown(Media_Stream* this)
{
	LOG_FUNCTION_ENTRY();
	// NOTE: Consider logging result
	if (this->event_queue != 0) IMFMediaEventQueue_Shutdown(this->event_queue);

	MediaStream__ReleaseChildren(this);
}

HRESULT
MediaStream__SetD3DManager(Media_Stream* this, IUnknown* manager)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (manager == 0) result = E_POINTER;
	else
	{
		/*
		result = IMFVideoSampleAllocatorEx_SetDirectXManager(this->sample_allocator, manager);
		if (SUCCEEDED(result))
		{
			FrameGenerator_SetD3DManager(&this->frame_generator, manager, TEMP_WIDTH, TEMP_HEIGHT);
		}
		*/
		result = E_NOTIMPL;
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}

HRESULT
MediaStream__SetAllocator(Media_Stream* this, IUnknown* allocator)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = E_FAIL;

	if (allocator == 0) result = E_POINTER;
	else
	{
		if (this->sample_allocator != 0) IMFVideoSampleAllocatorEx_Release(this->sample_allocator);
		result = IMFVideoSampleAllocatorEx_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorEx, &this->sample_allocator);
	}

	LOG_FUNCTION_RESULT(result);
	return result;
}
