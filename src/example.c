#define COBJMACROS
#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <initguid.h>
#include <propvarutil.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfvirtualcamera.h>
#include <mferror.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

#include <stdio.h>
#include <stdint.h>

#include "holo_cam.h"

#define PORT "3003"

IMFSourceReader*
DEBUG_GetCameraReader()
{
	IMFSourceReader* device = 0;

	HRESULT result = S_OK;

	IMFAttributes* attr      = 0;
	IMFActivate** devices    = 0;
	unsigned int devices_len = 0;
	result = MFCreateAttributes(&attr, 1);
	result = IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	result = MFEnumDeviceSources(attr, &devices, &devices_len);

	wchar_t symbolic_name[1024] = {0};
	wchar_t friendly_name[1024] = {0};
	result = IMFActivate_GetString(devices[0], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolic_name[0], sizeof(symbolic_name)/sizeof(symbolic_name[0]), &(UINT32){0});
	result = IMFActivate_GetString(devices[0], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendly_name[0], sizeof(friendly_name)/sizeof(friendly_name[0]), &(UINT32){0});

	IMFAttributes* device_attr = 0;
	result = MFCreateAttributes(&device_attr, 2);
	result = IMFAttributes_SetGUID(device_attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	result = IMFAttributes_SetString(device_attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symbolic_name);

	IMFMediaSource* media_source = 0;
	result = MFCreateDeviceSource(device_attr, &media_source);

	result = MFCreateSourceReaderFromMediaSource(media_source, 0, &device);

	IMFMediaType* media_type = 0;
	result = MFCreateMediaType(&media_type);
	result = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
	result = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
	result = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (unsigned __int64)1920 << 32 | 1080);
	result = IMFSourceReader_SetCurrentMediaType(device, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, media_type);

	IMFAttributes_Release(attr);
	for (unsigned int i = 0; i < devices_len; ++i) IMFActivate_Release(devices[i]);
	CoTaskMemFree(devices);
	IMFAttributes_Release(device_attr);
	IMFMediaSource_Release(media_source);
	IMFMediaType_Release(media_type);

	return device;
}

static int
Clamp(int min, int max, int n)
{
	return (n < min ? min : (n > max ? max : n));
}

