#include "pch.h"
#include "resource.h"
#include "Util.hpp"
#include "module.hpp"
#if defined _M_IX86
	#error "Currently not support this architecture"
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
	#error "Currently not support this architecture"
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
	#error "Currently not support this architecture"
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


 _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR 
[[nodiscard]] void* operator new(size_t size) noexcept(false)
{
	auto memory = HeapAlloc(OpenGlass::Util::g_processHeap, 0, size);
	THROW_LAST_ERROR_IF_NULL(memory);
	return memory;
}
_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(size) _VCRT_ALLOCATOR 
[[nodiscard]] void* operator new(
	size_t size,
	std::nothrow_t const&
) noexcept
{
	return HeapAlloc(OpenGlass::Util::g_processHeap, 0, size);
}
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
[[nodiscard]] void* operator new[](
	size_t size
)
{
	auto memory = HeapAlloc(OpenGlass::Util::g_processHeap, 0, size);
	THROW_LAST_ERROR_IF_NULL(memory);
	return memory;
}
_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
[[nodiscard]] void* operator new[](
	size_t size,
	std::nothrow_t const&
) noexcept
{
	return HeapAlloc(OpenGlass::Util::g_processHeap, 0, size);
}
void operator delete(void* ptr) noexcept
{
	FAIL_FAST_IF_NULL(ptr);
	HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
}
void operator delete(
	void* ptr,
	std::nothrow_t const&
) noexcept
{
	if (ptr)
	{
		HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
	}
}
void operator delete[](
	void* ptr
) noexcept
{
	FAIL_FAST_IF_NULL(ptr);
	HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
}
void operator delete[](
	void* ptr,
	std::nothrow_t const&
) noexcept
{
	if (ptr)
	{
		HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
	}
}
void operator delete(
	void* ptr,
	[[maybe_unused]] size_t size
) noexcept 
{
	FAIL_FAST_IF_NULL(ptr);
	HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
}
void operator delete[](
	void* ptr,
	[[maybe_unused]] size_t size
) noexcept
{
	FAIL_FAST_IF_NULL(ptr);
	HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
}

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  dwReason,
	[[maybe_unused]] LPVOID lpReserved
)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hModule);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
	}
	return TRUE;
}

