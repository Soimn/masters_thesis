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
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (riid)
	{
		LogGUID("[Holo] --- MediaSource__QueryInterface riid is ", riid);
	}

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
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
MediaSource_SampleAllocatorControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

HRESULT
MediaSource_KsControl__QueryInterface(void* raw_this, REFIID riid, void** handle)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->QueryInterface(this, riid, handle);
}

ULONG
MediaSource__AddRef(Media_Source* this)
{
	LOG_FUNCTION_ENTRY();

	u32 ref_count = InterlockedIncrement(&this->ref_count);
	if (ref_count == 1) Log("[Holo] --- MediaSource was ressurected");

	return ref_count;
}

HRESULT
MediaSource_GetService__AddRef(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->AddRef(this);
}

HRESULT
MediaSource_SampleAllocatorControl__AddRef(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->AddRef(this);
}

HRESULT
MediaSource_KsControl__AddRef(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->AddRef(this);
}

// NOTE: Requires lock held
void
MediaSource__ReleaseChildren(Media_Source* this)
{
	LOG_FUNCTION_ENTRY();
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
		if (this->streams[i] != 0) this->streams[i]->lpVtbl->Release(this->streams[i]);
		this->streams[i] = 0;
	}
}

ULONG
MediaSource__Release(Media_Source* this)
{
	LOG_FUNCTION_ENTRY();
	u32 ref_count = InterlockedDecrement(&this->ref_count);

	if (ref_count == 0)
	{
		AcquireSRWLockExclusive(&this->lock);
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
		ReleaseSRWLockExclusive(&this->lock);
	}

	return ref_count;
}

HRESULT
MediaSource_GetService__Release(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, GetService);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource_SampleAllocatorControl__Release(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource_KsControl__Release(void* raw_this)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, KsControl);
	return this->lpVtbl->Release(this);
}

HRESULT
MediaSource__BeginGetEvent(Media_Source* this, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0) result = MF_E_SHUTDOWN;
	else                        result = IMFMediaEventQueue_BeginGetEvent(this->event_queue, pCallback, punkState);

	ReleaseSRWLockExclusive(&this->lock);

	Log("[Holo] --- MediaSource__BeginGetEvent result: %d", result);

	return result;
}

