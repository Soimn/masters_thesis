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

			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