struct ExecutionParameters
{
	enum class CommandAction : UCHAR
	{
		Unknown,
		Startup,
		Shutdown,
		Install,
		Uninstall,
		Help
	} type{ CommandAction::Unknown };
	bool reserved{ false };
};
ExecutionParameters AnalyseCommandLine(LPCWSTR lpCmdLine)
{
	int args{ 0 };
	auto argv = CommandLineToArgvW(lpCmdLine, &args);
	ExecutionParameters params{};

	for (int i = 0; i < args; i++)
	{
		if (!_wcsicmp(argv[i], L"/startup") || !_wcsicmp(argv[i], L"-startup") || !_wcsicmp(argv[i], L"--startup"))
		{
			params.type = ExecutionParameters::CommandAction::Startup;
		}
		if (!_wcsicmp(argv[i], L"/shutdown") || !_wcsicmp(argv[i], L"-shutdown") || !_wcsicmp(argv[i], L"--shutdown"))
		{
			params.type = ExecutionParameters::CommandAction::Shutdown;
		}
		if (!_wcsicmp(argv[i], L"/install") || !_wcsicmp(argv[i], L"/i") || !_wcsicmp(argv[i], L"-install") || !_wcsicmp(argv[i], L"-i") || !_wcsicmp(argv[i], L"--install"))
		{
			params.type = ExecutionParameters::CommandAction::Install;
		}
		if (!_wcsicmp(argv[i], L"/uninstall") || !_wcsicmp(argv[i], L"/u") || !_wcsicmp(argv[i], L"-uninstall") || !_wcsicmp(argv[i], L"-u") || !_wcsicmp(argv[i], L"--uninstall"))
		{
			params.type = ExecutionParameters::CommandAction::Uninstall;
		}
		if (!_wcsicmp(argv[i], L"/help") || !_wcsicmp(argv[i], L"/h") || !_wcsicmp(argv[i], L"-help") || !_wcsicmp(argv[i], L"-h") || !_wcsicmp(argv[i], L"--help") || !_wcsicmp(argv[i], L"-?") || !_wcsicmp(argv[i], L"/?"))
		{
			params.type = ExecutionParameters::CommandAction::Help;
		}
	}

	return params;
}
EXTERN_C int CALLBACK OpenGlass_RunDLL(
	[[maybe_unused]] HWND hWnd,
	[[maybe_unused]] HINSTANCE hInstance,
	LPCSTR    lpCmdLine,
	[[maybe_unused]] int       nCmdShow
)
{
	using namespace OpenGlass;

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	RETURN_IF_FAILED(SetThreadDescription(GetCurrentThread(), L"OpenGlass Client Thread"));

	RETURN_IF_FAILED(RoInitialize(RO_INIT_MULTITHREADED));
	wil::unique_rouninitialize_call wrtScope{};

	// Convert the ansi string back to unicode string
	HRESULT hr{ S_OK };
	std::unique_ptr<WCHAR[]> convertedCommandLine{};
	RETURN_IF_FAILED(Util::MB2WC(convertedCommandLine, lpCmdLine));

	auto params = AnalyseCommandLine(convertedCommandLine.get());
	switch (params.type)
	{
	case ExecutionParameters::CommandAction::Startup:
	{
		hr = StartupService();
		if (FAILED(hr))
		{
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				Util::GetResourceStringView<IDS_STRING102>().data(),
				Util::to_error_string(hr).c_str(),
				TDCBF_CLOSE_BUTTON,
				TD_ERROR_ICON,
				nullptr
			);
		}
		break;
	}
	case ExecutionParameters::CommandAction::Shutdown:
	{
		hr = ShutdownService();
		if (true)
		{
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				Util::GetResourceStringView<IDS_STRING102>().data(),
				Util::to_error_string(hr).c_str(),
				TDCBF_CLOSE_BUTTON,
				FAILED(hr) ? TD_ERROR_ICON : TD_INFORMATION_ICON,
				nullptr
			);
		}
		break;
	}
	case ExecutionParameters::CommandAction::Install:
	{
		hr = InstallApp();
		if (true)
		{
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				Util::GetResourceStringView<IDS_STRING102>().data(),
				Util::to_error_string(hr).c_str(),
				TDCBF_CLOSE_BUTTON,
				FAILED(hr) ? TD_ERROR_ICON : TD_INFORMATION_ICON,
				nullptr
			);
		}
		break;
	}
	case ExecutionParameters::CommandAction::Uninstall:
	{
		hr = UninstallApp();
		if (true)
		{
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				Util::GetResourceStringView<IDS_STRING102>().data(),
				Util::to_error_string(hr).c_str(),
				TDCBF_CLOSE_BUTTON,
				FAILED(hr) ? TD_ERROR_ICON : TD_INFORMATION_ICON,
				nullptr
			);
		}
		break;
	}
	case ExecutionParameters::CommandAction::Help:
	{
		hr = S_OK;
		if (true)
		{
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				L"Help",
				L"--help, -help, -h, /h, -?, /?\n--startup, -startup, /startup\n--shutdown, -shutdown, /shutdown\n--install, -i, /install, /i\n--uninstall, -u, /uninstall, /u",
				TDCBF_OK_BUTTON,
				TD_INFORMATION_ICON,
				nullptr
			);
		}
		break;
	}
	default:
		hr = E_INVALIDARG;
		if (true)
		{
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				Util::GetResourceStringView<IDS_STRING102>().data(),
				Util::to_error_string(hr).c_str(),
				TDCBF_CLOSE_BUTTON,
				TD_ERROR_ICON,
				nullptr
			);
		}
		break;
	}

	return hr;
}