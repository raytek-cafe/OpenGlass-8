#pragma once
#include "pch.h"

namespace OpenGlass
{
	class RegistryConfig
	{
	public:
		RegistryConfig(bool useHklm);

		[[nodiscard]] DWORD GetDword(const std::wstring& valueName, DWORD defaultValue) const;
		void SetDword(const std::wstring& valueName, DWORD value);
		[[nodiscard]] bool TryGetDword(const std::wstring& valueName, DWORD& value) const;

		[[nodiscard]] std::wstring GetString(const std::wstring& valueName, const std::wstring& defaultValue) const;
		void SetString(const std::wstring& valueName, const std::wstring& value);
		[[nodiscard]] bool TryGetString(const std::wstring& valueName, std::wstring& value) const;

		void DeleteValue(const std::wstring& valueName);
		[[nodiscard]] bool HasValue(const std::wstring& valueName) const;
		[[nodiscard]] bool HasKey() const;

		[[nodiscard]] bool IsHklm() const noexcept { return m_useHklm; }

	private:
		wil::unique_hkey OpenKey(bool readOnly) const;
		
		bool m_useHklm;
		std::wstring m_subKey;
	};
}
