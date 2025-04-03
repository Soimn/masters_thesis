#pragma once

#include <stdint.h>
#include <stdbool.h>

// Core

typedef struct Holo_Settings
{
	bool dont_init_media_foundation;
	bool dont_init_winsock;
} Holo_Settings;

bool Holo_Init(Holo_Settings settings);
void Holo_Cleanup(void);

typedef struct Holo_Cam Holo_Cam;
Holo_Cam* HoloCam_Create(wchar_t* unique_name, uint16_t width, uint16_t height, uint8_t fps, uint16_t port);
void HoloCam_Destroy(Holo_Cam** cam);
bool HoloCam_Present(Holo_Cam* cam, uint32_t* image);

// Convencience

typedef struct Holo_Camera_Name
{
	wchar_t* friendly_name;
	wchar_t* symbolic_name;
} Holo_Camera_Name;

Holo_Camera_Name* Holo_GetCameraNames(unsigned int* names_len);
void Holo_FreeCameraNames(Holo_Camera_Name* names);

typedef struct Holo_Camera_Reader Holo_Camera_Reader;
Holo_Camera_Reader* HoloCameraReader_Create(wchar_t* symbolic_name, uint16_t width, uint16_t height, uint8_t fps);
void HoloCameraReader_Destroy(Holo_Camera_Reader** reader);
void HoloCameraReader_ReadFrame(Holo_Camera_Reader* reader, uint32_t* frame);

#if defined(HOLOCAM_IMPLEMENTATION) && !defined(HOLOCAM_IDS)
#define HOLOCAM_IDS
#endif

#ifdef HOLOCAM_IDS
#include <initguid.h>

// {82F4542B-76F9-4DA4-A049-5B376F8255DD}
DEFINE_GUID(CLSID_HOLOCAM, 0x82f4542b, 0x76f9, 0x4da4, 0xa0, 0x49, 0x5b, 0x37, 0x6f, 0x82, 0x55, 0xdd);
#define CLSID_HOLOCAM_STRING L"{82F4542B-76F9-4DA4-A049-5B376F8255DD}"

#define HOLOCAM_MAX_CAMERA_COUNT 256
#define HOLOCAM_MAX_CAMERA_STREAM_COUNT 8

// {79809298-5754-42D6-974A-EF1292A150FF}
DEFINE_GUID(GUID_HOLOCAM_PORT, 0x79809298, 0x5754, 0x42d6, 0x97, 0x4a, 0xef, 0x12, 0x92, 0xa1, 0x50, 0xff);

// {4A7C1F91-A83F-425C-AADF-827682DDFADF}
DEFINE_GUID(GUID_HOLOCAM_FRAME_SIZE, 0x4a7c1f91, 0xa83f, 0x425c, 0xaa, 0xdf, 0x82, 0x76, 0x82, 0xdd, 0xfa, 0xdf);

// {B4DA981F-618B-4715-A732-9E80D21416CB}
DEFINE_GUID(GUID_HOLOCAM_FPS, 0xb4da981f, 0x618b, 0x4715, 0xa7, 0x32, 0x9e, 0x80, 0xd2, 0x14, 0x16, 0xcb);
#endif

#ifdef HOLOCAM_IMPLEMENTATION

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
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfsensorgroup.lib")
#pragma comment(lib, "Ws2_32.lib")

static Holo_Settings HoloSettings = {0};

bool
Holo_Init(Holo_Settings settings)
{
	bool encountered_errors = false;

	HoloSettings = settings;

	if (!settings.dont_init_media_foundation)
	{
		if      (!SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))) encountered_errors = true;
		else if (!SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET)))
		{
			CoUninitialize();
			encountered_errors = true;
		}
	}

	if (!encountered_errors && !settings.dont_init_winsock)
	{
		if (WSAStartup(MAKEWORD(2, 2), &(WSADATA){0}) != 0)
		{
			if (!settings.dont_init_media_foundation)
			{
				MFShutdown();
				CoUninitialize();
			}

			encountered_errors = true;
		}
	}

	return !encountered_errors;
}

void
Holo_Cleanup()
{
	if (!HoloSettings.dont_init_winsock) WSACleanup();

	if (!HoloSettings.dont_init_media_foundation)
	{
		MFShutdown();
		CoUninitialize();
	}
}

typedef struct Holo_Cam
{
	struct IMFVirtualCamera* handle;
	SOCKET socket;
	uint16_t width;
	uint16_t height;
} Holo_Cam;

