#pragma once
#include "framework.hpp"

_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
_Use_decl_annotations_ STDAPI DllCanUnloadNow();
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();

STDAPI StartupService() noexcept;
STDAPI ShutdownService() noexcept;
STDAPI InstallApp() noexcept;
STDAPI UninstallApp() noexcept;