#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "wil.hpp"

namespace OpenGlass::HookHelper
{
	struct pss_snapshot
	{
		static void close(HPSS snapshot) WI_NOEXCEPT
		{
			::PssFreeSnapshot(GetCurrentProcess(), snapshot);
		}
	};
	typedef wil::unique_any<HPSS, decltype(&pss_snapshot::close), pss_snapshot::close, wil::details::pointer_access_all> unique_pss_snapshot_local;
	typedef wil::unique_any<HPSSWALK, decltype(&::PssWalkMarkerFree), ::PssWalkMarkerFree, wil::details::pointer_access_all> unique_pss_walk_marker;

	FORCEINLINE auto unprotect(PVOID address, SIZE_T size)
	{
		THROW_HR_IF_NULL(E_INVALIDARG, address);
		DWORD oldProtect{ 0 };
		THROW_IF_WIN32_BOOL_FALSE(
			VirtualProtect(
				address,
				size,
				PAGE_EXECUTE_READWRITE,
				&oldProtect
			)
		);
		return wil::scope_exit([=]
		{
			auto unused = 0ul;
			THROW_IF_WIN32_BOOL_FALSE(
				VirtualProtect(
					address,
					size,
					oldProtect,
					&unused
				)
			);
		});
	}
	void PatchInstructions(void* memory, const UCHAR* data, size_t length);
	void WritePointerInternal(PVOID* pointerAddress, PVOID value, PVOID* originalValue = nullptr);
	template <typename T1, typename T2> FORCEINLINE void WritePointer(T1 address, T2 value, T2* originalValue = nullptr) { WritePointerInternal(reinterpret_cast<PVOID*>(address), reinterpret_cast<PVOID>(value), reinterpret_cast<PVOID*>(originalValue)); }
	
	HMODULE GetProcessModule(HANDLE processHandle, std::wstring_view dllPath);

	void WalkIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(PVOID* functionAddress, LPCSTR functionNameOrOrdinal, bool importedByName)> callback);
	void WalkDelayloadIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(HMODULE* moduleHandle, PVOID* functionAddress, LPCSTR functionNameOrOrdinal, bool importedByName)> callback);

	PVOID* GetIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal);
	std::pair<HMODULE*, PVOID*> GetDelayloadIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, bool resolveAPI = false);
	PVOID WriteIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, PVOID detourFunction);
	std::pair<HMODULE, PVOID> WriteDelayloadIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, PVOID detourFunction, std::optional<HMODULE> newModuleHandle = std::nullopt);
	void ResolveDelayloadIAT(const std::pair<HMODULE*, PVOID*>& info, PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal);

	template <typename T=PVOID>
	FORCEINLINE T* vftbl_of(const void* This)
	{
		return reinterpret_cast<T*>(*reinterpret_cast<PVOID*>(const_cast<void*>(This)));
	}

	struct OffsetStorage
	{
		LONGLONG value{ 0 };

		FORCEINLINE bool IsValid() const { return value != 0; }
		template <typename T = PVOID, typename T2 = PVOID>
		FORCEINLINE T To(T2 baseAddress) const { if (baseAddress == 0 || !IsValid()) { return 0; } return reinterpret_cast<T>(RVA_TO_ADDR(baseAddress, value)); }
		template <typename T>
		FORCEINLINE void To(PVOID baseAddress, T& value) const { value = To<T>(baseAddress); }
		template <typename T = PVOID>
		static FORCEINLINE auto From(T baseAddress, T targetAddress) { return OffsetStorage{ LONGLONG(targetAddress) - LONGLONG(baseAddress) }; }
		static FORCEINLINE auto From(PVOID baseAddress, PVOID targetAddress) { if (!baseAddress || !targetAddress) { return OffsetStorage{ 0 }; } return OffsetStorage{ reinterpret_cast<LONGLONG>(targetAddress) - reinterpret_cast<LONGLONG>(baseAddress) }; }
	};

	namespace Detours
	{
		// Call single or multiple Attach/Detach in the callback
		HRESULT Write(const std::function<void()>&& callback);
		// Install an inline hook using Detours
		void Attach(PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
		// Uninstall an inline hook using Detours
		void Detach(PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);

		template <typename T>
		FORCEINLINE void Attach(T* org, T detour)
		{
			Attach(reinterpret_cast<PVOID*>(org), reinterpret_cast<PVOID>(detour));
		}
		template <typename T>
		FORCEINLINE void Detach(T* org, T detour)
		{
			Detach(reinterpret_cast<PVOID*>(org), reinterpret_cast<PVOID>(detour));
		}
	}
}