HRESULT
MediaSource__EndGetEvent(Media_Source* this, IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	LOG_FUNCTION_ENTRY();
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
MediaSource__GetEvent(Media_Source* this, DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	LOG_FUNCTION_ENTRY();
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
MediaSource__QueueEvent(Media_Source* this, MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0) result = MF_E_SHUTDOWN;
	else                        result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, met, guidExtendedType, hrStatus, pvValue);

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
MediaSource__CreatePresentationDescriptor(Media_Source* this, IMFPresentationDescriptor** ppPresentationDescriptor)
{
	LOG_FUNCTION_ENTRY();
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

	if (SUCCEEDED(result)) OutputDebugStringA("[Holo] ---v SUCCESS");
	else                   OutputDebugStringA("[Holo] ---v FAIL");
	return result;
}

HRESULT
MediaSource__GetCharacteristics(Media_Source* this, DWORD* pdwCharacteristics)
{
	LOG_FUNCTION_ENTRY();
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
MediaSource__Pause(Media_Source* this)
{
	LOG_FUNCTION_ENTRY();
	return MF_E_INVALID_STATE_TRANSITION;
}

HRESULT
MediaSource__Shutdown(Media_Source* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	// NOTE: These should be 0 ONLY when the camera has shutdown
	if (this->event_queue == 0 || this->presentation_descriptor == 0) result = MF_E_SHUTDOWN;
	else
	{
		// NOTE: consider logging result
		IMFMediaEventQueue_Shutdown(this->event_queue);

		for (u32 i = 0; i < this->streams_len; ++i) MediaStream__Shutdown(this->streams[i]);

		MediaSource__ReleaseChildren(this);

		result = S_OK;
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

// NOTE: Lock must be held when calling this
HRESULT
MediaSource__GetStreamIndexFromIdentifier(Media_Source* this, DWORD identifier, u32* index)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result = S_OK;

	for (u32 i = 0; i < this->streams_len && SUCCEEDED(result); ++i)
	{
		DWORD id;

		IMFStreamDescriptor* stream_descriptor = 0;

		do
		{
			BREAK_IF_FAILED(result, this->streams[i]->lpVtbl->GetStreamDescriptor(this->streams[i], &stream_descriptor));
			BREAK_IF_FAILED(result, IMFStreamDescriptor_GetStreamIdentifier(stream_descriptor, &id));
		} while (0);

		if (stream_descriptor != 0) IMFStreamDescriptor_Release(stream_descriptor);

		if (SUCCEEDED(result) && id == identifier)
		{
			*index = i;
			break;
		}
	}

	return result;
}

HRESULT
MediaSource__Start(Media_Source* this, IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if      (pPresentationDescriptor == 0 || pvarStartPosition == 0)           result = E_INVALIDARG;
	else if (pguidTimeFormat != 0 && !IsEqualIID(pguidTimeFormat, &GUID_NULL)) result = MF_E_UNSUPPORTED_TIME_FORMAT;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		if (this->event_queue == 0 || this->presentation_descriptor == 0) result = MF_E_SHUTDOWN;
		else
		{
			PROPVARIANT time = { .vt = VT_I8,  .hVal.QuadPart = MFGetSystemTime() };
			do
			{
				DWORD stream_descriptor_count;
				BREAK_IF_FAILED(result, IMFPresentationDescriptor_GetStreamDescriptorCount(pPresentationDescriptor, &stream_descriptor_count));
				BREAK_IF_FAILED(result, (stream_descriptor_count == this->streams_len ? S_OK : E_INVALIDARG));

				for (u32 i = 0; i < stream_descriptor_count && SUCCEEDED(result); ++i)
				{
					IMFStreamDescriptor* descriptor     = 0;
					IMFStreamDescriptor* old_descriptor = 0;
					IMFMediaTypeHandler* type_handler   = 0;
					IMFMediaType* type                  = 0;
					IUnknown* stream_unknown            = 0;
					
					do
					{
						BOOL is_selected = FALSE;
						BREAK_IF_FAILED(result, IMFPresentationDescriptor_GetStreamDescriptorByIndex(pPresentationDescriptor, i, &is_selected, &descriptor));

						DWORD id = 0;
						BREAK_IF_FAILED(result, IMFStreamDescriptor_GetStreamIdentifier(descriptor, &id));

						u32 idx = 0;
						BREAK_IF_FAILED(result, MediaSource__GetStreamIndexFromIdentifier(this, id, &idx));

						BOOL was_selected = FALSE;
						BREAK_IF_FAILED(result, IMFPresentationDescriptor_GetStreamDescriptorByIndex(this->presentation_descriptor, idx, &was_selected, &old_descriptor));

						if (is_selected)
						{
							BREAK_IF_FAILED(result, IMFPresentationDescriptor_SelectStream(this->presentation_descriptor, idx));

							BREAK_IF_FAILED(result, IMFStreamDescriptor_GetMediaTypeHandler(descriptor, &type_handler));
							BREAK_IF_FAILED(result, IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &type));

							BREAK_IF_FAILED(result, this->streams[idx]->lpVtbl->QueryInterface(this->streams[idx], &IID_IUnknown, &stream_unknown));
							Log("[Holo] --- AAAAAAAAAAA");
							result = IMFMediaEventQueue_QueueEventParamUnk(this->event_queue, (was_selected ? MEUpdatedStream : MENewStream), &GUID_NULL, S_OK, stream_unknown);
							Log("[Holo] --- BBBBBBBBBBB %d", result);
							if (!SUCCEEDED(result)) break;

							BREAK_IF_FAILED(result, MediaStream__Start(this->streams[idx], type));
						}
						else if (was_selected)
						{
							BREAK_IF_FAILED(result, IMFPresentationDescriptor_DeselectStream(this->presentation_descriptor, idx));
							BREAK_IF_FAILED(result, MediaStream__Stop(this->streams[idx], false));
						}
					} while (0);

					if (stream_unknown != 0) IUnknown_Release(stream_unknown);
					if (type           != 0) IMFMediaType_Release(type);
					if (type_handler   != 0) IMFMediaTypeHandler_Release(type_handler);
					if (old_descriptor != 0) IMFStreamDescriptor_Release(old_descriptor);
					if (descriptor     != 0) IMFStreamDescriptor_Release(descriptor);
				}
				if (!SUCCEEDED(result)) break;
				
				BREAK_IF_FAILED(result, IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MESourceStarted, &GUID_NULL, S_OK, &time));
			} while (0);
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	if (!SUCCEEDED(result))
	{
		Log("Failed to start, %d", result);
	}
	return result;
}

HRESULT
MediaSource__Stop(Media_Source* this)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	AcquireSRWLockExclusive(&this->lock);

	if (this->event_queue == 0 || this->presentation_descriptor == 0) result = MF_E_SHUTDOWN;
	else
	{
		PROPVARIANT time = { .vt = VT_I8,  .hVal.QuadPart = MFGetSystemTime() };

		result = S_OK;
		for (u32 i = 0; i < this->streams_len && SUCCEEDED(result); ++i)
		{
			result = MediaStream__Stop(this->streams[i], true);
			if (SUCCEEDED(result))
			{
				result = IMFPresentationDescriptor_DeselectStream(this->presentation_descriptor, i);
			}
		}

		if (SUCCEEDED(result))
		{
			result = IMFMediaEventQueue_QueueEventParamVar(this->event_queue, MESourceStopped, &GUID_NULL, S_OK, &time);
		}
	}

	ReleaseSRWLockExclusive(&this->lock);

	return result;
}

HRESULT
MediaSource__GetSourceAttributes(Media_Source* this, IMFAttributes** ppAttributes)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (ppAttributes == 0) result = E_POINTER;
	else
	{
		*ppAttributes = 0;

		AcquireSRWLockExclusive(&this->lock);

		result = IMFAttributes_QueryInterface(this->attributes, &IID_IMFAttributes, ppAttributes);

		ReleaseSRWLockExclusive(&this->lock);
	}

	if (SUCCEEDED(result)) OutputDebugStringA("[Holo] ---v SUCCESS");
	else                   OutputDebugStringA("[Holo] ---v FAIL");
	return result;
}

