#include "pch.h"
#include "resource.h"
#include "Util.hpp"
#include "module.hpp"
#include "GlassService.hpp"
#if defined _M_X64
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
	#error "This platform is currently not supported"
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

bool VerifyOpenGlassHostSignature(HMODULE hostModule) noexcept
{
	WCHAR hostPath[MAX_PATH]{};
	const auto hostPathLength = GetModuleFileNameW(hostModule, hostPath, ARRAYSIZE(hostPath));
	if (!hostPathLength || hostPathLength >= ARRAYSIZE(hostPath))
	{
		return false;
	}

	WINTRUST_FILE_INFO fileInfo{};
	fileInfo.cbStruct = sizeof(fileInfo);
	fileInfo.pcwszFilePath = hostPath;

	WINTRUST_DATA trustData{};
	trustData.cbStruct = sizeof(trustData);
	trustData.dwUIChoice = WTD_UI_NONE;
	trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	trustData.dwUnionChoice = WTD_CHOICE_FILE;
	trustData.pFile = &fileInfo;
	trustData.dwStateAction = WTD_STATEACTION_VERIFY;
	// Don't do network activity in DllMain.
	trustData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL | WTD_REVOCATION_CHECK_NONE;
	trustData.dwUIContext = WTD_UICONTEXT_EXECUTE;

	GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	const auto verifyStatus = WinVerifyTrust(nullptr, &action, &trustData);

	trustData.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust(nullptr, &action, &trustData);

	return verifyStatus == ERROR_SUCCESS;
}

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  dwReason,
	[[maybe_unused]] LPVOID lpReserved
)
{
	using namespace OpenGlass;

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hModule);

			const auto dwmModule = GetModuleHandleW(L"dwm.exe");
			const auto hostModule = GetModuleHandleW(L"OpenGlassHost.exe");

			if (!dwmModule && !hostModule)
			{
				return FALSE;
			}

			if (dwmModule)
			{
				if (!GlassService::IsActive())
				{
					return FALSE;
				}
				if (!GlassService::IsDwmProcess(GetCurrentProcess()))
				{
					return FALSE;
				}
			}

			if (hostModule)
			{
				if (GlassService::IsActive())
				{
					return FALSE;
				}

#if defined(OPENGLASS_ENFORCE_SIGNING) && (OPENGLASS_ENFORCE_SIGNING)
				if (!VerifyOpenGlassHostSignature(hostModule))
				{
					return FALSE;
				}
#endif
			}

			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
	}
	return TRUE;
}
