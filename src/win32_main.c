// Attributions
// Media Foundation API usage example by Mārtiņš Možeiko (https://gist.github.com/mmozeiko/a5adab1ad11ea6d0643ceb67bb8e3e19) and
// mediafoundationsamples by Aaron Clauson (https://github.com/sipsorcery/mediafoundationsamples/tree/master)
// were used to supplement the msdoc documentation

#define COBJMACROS
#define UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#include <windows.h>
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
#undef far
#undef near

#include <stdio.h>

#include "holo.h"

#ifdef HOLO_DEBUG
#  pragma comment(lib, "libucrtd.lib")
#  pragma comment(lib, "libvcruntimed.lib")
#else
#  pragma comment(lib, "libvcruntime.lib")
#endif

// TODO: prune
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfsensorgroup.lib")

#define PAGE_SIZE_LG2 12
#define PAGE_SIZE (1ULL << PAGE_SIZE_LG2)

bool
IsRunInDebugger()
{
	return IsDebuggerPresent();
}

static void
FatalError(char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, args);

	va_end(args);

	MessageBoxA(0, buffer, "Fatal Error", MB_OK | MB_ICONERROR);
}

static void
WideFatalError(wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);

	wchar_t buffer[1024];
	_vsnwprintf_s(buffer, ARRAY_LEN(buffer), _TRUNCATE, format, args);

	va_end(args);

	MessageBoxW(0, buffer, L"Fatal Error", MB_OK | MB_ICONERROR);
}

// NOTE: result must be freed with LocalFree or process exit
static wchar_t*
WideErrorMessageFromHRESULT(HRESULT hresult)
{
	wchar_t* error_msg = 0;
	DWORD result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, hresult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&error_msg, 0, 0);

	if (result == 0) error_msg = L"Failed to describe error";

	return error_msg;
}

void
AssertHandler(const char* file, int line, const char* expr)
{
	FatalError("Assertion Failed\n%s(%llu): %s", file, line, expr);
	ExitProcess(-1);
}

Arena
Arena_Create(umm reserve_size)
{
	reserve_size = ALIGN(reserve_size, PAGE_SIZE);

	u8* memory = VirtualAlloc(0, reserve_size + PAGE_SIZE, MEM_RESERVE, PAGE_READWRITE);
	
	if (memory == 0 || !VirtualAlloc(memory, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE))
	{
		//// ERROR
		FatalError("Failed to allocate memory");
		ExitProcess(-1);
	}

	return (Arena){
		.reserved  = reserve_size,
		.committed = PAGE_SIZE,
		.cursor    = 0,
		.memory    = memory,
	};
}

void
Arena_Destroy(Arena* arena)
{
	VirtualFree(arena->memory, 0, MEM_RELEASE);
	arena->memory = 0;
}

void*
Arena_Push(Arena* arena, u64 size, u8 alignment)
{
	ASSERT(size < 1ULL << 48);

	u64 result_cursor = ALIGN(arena->cursor, alignment);
	arena->cursor = result_cursor + size;

	if (arena->cursor > arena->committed)
	{
		u64 to_commit     = ALIGN(arena->cursor - arena->committed, PAGE_SIZE);
		void* commit_base = arena->memory + arena->committed;

		arena->committed += to_commit;

		if (arena->committed > arena->reserved)
		{
			//// ERROR
			FatalError("Arena overcommit");
			ExitProcess(-1);
		}
		else if (!VirtualAlloc(commit_base, to_commit, MEM_COMMIT, PAGE_READWRITE))
		{
			//// ERROR
			FatalError("Failed to commit memory");
			ExitProcess(-1);
		}
	}

	return arena->memory + result_cursor;
}

static bool
GetVideoCaptureDeviceNames(Arena* arena, u32* names_len, wchar_t*** friendly_names, wchar_t*** symbolic_names)
{
	bool succeeded = false;

	Arena_Marker marker = Arena_GetMarker(arena);

	IMFAttributes* attr   = 0;
	IMFActivate** devices = 0;
	u32 devices_len       = 0;
	if (SUCCEEDED(MFCreateAttributes(&attr, 1))                                                                                      &&
			SUCCEEDED(IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) &&
			SUCCEEDED(MFEnumDeviceSources(attr, &devices, &devices_len)))
	{
		*names_len      = devices_len;
		*friendly_names = Arena_PushArray(arena, wchar_t*, *names_len);
		*symbolic_names = Arena_PushArray(arena, wchar_t*, *names_len);

		u32 successful_runs = 0;
		for (u32 i = 0; i < devices_len; ++i)
		{
			u32 friendly_name_cap = 0;
			u32 symbolic_name_cap = 0;
			if (SUCCEEDED(IMFActivate_GetStringLength(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendly_name_cap)) &&
			    SUCCEEDED(IMFActivate_GetStringLength(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolic_name_cap)))
			{
				friendly_name_cap += 1;
				symbolic_name_cap += 1;

				(*friendly_names)[i] = Arena_PushArray(arena, wchar_t, friendly_name_cap);
				(*symbolic_names)[i] = Arena_PushArray(arena, wchar_t, symbolic_name_cap);

				if (SUCCEEDED(IMFActivate_GetString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, (*friendly_names)[i], friendly_name_cap, &(u32){0})) &&
						SUCCEEDED(IMFActivate_GetString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, (*symbolic_names)[i], symbolic_name_cap, &(u32){0})))
				{
					successful_runs += 1;
				}
			}

			IMFActivate_Release(devices[i]);
		}

		if (successful_runs == devices_len)
		{
			succeeded = true;
		}

		CoTaskMemFree(devices);
	}

	if (attr != 0) IMFAttributes_Release(attr);

	if (!succeeded) Arena_PopToMarker(arena, marker);
	return succeeded;
}

