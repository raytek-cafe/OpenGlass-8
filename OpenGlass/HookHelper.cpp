#include "pch.h"
#include "HookHelper.hpp"
#include "KNSoft/SlimDetours/SlimDetours.h"
#pragma comment(lib, "KNSoft.SlimDetours.lib")

using namespace OpenGlass;

HMODULE HookHelper::GetRemoteModuleBase(DWORD processId, LPCWSTR moduleName)
{
	HMODULE result{ nullptr };
	wil::unique_handle snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId) };
	if (!snapshot.is_valid())
	{
		return result;
	}

	wil::unique_handle processHandle{ OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId) };
	if (!processHandle)
	{
		return result;
	}

	MODULEENTRY32W me{ sizeof(me) };
	if (!Module32FirstW(snapshot.get(), &me))
	{
		return result;
	}

	do
	{
		if (!_wcsicmp(me.szModule, moduleName))
		{
			result = me.hModule;
			break;
		}
		WCHAR modulePath[MAX_PATH]{};
		if (GetModuleFileNameExW(processHandle.get(), me.hModule, modulePath, std::size(modulePath)))
		{
			if (!_wcsicmp(modulePath, moduleName))
			{
				result = me.hModule;
				break;
			}
		}
	} while (Module32NextW(snapshot.get(), &me));

	return result;
}

const uint8_t* HookHelper::FindPattern(std::span<const uint8_t> base, std::span<const uint16_t> pat)
{
	if (base.empty() || pat.empty() || pat.size() > base.size())
	{
		return nullptr;
	}

	const uint8_t* first = base.data();
	const uint8_t* last = first + base.size();

	const auto it = std::search(first, last, pat.begin(), pat.end(), [](uint8_t mem, uint16_t p)
	{
		return p == c_patwc || mem == static_cast<uint8_t>(p);
	});

	return (it == last) ? nullptr : it;
}

void HookHelper::PatchPointer(void** from, void* to, void** original)
{
	const auto unprotectedScope = Unprotect({ reinterpret_cast<uint8_t*>(from), sizeof(from)});
	if (original)
	{
		*original = InterlockedExchangePointer(from, to);
	}
	else
	{
		InterlockedExchangePointer(from, to);
	}
}

void HookHelper::PatchInstructions(uint8_t* base, std::span<const uint8_t> data)
{
	const auto unprotectedScope = HookHelper::Unprotect({ base, data.size_bytes() });
	memcpy_s(
		base,
		data.size_bytes(),
		data.data(),
		data.size_bytes()
	);
	THROW_IF_WIN32_BOOL_FALSE(FlushInstructionCache(GetCurrentProcess(), base, data.size_bytes()));
}

void HookHelper::PatchIAT(
	HMODULE moduleHandle,
	std::span<const ImportDllDetourInfo> dllsToDetour
)
{
	ULONG size = 0;
	auto importDescriptor = static_cast<PIMAGE_IMPORT_DESCRIPTOR>(
		ImageDirectoryEntryToDataEx(
			reinterpret_cast<PVOID>(moduleHandle),
			TRUE,
			IMAGE_DIRECTORY_ENTRY_IMPORT,
			&size,
			nullptr
		)
	);
	THROW_LAST_ERROR_IF_NULL(importDescriptor);

	while (importDescriptor->Name)
	{
		const ImportDllDetourInfo* currentDllDetourInfo = nullptr;
		const auto dllName = reinterpret_cast<LPCSTR>(reinterpret_cast<ULONG_PTR>(moduleHandle) + importDescriptor->Name);

		for (const auto& dllDetourInfo : dllsToDetour)
		{
			if (!_stricmp(dllName, dllDetourInfo.importDllName))
			{
				currentDllDetourInfo = &dllDetourInfo;
				break;
			}
		}

		if (currentDllDetourInfo)
		{
			auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<ULONG_PTR>(moduleHandle) + importDescriptor->FirstThunk);
			auto nameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<ULONG_PTR>(moduleHandle) + importDescriptor->OriginalFirstThunk);

			while (thunk->u1.Function)
			{
				LPCSTR importFunction = nullptr;
				auto functionAddress = reinterpret_cast<PVOID*>(&thunk->u1.Function);

				bool importedByName = !IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal);
				if (importedByName)
				{
					importFunction = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(RVA_TO_ADDR(moduleHandle, nameThunk->u1.AddressOfData))->Name;
				}
				else
				{
					importFunction = MAKEINTRESOURCEA(IMAGE_ORDINAL(nameThunk->u1.Ordinal));
				}

				for (const auto& functionDetourInfo : currentDllDetourInfo->functionsToDetour)
				{
					if (!importedByName)
					{
						if (functionDetourInfo.importFunction == importFunction)
						{
							PatchPointer(
								functionAddress,
								functionDetourInfo.detour,
								functionDetourInfo.original
							);
						}
					}
					else if (functionDetourInfo.importFunction && !strcmp(functionDetourInfo.importFunction, importFunction))
					{
						PatchPointer(
							functionAddress,
							functionDetourInfo.detour,
							functionDetourInfo.original
						);
					}
				}

				thunk++;
				nameThunk++;
			}
		}

		importDescriptor++;
	}
}

