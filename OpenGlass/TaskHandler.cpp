#include "pch.h"
#include "TaskHandler.hpp"
#include "GlassService.hpp"

using namespace OpenGlass;

STDMETHODIMP CTaskHandler::Start(
	[[maybe_unused]] IUnknown* pHandlerServices,
	[[maybe_unused]] BSTR data
)
{
	m_serverThreadHandle.reset(
		CreateThread(
			nullptr, 
			0, 
			GlassService::ServerThreadEntryPoint, 
			nullptr, 
			0,
			nullptr
		)
	);
	RETURN_LAST_ERROR_IF_NULL(m_serverThreadHandle);
	m_injectionThreadHandle.reset(
		CreateThread(
			nullptr,
			0,
			GlassService::InjectionThreadEntryPoint,
			nullptr,
			0,
			nullptr
		)
	);
	RETURN_LAST_ERROR_IF_NULL(m_injectionThreadHandle);
	return S_OK;
}

STDMETHODIMP CTaskHandler::Stop(HRESULT* pRetCode)
{
	RETURN_IF_FAILED(
		GlassService::ControlThread(
			m_injectionThreadHandle.get(),
			GlassService::ThreadStatus::Stopped
		)
	);
	RETURN_IF_FAILED(
		GlassService::ControlThread(
			m_serverThreadHandle.get(),
			GlassService::ThreadStatus::Stopped
		)
	);
	RETURN_LAST_ERROR_IF(WaitForSingleObject(m_injectionThreadHandle.get(), INFINITE) != WAIT_OBJECT_0);
	RETURN_LAST_ERROR_IF(WaitForSingleObject(m_serverThreadHandle.get(), INFINITE) != WAIT_OBJECT_0);

	RETURN_IF_WIN32_BOOL_FALSE(GetExitCodeThread(m_injectionThreadHandle.get(), reinterpret_cast<DWORD*>(pRetCode)));
	m_serverThreadHandle.reset();
	m_injectionThreadHandle.reset();
	return S_OK;
}

STDMETHODIMP CTaskHandler::Pause()
{
	return GlassService::ControlThread(
		m_injectionThreadHandle.get(),
		GlassService::ThreadStatus::Paused
	);
}

STDMETHODIMP CTaskHandler::Resume()
{
	return GlassService::ControlThread(
		m_injectionThreadHandle.get(),
		GlassService::ThreadStatus::Running
	);
}