#include "pch.h"
#include "Util.hpp"
#include "HookHelper.hpp"
#include "SymbolParser.hpp"

using namespace OpenGlass;

BOOL CALLBACK CSymbolParser::EnumSymbolsCallback(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext
)
{
	return static_cast<BOOL>(reinterpret_cast<SymbolParserCallback*>(UserContext)(pSymInfo, SymbolSize));
}

BOOL CALLBACK CSymbolParser::SymCallback(
	[[maybe_unused]] HANDLE hProcess,
	ULONG ActionCode,
	ULONG64 CallbackData,
	ULONG64 UserContext
)
{
	if (ActionCode == CBA_EVENT)
	{
		auto& symbolParser = *reinterpret_cast<CSymbolParser*>(UserContext);
		auto event = reinterpret_cast<PIMAGEHLP_CBA_EVENTW>(CallbackData);

		return static_cast<BOOL>(symbolParser.m_eventCallback(event));
	}
	return FALSE;
}

HMODULE WINAPI CSymbolParser::MyLoadLibraryExW(
	LPCWSTR lpLibFileName,
	HANDLE hFile,
	DWORD dwFlags
)
{
	if (std::wstring loweredLibFileName{ lpLibFileName }; wcsstr(CharLowerW(loweredLibFileName.data()), L"symsrv.dll")) [[likely]]
	{
		const auto symsrvFilePath = Util::make_current_folder_file(L"symsrv.dll");
		return LoadLibraryW(symsrvFilePath.get());
	}

	return LoadLibraryExW(lpLibFileName, hFile, dwFlags);
}

CSymbolParser::CSymbolParser(SymbolEventCallback* callback) : m_eventCallback{ callback }
{
	m_LoadLibraryExW_Org = HookHelper::WriteIAT(GetModuleHandleW(L"dbghelp.dll"), "api-ms-win-core-libraryloader-l1-1-0.dll", "LoadLibraryExW", MyLoadLibraryExW);
	THROW_IF_WIN32_BOOL_FALSE(
		SymInitializeW(
			GetCurrentProcess(),
			(
				std::wstring{ L"SRV*" } +
				Util::make_current_folder_file(L"symbols").get() +
				L"*https://msdl.microsoft.com/download/symbols"
			).c_str(),
			FALSE
		)
	);

	// SYMOPT_PUBLICS_ONLY is required for ensuring all retrived function names are decorated
	// before calling UnDecorateSymbolName
	SymSetOptions(
		SYMOPT_DEBUG | 
		SYMOPT_PUBLICS_ONLY | 
		SYMOPT_NO_PROMPTS
	);
	THROW_IF_WIN32_BOOL_FALSE(
		SymRegisterCallbackW64(
			GetCurrentProcess(), 
			SymCallback, 
			reinterpret_cast<ULONG64>(this)
		)
	);
}

CSymbolParser::~CSymbolParser() noexcept
{
	THROW_IF_WIN32_BOOL_FALSE(SymCleanup(GetCurrentProcess()));
	m_LoadLibraryExW_Org = HookHelper::WriteIAT(GetModuleHandleW(L"dbghelp.dll"), "api-ms-win-core-libraryloader-l1-1-0.dll", "LoadLibraryExW", m_LoadLibraryExW_Org);
}

HRESULT CSymbolParser::LoadAndParse(PCWSTR moduleName, SymbolParserCallback* callback) try
{
	std::wstring filePath{};
	{
		wil::unique_hmodule moduleHandle
		{ 
			LoadLibraryExW(
				moduleName, 
				nullptr, 
				DONT_RESOLVE_DLL_REFERENCES
			) 
		};
		THROW_LAST_ERROR_IF_NULL(moduleHandle);
		THROW_IF_FAILED((wil::GetModuleFileNameW<std::wstring, MAX_PATH>(moduleHandle.get(), filePath)));
	}

	const auto moduleBase = SymLoadModuleExW(
		GetCurrentProcess(), 
		nullptr, 
		filePath.data(), 
		nullptr, 
		0, 
		0, 
		nullptr, 
		0
	);
	THROW_LAST_ERROR_IF(moduleBase == 0);
	const auto symCleanUp = wil::scope_exit([=]
	{
		if (moduleBase != 0)
		{
			SymUnloadModule64(GetCurrentProcess(), moduleBase);
		}
	});

	IMAGEHLP_MODULEW64 moduleInfo{ sizeof(moduleInfo) };
	THROW_IF_WIN32_BOOL_FALSE(
		SymGetModuleInfoW64(
			GetCurrentProcess(), 
			moduleBase, 
			&moduleInfo
		)
	);
	THROW_WIN32_IF_MSG(
		ERROR_FILE_NOT_FOUND, 
		*moduleInfo.LoadedPdbName == L'\0', 
		"Symbols NOT loaded for %ls\n", 
		moduleName
	);
	THROW_WIN32_IF_MSG(
		ERROR_FILE_CORRUPT, 
		(moduleInfo.SymType & 0xFFFFFFFB) == 0, 
		"Invalid symbol type %d for module %ls\n", 
		moduleInfo.SymType, 
		moduleName
	);

	THROW_IF_WIN32_BOOL_FALSE(
		SymEnumSymbols(
			GetCurrentProcess(), 
			moduleBase, 
			nullptr, 
			EnumSymbolsCallback, 
			callback
		)
	);

	return S_OK;
}
CATCH_RETURN()