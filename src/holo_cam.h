#pragma once

#include <stdint.h>
#include <stdbool.h>

// NOTE: preconditions: COM, MF and WSA init

bool Holo_InitDependencies(void);
void Holo_CleanupDepdencies(void);

typedef struct Holo_Cam Holo_Cam;
Holo_Cam* HoloCam_Create(wchar_t* unique_name, uint16_t width, uint16_t height, uint16_t port);
void HoloCam_Destroy(Holo_Cam** cam);
bool HoloCam_Present(Holo_Cam* cam, uint32_t* image);

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

// {ED443DC9-32AF-48DA-9859-DA809784E768}
DEFINE_GUID(PROPSETID_HOLOCAM, 0xed443dc9, 0x32af, 0x48da, 0x98, 0x59, 0xda, 0x80, 0x97, 0x84, 0xe7, 0x68);

// {79809298-5754-42D6-974A-EF1292A150FF}
DEFINE_GUID(GUID_HOLOCAM_PORT, 0x79809298, 0x5754, 0x42d6, 0x97, 0x4a, 0xef, 0x12, 0x92, 0xa1, 0x50, 0xff);

// {4A7C1F91-A83F-425C-AADF-827682DDFADF}
DEFINE_GUID(GUID_HOLOCAM_FRAME_SIZE, 0x4a7c1f91, 0xa83f, 0x425c, 0xaa, 0xdf, 0x82, 0x76, 0x82, 0xdd, 0xfa, 0xdf);
#endif

#ifdef HOLOCAM_IMPLEMENTATION

#define COBJMACROS
#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#include <mfvirtualcamera.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

bool
Holo_InitDependencies()
{
	return (SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))   &&
					SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET)) && // TODO: Fix version?
					WSAStartup(MAKEWORD(2, 2), &(WSADATA){0}) == 0);
}

void
Holo_CleanupDepdencies()
{
	WSACleanup();
	MFShutdown();
	CoUninitialize();
}

typedef struct Holo_Cam
{
	struct IMFVirtualCamera* handle;
	SOCKET socket;
	uint16_t width;
	uint16_t height;
} Holo_Cam;

Holo_Cam*
HoloCam_Create(wchar_t* unique_name, uint16_t width, uint16_t height, uint16_t port)
{
	Holo_Cam* cam = 0;

	IMFVirtualCamera* cam_handle = 0;
	HRESULT result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, unique_name, CLSID_HOLOCAM_STRING, 0, 0, &cam_handle);

	char port_string[6] = {
		'0' + (port/10000) % 10,
		'0' + (port/1000) % 10,
		'0' + (port/100) % 10,
		'0' + (port/10) % 10,
		'0' + (port/1) % 10,
		0,
	};

	if (SUCCEEDED(result)) result = IMFVirtualCamera_SetBlob(cam_handle, &GUID_HOLOCAM_PORT, port_string, sizeof(port_string));
	if (SUCCEEDED(result)) result = IMFVirtualCamera_SetUINT64(cam_handle, &GUID_HOLOCAM_FRAME_SIZE, (uint64_t)width << 32 | height);
	if (SUCCEEDED(result)) result = IMFVirtualCamera_Start(cam_handle, 0);

	SOCKET sock = 0;
	if (SUCCEEDED(result))
	{
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
	return (recv(cam->socket, &(char){0}, 1, MSG_WAITALL) <= 0);
}

#endif
