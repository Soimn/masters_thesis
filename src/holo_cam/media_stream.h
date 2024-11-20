typedef struct IHoloCamMediaStream IHoloCamMediaStream;
typedef struct IHoloCamMediaStreamVtbl
{
	// IUnknown
	HRESULT (*QueryInterface) (IHoloCamMediaStream* this, REFIID riid, void** ppvObject);
	ULONG (*AddRef)           (IHoloCamMediaStream* this);
	ULONG (*Release)          (IHoloCamMediaStream* this);

	// IMFMediaEventGenerator
	HRESULT (*GetEvent)      (IHoloCamMediaStream* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*BeginGetEvent) (IHoloCamMediaStream* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)   (IHoloCamMediaStream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)    (IHoloCamMediaStream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaStream
	HRESULT (*GetMediaSource)      (IHoloCamMediaStream* this, IMFMediaSource** ppMediaSource);
	HRESULT (*GetStreamDescriptor) (IHoloCamMediaStream* this, IMFStreamDescriptor** ppStreamDescriptor);
	HRESULT (*RequestSample)       (IHoloCamMediaStream* this, IUnknown* pToken);

	// IMFMediaStream2
	HRESULT (*SetStreamState) (IHoloCamMediaStream* this, MF_STREAM_STATE value);
	HRESULT (*GetStreamState) (IHoloCamMediaStream* this, MF_STREAM_STATE* value);
} IHoloCamMediaStreamVtbl;

typedef struct IHoloCamMediaStream
{
	IHoloCamMediaStreamVtbl* lpVtbl;
	u32 ref_count;
	SRWLOCK lock;
	u32 stream_id;
	IMFAttributes* attributes;
	IMFMediaType* media_types[2];
	IMFMediaEventQueue* event_queue;
	IMFStreamDescriptor* stream_descriptor;
} IHoloCamMediaStream;

HRESULT
IHoloCamMediaStream__QueryInterface(IHoloCamMediaStream* this, REFIID riid, void** ppvObject)
{
	HRESULT result;

	if (ppvObject == 0) result = E_POINTER;
	else
	{
		*ppvObject = 0;

		if (!IsEqualIID(riid, &IID_IUnknown)               &&
				!IsEqualIID(riid, &IID_IMFMediaEventGenerator) &&
				!IsEqualIID(riid, &IID_IMFMediaStream)         &&
				!IsEqualIID(riid, &IID_IMFMediaStream2))
		{
			result = E_NOINTERFACE;
		}
		else
		{
			*ppvObject = this;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
	}

	return result;
}

ULONG
IHoloCamMediaStream__AddRef(IHoloCamMediaStream* this)
{
	AcquireSRWLockExclusive(&this->lock);
	this->ref_count += 1;
	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

// NOTE: Requires lock held
void
IHoloCamMediaStream__ReleaseChildren(IHoloCamMediaStream* this)
{
	if (this->attributes != 0)
	{
		IMFAttributes_Release(this->attributes);
		this->attributes = 0;
	}

	for (u32 i = 0; i < ARRAY_LEN(this->media_types); ++i)
	{
		if (this->media_types[i] != 0)
		{
			IMFMediaType_Release(this->media_types[i]);
			this->media_types[i] = 0;
		}
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
}

ULONG
IHoloCamMediaStream__Release(IHoloCamMediaStream* this)
{
	AcquireSRWLockExclusive(&this->lock);

	if (this->ref_count != 0)
	{
		this->ref_count -= 1;
		if (this->ref_count == 0) IHoloCamMediaStream__ReleaseChildren(this);
	}

	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
}

HRESULT
IHoloCamMediaStream__GetEvent(IHoloCamMediaStream* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__GetEvent\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__BeginGetEvent(IHoloCamMediaStream* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__BeginGetEvent\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__EndGetEvent(IHoloCamMediaStream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__EndGetEvent\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__QueueEvent(IHoloCamMediaStream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__QueueEvent\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__GetMediaSource(IHoloCamMediaStream* this, IMFMediaSource** ppMediaSource)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__GetMediaSource\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__GetStreamDescriptor(IHoloCamMediaStream* this, IMFStreamDescriptor** ppStreamDescriptor)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__GetStreamDescriptor\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__RequestSample(IHoloCamMediaStream* this, IUnknown* pToken)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__RequestSample\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__SetStreamState(IHoloCamMediaStream* this, MF_STREAM_STATE value)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__SetStreamState\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream__GetStreamState(IHoloCamMediaStream* this, MF_STREAM_STATE* value)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream__GetStreamState\n");
	return E_NOTIMPL;
}

static IHoloCamMediaStreamVtbl IHoloCamMediaStream_Vtbl = {
	.QueryInterface      = IHoloCamMediaStream__QueryInterface,
	.AddRef              = IHoloCamMediaStream__AddRef,
	.Release             = IHoloCamMediaStream__Release,
	.GetEvent            = IHoloCamMediaStream__GetEvent,
	.BeginGetEvent       = IHoloCamMediaStream__BeginGetEvent,
	.EndGetEvent         = IHoloCamMediaStream__EndGetEvent,
	.QueueEvent          = IHoloCamMediaStream__QueueEvent,
	.GetMediaSource      = IHoloCamMediaStream__GetMediaSource,
	.GetStreamDescriptor = IHoloCamMediaStream__GetStreamDescriptor,
	.RequestSample       = IHoloCamMediaStream__RequestSample,
	.SetStreamState      = IHoloCamMediaStream__SetStreamState,
	.GetStreamState      = IHoloCamMediaStream__GetStreamState,
};

HRESULT
IHoloCamMediaStream__Init(IHoloCamMediaStream* this, u32 stream_id)
{
	*this = (IHoloCamMediaStream){
		.lpVtbl    = &IHoloCamMediaStream_Vtbl,
		.ref_count = 1,
		.lock      = (SRWLOCK)SRWLOCK_INIT,
		.stream_id = stream_id,
	};

	// TODO: Make dynamic
	u32 width  = 1920;
	u32 height = 1080;
	u32 fps    = 30;

	RET_IF_FAIL(MFCreateMediaType(&this->media_types[0]));
	RET_IF_FAIL(IMFMediaType_SetGUID(this->media_types[0], &MF_MT_MAJOR_TYPE, &MFMediaType_Video));
	RET_IF_FAIL(IMFMediaType_SetGUID(this->media_types[0], &MF_MT_SUBTYPE, &MFVideoFormat_YUY2));
	RET_IF_FAIL(IMFMediaType_SetUINT32(this->media_types[0], &MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	RET_IF_FAIL(IMFMediaType_SetUINT32(this->media_types[0], &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
	RET_IF_FAIL(IMFMediaType_SetUINT64(this->media_types[0], &MF_MT_FRAME_SIZE, ((u64)width << 32) | height));
	RET_IF_FAIL(IMFMediaType_SetUINT64(this->media_types[0], &MF_MT_FRAME_RATE, ((u64)fps << 32) | 1));
	RET_IF_FAIL(IMFMediaType_SetUINT32(this->media_types[0], &MF_MT_AVG_BITRATE, width*height*fps*16)); // NOTE: YUY2 is 16bpp
	RET_IF_FAIL(IMFMediaType_SetUINT64(this->media_types[0], &MF_MT_PIXEL_ASPECT_RATIO, ((u64)1 << 32) | 1));

	RET_IF_FAIL(MFCreateMediaType(&this->media_types[1]));
	RET_IF_FAIL(IMFMediaType_SetGUID(this->media_types[1], &MF_MT_MAJOR_TYPE, &MFMediaType_Video));
	RET_IF_FAIL(IMFMediaType_SetGUID(this->media_types[1], &MF_MT_SUBTYPE, &MFVideoFormat_RGB32));
	RET_IF_FAIL(IMFMediaType_SetUINT32(this->media_types[1], &MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	RET_IF_FAIL(IMFMediaType_SetUINT32(this->media_types[1], &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
	RET_IF_FAIL(IMFMediaType_SetUINT64(this->media_types[1], &MF_MT_FRAME_SIZE, ((u64)width << 32) | height));
	RET_IF_FAIL(IMFMediaType_SetUINT64(this->media_types[1], &MF_MT_FRAME_RATE, ((u64)fps << 32) |1));
	RET_IF_FAIL(IMFMediaType_SetUINT32(this->media_types[1], &MF_MT_AVG_BITRATE, width*height*fps*32)); // NOTE: RGB32 is 32bpp
	RET_IF_FAIL(IMFMediaType_SetUINT64(this->media_types[1], &MF_MT_PIXEL_ASPECT_RATIO, ((u64)1 << 32) | 1));

	RET_IF_FAIL(MFCreateAttributes(&this->attributes, 10));
	RET_IF_FAIL(IMFAttributes_SetGUID(this->attributes, &MF_DEVICESTREAM_STREAM_CATEGORY, &PINNAME_VIDEO_CAPTURE));
	RET_IF_FAIL(IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_STREAM_ID, this->stream_id));
	RET_IF_FAIL(IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
	RET_IF_FAIL(IMFAttributes_SetUINT32(this->attributes, &MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes_Color));

	RET_IF_FAIL(MFCreateEventQueue(&this->event_queue));

	RET_IF_FAIL(MFCreateStreamDescriptor(this->stream_id, ARRAY_LEN(this->media_types), this->media_types, &this->stream_descriptor));
	
	IMFMediaTypeHandler* media_type_handler;
	RET_IF_FAIL(IMFStreamDescriptor_GetMediaTypeHandler(this->stream_descriptor, &media_type_handler));
	RET_IF_FAIL(IMFMediaTypeHandler_SetCurrentMediaType(media_type_handler, this->media_types[0]));
	IMFMediaTypeHandler_Release(media_type_handler);

	RET_IF_FAIL(IMFStreamDescriptor_SetGUID(this->stream_descriptor, &MF_DEVICESTREAM_STREAM_CATEGORY, &PINNAME_VIDEO_CAPTURE));
	RET_IF_FAIL(IMFStreamDescriptor_SetUINT32(this->stream_descriptor, &MF_DEVICESTREAM_STREAM_ID, this->stream_id));
	RET_IF_FAIL(IMFStreamDescriptor_SetUINT32(this->stream_descriptor, &MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
	RET_IF_FAIL(IMFStreamDescriptor_SetUINT32(this->stream_descriptor, &MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, MFFrameSourceTypes_Color));

	return S_OK;
}

HRESULT
IHoloCamMediaStream_Start(IHoloCamMediaStream* this, IMFMediaType* media_type)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream_Stop\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream_Stop(IHoloCamMediaStream* this)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream_Stop\n");
	return E_NOTIMPL;
}

HRESULT
IHoloCamMediaStream_Shutdown(IHoloCamMediaStream* this)
{
	// TODO
	OutputDebugStringA("IHoloCamMediaStream_Stop\n");
	return E_NOTIMPL;
}