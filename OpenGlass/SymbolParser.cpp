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
	return static_cast<BOOL>(reinterpret_cast<Callback*>(UserContext)(pSymInfo, SymbolSize));
}

CSymbolParser::CSymbolParser(LPCWSTR symbolsPath)
{
	THROW_IF_WIN32_BOOL_FALSE(
		SymInitializeW(
			GetCurrentProcess(),
			symbolsPath,
			FALSE
		)
	);

	// SYMOPT_PUBLICS_ONLY is required for ensuring all retrived function names are decorated
	// before calling UnDecorateSymbolName
	SymSetOptions(
		SYMOPT_PUBLICS_ONLY |
		SYMOPT_EXACT_SYMBOLS |
		SYMOPT_IGNORE_NT_SYMPATH
	);
}

CSymbolParser::~CSymbolParser() noexcept
{
	THROW_IF_WIN32_BOOL_FALSE(SymCleanup(GetCurrentProcess()));
}

HRESULT CSymbolParser::ParsePdb(HMODULE moduleHandle, Callback* callback) try
{
	CHAR modulePath[MAX_PATH]{};
	THROW_LAST_ERROR_IF(GetModuleFileNameA(moduleHandle, modulePath, MAX_PATH) == 0);

	DWORD64 moduleBase = 0;
	{
		MODULEINFO moduleInfo{};
		GetModuleInformation(GetCurrentProcess(), moduleHandle, &moduleInfo, sizeof(moduleInfo));
		moduleBase = SymLoadModuleEx(
			GetCurrentProcess(),
			nullptr,
			modulePath,
			nullptr,
			reinterpret_cast<DWORD64>(moduleInfo.lpBaseOfDll),
			moduleInfo.SizeOfImage,
			nullptr,
			0
		);
		THROW_LAST_ERROR_IF(moduleBase == 0);
	}
	const auto symCleanUp = wil::scope_exit([=]
	{
		if (moduleBase != 0)
		{
			SymUnloadModule64(GetCurrentProcess(), moduleBase);
		}
	});

	{
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
			moduleInfo.LoadedPdbName[0] == L'\0',
			"Symbols NOT loaded for module %p",
			moduleHandle
		);
		THROW_WIN32_IF_MSG(
			ERROR_FILE_CORRUPT,
			(moduleInfo.SymType & 0xFFFFFFFB) == 0,
			"Invalid symbol type %d for module %p",
			moduleInfo.SymType,
			moduleHandle
		);
	}

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
