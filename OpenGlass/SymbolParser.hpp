#pragma once
#include "pch.h"
#include "HookHelper.hpp"

namespace OpenGlass
{
	class CSymbolParser
	{
		static BOOL CALLBACK EnumSymbolsCallback(
			PSYMBOL_INFO pSymInfo,
			ULONG SymbolSize,
			PVOID UserContext
		);
	public:
		CSymbolParser(LPCWSTR symbolsPath);
		~CSymbolParser() noexcept;

		using Callback = bool(PSYMBOL_INFO info, ULONG size);
		HRESULT ParsePdb(HMODULE moduleHandle, Callback* callback);
	};
}
