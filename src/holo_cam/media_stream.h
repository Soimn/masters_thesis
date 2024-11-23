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
	u32 queue_id;
	bool has_shutdown;
	MF_STREAM_STATE state;
	IMFMediaSource* parent;
	IMFMediaStream* stream;
	IMFMediaEventQueue* event_queue;
	IMFStreamDescriptor* stream_descriptor;
	IHoloCamCallback callback;
} IHoloCamMediaStream;

HRESULT
IHoloCamMediaStream__QueryInterface(IHoloCamMediaStream* this, REFIID riid, void** ppvObject)
{
	HRESULT result;

	if (ppvObject == 0) result = E_POINTER;
	else
	{
		*ppvObject = 0;

		if (IsEqualIID(riid, &IID_IUnknown)               ||
				IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
				IsEqualIID(riid, &IID_IMFMediaStream)         ||
				IsEqualIID(riid, &IID_IMFMediaStream2))
		{
			*ppvObject = this;
			this->lpVtbl->AddRef(this);
			result = S_OK;
		}
		else result = E_NOINTERFACE;
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
	if (this->stream != 0)
	{
		IMFMediaStream_Release(this->stream);
		this->stream = 0;
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
	HRESULT result;

	IMFMediaEventQueue* queue;
	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else                    result = IMFMediaEventQueue_QueryInterface(this->event_queue, &IID_IMFMediaEventQueue, &queue);

	ReleaseSRWLockExclusive(&this->lock);

	if (result == S_OK)
	{
		result = IMFMediaEventQueue_GetEvent(queue, dwFlags, ppEvent);
		IMFMediaEventQueue_Release(queue);
	}

	return result;
}

HRESULT
IHoloCamMediaStream__BeginGetEvent(IHoloCamMediaStream* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else                    result = IMFMediaEventQueue_BeginGetEvent(this->event_queue, pCallback, punkState);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream__EndGetEvent(IHoloCamMediaStream* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else                    result = IMFMediaEventQueue_EndGetEvent(this->event_queue, pResult, ppEvent);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream__QueueEvent(IHoloCamMediaStream* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else                    result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, met, guidExtendedType, hrStatus, pvValue);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream__GetMediaSource(IHoloCamMediaStream* this, IMFMediaSource** ppMediaSource)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else                    result = IMFMediaSource_QueryInterface(this->parent, &IID_IMFMediaSource, ppMediaSource);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream__GetStreamDescriptor(IHoloCamMediaStream* this, IMFStreamDescriptor** ppStreamDescriptor)
{
	HRESULT result;

	if (ppStreamDescriptor == 0) result = E_POINTER;
	else
	{
		*ppStreamDescriptor = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (this->has_shutdown) result = MF_E_SHUTDOWN;
		else                    result = IMFStreamDescriptor_QueryInterface(this->stream_descriptor, &IID_IMFStreamDescriptor, ppStreamDescriptor);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaStream__RequestSample(IHoloCamMediaStream* this, IUnknown* pToken)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else                    result = IMFMediaStream_RequestSample(this->stream, pToken);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream__SetStreamState(IHoloCamMediaStream* this, MF_STREAM_STATE value)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else
	{
		IMFMediaStream2* media_stream_2;
		result = IMFMediaStream_QueryInterface(this->stream, &IID_IMFMediaStream2, &media_stream_2);
		if (SUCCEEDED(result))
		{
			result = IMFMediaStream2_SetStreamState(media_stream_2, value);
			IMFMediaStream2_Release(media_stream_2);
		}
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream__GetStreamState(IHoloCamMediaStream* this, MF_STREAM_STATE* value)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->has_shutdown) result = MF_E_SHUTDOWN;
	else
	{
		IMFMediaStream2* media_stream_2;
		result = IMFMediaStream_QueryInterface(this->stream, &IID_IMFMediaStream2, &media_stream_2);
		if (SUCCEEDED(result))
		{
			result = IMFMediaStream2_GetStreamState(media_stream_2, value);
			IMFMediaStream2_Release(media_stream_2);
		}
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
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

HRESULT IHoloCamMediaStream__ProcessSample(IHoloCamMediaStream* this, IMFSample* sample);

void
IHoloCamMediaStream__OnMediaStreamEvent(IHoloCamMediaStream* this, IMFAsyncResult* pAsyncResult)
{
	IMFMediaEvent* event         = 0;
	IMFMediaStream* media_stream = 0;

	bool should_forward = true;

	IUnknown* unknown_result;
	HRESULT result = IMFAsyncResult_GetState(pAsyncResult, &unknown_result);
	if (SUCCEEDED(result))
	{
		result = IUnknown_QueryInterface(unknown_result, &IID_IMFMediaStream, &media_stream);
		IUnknown_Release(unknown_result);

		if (SUCCEEDED(result)) result = IMFMediaStream_EndGetEvent(media_stream, pAsyncResult, &event);
		if (SUCCEEDED(result))
		{
			MediaEventType event_type = MEUnknown;
			result = IMFMediaEvent_GetType(event, &event_type);
			if (SUCCEEDED(result))
			{
				if (event_type == MEMediaSample)
				{
					PROPVARIANT prop_var = {0};
					result = IMFMediaEvent_GetValue(event, &prop_var);
					if (SUCCEEDED(result))
					{
						if (prop_var.vt != VT_UNKNOWN) result = MF_E_UNEXPECTED;
						else
						{
							IMFSample* sample;
							result = IUnknown_QueryInterface(prop_var.punkVal, &IID_IMFSample, &sample);
							if (SUCCEEDED(result))
							{
								result = IHoloCamMediaStream__ProcessSample(this, sample);
								should_forward = false;
								IMFSample_Release(sample);
							}
						}
					}
				}
			}
		}
	}

	AcquireSRWLockExclusive(&this->lock);
	
	if (SUCCEEDED(result) && !this->has_shutdown)
	{
		if (should_forward) IMFMediaEventQueue_QueueEvent(this->event_queue, event);
		IMFMediaStream_BeginGetEvent(media_stream, (IMFAsyncCallback*)&this->callback, (IUnknown*)this->stream);
	}

	// TODO: consider propagating error

	ReleaseSRWLockExclusive(&this->lock);

	if (event != 0) IMFMediaEvent_Release(event);
}

HRESULT
IHoloCamMediaStream__ProcessSample(IHoloCamMediaStream* this, IMFSample* sample)
{
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (!this->has_shutdown)
	{
		// TODO: processing

		result = IMFMediaEventQueue_QueueEventParamUnk(this->event_queue, MEMediaSample, &GUID_NULL, S_OK, (IUnknown*)sample);
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaStream_Shutdown(IHoloCamMediaStream* this)
{
	AcquireSRWLockExclusive(&this->lock);

	this->has_shutdown = true;

	IMFMediaEventQueue_Shutdown(this->event_queue);

	IHoloCamMediaStream__ReleaseChildren(this);

	ReleaseSRWLockExclusive(&this->lock);

	return S_OK;
}

HRESULT
IHoloCamMediaStream_SetMediaStream(IHoloCamMediaStream* this, IMFMediaStream* stream)
{
	HRESULT result;

	if (stream == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		if (this->has_shutdown) result = MF_E_SHUTDOWN;
		else
		{
			IMFMediaStream_Release(this->stream);

			result = IMFMediaStream_QueryInterface(stream, &IID_IMFMediaStream, &this->stream);
			if (SUCCEEDED(result)) result = IMFMediaStream_BeginGetEvent(this->stream, (IMFAsyncCallback*)&this->callback, (IUnknown*)this->stream);
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

// TODO: Should the parent pointers reference count be increased?
//       It shouldn't matter since the stream is always destroyed before the source, but still.
//       The problem is if this causes a ref count deadlock somehow.
HRESULT
IHoloCamMediaStream__Init(IHoloCamMediaStream* this, IMFMediaSource* parent, IMFStreamDescriptor* stream_descriptor, u32 queue_id)
{
	*this = (IHoloCamMediaStream){
		.lpVtbl       = &IHoloCamMediaStream_Vtbl,
		.ref_count    = 1,
		.lock         = (SRWLOCK)SRWLOCK_INIT,
		.queue_id     = queue_id,
		.has_shutdown = false,
		.state        = MF_STREAM_STATE_STOPPED,
		.parent       = parent,
	};

	RET_IF_FAIL(MFCreateEventQueue(&this->event_queue));
	RET_IF_FAIL(IMFStreamDescriptor_QueryInterface(stream_descriptor, &IID_IMFStreamDescriptor, &this->stream_descriptor));
	RET_IF_FAIL(IMFStreamDescriptor_GetStreamIdentifier(this->stream_descriptor, &this->stream_id));

	IHoloCamCallback__Init(&this->callback, this, (void (*)(void*, IMFAsyncResult*))&IHoloCamMediaStream__OnMediaStreamEvent, queue_id);

	return S_OK;
}
