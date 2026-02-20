#include "pch.h"
#include "RegistryConfig.hpp"

namespace OpenGlass
{
	RegistryConfig::RegistryConfig(bool useHklm)
		: m_useHklm(useHklm)
		, m_subKey(L"SOFTWARE\\Microsoft\\Windows\\DWM")
	{
	}

	wil::unique_hkey RegistryConfig::OpenKey(bool readOnly) const
	{
		HKEY root = m_useHklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
		wil::unique_hkey key;
		
		wil::reg::key_access access = wil::reg::key_access::read;
		if (!readOnly)
		{
			access = wil::reg::key_access::readwrite;
		}

		// Try to open existing key
		if (FAILED(wil::reg::open_unique_key_nothrow(root, m_subKey.c_str(), key, access)))
		{
			// If writing, try to create it
			if (!readOnly)
			{
				if (FAILED(wil::reg::create_unique_key_nothrow(root, m_subKey.c_str(), key, access)))
				{
					// Fallback
				}
			}
		}

		return key;
	}

	DWORD RegistryConfig::GetDword(const std::wstring& valueName, DWORD defaultValue) const
	{
		auto key = OpenKey(true);
		if (!key) return defaultValue;

		DWORD value = 0;
		if (SUCCEEDED(wil::reg::get_value_dword_nothrow(key.get(), valueName.c_str(), &value)))
		{
			return value;
		}
		return defaultValue;
	}

	void RegistryConfig::SetDword(const std::wstring& valueName, DWORD value)
	{
		auto key = OpenKey(false);
		if (!key) return; 

		wil::reg::set_value_dword_nothrow(key.get(), valueName.c_str(), value);
	}

	bool RegistryConfig::TryGetDword(const std::wstring& valueName, DWORD& value) const
	{
		auto key = OpenKey(true);
		if (!key) return false;

		return SUCCEEDED(wil::reg::get_value_dword_nothrow(key.get(), valueName.c_str(), &value));
	}

	std::wstring RegistryConfig::GetString(const std::wstring& valueName, const std::wstring& defaultValue) const
	{
		auto key = OpenKey(true);
		if (!key) return defaultValue;

		wil::unique_cotaskmem_string result;
		if (SUCCEEDED(wil::reg::get_value_string_nothrow(key.get(), valueName.c_str(), result)))
		{
			return std::wstring(result.get());
		}
		return defaultValue;
	}

	void RegistryConfig::SetString(const std::wstring& valueName, const std::wstring& value)
	{
		auto key = OpenKey(false);
		if (!key) return;

		wil::reg::set_value_string_nothrow(key.get(), valueName.c_str(), value.c_str());
	}

	bool RegistryConfig::TryGetString(const std::wstring& valueName, std::wstring& value) const
	{
		auto key = OpenKey(true);
		if (!key) return false;

		wil::unique_cotaskmem_string result;
		if (SUCCEEDED(wil::reg::get_value_string_nothrow(key.get(), valueName.c_str(), result)))
		{
			value = result.get();
			return true;
		}
		return false;
	}

	void RegistryConfig::DeleteValue(const std::wstring& valueName)
	{
		auto key = OpenKey(false);
		if (!key) return;
		RegDeleteValueW(key.get(), valueName.c_str());
	}

	bool RegistryConfig::HasValue(const std::wstring& valueName) const
	{
		auto key = OpenKey(true);
		if (!key) return false;
		return RegQueryValueExW(key.get(), valueName.c_str(), nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
	}

	bool RegistryConfig::HasKey() const
	{
		auto key = OpenKey(true);
		return key != nullptr;
	}
}
