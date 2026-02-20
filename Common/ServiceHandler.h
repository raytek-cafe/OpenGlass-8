#pragma once
#include <windef.h>

#ifdef OPENGLASS_EXPORTS
#define OPENGLASS_API __declspec(dllexport)
#else
#define OPENGLASS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
constexpr PCWSTR c_serviceName = L"OpenGlassHost";
constexpr PCWSTR c_serviceDesc = L"OpenGlass Host Service";
#else
#define SERVICE_NAME L"OpenGlassHost"
#define SERVICE_DESC L"OpenGlass Host Service"
#endif

EXTERN_C OPENGLASS_API VOID WINAPI ServiceMain(
	DWORD dwNumServicesArgs,
	LPWSTR* lpServiceArgVectors
);
