#pragma once
#include "framework.hpp"
#include "wil.hpp"

namespace OpenGlass
{
	// {E7C32A02-29E3-42C2-87A1-BE1EFA9C8E7D}
	DEFINE_GUID(CLSID_TaskHandler,
		0xe7c32a02, 0x29e3, 0x42c2, 0x87, 0xa1, 0xbe, 0x1e, 0xfa, 0x9c, 0x8e, 0x7d);
	constexpr auto CLSID_TaskHandler_STR = L"{E7C32A02-29E3-42C2-87A1-BE1EFA9C8E7D}";
	class CTaskHandler : public winrt::implements<CTaskHandler, ITaskHandler, winrt::non_agile, winrt::no_weak_ref>
	{
		wil::unique_handle m_serverThreadHandle{ nullptr };
		wil::unique_handle m_injectionThreadHandle{ nullptr };
	public:
		STDMETHODIMP Start(
			IUnknown* pHandlerServices,
			BSTR data
		) override;
		STDMETHODIMP Stop(HRESULT* pRetCode) override;
		STDMETHODIMP Pause() override;
		STDMETHODIMP Resume() override;
	};
}