Holo_Cam*
HoloCam_Create(wchar_t* unique_name, uint16_t width, uint16_t height, uint8_t fps, uint16_t port)
{
	Holo_Cam* cam = 0;

	IMFVirtualCamera* cam_handle = 0;
	HRESULT result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, unique_name, CLSID_HOLOCAM_STRING, 0, 0, &cam_handle);

	if (SUCCEEDED(result)) result = IMFVirtualCamera_SetUINT32(cam_handle, &GUID_HOLOCAM_PORT, port);
	if (SUCCEEDED(result)) result = IMFVirtualCamera_SetUINT64(cam_handle, &GUID_HOLOCAM_FRAME_SIZE, (uint64_t)width << 32 | height);
	if (SUCCEEDED(result)) result = IMFVirtualCamera_SetUINT32(cam_handle, &GUID_HOLOCAM_FPS, fps);
	if (SUCCEEDED(result)) result = IMFVirtualCamera_Start(cam_handle, 0);

	SOCKET sock = 0;
	if (SUCCEEDED(result))
	{
		char port_string[6] = {
			'0' + (port/10000) % 10,
			'0' + (port/1000) % 10,
			'0' + (port/100) % 10,
			'0' + (port/10) % 10,
			'0' + (port/1) % 10,
			0,
		};

		struct addrinfo* info;
		struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_TCP, .ai_flags = AI_PASSIVE };
		if (getaddrinfo("localhost", port_string, &hints, &info) != 0) result = E_FAIL;
		else
		{
			sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

			if (sock == INVALID_SOCKET) result = E_FAIL;
			else
			{
				while (connect(sock, info->ai_addr, (int)info->ai_addrlen));

				cam = CoTaskMemAlloc(sizeof(Holo_Cam));

				if (cam == 0) result = E_FAIL;
				else
				{
					*cam = (Holo_Cam){
						.handle = cam_handle,
						.socket = sock,
						.width  = width,
						.height = height,
					};
				}
			}
		}
	}

	if (!SUCCEEDED(result))
	{
		if (sock != 0)
		{
			shutdown(sock, SD_SEND);
			closesocket(sock);
		}

		if (cam_handle != 0) IMFVirtualCamera_Release(cam_handle);
		if (cam != 0) CoTaskMemFree(cam);
	}

	return (SUCCEEDED(result) ? cam : 0);
}

void
HoloCam_Destroy(Holo_Cam** cam)
{
	if (*cam != 0)
	{
		if ((*cam)->socket != 0)
		{
			shutdown((*cam)->socket, SD_SEND);
			closesocket((*cam)->socket);
		}

		if ((*cam)->handle != 0)
		{
			IMFVirtualCamera_Shutdown((*cam)->handle);
			IMFVirtualCamera_Release((*cam)->handle);
		}

		CoTaskMemFree(*cam);
	}
}

bool
HoloCam_Present(Holo_Cam* cam, uint32_t* image)
{
	send(cam->socket, (char*)image, (uint32_t)cam->width*cam->height*4, 0);
	return (recv(cam->socket, &(char){0}, 1, MSG_WAITALL) == 1);
}

