#pragma once
#include "pch.h"
#include "HookHelper.hpp"

namespace OpenGlass
{
	using SymbolEventCallback = bool(PIMAGEHLP_CBA_EVENTW event);
	using SymbolParserCallback = bool(PSYMBOL_INFO info, ULONG size);

	class CSymbolParser
	{
		static HMODULE WINAPI MyLoadLibraryExW(
			LPCWSTR lpLibFileName,
			HANDLE hFile,
			DWORD dwFlags
		);
		static BOOL CALLBACK EnumSymbolsCallback(
			PSYMBOL_INFO pSymInfo,
			ULONG SymbolSize,
			PVOID UserContext
		);
		static BOOL CALLBACK SymCallback(
			HANDLE hProcess,
			ULONG ActionCode,
			ULONG64 CallbackData,
			ULONG64 UserContext
		);

		PVOID m_LoadLibraryExW_Org{ nullptr };
		SymbolEventCallback* m_eventCallback{ nullptr };
	public:
		CSymbolParser(SymbolEventCallback* callback);
		~CSymbolParser() noexcept;

		HRESULT LoadAndParse(PCWSTR moduleName, SymbolParserCallback* callback);
	};
}