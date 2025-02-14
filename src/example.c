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

#include <stdio.h>

#include "holo_cam.h"

#define PORT "3003"

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
			if      (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)                fprintf(stderr, "failed to startup winsock\n");
			else if (getaddrinfo("localhost", PORT, &hints, &info) != 0) fprintf(stderr, "failed to get address info\n");
			else
			{
				SOCKET sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
				if (sock == INVALID_SOCKET) fprintf(stderr, "failed to create socket\n");
				else
				{
					IMFVirtualCamera* cam;
					HRESULT result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, L"Holo Cam", CLSID_HOLOCAM_STRING, 0, 0, &cam);

					IMFVirtualCamera_SetBlob(cam, &GUID_HOLOCAM_PORT, PORT, sizeof(PORT));
					result = IMFVirtualCamera_Start(cam, 0);

					if (connect(sock, info->ai_addr, (int)info->ai_addrlen) == SOCKET_ERROR) fprintf(stderr, "failed to connect\n");
					else
					{
						for (unsigned int request_dims[2]; recv(sock, (char*)request_dims, sizeof(request_dims), MSG_WAITALL) > 0; )
						{
							static unsigned int jjj = 0;
							static unsigned int image[1280*960] = {0};
							for (unsigned int i = 0; i < sizeof(image)/sizeof(image[0]); ++i) image[i] = (i % 2 == 0 ? (0xFF00FF ^ jjj++) : 0);

							send(sock, (char*)image, sizeof(image), 0);
						}

						shutdown(sock, SD_SEND);
					}

					closesocket(sock);

					IMFVirtualCamera_Shutdown(cam);
					IMFVirtualCamera_Release(cam);
				}

				WSACleanup();
			}

			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