void HookHelper::PatchDelayloadIAT(
	HMODULE moduleHandle,
	std::span<const ImportDllDetourInfo> dllsToDetour
)
{
	ULONG size = 0;
	auto importDescriptor = static_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(
		ImageDirectoryEntryToDataEx(
			reinterpret_cast<PVOID>(moduleHandle),
			TRUE,
			IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
			&size,
			nullptr
		)
	);
	THROW_LAST_ERROR_IF_NULL(importDescriptor);

	while (importDescriptor->DllNameRVA)
	{
		const ImportDllDetourInfo* currentDllDetourInfo = nullptr;
		const auto dllName = reinterpret_cast<LPCSTR>(RVA_TO_ADDR(reinterpret_cast<PVOID>(moduleHandle), importDescriptor->DllNameRVA));

		for (const auto& dllDetourInfo : dllsToDetour)
		{
			if (!_stricmp(dllName, dllDetourInfo.importDllName))
			{
				currentDllDetourInfo = &dllDetourInfo;
				break;
			}
		}

		if (currentDllDetourInfo)
		{
			THROW_HR_IF(E_NOINTERFACE, importDescriptor->Attributes.RvaBased != 1);;

			auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(RVA_TO_ADDR(reinterpret_cast<PVOID>(moduleHandle), importDescriptor->ImportAddressTableRVA));
			auto nameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(RVA_TO_ADDR(reinterpret_cast<PVOID>(moduleHandle), importDescriptor->ImportNameTableRVA));

			while (thunk->u1.Function)
			{
				LPCSTR importFunction = nullptr;
				auto functionAddress = reinterpret_cast<PVOID*>(&thunk->u1.Function);

				bool importedByName = !IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal);
				if (importedByName)
				{
					importFunction = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(RVA_TO_ADDR(moduleHandle, nameThunk->u1.AddressOfData))->Name;
				}
				else
				{
					importFunction = MAKEINTRESOURCEA(IMAGE_ORDINAL(nameThunk->u1.Ordinal));
				}

				for (const auto& functionDetourInfo : currentDllDetourInfo->functionsToDetour)
				{
					if (!importedByName)
					{
						if (functionDetourInfo.importFunction == importFunction)
						{
							PatchPointer(
								functionAddress,
								functionDetourInfo.detour,
								functionDetourInfo.original
							);
						}
					}
					else if (functionDetourInfo.importFunction && !strcmp(functionDetourInfo.importFunction, importFunction))
					{
						PatchPointer(
							functionAddress,
							functionDetourInfo.detour,
							functionDetourInfo.original
						);
					}
				}

				thunk++;
				nameThunk++;
			}
		}

		importDescriptor++;
	}
}

void HookHelper::PatchFunctions(std::span<const DetourInfo> functionsToDetour, bool enable)
{
	THROW_IF_FAILED(SlimDetoursTransactionBegin());
	for (const auto& detourInfo : functionsToDetour)
	{
		if (detourInfo.original || detourInfo.detour)
		{
			HRESULT hr = enable ?
				SlimDetoursAttach(detourInfo.original, detourInfo.detour) :
				SlimDetoursDetach(detourInfo.original, detourInfo.detour);
			if (FAILED(hr))
			{
				THROW_IF_FAILED(SlimDetoursTransactionAbort());
				THROW_HR(hr);
			}
		}
	}
	THROW_IF_FAILED(SlimDetoursTransactionCommit());
}