bool
CreateVideoCaptureDevice(wchar_t* symbolic_name, GUID format, u32 width, u32 height, IMFSourceReader** device)
{
	bool succeeded = false;

	IMFAttributes* attr = 0;

	if (SUCCEEDED(MFCreateAttributes(&attr, 1)))
	{
		IMFMediaSource* media_source;
		if (SUCCEEDED(IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) &&
				SUCCEEDED(IMFAttributes_SetString(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symbolic_name))            &&
				SUCCEEDED(MFCreateDeviceSource(attr, &media_source)))
		{
			if (!SUCCEEDED(MFCreateSourceReaderFromMediaSource(media_source, 0, device))) *device = 0;
			IMFMediaSource_Release(media_source);

			if (*device != 0)
			{
				IMFMediaType* media_type;
				if (SUCCEEDED(MFCreateMediaType(&media_type))                                                   &&
						SUCCEEDED(IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video))          &&
						SUCCEEDED(IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &format))                        &&
						SUCCEEDED(IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (u64)width << 32 | height)) &&
						SUCCEEDED(IMFSourceReader_SetCurrentMediaType(*device, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, media_type)))
				{
					succeeded = true;
				}

				if (media_type != 0) IMFMediaType_Release(media_type);
			}
		}

		IMFAttributes_Release(attr);
	}

	if (!succeeded && *device != 0)
	{
		IMFSourceReader_Release(*device);
		*device = 0;
	}

	return succeeded;
}

bool
GrabFrameBlocking(IMFSourceReader* device, u8* frame)
{
	bool succeeded = false;

	for (;;)
	{
		IMFSample* sample;
		u32 stream_idx;
		u32 stream_flags;
		s64 timestamp;

		if (!SUCCEEDED(IMFSourceReader_ReadSample(device, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &stream_idx, &stream_flags, &timestamp, &sample))) break;
		else
		{
			if (stream_flags & MF_SOURCE_READERF_STREAMTICK) continue; // TODO: Can STREAMTICK overlap with any other flag, and is it a problem to skip processing those?
			else
			{
				IMFMediaBuffer* buffer;
				if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
				{
					BYTE* data;
					u32 data_len;
					if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &data, 0, &data_len)))
					{
						Copy(frame, data, data_len);
						succeeded = true;

						IMFMediaBuffer_Unlock(buffer);
					}

					IMFMediaBuffer_Release(buffer);
				}
			}

			IMFSample_Release(sample);

			break;
		}
	}

	return succeeded;
}

int
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, int show_cmd)
{
	Arena arena = Arena_Create(1 << 30);

	u32* test = VirtualAlloc(0, 1920*1080*2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	HRESULT com_init = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (!SUCCEEDED(com_init)) WideFatalError(L"Failed to initialize COM\n%s", WideErrorMessageFromHRESULT(com_init));
	else
	{
		HRESULT mf_init = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
		if (!SUCCEEDED(mf_init)) WideFatalError(L"Failed to initialize Media Foundation\n%s", WideErrorMessageFromHRESULT(mf_init));
		else
		{
			IMFSourceReader* physical_device = 0;
			ARENA_SCOPED_TEMP(&arena) {
				u32 names_len            = 0;
				wchar_t** friendly_names = 0;
				wchar_t** symbolic_names = 0;
				if      (!GetVideoCaptureDeviceNames(&arena, &names_len, &friendly_names, &symbolic_names))              FatalError("Failed to get names");
				else if (names_len < 1)                                                                                  FatalError("No video capture devices connected");
				else if (!CreateVideoCaptureDevice(symbolic_names[0], MFVideoFormat_YUY2, 1920, 1080, &physical_device)) WideFatalError(L"Failed to create logical device for %s", friendly_names[0]);
			}

			if (physical_device != 0)
			{
				IMFVirtualCamera* virtual_camera;
				
				GUID virtual_camera_guid;
				CoCreateGuid(&virtual_camera_guid);

				wchar_t virtual_camera_guid_string[128];
				StringFromGUID2(&virtual_camera_guid, virtual_camera_guid_string, ARRAY_LEN(virtual_camera_guid_string));

				if (SUCCEEDED(MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session, MFVirtualCameraAccess_CurrentUser, L"HOLO Virtual Camera", virtual_camera_guid_string, 0, 0, &virtual_camera)))
				{
					IMFVirtualCamera_Start(virtual_camera, 0);
					FatalError("HHELSDFØLK");
				}

				IMFSourceReader_Release(physical_device);
			}

			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