HRESULT
MediaSource__GetStreamAttributes(Media_Source* this, DWORD dwStreamIdentifier, IMFAttributes** ppAttributes)
{
	LOG_FUNCTION_ENTRY();
	OutputDebugStringA(dwStreamIdentifier == 0 ? "[HOLO] ---v dwStreamIdentifier is 0" : "[HOLO] ---v dwStreamIdentifier is non zero");
	HRESULT result;

	if (ppAttributes == 0) result = E_POINTER;
	else
	{
		*ppAttributes = 0;

		AcquireSRWLockExclusive(&this->lock);

		if (dwStreamIdentifier >= this->streams_len) result = E_FAIL;
		else                                         result = IMFAttributes_QueryInterface(this->streams[dwStreamIdentifier]->attributes, &IID_IMFAttributes, ppAttributes);

		ReleaseSRWLockExclusive(&this->lock);
	}

	if (SUCCEEDED(result)) OutputDebugStringA("[Holo] ---v SUCCESS");
	else                   OutputDebugStringA("[Holo] ---v FAIL");
	return result;
}

HRESULT
MediaSource__SetD3DManager(Media_Source* this, IUnknown* pManager)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (pManager == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		// NOTE: stream pointers are ONLY 0 when the camera has shutdown
		if (this->streams_len > 0 && this->streams[0] == 0) result = MF_E_SHUTDOWN;
		else
		{
			/*
			result = S_OK;
			for (u32 i = 0; i < this->streams_len && SUCCEEDED(result); ++i)
			{
				result = MediaStream__SetD3DManager(this->streams[i], pManager);
			}*/
			result = E_NOTIMPL;
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaSource_GetService__GetService(void* raw_this, REFGUID guidService, REFIID riid, void** ppvObject)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (ppvObject == 0) result = E_POINTER;
	else
	{
		*ppvObject = 0;
		result = MF_E_UNSUPPORTED_SERVICE;
	}

	if      (SUCCEEDED(result))                  OutputDebugStringA("[Holo] ---v SUCCESS");
	else if (result == MF_E_UNSUPPORTED_SERVICE) OutputDebugStringA("[Holo] ---v FAIL SUCCESSFULLY");
	else                                         OutputDebugStringA("[Holo] ---v FAIL");
	return result;
}

HRESULT
MediaSource_SampleAllocatorControl__GetAllocatorUsage(void* raw_this, DWORD dwOutputStreamID, DWORD* pdwInputStreamID, MFSampleAllocatorUsage* peUsage)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	HRESULT result;

	if (pdwInputStreamID == 0 || peUsage == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		u32 idx;
		result = MediaSource__GetStreamIndexFromIdentifier(this, dwOutputStreamID, &idx);
		Log("[Holo] --- MediaSource__GetAllocatorUsage idx for id %d is %d", dwOutputStreamID, idx);
		if (SUCCEEDED(result))
		{
			*pdwInputStreamID = dwOutputStreamID;
			peUsage           = MFSampleAllocatorUsage_UsesProvidedAllocator;
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	Log("[Holo] --- MediaSource__GetAllocatorUsage resulted in code %d", result);

	return result;
}

HRESULT
MediaSource_SampleAllocatorControl__SetDefaultAllocator(void* raw_this, DWORD dwOutputStreamID, IUnknown* pAllocator)
{
	LOG_FUNCTION_ENTRY();
	Media_Source* this = MEDIASOURCE_ADJ_THIS(raw_this, SampleAllocatorControl);
	HRESULT result;

	if (pAllocator == 0) result = E_POINTER;
	else
	{
		AcquireSRWLockExclusive(&this->lock);

		u32 idx;
		result = MediaSource__GetStreamIndexFromIdentifier(this, dwOutputStreamID, &idx);
		if (SUCCEEDED(result))
		{
			result = MediaStream__SetAllocator(this->streams[idx], pAllocator);
		}

		ReleaseSRWLockExclusive(&this->lock);
	}

	return result;
}

HRESULT
MediaSource_KsControl__KsEvent(void* raw_this, KSEVENT* Event, ULONG EventLength, void* EventData, ULONG DataLength, ULONG* BytesReturned)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (BytesReturned == 0) result = E_POINTER;
	else                   	result = ERROR_SET_NOT_FOUND;

	return result;
}

HRESULT
MediaSource_KsControl__KsMethod(void* raw_this, KSMETHOD* Method, ULONG MethodLength, void* MethodData, ULONG DataLength, ULONG* BytesReturned)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (Method == 0 || BytesReturned == 0) result = E_POINTER;
	else                                   result = ERROR_SET_NOT_FOUND;

	return result;
}