static uint32_t
YUVToRGB(uint8_t y, uint8_t u, uint8_t v)
{
	int c = (int)y - 16;
	int d = (int)u - 128;
	int e = (int)v - 128;

	int r = Clamp(0, 255, (298*c +    0*d +  409*e + 128) >> 8);
	int g = Clamp(0, 255, (298*c + -100*d + -209*e + 128) >> 8);
	int b = Clamp(0, 255, (298*c +  516*d +    0*e + 128) >> 8);

	return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

static uint8_t
UpscaleS(uint16_t sm1, uint16_t s, uint16_t sp1, uint16_t sp2)
{
	return (uint8_t)Clamp(0, 255, (9*(s + sp1) - (sm1 + sp1) + 8) / 16);
}

static void
UpscaleUV(uint16_t uvm1, uint16_t uv, uint16_t uvp1, uint16_t uvp2, uint8_t* u, uint8_t* v)
{
	*u = UpscaleS(uvm1 & 0xFF, uv & 0xFF, uvp1 & 0xFF, uvp2 & 0xFF);
	*v = UpscaleS(uvm1 >> 8, uv >> 8, uvp1 >> 8, uvp2 >> 8);
}

static void
NV12ToRGB(unsigned int width, unsigned int height, uint8_t* nv12, uint32_t* rgb)
{
	uint8_t* ys   = nv12;
	uint16_t* uvs = (uint16_t*)(nv12 + width*height);

	for (unsigned int j = 0; j < height; j += 2)
	{
		for (unsigned int i = 0; i < width; i += 2)
		{
			unsigned int hw = width/2;
			unsigned int hh = height/2;

			unsigned int him1 = (i/2 > 0 ? i/2 - 1 : 0);
			unsigned int hi   = i/2;
			unsigned int hip1 = (i/2 + 1 < hw ? i/2 + 1 : hw);
			unsigned int hip2 = (i/2 + 2 < hw ? i/2 + 2 : hw);

			unsigned int hjm1 = (j/2 > 0 ? j/2 - 1 : 0);
			unsigned int hj   = j/2;
			unsigned int hjp1 = (j/2 + 1 < hh ? j/2 + 1 : hh);
			unsigned int hjp2 = (j/2 + 2 < hh ? j/2 + 2 : hh);

			/*
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  | him1, hjm1 |            |  hi, hjm1 |            | hip1, hjm1 |            | hip2, hjm1 |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  |            |            |           |            |            |            |            |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  | him1, hj   |            |  hi, hj   |     a      | hip1, hj   |            | hip2, hj   |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  |     b      |            |     c     |     d      |     e      |            |      f     |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  | him1, hjp1 |            |  hi, hjp1 |            | hip1, hjp1 |            | hip2, hjp1 |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  |            |            |           |            |            |            |            |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *  | him1, hjp2 |            |  hi, hjp2 |            | hip1, hjp2 |            | hip2, hjp2 |
			 *  +------------+------------+-----------+------------+------------+------------+------------+
			 *
			 */

			uint8_t a_u, a_v;
			UpscaleUV(uvs[hj*hw + him1], uvs[hj*hw + hi], uvs[hj*hw + hip1], uvs[hj*hw + hip2], &a_u, &a_v);

			uint8_t b_u, b_v;
			UpscaleUV(uvs[hjm1*hw + him1], uvs[hj*hw + him1], uvs[hjp1*hw + him1], uvs[hjp2*hw + him1], &b_u, &b_v);

			uint8_t c_u, c_v;
			UpscaleUV(uvs[hjm1*hw + hi], uvs[hj*hw + hi], uvs[hjp1*hw + hi], uvs[hjp2*hw + hi], &c_u, &c_v);

			uint8_t e_u, e_v;
			UpscaleUV(uvs[hjm1*hw + hip1], uvs[hj*hw + hip1], uvs[hjp1*hw + hip1], uvs[hjp2*hw + hip1], &e_u, &e_v);

			uint8_t f_u, f_v;
			UpscaleUV(uvs[hjm1*hw + hip2], uvs[hj*hw + hip2], uvs[hjp1*hw + hip2], uvs[hjp2*hw + hip2], &f_u, &f_v);

			rgb[    j*width + i  ] = YUVToRGB(ys[    j*width + i  ], (uint8_t)uvs[hj*hw + hi], (uint8_t)(uvs[hj*hw + hi] >> 8));
			rgb[    j*width + i+1] = YUVToRGB(ys[    j*width + i+1], a_u, a_v);
			rgb[(j+1)*width + i  ] = YUVToRGB(ys[(j+1)*width + i  ], c_u, c_v);
			rgb[(j+1)*width + i+1] = YUVToRGB(ys[(j+1)*width + i+1], UpscaleS(b_u, c_u, e_u, f_u), UpscaleS(b_v, c_v, e_v, f_v));
		}
	}
}

#if 0

					// NOTE: d3d11 code based on https://gist.github.com/d7samurai/261c69490cce0620d0bfc93003cd1052

					ID3D11Device* device = 0;
					ID3D11DeviceContext* context = 0;
					if (!SUCCEEDED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &(D3D_FEATURE_LEVEL){ D3D_FEATURE_LEVEL_11_0 }, 1, D3D11_SDK_VERSION, &device, 0, &context)))
					{
						D3D11_TEXTURE2D_DESC input_desc = {
							.Width          = 1280,
							.Height         = 960,
							.MipLevels      = 1,
							.ArraySize      = 1,
							.Format         = DXGI_FORMAT_B8G8R8A8_UNORM,
							.SampleDesc     = { .Count = 0, .Quality = 0 },
							.Usage          = D3D11_USAGE_DEFAULT,
							.BindFlags      = D3D11_BIND_SHADER_RESOURCE,
							.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
							.MiscFlags      = 0,
						};

						D3D11_TEXTURE2D_DESC output_desc = {
							.Width          = 1280,
							.Height         = 960,
							.MipLevels      = 1,
							.ArraySize      = 1,
							.Format         = DXGI_FORMAT_B8G8R8A8_UNORM,
							.SampleDesc     = { .Count = 0, .Quality = 0 },
							.Usage          = D3D11_USAGE_DEFAULT,
							.BindFlags      = D3D11_BIND_RENDER_TARGET,
							.CPUAccessFlags = D3D11_CPU_ACCESS_READ,
							.MiscFlags      = 0,
						};

						D3D11_RENDER_TARGET_VIEW_DESC view_desc = {
							.Format        = DXGI_FORMAT_B8G8R8A8_UNORM,
							.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
						};

						ID3D11Texture2D* input_texture = 0;
						ID3D11Texture2D* output_texture = 0;
						ID3D11RenderTargetView* render_target = 0;
						if (!SUCCEEDED(ID3D11Device_CreateTexture2D(device, &input_desc, 0, &input_texture)) ||
								!SUCCEEDED(ID3D11Device_CreateTexture2D(device, &output_desc, 0, &output_texture)) ||
								!SUCCEEDED(ID3D11Device_CreateRenderTargetView(device, output_texture, &view_desc, &render_target)))
						{
							fprintf(stderr, "Failed to create textures\n");
						}
						else
						{
							
						}

						if (input_texture != 0) ID3D11Texture2D_Release(input_texture);
						if (output_texture != 0) ID3D11Texture2D_Release(output_texture);

						ID3D11DeviceContext_Release(context);
						ID3D11Device_Release(device);
					}
