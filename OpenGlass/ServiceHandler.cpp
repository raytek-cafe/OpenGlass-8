#include "pch.h"
#include "ServiceHandler.h"
#include "GlassService.hpp"
#include "Util.hpp"

struct ServiceHandlerContext
{
	SERVICE_STATUS_HANDLE statusHandle{ nullptr };
	wil::unique_handle serverThreadHandle{ nullptr };
	wil::unique_handle injectionThreadHandle{ nullptr };
};

void ReportServiceStatus(
	SERVICE_STATUS_HANDLE statusHandle,
	DWORD currentState,
	DWORD win32ExitCode = NO_ERROR,
	DWORD waitHint = 0
)
{
	static SERVICE_STATUS s_status
	{
		.dwServiceType = SERVICE_WIN32_OWN_PROCESS,
		.dwCurrentState = SERVICE_START_PENDING,
		.dwControlsAccepted = 0,
		.dwWin32ExitCode = NO_ERROR,
		.dwServiceSpecificExitCode = 0,
		.dwCheckPoint = 0,
		.dwWaitHint = 0
	};
	s_status.dwCurrentState = currentState;
	s_status.dwWin32ExitCode = win32ExitCode;
	s_status.dwWaitHint = waitHint;

	switch (currentState)
	{
	case SERVICE_START_PENDING:
	case SERVICE_STOP_PENDING:
	case SERVICE_PAUSE_PENDING:
	case SERVICE_CONTINUE_PENDING:
	{
		s_status.dwControlsAccepted = 0;
		s_status.dwCheckPoint++;
		break;
	}
	case SERVICE_RUNNING:
	{
		s_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
		s_status.dwCheckPoint = 0;
		break;
	}
	case SERVICE_PAUSED:
	{
		s_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
		s_status.dwCheckPoint = 0;
		break;
	}
	default:
		s_status.dwControlsAccepted = 0;
		s_status.dwCheckPoint = 0;
		break;
	}

	LOG_IF_WIN32_BOOL_FALSE(SetServiceStatus(statusHandle, &s_status));
}

DWORD WINAPI HandlerEx(
	DWORD dwControl,
	[[maybe_unused]] DWORD dwEventType,
	[[maybe_unused]] LPVOID lpEventData,
	[[maybe_unused]] LPVOID lpContext
)
{
	using namespace OpenGlass;

	const auto& context = *reinterpret_cast<ServiceHandlerContext*>(lpContext);

	switch (dwControl)
	{
	case SERVICE_CONTROL_INTERROGATE:
	{
		return NO_ERROR;
	}

	case SERVICE_CONTROL_STOP:
	{
		ReportServiceStatus(context.statusHandle, SERVICE_STOP_PENDING, NO_ERROR, 10);
		LOG_IF_FAILED(
			GlassService::ControlThread(
				context.injectionThreadHandle.get(),
				GlassService::ThreadStatus::Stopped
			)
		);
		return NO_ERROR;
	}

	case SERVICE_CONTROL_PAUSE:
	{
		ReportServiceStatus(context.statusHandle, SERVICE_PAUSE_PENDING, NO_ERROR, 10);
		LOG_IF_FAILED(
			GlassService::ControlThread(
				context.injectionThreadHandle.get(),
				GlassService::ThreadStatus::Paused
			)
		);
		ReportServiceStatus(context.statusHandle, SERVICE_PAUSED);
		return NO_ERROR;
	}

	case SERVICE_CONTROL_CONTINUE:
	{
		ReportServiceStatus(context.statusHandle, SERVICE_CONTINUE_PENDING, NO_ERROR, 10);
		LOG_IF_FAILED(
			GlassService::ControlThread(
				context.injectionThreadHandle.get(),
				GlassService::ThreadStatus::Running
			)
		);
		ReportServiceStatus(context.statusHandle, SERVICE_RUNNING);
		return NO_ERROR;
	}

	default:
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
}

EXTERN_C VOID WINAPI ServiceMain(
	[[maybe_unused]] DWORD dwNumServicesArgs,
	[[maybe_unused]] LPWSTR* lpServiceArgVectors
)
{
	using namespace OpenGlass;

	wil::SetResultLoggingCallback([](const wil::FailureInfo& failure) static noexcept
	{
		WCHAR logMessage[MAX_PATH]{};
		if (FAILED(wil::GetFailureLogString(logMessage, MAX_PATH, failure)))
		{
			return;
		}

		OutputDebugStringW(logMessage);
	});

	ServiceHandlerContext context{};

	// Register service control handler
	context.statusHandle = RegisterServiceCtrlHandlerExW(c_serviceName, HandlerEx, &context);
	if (!context.statusHandle)
	{
		const auto error = GetLastError();
		LOG_WIN32(error);
		ReportServiceStatus(context.statusHandle, SERVICE_STOPPED, error, 0);
		return;
	}

	// Initialize service state
	ReportServiceStatus(context.statusHandle, SERVICE_START_PENDING, NO_ERROR, 10);

	// Start the OpenGlass service
	context.serverThreadHandle.reset(
		CreateThread(
			nullptr,
			0,
			GlassService::ServerThreadEntryPoint,
			nullptr,
			0,
			nullptr
		)
	);
	if (!context.serverThreadHandle)
	{
		const auto error = GetLastError();
		LOG_WIN32(error);
		ReportServiceStatus(context.statusHandle, SERVICE_STOPPED, error, 0);
		return;
	}

	context.injectionThreadHandle.reset(
		CreateThread(
			nullptr,
			0,
			GlassService::InjectionThreadEntryPoint,
			nullptr,
			0,
			nullptr
		)
	);
	if (!context.injectionThreadHandle)
	{
		const auto error = GetLastError();
		LOG_WIN32(error);
		ReportServiceStatus(context.statusHandle, SERVICE_STOP_PENDING, 10);
		LOG_IF_FAILED(
			GlassService::ControlThread(
				context.injectionThreadHandle.get(),
				GlassService::ThreadStatus::Stopped
			)
		);
		LOG_LAST_ERROR_IF(WaitForSingleObject(context.injectionThreadHandle.get(), INFINITE) == WAIT_FAILED);
		ReportServiceStatus(context.statusHandle, SERVICE_STOPPED, error, 0);
		return;
	}

	// Report running status
	ReportServiceStatus(context.statusHandle, SERVICE_RUNNING);

	// Wait for stop signal
	LOG_LAST_ERROR_IF(
		WaitForSingleObject(
			context.injectionThreadHandle.get(),
			INFINITE
		) == WAIT_FAILED
	);

	LOG_IF_FAILED(
		GlassService::ControlThread(
			context.serverThreadHandle.get(),
			GlassService::ThreadStatus::Stopped
		)
	);
	LOG_LAST_ERROR_IF(
		WaitForSingleObject(
			context.serverThreadHandle.get(),
			INFINITE
		) == WAIT_FAILED
	);
	ReportServiceStatus(context.statusHandle, SERVICE_STOPPED);

	return;
}