HRESULT
MediaSource_KsControl__KsProperty(void* raw_this, KSPROPERTY* Property, ULONG PropertyLength, void* PropertyData, ULONG DataLength, ULONG* BytesReturned)
{
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	if (Property == 0 || BytesReturned == 0) result = E_POINTER;
	else                                     result = ERROR_SET_NOT_FOUND;

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
	LOG_FUNCTION_ENTRY();
	HRESULT result;

	IMFSensorProfileCollection* sensor_collection = 0;
	IMFSensorProfile* legacy_profile              = 0;
	IMFSensorProfile* profile                     = 0;
	IMFStreamDescriptor* stream_descriptors[ARRAY_LEN(this->streams)] = {0};
	do
	{
		BREAK_IF_FAILED(result, MFCreateEventQueue(&this->event_queue));

		BREAK_IF_FAILED(result, MFCreateAttributes(&this->attributes, 0));
		BREAK_IF_FAILED(result, IMFAttributes_CopyAllItems(parent_attributes, this->attributes));

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
				BREAK_IF_FAILED(result, MediaStream__Init(this->streams[i], i, this));
				BREAK_IF_FAILED(result, this->streams[i]->lpVtbl->GetStreamDescriptor(this->streams[i], &stream_descriptors[i]));
			}
		}
		if (!SUCCEEDED(result)) break;

		BREAK_IF_FAILED(result, MFCreatePresentationDescriptor(this->streams_len, &stream_descriptors[0], &this->presentation_descriptor));
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
