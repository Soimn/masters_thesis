typedef struct IHoloCamMediaSource IHoloCamMediaSource;
typedef struct IHoloCamMediaSourceVtbl
{
	// IUknown
	HRESULT (*QueryInterface) (IHoloCamMediaSource* this, REFIID riid, void** handle);
	ULONG   (*AddRef)         (IHoloCamMediaSource* this);
	ULONG   (*Release)        (IHoloCamMediaSource* this);

	// IMFMediaEventGenerator
	HRESULT (*GetEvent)                     (IHoloCamMediaSource* this, DWORD dwFlags, IMFMediaEvent** ppEvent);
	HRESULT (*BeginGetEvent)                (IHoloCamMediaSource* this, IMFAsyncCallback* pCallback, IUnknown* punkState);
	HRESULT (*EndGetEvent)                  (IHoloCamMediaSource* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	HRESULT (*QueueEvent)                   (IHoloCamMediaSource* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

	// IMFMediaSource
	HRESULT (*GetCharacteristics)           (IHoloCamMediaSource* this, DWORD* pdwCharacteristics);
	HRESULT (*CreatePresentationDescriptor) (IHoloCamMediaSource* this, IMFPresentationDescriptor** ppPresentationDescriptor);
	HRESULT (*Start)                        (IHoloCamMediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition);
	HRESULT (*Stop)                         (IHoloCamMediaSource* this);
	HRESULT (*Pause)                        (IHoloCamMediaSource* this);
	HRESULT (*Shutdown)                     (IHoloCamMediaSource* this);

	// IMFMediaSourceEx
	HRESULT (*GetSourceAttributes)          (IHoloCamMediaSource* this, IMFAttributes** ppAttributes);
	HRESULT (*GetStreamAttributes)          (IHoloCamMediaSource* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes);
	HRESULT (*SetD3DManager)                (IHoloCamMediaSource* this, IUnknown* pManager);
} IHoloCamMediaSourceVtbl;

typedef enum IHoloCamMediaSource_State
{
	IHoloCamMediaSourceState_Shutdown = 0,
	IHoloCamMediaSourceState_Stopped  = 1,
	IHoloCamMediaSourceState_Started  = 2,
} IHoloCamMediaSource_State;

typedef struct IHoloCamMediaSource
{
	IHoloCamMediaSourceVtbl* lpVtbl;
	u32 ref_count;
	SRWLOCK lock;
	IHoloCamMediaSource_State state;
	IMFAttributes* attributes;
	IMFMediaEventQueue* event_queue;
	IHoloCamMediaStream media_streams[1];
	IMFPresentationDescriptor* presentation_descriptor;
} IHoloCamMediaSource;

HRESULT
IHoloCamMediaSource__QueryInterface(IHoloCamMediaSource* this, REFIID riid, void** handle)
{
	HRESULT result;

	if (handle == 0) result = E_POINTER;
	else
	{
		*handle = 0;

		if (!IsEqualIID(riid, &IID_IUnknown)               &&
				!IsEqualIID(riid, &IID_IMFMediaEventGenerator) &&
				!IsEqualIID(riid, &IID_IMFMediaSource)         &&
				!IsEqualIID(riid, &IID_IMFMediaSourceEx)       &&
				!IsEqualIID(riid, &IID_IHoloCamMediaSource))
		{
			result = E_NOINTERFACE;
		}
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
	AcquireSRWLockExclusive(&this->lock);
	this->ref_count += 1;
	u32 ref_count = this->ref_count;
	ReleaseSRWLockExclusive(&this->lock);

	return ref_count;
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

	for (u32 i = 0; i < ARRAY_LEN(this->media_streams); ++i)
	{
		this->media_streams[i].lpVtbl->Release(&this->media_streams[i]);
	}
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
		else                                                  result = IMFPresentationDescriptor_Clone(this->presentation_descriptor, ppPresentationDescriptor);

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
		*pdwCharacteristics = MFMEDIASOURCE_IS_LIVE;
		result = S_OK;
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
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->state == IHoloCamMediaSourceState_Shutdown) result = S_OK;
	else
	{
		IMFMediaEventQueue_Shutdown(this->event_queue);

		IHoloCamMediaSource__ReleaseChildren(this);

		this->state = IHoloCamMediaSourceState_Shutdown;
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
IHoloCamMediaSource__Start(IHoloCamMediaSource* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
	HRESULT result;

	if      (pPresentationDescriptor == 0 || pvarStartPosition == 0)           result = E_INVALIDARG;
	else if (pguidTimeFormat != 0 && !IsEqualIID(pguidTimeFormat, &GUID_NULL)) result = MF_E_UNSUPPORTED_TIME_FORMAT;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		if (this->state == IHoloCamMediaSourceState_Shutdown) result = MF_E_SHUTDOWN;
		else
		{
			u32 stream_count;
			if (!SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorCount(pPresentationDescriptor, &stream_count)) || stream_count != ARRAY_LEN(this->media_streams)) result = E_INVALIDARG;
			else
			{
				PROPVARIANT start_time = { .vt = VT_I8, .hVal.QuadPart = MFGetSystemTime() };

				result = S_OK;
				for (u32 i = 0; i < stream_count && result == S_OK; ++i)
				{
					IMFStreamDescriptor* stream_desc        = 0;
					IMFStreamDescriptor* local_stream_desc  = 0;
					IMFMediaTypeHandler* media_type_handler = 0;
					IMFMediaType* media_type                = 0;
					do
					{
						BOOL is_selected = FALSE;
						result = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pPresentationDescriptor, i, &is_selected, &stream_desc);
						if (!SUCCEEDED(result)) break;

						u32 stream_id = 0;
						result = IMFStreamDescriptor_GetStreamIdentifier(stream_desc, &stream_id);
						if (!SUCCEEDED(result)) break;

						u32 stream_idx;
						BOOL was_selected;
						for (u32 j = 0; j < stream_count; ++j)
						{
							stream_idx        = 0;
							was_selected      = FALSE;
							local_stream_desc = 0;

							result = IMFPresentationDescriptor_GetStreamDescriptorByIndex(this->presentation_descriptor, j, &was_selected, &local_stream_desc);
							if (!SUCCEEDED(result)) break;

							u32 id;
							result = IMFStreamDescriptor_GetStreamIdentifier(local_stream_desc, &id);
							if (!SUCCEEDED(result)) break;

							if (id == stream_id) break;
							else
							{
								IMFStreamDescriptor_Release(local_stream_desc);
								continue;
							}
						}
						if (!SUCCEEDED(result)) break;

						if (is_selected)
						{
							result = IMFPresentationDescriptor_SelectStream(this->presentation_descriptor, stream_idx);
							if (!SUCCEEDED(result)) break;

							result = IMFStreamDescriptor_GetMediaTypeHandler(stream_desc, &media_type_handler);
							if (!SUCCEEDED(result)) break;

							result = IMFMediaTypeHandler_GetCurrentMediaType(media_type_handler, &media_type);
							if (!SUCCEEDED(result)) break;

							// TODO: consider sening MEUpdatedStream or MENewStream events

							result = IHoloCamMediaStream_Start(&this->media_streams[stream_idx], media_type);
						}
						else if (was_selected)
						{
							result = IMFPresentationDescriptor_DeselectStream(this->presentation_descriptor, stream_idx);
							if (!SUCCEEDED(result)) break;
							result = IHoloCamMediaStream_Stop(&this->media_streams[stream_idx]);
							if (!SUCCEEDED(result)) break;
						}
					} while (0);

					if (stream_desc        != 0) IMFStreamDescriptor_Release(stream_desc);
					if (local_stream_desc  != 0) IMFStreamDescriptor_Release(local_stream_desc);
					if (media_type_handler != 0) IMFMediaTypeHandler_Release(media_type_handler);
					if (media_type         != 0) IMFMediaType_Release(media_type);
				}

				if (SUCCEEDED(result))
				{
					result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MESourceStarted, &GUID_NULL, S_OK, &start_time);
					if (SUCCEEDED(result)) this->state = IHoloCamMediaSourceState_Started;
				}
			}
		}

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

		PROPVARIANT stop_time = { .vt = VT_I8, .hVal.QuadPart = MFGetSystemTime() };

		result = S_OK;
		for (u32 i = 0; i < ARRAY_LEN(this->media_streams); ++i)
		{
			result = IHoloCamMediaStream_Stop(&this->media_streams[i]);
			if (!SUCCEEDED(result)) break;

			result = IMFPresentationDescriptor_DeselectStream(this->presentation_descriptor, i);
			if (!SUCCEEDED(result)) break;
		}

		if (SUCCEEDED(result))
		{
			result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MESourceStopped, &GUID_NULL, S_OK, &stop_time);
		}
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

		if      (this->state == IHoloCamMediaSourceState_Shutdown)    result = MF_E_SHUTDOWN;
		else if (dwStreamIdentifier < ARRAY_LEN(this->media_streams)) result = MF_E_NOT_FOUND;
		else                                                          result = IMFAttributes_QueryInterface(this->media_streams[dwStreamIdentifier].attributes, &IID_IMFAttributes, ppAttributes);

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
IHoloCamMediaSource__SetD3DManager(IHoloCamMediaSource* this, IUnknown* pManager)
{
	// TODO
	OutputDebugStringA(__FUNCTION__);
	OutputDebugStringA("\n");
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
	if (this->ref_count != 0) return E_UNEXPECTED;

	*this = (IHoloCamMediaSource){
		.lpVtbl       = &IHoloCamMediaSource_Vtbl,
		.ref_count    = 1,
		.lock         = SRWLOCK_INIT,
		.state        = IHoloCamMediaSourceState_Shutdown,
	};

	{ // Attributes
		RET_IF_FAIL(MFCreateAttributes(&this->attributes, 1));
		RET_IF_FAIL(IMFAttributes_CopyAllItems(attributes, this->attributes));

		IMFSensorProfileCollection* profile_collection;
		RET_IF_FAIL(MFCreateSensorProfileCollection(&profile_collection));

		IMFSensorProfile* profile;
		RET_IF_FAIL(MFCreateSensorProfile(&KSCAMERAPROFILE_Legacy, 0, 0, &profile));
		RET_IF_FAIL(IMFSensorProfile_AddProfileFilter(profile, 0, L"((RES==;FRT<=30,1;SUT==))"));
		RET_IF_FAIL(IMFSensorProfileCollection_AddProfile(profile_collection, profile));
		IMFSensorProfile_Release(profile);

		RET_IF_FAIL(MFCreateSensorProfile(&KSCAMERAPROFILE_HighFrameRate, 0, 0, &profile));
		RET_IF_FAIL(IMFSensorProfile_AddProfileFilter(profile, 0, L"((RES==;FRT>=60,1;SUT==))"));
		RET_IF_FAIL(IMFSensorProfileCollection_AddProfile(profile_collection, profile));
		IMFSensorProfile_Release(profile);

		RET_IF_FAIL(IMFAttributes_SetUnknown(this->attributes, &MF_DEVICEMFT_SENSORPROFILE_COLLECTION, (IUnknown*)profile_collection));
		IMFSensorProfileCollection_Release(profile_collection);

		// TODO: consider setting MF_VIRTUALCAMERA_CONFIGURATION_APP_PACKAGE_FAMILY_NAME
	}
	
	RET_IF_FAIL(MFCreateEventQueue(&this->event_queue));

	IMFStreamDescriptor* stream_descriptors[ARRAY_LEN(this->media_streams)];
	for (u32 i = 0; i < ARRAY_LEN(this->media_streams); ++i)
	{
		RET_IF_FAIL(IHoloCamMediaStream__Init(&this->media_streams[i], 0, this));

		RET_IF_FAIL(this->media_streams[i].lpVtbl->GetStreamDescriptor(&this->media_streams[i], &stream_descriptors[i]));

	}

	RET_IF_FAIL(MFCreatePresentationDescriptor(ARRAY_LEN(stream_descriptors), stream_descriptors, &this->presentation_descriptor));
	for (u32 i = 0; i < ARRAY_LEN(stream_descriptors); ++i) IMFStreamDescriptor_Release(stream_descriptors[i]);

	this->state = IHoloCamMediaSourceState_Stopped;

	return S_OK;
}
