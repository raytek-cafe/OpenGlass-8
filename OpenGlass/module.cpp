#include "pch.h"
#include "resource.h"
#include "Util.hpp"
#include "GlassService.hpp"
#include "ServiceHandler.h"

_Use_decl_annotations_ STDAPI DllGetClassObject([[maybe_unused]] REFCLSID rclsid, [[maybe_unused]] REFIID riid, LPVOID* ppv)
{
	using namespace OpenGlass;

	if (ppv)
	{
		*ppv = nullptr;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

_Use_decl_annotations_ STDAPI DllCanUnloadNow()
{
	if (winrt::get_module_lock())
	{
		return S_FALSE;
	}

	winrt::clear_factory_cache();
	return S_OK;
}

STDAPI DllRegisterServer()
{
	return E_NOTIMPL;
}

STDAPI DllUnregisterServer()
{
	return E_NOTIMPL;
}
