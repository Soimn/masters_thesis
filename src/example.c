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
#undef COBJMACROS
#undef UNICODE
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN

#include <stdio.h>

#include "holo_cam.h"

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
			IMFVirtualCamera* cam;
			HRESULT result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, L"Holo Cam", CLSID_HOLOCAM_STRING, 0, 0, &cam);

			result = IMFVirtualCamera_Start(cam, 0);

			result = IMFVirtualCamera_SendCameraProperty(cam, &PROPSETID_HOLOCAM, 0, 0, &(int){0}, sizeof(int), &(DWORD){0xFF00FF}, sizeof(DWORD), &(ULONG){0});

			MessageBoxA(0, "Camera has started, press OK to continue", "HoloCam", MB_OK);

			IMFVirtualCamera* cam2;
			result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, L"Holo Cam", CLSID_HOLOCAM_STRING, 0, 0, &cam2);

			result = IMFVirtualCamera_Start(cam2, 0);

			result = IMFVirtualCamera_SendCameraProperty(cam2, &PROPSETID_HOLOCAM, 0, 0, &(int){0}, sizeof(int), &(DWORD){0x00FF00}, sizeof(DWORD), &(ULONG){0});

			MessageBoxA(0, "Camera 2 has started, press OK to continue", "HoloCam", MB_OK);

			IMFVirtualCamera* cam3;
			result = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, L"Holo Cam 3", CLSID_HOLOCAM_STRING, 0, 0, &cam3);

			result = IMFVirtualCamera_Start(cam3, 0);

			IMFVirtualCamera_SendCameraProperty(cam3, &PROPSETID_HOLOCAM, 0, 0, &(int){0}, sizeof(int), &(DWORD){0xFF0000}, sizeof(DWORD), &(ULONG){0});

			MessageBoxA(0, "Camera 3 has started, press OK to continue", "HoloCam", MB_OK);

			IMFVirtualCamera_Shutdown(cam);
			IMFVirtualCamera_Release(cam);
			IMFVirtualCamera_Shutdown(cam2);
			IMFVirtualCamera_Release(cam2);
			IMFVirtualCamera_Shutdown(cam3);
			IMFVirtualCamera_Release(cam3);

			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