#endif

int
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, int show_cmd)
{
	HRESULT com_init = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (!SUCCEEDED(com_init)) fprintf(stderr, "Failed to initialize COM\n");
	else
	{
		HRESULT mf_init = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
		if (!SUCCEEDED(mf_init)) fprintf(stderr, "Failed to initialize Media Foundation\n");
		else
		{
			WSADATA wsa_data;
			struct addrinfo* info;
			struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_TCP, .ai_flags = AI_PASSIVE };
			if      (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)         fprintf(stderr, "failed to startup winsock\n");
			else if (getaddrinfo("localhost", PORT, &hints, &info) != 0) fprintf(stderr, "failed to get address info\n");
			else
			{
				SOCKET sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

				if (sock == INVALID_SOCKET) fprintf(stderr, "failed to create socket\n");
				else
				{
					IMFSourceReader* camera_sampler = DEBUG_GetCameraReader();

					IMFVirtualCamera* cam;
					HRESULT result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, L"Holo Cam", CLSID_HOLOCAM_STRING, 0, 0, &cam);

					IMFVirtualCamera_SetBlob(cam, &GUID_HOLOCAM_PORT, PORT, sizeof(PORT));
					result = IMFVirtualCamera_Start(cam, 0);

					while (connect(sock, info->ai_addr, (int)info->ai_addrlen) == SOCKET_ERROR) fprintf(stderr, "failed to connect\n");

					for (unsigned int request_dims[2]; recv(sock, (char*)request_dims, sizeof(request_dims), MSG_WAITALL) > 0; )
					{
						static uint32_t rgb_frame[1280*960];

						for (;;)
						{
							IMFSample* sample = 0;
							UINT32 flags = 0;
							HRESULT r = IMFSourceReader_ReadSample(camera_sampler, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &(UINT32){0}, &flags, &(UINT64){0}, &sample);

							if (sample == 0) continue;

							IMFMediaBuffer* buffer = 0;
							r = IMFSample_ConvertToContiguousBuffer(sample, &buffer);
							
							BYTE* data      = 0;
							UINT32 data_len = 0;
							r = IMFMediaBuffer_Lock(buffer, &data, 0, &data_len);

							static uint32_t rgb_sample[1920*1080];

							NV12ToRGB(1920, 1080, data, rgb_sample);

							for (unsigned int j = 0; j < 960; ++j)
							{
								for (unsigned int i = 0; i < 1280; ++i)
								{
									rgb_frame[j*1280 + i] = rgb_sample[j*1920 + i];
								}
							}

							r = IMFMediaBuffer_Unlock(buffer);

							IMFMediaBuffer_Release(buffer);
							IMFSample_Release(sample);

							break;
						}

						for (unsigned int j = 0; j < 960; ++j)
						{
							for (unsigned int i = 0; i < 1280; ++i)
							{
								uint32_t color = rgb_frame[j*1280 + i];

								float r = ((color >> 16) & 0xFF) / 255.0f;
								float g = ((color >>  8) & 0xFF) / 255.0f;
								float b = ((color >>  0) & 0xFF) / 255.0f;

								r = (int)(r*4)/4.0f;
								g = (int)(g*4)/4.0f;
								b = (int)(b*4)/4.0f;

								rgb_frame[j*1280 + i] = (uint32_t)Clamp(0, 255, (int)(r*255)) << 16 | (uint32_t)Clamp(0, 255, (int)(g*255)) << 8 | (uint32_t)Clamp(0, 255, (int)(b*255));
							}
						}

						send(sock, (char*)&rgb_frame[0], sizeof(rgb_frame), 0);
					}

					shutdown(sock, SD_SEND);

					closesocket(sock);

					IMFVirtualCamera_Shutdown(cam);
					IMFVirtualCamera_Release(cam);
					IMFSourceReader_Release(camera_sampler);
				}

				WSACleanup();
			}

			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
