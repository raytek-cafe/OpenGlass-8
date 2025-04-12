#pragma once
#include "HookHelper.hpp"
#include <Windows.UI.Composition.h>
#include <windows.ui.composition.interop.h>
#include <winrt/windows.ui.composition.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.graphics.h>

namespace winrt
{
	using namespace ::winrt::Windows::UI::Composition;
	using namespace ::winrt::Windows::Graphics;
	using namespace ::winrt::Windows::Graphics::DirectX;
	using namespace ::winrt::Windows::Foundation;
	using namespace ::winrt::Windows::Foundation::Collections;
	using namespace ::winrt::Windows::Foundation::Numerics;
}
namespace abi
{
	using namespace ::ABI::Windows::UI::Composition;
}

namespace OpenGlass
{
	DECLARE_INTERFACE_IID_(IVisualTargetPartner, IUnknown, "DBA1813C-60C5-4A42-A4D2-3380CDDCE8A1")
	{
		IFACEMETHOD(GetRoot)(ABI::Windows::UI::Composition::IVisual * *rootVisual) PURE;
		IFACEMETHOD(SetRoot)(ABI::Windows::UI::Composition::IVisual * rootVisual) PURE;
	};

	DECLARE_INTERFACE_IID_(IInteropVisualTarget, IUnknown, "EACDD04C-117E-4E17-88F4-D1B12B0E3D89")
	{
		STDMETHOD(SetRoot)(IDCompositionVisual * visual) PURE;
	};

	DECLARE_INTERFACE_IID_(IDCompositionDesktopDevicePartner, IDCompositionDesktopDevice, "D14B6158-C3FA-4BCE-9C1F-B61D8665EAB0")
	{
		FORCEINLINE HRESULT CreateSharedResource(REFIID riid, PVOID * ppvObject)
		{
			return std::invoke(
				HookHelper::vftbl_of<decltype(&IDCompositionDesktopDevicePartner::CreateSharedResource)>(this)[27], 
				this, 
				riid, 
				ppvObject
			);
		}
		FORCEINLINE HRESULT OpenSharedResourceHandle(IUnknown * unknown, HANDLE * resourceHandle)
		{
			return std::invoke(
				HookHelper::vftbl_of<decltype(&IDCompositionDesktopDevicePartner::OpenSharedResourceHandle)>(this)[28], 
				this, 
				unknown, 
				resourceHandle
			);
		}
	};
}