// Attributions
// the Media Foundation API usage example by Mārtiņš Možeiko was used to supplement msdocs, https://gist.github.com/mmozeiko/a5adab1ad11ea6d0643ceb67bb8e3e19

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
	_vsnwprintf_s(buffer, ARRAY_SIZE(buffer), _TRUNCATE, format, args);

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
CreateVideoCaptureDevice(wchar_t* symbolic_name, IMFMediaSource** device)
{
	bool succeeded = false;

	IMFAttributes* attr = 0;

	if (SUCCEEDED(MFCreateAttributes(&attr, 1)))
	{
		if (SUCCEEDED(IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) &&
				SUCCEEDED(IMFAttributes_SetString(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symbolic_name))            &&
				SUCCEEDED(MFCreateDeviceSource(attr, device)))
		{
			succeeded = true;
		}

		IMFAttributes_Release(attr);
	}

	return succeeded;
}

int
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd)
{
	Arena arena = Arena_Create(1 << 30);

	HRESULT com_init = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (!SUCCEEDED(com_init)) WideFatalError(L"Failed to initialize COM\n%s", WideErrorMessageFromHRESULT(com_init));
	else
	{
		HRESULT mf_init = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
		if (!SUCCEEDED(mf_init)) WideFatalError(L"Failed to initialize Media Foundation\n%s", WideErrorMessageFromHRESULT(mf_init));
		else
		{
			ARENA_SCOPED_TEMP(&arena) {
				u32 names_len            = 0;
				wchar_t** friendly_names = 0;
				wchar_t** symbolic_names = 0;
				IMFMediaSource* device   = 0;
				if      (!GetVideoCaptureDeviceNames(&arena, &names_len, &friendly_names, &symbolic_names)) FatalError("Failed to get names");
				else if (names_len < 1)                                                                     FatalError("No video capture devices connected");
				else if (!CreateVideoCaptureDevice(symbolic_names[0], &device))                             WideFatalError(L"Failed to create logical device for %s", friendly_names[0]);
				else
				{
					IMFSourceReader* cam_reader = 0;
					if (!SUCCEEDED(MFCreateSourceReaderFromMediaSource(device, 0, &cam_reader))) cam_reader = 0;
					IMFMediaSource_Release(device);

					if (cam_reader != 0)
					{
						for (u32 i = 0; i < U32_MAX; ++i)
						{
							IMFMediaType* media_type = 0;
							HRESULT result = IMFSourceReader_GetNativeMediaType(cam_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &media_type);
							if      (result == MF_E_NO_MORE_TYPES) break;
							else if (!SUCCEEDED(result)) WideFatalError(L"Failed to query video capture device media format.\n%s", WideErrorMessageFromHRESULT(result));
							else
							{
								GUID major_type;
								IMFMediaType_GetMajorType(media_type, &major_type);

								if (!IsEqualGUID(&major_type, &MFMediaType_Video)) FatalError("Media type at index %u: non video type", i);
								else
								{
									GUID sub_type;
									HRESULT result = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &sub_type);
									if (!SUCCEEDED(result)) WideFatalError(L"Failed to get sub type.\n%s", WideErrorMessageFromHRESULT(result));
									else
									{
										wchar_t buffer[1024];
										StringFromGUID2(&sub_type, (LPOLESTR)buffer, ARRAY_SIZE(buffer));
										buffer[ARRAY_SIZE(buffer)-1] = 0;

										printf("Media type at index %u: %.*s", i, 4, (char*)&sub_type.Data1);
										wprintf(L"%s\n", buffer);
									}
								}

								IMFMediaType_Release(media_type);
							}
						}

						IMFSourceReader_Release(cam_reader);
					}
				}
			}

			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
