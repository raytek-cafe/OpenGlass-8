#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass::GlassService
{
	// client side apis
	enum class RequestType : UCHAR
	{
		OpenUserRegistry
	};
	struct RequestBuffer
	{
		RequestType type;
		HKEY dwmKey;
		HKEY personalizeKey;
	};
	HRESULT SendRequest(RequestBuffer& content);
	bool IsActive();
	bool IsDwmProcess(HANDLE processHandle);

	// service side apis
	enum class ThreadStatus
	{
		Paused,
		Running,
		Stopped
	};
	HRESULT ControlThread(
		HANDLE threadHandle,
		ThreadStatus newStatus
	);

	DWORD WINAPI ServerThreadEntryPoint(PVOID);
	DWORD WINAPI InjectionThreadEntryPoint(LPVOID);
}