Holo_Camera_Name*
Holo_GetCameraNames(unsigned int* names_len)
{
	bool encountered_errors = false;

	Holo_Camera_Name* names = 0;

	IMFAttributes* enum_attr = 0;
	IMFActivate** devices    = 0;
	UINT32 devices_len       = 0;
	do
	{
		if (!SUCCEEDED(MFCreateAttributes(&enum_attr, 1)) ||
				!SUCCEEDED(IMFAttributes_SetGUID(enum_attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
		{
			encountered_errors = true;
			break;
		}

		if (!SUCCEEDED(MFEnumDeviceSources(enum_attr, &devices, &devices_len)))
		{
			encountered_errors = true;
			break;
		}

		if (devices_len == 0)
		{
			names      = 0;
			*names_len = 0;
			break;
		}

		UINT32 total_cap = 0;
		for (UINT32 i = 0; i < devices_len; ++i)
		{
			UINT32 symbolic_len = 0;
			UINT32 friendly_len = 0;

			if (!SUCCEEDED(IMFActivate_GetStringLength(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolic_len)) ||
					!SUCCEEDED(IMFActivate_GetStringLength(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendly_len)))
			{
				encountered_errors = true;
				break;
			}

			total_cap += (symbolic_len+1) + (friendly_len+1);
		}
		if (encountered_errors) break;

		names = CoTaskMemAlloc((devices_len+1)*sizeof(Holo_Camera_Name) + total_cap*sizeof(wchar_t));
		if (names == 0)
		{
			encountered_errors = true;
			break;
		}

		wchar_t* name_cur = (wchar_t*)((uint8_t*)names + (devices_len+1)*sizeof(Holo_Camera_Name));

		for (UINT32 i = 0; i < devices_len; ++i)
		{
			UINT32 symbolic_len = 0;
			UINT32 friendly_len = 0;

			if (!SUCCEEDED(IMFActivate_GetStringLength(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolic_len)) ||
					!SUCCEEDED(IMFActivate_GetStringLength(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendly_len))                    ||
					(symbolic_len+1) + (friendly_len+1) > total_cap)
			{
				encountered_errors = true;
				break;
			}

			if (!SUCCEEDED(IMFActivate_GetString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, name_cur, symbolic_len + 1, 0)))
			{
				encountered_errors = true;
				break;
			}

			wchar_t* symbolic_name = name_cur;
			name_cur += symbolic_len + 1;

			if (!SUCCEEDED(IMFActivate_GetString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name_cur, friendly_len + 1, 0)))
			{
				encountered_errors = true;
				break;
			}

			wchar_t* friendly_name = name_cur;
			name_cur += friendly_len + 1;
			total_cap -= (symbolic_len+1) + (friendly_len+1);

			names[i] = (Holo_Camera_Name){ .symbolic_name = symbolic_name, .friendly_name = friendly_name };
		}
		if (encountered_errors) break;

		names[devices_len] = (Holo_Camera_Name){0};

		*names_len = devices_len;
	} while (0);

	if (enum_attr != 0) IMFAttributes_Release(enum_attr);

	if (devices != 0)
	{
		for (UINT32 i = 0; i < devices_len; ++i) IMFActivate_Release(devices[i]);

		CoTaskMemFree(devices);
	}

	if (encountered_errors && names != 0) CoTaskMemFree(names);

	return (encountered_errors ? 0 : names);
}

void
Holo_FreeCameraNames(Holo_Camera_Name* names)
{
	CoTaskMemFree(names);
}

typedef struct Holo_Camera_Reader
{
	IMFSourceReader* source_reader;
	uint16_t width;
	uint16_t height;
} Holo_Camera_Reader;

Holo_Camera_Reader*
HoloCameraReader_Create(wchar_t* symbolic_name, uint16_t width, uint16_t height, uint8_t fps)
{
	bool encountered_errors = false;

	Holo_Camera_Reader* result = 0;

	IMFAttributes* device_attr     = 0;
	IMFMediaSource* media_source   = 0;
	IMFMediaType* media_type       = 0;
	IMFSourceReader* source_reader = 0;

	if (!SUCCEEDED(MFCreateAttributes(&device_attr, 2))                                                                                      ||
			!SUCCEEDED(IMFAttributes_SetGUID(device_attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) ||
			!SUCCEEDED(IMFAttributes_SetString(device_attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symbolic_name))            ||
			!SUCCEEDED(MFCreateDeviceSource(device_attr, &media_source))                                                                         ||
			!SUCCEEDED(MFCreateSourceReaderFromMediaSource(media_source, 0, &source_reader))                                                     ||
			!SUCCEEDED(MFCreateMediaType(&media_type))                                                                                           ||
			!SUCCEEDED(IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video))                                                  ||
			!SUCCEEDED(IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12))                                                    ||
			!SUCCEEDED(IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (uint64_t)width << 32 | height))                                    ||
			!SUCCEEDED(IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, (uint64_t)fps << 32 | 1))                                           ||
			!SUCCEEDED(IMFSourceReader_SetCurrentMediaType(source_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, media_type)))
	{
		encountered_errors = true;
	}

	if (!encountered_errors)
	{
		result = CoTaskMemAlloc(sizeof(Holo_Camera_Reader));

		if (result == 0) encountered_errors = true;
		else
		{
			*result = (Holo_Camera_Reader){
				.source_reader = source_reader,
				.width         = width,
				.height        = height,
			};
		}
	}

	if (device_attr != 0) IMFAttributes_Release(device_attr);
	if (media_source != 0) IMFMediaSource_Release(media_source);
	if (media_type != 0) IMFMediaType_Release(media_type);

	if (encountered_errors)
	{
		if (source_reader != 0) IMFSourceReader_Release(source_reader);
		if (result != 0) CoTaskMemFree(result);
	}

	return (encountered_errors ? 0 : result);
}

void
HoloCameraReader_Destroy(Holo_Camera_Reader** reader)
{
	if (*reader != 0)
	{
		if ((*reader)->source_reader) IMFSourceReader_Release((*reader)->source_reader);

		CoTaskMemFree(*reader);
	}
}

static void NV12ToRGB(unsigned int width, unsigned int height, uint8_t* nv12, uint32_t* rgb);
void
HoloCameraReader_ReadFrame(Holo_Camera_Reader* reader, uint32_t* frame)
{
	UINT32 drain = MF_SOURCE_READER_CONTROLF_DRAIN;
	IMFSample* sample = 0;

	for (;;)
	{
		IMFSample* s = 0;
		UINT32 flags = 0;
		HRESULT r = IMFSourceReader_ReadSample(reader->source_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, drain, &(UINT32){0}, &flags, &(UINT64){0}, &s);

		if (flags & MF_SOURCE_READERF_ERROR)
		{
			// TODO
			//// ERROR
			break;
		}
		else if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
		{
			// TODO
			//// ERROR
			break;
		}
		else if (flags & MF_SOURCE_READERF_STREAMTICK) continue;
		else
		{
			if (s == 0)
			{
				drain = 0;
				continue;
			}
			else
			{
				if (sample != 0) IMFSample_Release(sample);
				sample = s;
				if (drain != 0) continue;
				else            break;
			}
		}
	}

	IMFMediaBuffer* buffer = 0;
	HRESULT r = IMFSample_ConvertToContiguousBuffer(sample, &buffer);
	
	BYTE* data      = 0;
	UINT32 data_len = 0;
	r = IMFMediaBuffer_Lock(buffer, &data, 0, &data_len);

	NV12ToRGB(1920, 1080, data, frame);

	r = IMFMediaBuffer_Unlock(buffer);

	IMFMediaBuffer_Release(buffer);
	IMFSample_Release(sample);
}

// TODO

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

#endif
