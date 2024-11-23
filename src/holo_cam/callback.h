typedef struct IHoloCamCallback IHoloCamCallback;
typedef struct IHoloCamCallbackVtbl
{
	HRESULT (*QueryInterface) (IHoloCamCallback* this, REFIID riid, void** ppvObject);
	ULONG   (*AddRef)         (IHoloCamCallback* this);
	ULONG   (*Release)        (IHoloCamCallback* this);
	HRESULT (*GetParameters)  (IHoloCamCallback* this, DWORD* pdwFlags, DWORD* pdwQueue);
	HRESULT (*Invoke)         (IHoloCamCallback* this, IMFAsyncResult* pAsyncResult);
} IHoloCamCallbackVtbl;

typedef struct IHoloCamCallback
{
	IHoloCamCallbackVtbl* lpVtbl;
	u32 ref_count;
	u32 queue_id;
	void* obj;
	void (*invoke_func)(void* this, IMFAsyncResult* pAsyncResult);
} IHoloCamCallback;

HRESULT
IHoloCamCallback__QueryInterface(IHoloCamCallback* this, REFIID riid, void** ppvObject)
{
	HRESULT result;

	if (ppvObject == 0) result = E_POINTER;
	else
	{
		if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMFAsyncCallback))
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
IHoloCamCallback__AddRef(IHoloCamCallback* this)
{
	this->ref_count += 1;
	return this->ref_count;
}

ULONG
IHoloCamCallback__Release(IHoloCamCallback* this)
{
	this->ref_count -= 1;
	return this->ref_count;
}

HRESULT
IHoloCamCallback__GetParameters(IHoloCamCallback* this, DWORD* pdwFlags, DWORD* pdwQueue)
{
	*pdwFlags = 0;
	*pdwQueue = this->queue_id;
	return S_OK;
}

HRESULT
IHoloCamCallback__Invoke(IHoloCamCallback* this, IMFAsyncResult* pAsyncResult)
{
	this->invoke_func(this->obj, pAsyncResult);
	return S_OK;
}

static IHoloCamCallbackVtbl IHoloCamCallback_Vtbl = {
	.QueryInterface = IHoloCamCallback__QueryInterface,
	.AddRef         = IHoloCamCallback__AddRef,
	.Release        = IHoloCamCallback__Release,
	.GetParameters  = IHoloCamCallback__GetParameters,
	.Invoke         = IHoloCamCallback__Invoke,
};

void
IHoloCamCallback__Init(IHoloCamCallback* this, void* obj, void (*invoke_func)(void*, IMFAsyncResult*), u32 queue_id)
{
	*this = (IHoloCamCallback){
		.lpVtbl      = &IHoloCamCallback_Vtbl,
		.ref_count   = 1,
		.queue_id    = queue_id,
		.obj         = obj,
		.invoke_func = invoke_func,
	};
}
