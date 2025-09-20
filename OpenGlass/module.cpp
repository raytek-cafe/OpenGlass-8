#include "pch.h"
#include "resource.h"
#include "Util.hpp"
#include "GlassService.hpp"
#include "TaskHandler.hpp"

_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	using namespace OpenGlass;

	if (ppv)
	{
		*ppv = nullptr;
	}
	if (
		GetModuleHandleW(L"dllhost.exe") != GetModuleHandleW(nullptr) ||
		!Util::IsRunAsLocalSystem()
	)
	{
		return E_ACCESSDENIED;
	}
	if (rclsid == CLSID_TaskHandler)
	{
		return winrt::make<CClassFactoryT<CTaskHandler>>().as(riid, ppv);
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

STDAPI StartupService() noexcept
{
	using namespace OpenGlass;

	if (GlassService::IsRunning())
	{
		RETURN_WIN32(ERROR_SERVICE_ALREADY_RUNNING);
	}

	winrt::com_ptr<ITaskService> taskService{ nullptr };
	RETURN_IF_FAILED(
		CoCreateInstance(
			CLSID_TaskScheduler,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(taskService.put())
		)
	);
	RETURN_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

	winrt::com_ptr<ITaskFolder> rootFolder{ nullptr };
	RETURN_IF_FAILED(taskService->GetFolder(_bstr_t{ "\\" }, rootFolder.put()));

	winrt::com_ptr<IRegisteredTask> registeredTask{ nullptr };
	HRESULT hr
	{
		rootFolder->GetTask(
			_bstr_t{L"OpenGlass Host"},
			registeredTask.put()
		)
	};
	RETURN_IF_FAILED(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ? HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) : hr);

	VARIANT_BOOL enabled{ FALSE };
	RETURN_IF_FAILED(registeredTask->get_Enabled(&enabled));
	RETURN_HR_IF(SCHED_E_TASK_DISABLED, !enabled);

	[[maybe_unused]] winrt::com_ptr<IRunningTask> runningTask{ nullptr };
	// TASK_RUN_IGNORE_CONSTRAINTS is required otherwise the task will be queued instead of directly run when battery saver is on
	RETURN_IF_FAILED(registeredTask->RunEx(_variant_t{}, TASK_RUN_IGNORE_CONSTRAINTS, 0, _bstr_t{}, runningTask.put()));

	LONG lastTaskResult{};
	RETURN_IF_FAILED(registeredTask->get_LastTaskResult(&lastTaskResult));

	return lastTaskResult;
}
STDAPI ShutdownService() noexcept
{
	using namespace OpenGlass;

	if (!GlassService::IsRunning())
	{
		RETURN_WIN32(ERROR_SERVICE_NOT_ACTIVE);
	}

	winrt::com_ptr<ITaskService> taskService{ nullptr };
	RETURN_IF_FAILED(
		CoCreateInstance(
			CLSID_TaskScheduler,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(taskService.put())
		)
	);
	RETURN_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

	winrt::com_ptr<ITaskFolder> rootFolder{ nullptr };
	RETURN_IF_FAILED(taskService->GetFolder(_bstr_t{ "\\" }, rootFolder.put()));

	winrt::com_ptr<IRegisteredTask> registeredTask{ nullptr };
	HRESULT hr
	{
		rootFolder->GetTask(
			_bstr_t{L"OpenGlass Host"},
			registeredTask.put()
		)
	};
	RETURN_IF_FAILED(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ? HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) : hr);

	RETURN_IF_FAILED(registeredTask->Stop(0));

	return S_OK;
}
STDAPI InstallApp() noexcept
{
	using namespace OpenGlass;

	winrt::com_ptr<ITaskService> taskService{ nullptr };
	RETURN_IF_FAILED(
		CoCreateInstance(
			CLSID_TaskScheduler,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(taskService.put())
		)
	);
	RETURN_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

	winrt::com_ptr<ITaskFolder> rootFolder{ nullptr };
	RETURN_IF_FAILED(taskService->GetFolder(_bstr_t{ "\\" }, rootFolder.put()));

	winrt::com_ptr<ITaskDefinition> taskDefinition{ nullptr };
	RETURN_IF_FAILED(taskService->NewTask(0, taskDefinition.put()));

	winrt::com_ptr<IRegistrationInfo> regInfo{ nullptr };
	RETURN_IF_FAILED(taskDefinition->get_RegistrationInfo(regInfo.put()));
	RETURN_IF_FAILED(regInfo->put_Author(_bstr_t{ L"ALTaleX" }));
	RETURN_IF_FAILED(regInfo->put_Description(_bstr_t{ OpenGlass::Util::GetResourceStringView<IDS_STRING110>().data() }));

	{
		winrt::com_ptr<IPrincipal> principal{ nullptr };
		RETURN_IF_FAILED(taskDefinition->get_Principal(principal.put()));

		RETURN_IF_FAILED(principal->put_UserId(_bstr_t{ L"SYSTEM" }));
		RETURN_IF_FAILED(principal->put_LogonType(TASK_LOGON_SERVICE_ACCOUNT));
		RETURN_IF_FAILED(principal->put_DisplayName(_bstr_t{ L"SYSTEM" }));
		RETURN_IF_FAILED(principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST));
	}

	{
		winrt::com_ptr<ITaskSettings> setting{ nullptr };
		RETURN_IF_FAILED(taskDefinition->get_Settings(setting.put()));

		RETURN_IF_FAILED(setting->put_StopIfGoingOnBatteries(VARIANT_FALSE));
		RETURN_IF_FAILED(setting->put_DisallowStartIfOnBatteries(VARIANT_FALSE));
		RETURN_IF_FAILED(setting->put_AllowDemandStart(VARIANT_TRUE));
		RETURN_IF_FAILED(setting->put_AllowHardTerminate(VARIANT_TRUE));
		RETURN_IF_FAILED(setting->put_StartWhenAvailable(VARIANT_FALSE));
		RETURN_IF_FAILED(setting->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW));
	}

	{
		winrt::com_ptr<IComHandlerAction> comHandlerAction{ nullptr };
		{
			winrt::com_ptr<IAction> action{ nullptr };
			{
				winrt::com_ptr<IActionCollection> actionColl{ nullptr };
				RETURN_IF_FAILED(taskDefinition->get_Actions(actionColl.put()));
				RETURN_IF_FAILED(actionColl->Create(TASK_ACTION_COM_HANDLER, action.put()));
			}
			RETURN_HR_IF(E_NOINTERFACE, action.try_as(comHandlerAction) == false);
		}

		std::wstring modulePath{};
		RETURN_IF_FAILED((wil::GetModuleFileNameW<std::wstring, MAX_PATH>(wil::GetModuleInstanceHandle(), modulePath)));

		RETURN_IF_FAILED(comHandlerAction->put_ClassId(_bstr_t{ CLSID_TaskHandler_STR }));
		RETURN_IF_FAILED(comHandlerAction->put_Data(_bstr_t{}));
	}

	winrt::com_ptr<ITriggerCollection> triggerColl{ nullptr };
	RETURN_IF_FAILED(taskDefinition->get_Triggers(triggerColl.put()));

	winrt::com_ptr<ITrigger> trigger{ nullptr };
	RETURN_IF_FAILED(triggerColl->Create(TASK_TRIGGER_BOOT, trigger.put()));

	winrt::com_ptr<IRegisteredTask> registeredTask{ nullptr };
	RETURN_IF_FAILED(
		rootFolder->RegisterTaskDefinition(
			_bstr_t{ L"OpenGlass Host" },
			taskDefinition.get(),
			TASK_CREATE_OR_UPDATE,
			_variant_t{},
			_variant_t{},
			TASK_LOGON_NONE,
			_variant_t{},
			registeredTask.put()
		)
	);

	wil::unique_hkey key{ nullptr };
	// HKCR.AppID.{}
	RETURN_IF_FAILED(
		wil::reg::create_unique_key_nothrow(
			HKEY_CLASSES_ROOT,
			(std::wstring{ L"AppID\\" } + CLSID_TaskHandler_STR).c_str(),
			key,
			wil::reg::key_access::readwrite
		)
	);
	RETURN_IF_FAILED(
		wil::reg::set_value_string_nothrow(
			key.get(),
			nullptr,
			L"OpenGlass Host"
		)
	);
	RETURN_IF_FAILED(
		wil::reg::set_value_string_nothrow(
			key.get(),
			L"DllSurrogate",
			L""
		)
	);
	// HKCR.CLSID.{}
	RETURN_IF_FAILED(
		wil::reg::create_unique_key_nothrow(
			HKEY_CLASSES_ROOT,
			(std::wstring{ L"CLSID\\" } + CLSID_TaskHandler_STR).c_str(),
			key,
			wil::reg::key_access::readwrite
		)
	);
	RETURN_IF_FAILED(
		wil::reg::set_value_string_nothrow(
			key.get(),
			nullptr,
			L"OpenGlass Host"
		)
	);
	RETURN_IF_FAILED(
		wil::reg::set_value_string_nothrow(
			key.get(),
			L"AppID",
			CLSID_TaskHandler_STR
		)
	);
	// HKCR.CLSID.{}.InProcServer32
	RETURN_IF_FAILED(
		wil::reg::create_unique_key_nothrow(
			HKEY_CLASSES_ROOT,
			(std::wstring{ L"CLSID\\" } + CLSID_TaskHandler_STR + L"\\InProcServer32").c_str(),
			key,
			wil::reg::key_access::readwrite
		)
	);
	RETURN_IF_FAILED(
		wil::reg::set_value_string_nothrow(
			key.get(),
			nullptr,
			Util::g_thisModulePath.c_str()
		)
	);
	RETURN_IF_FAILED(
		wil::reg::set_value_string_nothrow(
			key.get(),
			L"ThreadingModel",
			L"Both"
		)
	);

	return S_OK;
}

STDAPI UninstallApp() noexcept
{
	using namespace OpenGlass;

	RETURN_IF_FAILED(
		SHDeleteKeyW(
			HKEY_CLASSES_ROOT,
			(std::wstring{ L"CLSID\\" } + CLSID_TaskHandler_STR).c_str()
		)
	);
	RETURN_IF_FAILED(
		SHDeleteKeyW(
			HKEY_CLASSES_ROOT,
			(std::wstring{ L"AppID\\" } + CLSID_TaskHandler_STR).c_str()
		)
	);

	winrt::com_ptr<ITaskService> taskService{ nullptr };
	RETURN_IF_FAILED(
		CoCreateInstance(
			CLSID_TaskScheduler,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(taskService.put())
		)
	);
	RETURN_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

	winrt::com_ptr<ITaskFolder> rootFolder{ nullptr };
	RETURN_IF_FAILED(taskService->GetFolder(_bstr_t{ "\\" }, rootFolder.put()));

	HRESULT hr
	{
		rootFolder->DeleteTask(
			_bstr_t{ L"OpenGlass Host" },
			0
		)
	};
	RETURN_IF_FAILED(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ? HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) : hr);

	return S_OK;
}
