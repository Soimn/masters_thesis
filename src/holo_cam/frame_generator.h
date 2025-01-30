typedef struct Frame_Generator
{
	u32 width;
	u32 height;
	IMFDXGIDeviceManager* manager;
	HANDLE device_handle;
	ID3D11Texture2D* texture;
} Frame_Generator;

// TODO
// NOTE: The Direct2D c headers were removed

HRESULT
FrameGenerator_SetD3DManager(Frame_Generator* this, IUnknown* manager, u32 width, u32 height)
{
	HRESULT result;

	if      (manager == 0)              result = E_POINTER;
	else if (width == 0 || height == 0) result = E_INVALIDARG;
	else
	{
		ID3D11Device* device  = 0;
		IDXGISurface* surface = 0;
		ID2D1Factory* factory = 0;
		do
		{
			if (this->manager != 0) IMFDXGIDeviceManager_Release(this->manager);
			BREAK_IF_FAILED(result, IMFDXGIDeviceManager_QueryInterface(manager, &IID_IMFDXGIDeviceManager, &this->manager));
			BREAK_IF_FAILED(result, IMFDXGIDeviceManager_OpenDeviceHandle(this->manager, &this->device_handle));

			BREAK_IF_FAILED(result, IMFDXGIDeviceManager_GetVideoService(this->manager, this->device_handle, &IID_ID3D11Device, &device));

			D3D11_TEXTURE2D_DESC texture_description = {
				.Width       = width,
				.Height      = height,
				.MipLevels   = 1,
				.ArraySize   = 1,
				.Format      = DXGI_FORMAT_B8G8R8A8_UNORM,
				.SampleDesc  = { .Count = 1, .Quality = 0 },
				.Usage       = D3D11_USAGE_DEFAULT,
				.BindFlags   = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
			};

			BREAK_IF_FAILED(result, ID3D11Device_CreateTexture2D(device, &texture_description, 0, &this->texture));
			BREAK_IF_FAILED(result, ID3D11Texture2D_QueryInterface(this->texture, &IID_IDXGISurface, &surface));

			BREAK_IF_FAILED(result, D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &IID_ID2D1Factory, 0, &factory));

			// TODO
		} while (0);

		if (device  != 0) ID3D11Device_Release(device);
		if (surface != 0) IDXGISurface_Release(surface);
		if (factory != 0) //TODO
	}

	return result;
}

bool
FrameGenerator_HasD3DManager(Frame_Generator* this)
{
	return false; // TODO
}

HRESULT
FrameGenerator_EnsureRenderTarget(Frame_Generator* this, u32 width, u32 height)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

HRESULT
FrameGenerator_Generate(Frame_Generator* this, IMFSample* sample, REFGUID format, IMFSample** generated_sample)
{
	HRESULT result;

	result = E_NOTIMPL;

	return result;
}

void
FrameGenerator_Teardown(Frame_Generator* this)
{
	if (this->texture != 0)
	{
		ID3D11Texture2D_Release(this->texture);
		this->texture = 0;
	}

	if (this->manager != 0)
	{
		if (this->device_handle != INVALID_HANDLE_VALUE)
		{
			IMFDXGIDeviceManager_CloseDeviceHandle(this->manager, this->device_handle);
			this->device_handle = INVALID_HANDLE_VALUE;
		}

		IMFDXGIDeviceManager_Release(this->manager);
		this->manager = 0;
	}
}

void
FrameGenerator_Init(Frame_Generator* this)
{
	*this = (Frame_Generator){
		.width         = 0,
		.height        = 0,
		.manager       = 0,
		.device_handle = INVALID_HANDLE_VALUE,
	};
}
