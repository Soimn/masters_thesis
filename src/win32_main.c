#include "common.h"

void AssertHandler(const char* file, int line, const char* ex);
#ifdef HOLO_DEBUG
#  define ASSERT(EX) ((EX) ? 1 : ((IsDebuggerPresent() ? *(volatile int*)0 = 0 : AssertHandler(__FILE__, __LINE__, #EX)), 0))
#else
#  define ASSERT(EX)
#endif

#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")

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

void
AssertHandler(const char* file, int line, const char* expr)
{
	FatalError("Assertion Failed\n%s(%llu): %s", file, line, expr);
	ExitProcess(-1);
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

#define PAGE_SIZE_LG2 12
#define PAGE_SIZE (1ULL << PAGE_SIZE_LG2)

typedef struct Arena
{
	u64 reserved;
	u64 cursor;
	u64 committed;
	u8* memory;
} Arena;

typedef struct Arena_Marker
{
	u64 _value;
} Arena_Marker;

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

#define Arena_PushArray(ARENA, T, LEN) (T*)Arena_Push((ARENA), sizeof(T)*(LEN), ALIGNOF(T))

wchar_t*
Arena_PushWideString(Arena* arena, wchar_t* string)
{
	wchar_t* scan = string;
	while (*scan != 0) ++scan;
	u64 len = scan - string;

	wchar_t* result = Arena_PushArray(arena, wchar_t, len + 1);
	Copy(result, string, len);

	return result;
}

void
Arena_Clear(Arena* arena)
{
	arena->cursor = 0;
}

Arena_Marker
Arena_GetMarker(Arena* arena)
{
	return (Arena_Marker){ ._value = arena->cursor };
}

Arena_Marker
ArenaMarker_Advance(Arena_Marker marker, u32 amount)
{
	return (Arena_Marker){ ._value = marker._value + amount };
}

void
Arena_PopToMarker(Arena* arena, Arena_Marker marker)
{
	ASSERT(arena->cursor >= marker._value);
	arena->cursor = marker._value;
}

#define ARENA_SCOPED_TEMP(ARENA) for (Arena_Marker arena__scoped_temp_marker = Arena_GetMarker(ARENA); arena__scoped_temp_marker._value != -1; Arena_PopToMarker((ARENA), arena__scoped_temp_marker), arena__scoped_temp_marker._value = -1)

int
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, int show_cmd)
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
			MFShutdown();
		}

		CoUninitialize();
	}

	return 0;
}
