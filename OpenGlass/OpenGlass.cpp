#include "pch.h"
#include "resource.h"
#include "Util.hpp"
#include "Shared.hpp"
#include "OpenGlass.hpp"
#include "HookHelper.hpp"
#include "SymbolParser.hpp"
#include "OSHelper.hpp"
#include "uDWMProjection.hpp"
#include "dwmcoreProjection.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass
{
	void Startup();
	void Shutdown();
	bool g_startup{ false };

	LONG NTAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
	LPTOP_LEVEL_EXCEPTION_FILTER g_old{ nullptr };

	_Function_class_(EFFECTIVE_POWER_MODE_CALLBACK)
	VOID CALLBACK EffectivePowerModeCallback(
		EFFECTIVE_POWER_MODE mode,
		PVOID context
	);
	LRESULT CALLBACK DwmNotificationWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND g_notificationWindow{ nullptr };
	bool g_hotkeyRegistered{ false };
	UINT g_msgToggleHotKeyState{ RegisterWindowMessageW(L"OpenGlass.ToggleHotKeyState")};
	PVOID g_powerNotify{ nullptr };
	WNDPROC g_oldWndProc{ nullptr };

	DWORD SymbolLoaderUIThreadEntryPoint(PVOID);
	bool g_asyncTaskDialogCreated{ false };
	HWND g_symbolDownloaderHwnd{ nullptr };
	ULONGLONG g_progressCompleted{ 0 };
	ULONGLONG g_progressPreviouslyCompleted{ 0 };
	const ULONGLONG g_progressTotal{ 200 };
	enum class DownloaderStatus : UCHAR
	{
		None,
		Indeterminate,
		Normal,
		Error,
		Paused,
	};
	auto g_status = DownloaderStatus::None;

	bool InitializeProjectionBySymbols();
	bool SymEventCallback(PIMAGEHLP_CBA_EVENTW event);
	bool g_symbolIsDownloading{ false };
	bool g_symbolRequiresDownloading{ false };
	bool g_symbolDownloadCompleted{ false };
	std::wstring g_detailsInfo{};
}

LONG NTAPI OpenGlass::TopLevelExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	DWORD value{ 0ul };
	wil::reg::get_value_dword_nothrow(
		GlassEngine::GetDwmLocalMachineKey(),
		L"DisableMemoryDump",
		&value
	);

	if (value)
	{
		// DWM wont exit after returning,
		// here we manually terminate the process
		ExitProcess(exceptionInfo->ExceptionRecord->ExceptionCode);
	}

	const auto dumpFolder = Util::make_current_folder_file(L"dumps");
	if (!PathFileExistsW(dumpFolder.get()))
	{
		FAIL_FAST_IF_WIN32_BOOL_FALSE(CreateDirectoryW(dumpFolder.get(), nullptr));
	}

	WCHAR dumpFileName[MAX_PATH]{};
	SYSTEMTIME localTime{};
	GetLocalTime(&localTime);
	swprintf_s(
		dumpFileName,
		L"dumps\\minidump-%04d-%02d-%02d-%02d-%02d-%02d.dmp",
		localTime.wYear,
		localTime.wMonth,
		localTime.wDay,
		localTime.wHour,
		localTime.wMinute,
		localTime.wSecond
	);

	wil::unique_hfile fileHandle
	{
		CreateFile2(
			Util::make_current_folder_file(dumpFileName).get(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			CREATE_ALWAYS,
			nullptr
		)
	};
	FAIL_FAST_IF(!fileHandle.is_valid());

	MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo{ GetCurrentThreadId(), exceptionInfo, FALSE };
	FAIL_FAST_IF_WIN32_BOOL_FALSE(
		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			fileHandle.get(),
			static_cast<MINIDUMP_TYPE>(
				MINIDUMP_TYPE::MiniDumpWithProcessThreadData |
				MINIDUMP_TYPE::MiniDumpWithThreadInfo |
				MINIDUMP_TYPE::MiniDumpWithFullMemory |
				MINIDUMP_TYPE::MiniDumpWithUnloadedModules
				),
			&minidumpExceptionInfo,
			nullptr,
			nullptr
		)
	);

	// DWM wont exit after returning,
	// here we manually terminate the process
	ExitProcess(exceptionInfo->ExceptionRecord->ExceptionCode);
}

_Function_class_(EFFECTIVE_POWER_MODE_CALLBACK)
VOID CALLBACK OpenGlass::EffectivePowerModeCallback(
	EFFECTIVE_POWER_MODE mode,
	[[maybe_unused]] PVOID context
)
{
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		ExitProcess(static_cast<UINT>(E_ABORT));
	}
	if (mode < (os::buildNumber < os::build_w11_24h2 ? 1 : 2))
	{
		Shared::g_xxSaver = true;
	}
	else
	{
		Shared::g_xxSaver = false;
	}
	GlassEngine::Update(GlassEngine::UpdateType::Framework);
}

LRESULT CALLBACK OpenGlass::DwmNotificationWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == g_msgToggleHotKeyState)
	{
		if (g_hotkeyRegistered)
		{
			LOG_IF_WIN32_BOOL_FALSE(
				UnregisterHotKey(
					g_notificationWindow,
					1
				)
			);
		}
		else
		{
			LOG_IF_WIN32_BOOL_FALSE(
				RegisterHotKey(
					g_notificationWindow,
					1,
					MOD_NOREPEAT | MOD_CONTROL | MOD_SHIFT | MOD_WIN,
					'X'
				)
			);
		}
		g_hotkeyRegistered = !g_hotkeyRegistered;
	}
	switch (uMsg)
	{
		case WM_HOTKEY:
		{
			if (wParam == 1)
			{
				THROW_WIN32(ERROR_CANCELLED);
			}
			break;
		}
		case WM_CLOSE:
		{
			if (g_hotkeyRegistered)
			{
				LOG_IF_WIN32_BOOL_FALSE(
					UnregisterHotKey(
						g_notificationWindow,
						1
					)
				);
				g_hotkeyRegistered = false;
				g_notificationWindow = nullptr;
			}
			break;
		}
		case WM_WININICHANGE:
		case WM_DWMCOLORIZATIONCOLORCHANGED: // accent color changed
		{
			GlassEngine::Update(GlassEngine::UpdateType::Backdrop);
			break;
		}
		case WM_THEMECHANGED: // theme switched, we can handle this to load our custom theme atlas
		{
			GlassEngine::Update(GlassEngine::UpdateType::Theme);
			break;
		}
		case WM_WTSSESSION_CHANGE: // session changed, user has just logged in/off
		{
			if (wParam == WTS_SESSION_LOGON)
			{
				GlassEngine::LoadRegistry();
			}
			else if (wParam == WTS_SESSION_LOGOFF)
			{
				GlassEngine::UnloadRegistry();
			}
			break;
		}
		default:
			break;
	}

	return g_oldWndProc(hWnd, uMsg, wParam, lParam);
}

DWORD OpenGlass::SymbolLoaderUIThreadEntryPoint(PVOID)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	THROW_IF_FAILED(SetThreadDescription(GetCurrentThread(), L"OpenGlass Symbol Loader UI Thread"));

	THROW_IF_FAILED(RoInitialize(RO_INIT_MULTITHREADED));
	wil::unique_rouninitialize_call wrtScope{};

	INITCOMMONCONTROLSEX icex{ .dwSize{sizeof(icex)}, .dwICC{ICC_PROGRESS_CLASS} };
	THROW_IF_WIN32_BOOL_FALSE(InitCommonControlsEx(&icex));

	winrt::com_ptr<ITaskbarList4> m_taskbarList{ nullptr };

	THROW_IF_FAILED(
		CoCreateInstance(
			CLSID_TaskbarList,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(m_taskbarList.put())
		)
	);

	wil::unique_hicon icon{ LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(18)) };
	auto callback = [](HWND hwnd, UINT uNotification, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam, LONG_PTR lpRefData) -> HRESULT
	{
		const auto taskbarList = reinterpret_cast<ITaskbarList4*>(lpRefData);
		switch (uNotification)
		{
		case TDN_CREATED:
		{
			const auto instruction = Util::GetResourceStringView<IDS_STRING111>();
			SendMessageW(hwnd, TDM_SET_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(instruction.data()));

			BOOL value{ TRUE };
			THROW_IF_FAILED(DwmSetWindowAttribute(hwnd, DWMWA_EXCLUDED_FROM_PEEK, &value, sizeof(value)));

			g_symbolDownloaderHwnd = hwnd;
			break;
		}
		case TDN_DESTROYED:
		{
			g_symbolDownloaderHwnd = nullptr;
			break;
		}
		case TDN_TIMER:
		{
			static auto s_prevStatus = DownloaderStatus::None;
			if (s_prevStatus != g_status)
			{
				static bool s_marquee{ false };
				if (const auto marquee{ g_status == DownloaderStatus::Indeterminate }; s_marquee != marquee)
				{
					s_marquee = marquee;
					SendMessageW(hwnd, TDM_SET_MARQUEE_PROGRESS_BAR, static_cast<WPARAM>(marquee), 0);
					SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_MARQUEE, static_cast<WPARAM>(marquee), 0);
				}

				s_prevStatus = g_status;
				switch (g_status)
				{
				case DownloaderStatus::None:
				{
					THROW_IF_FAILED(taskbarList->SetProgressState(hwnd, TBPF_NOPROGRESS));
					SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0);
					break;
				}
				case DownloaderStatus::Indeterminate:
				{
					THROW_IF_FAILED(taskbarList->SetProgressState(hwnd, TBPF_INDETERMINATE));
					break;
				}
				case DownloaderStatus::Normal:
				{
					THROW_IF_FAILED(taskbarList->SetProgressState(hwnd, TBPF_NORMAL));
					SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0);
					break;
				}
				case DownloaderStatus::Error:
				{
					THROW_IF_FAILED(taskbarList->SetProgressState(hwnd, TBPF_ERROR));
					SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_ERROR, 0);
					break;
				}
				case DownloaderStatus::Paused:
				{
					THROW_IF_FAILED(taskbarList->SetProgressState(hwnd, TBPF_PAUSED));
					SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_PAUSED, 0);
					break;
				}
				default:
					break;
				}
			}
			const auto actualProgress = g_progressCompleted < 100 ? g_progressCompleted : g_progressCompleted - 100;
			if (g_status != DownloaderStatus::Indeterminate)
			{
				THROW_IF_FAILED(taskbarList->SetProgressValue(hwnd, g_progressCompleted, g_progressTotal));
				SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_POS, actualProgress, 0);
			}
			const auto instruction = Util::GetResourceString<IDS_STRING111>() + (g_progressPreviouslyCompleted < 100 ? L" (1/2)" : L" (2/2)");
			const auto content = (g_progressPreviouslyCompleted < 100 ? L"uDWM.pdb" : L"dwmcore.pdb") + (g_status == DownloaderStatus::Indeterminate ? std::wstring{} : (L" (" + std::to_wstring(actualProgress)) + L"%)");
			SendMessageW(hwnd, TDM_SET_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(instruction.c_str()));
			SendMessageW(hwnd, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, reinterpret_cast<LPARAM>(content.c_str()));

			break;
		}
		}
		return S_OK;
	};
	TASKDIALOGCONFIG config
	{
		sizeof(TASKDIALOGCONFIG),
		nullptr,
		nullptr,
		TDF_SIZE_TO_CONTENT | TDF_SHOW_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_USE_HICON_MAIN,
		TDCBF_CLOSE_BUTTON,
		nullptr,
		{.hMainIcon{icon.get()}},
		nullptr,
		nullptr,
		0,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		{},
		nullptr,
		callback,
		reinterpret_cast<LONG_PTR>(m_taskbarList.get()),
		0
	};

	int button{};
	THROW_IF_FAILED(
		TaskDialogIndirect(
			&config,
			&button,
			nullptr,
			nullptr
		)
	);

	return S_OK;
}

bool OpenGlass::SymEventCallback(PIMAGEHLP_CBA_EVENTW event)
{
	std::wstring_view description{ event->desc };
	if (!description.starts_with(L"DBGHELP"))
	{
		g_detailsInfo.append(description);
	}

	if (std::wcsstr(event->desc, L"HTTPGET: "))
	{
		g_symbolIsDownloading = true;
		g_symbolRequiresDownloading = true;
		g_progressCompleted = g_progressPreviouslyCompleted;
		g_status = DownloaderStatus::Indeterminate;

		if (!g_asyncTaskDialogCreated)
		{
			g_asyncTaskDialogCreated = true;
			[[maybe_unused]] wil::unique_handle uiThread
			{
				CreateThread(
					nullptr,
					0,
					SymbolLoaderUIThreadEntryPoint,
					nullptr,
					0,
					nullptr
				)
			};
		}
	}
	if (g_symbolIsDownloading)
	{
		if (auto end = std::wcsstr(event->desc, L" percent"); end)
		{
			WCHAR number[12]{};
			auto begin = end;
			while ((*(--begin)) != L' ');
			begin += 1;
			// "\b\b\b\b\b\b\b\b\b\b\b\b 100 percent"
			std::wmemcpy(number, begin, std::min(std::size(number), static_cast<size_t>(end - begin + 1)));
			g_progressCompleted = g_progressPreviouslyCompleted + _wtol(number);
			g_status = DownloaderStatus::Normal;
		}
		else
		{
			g_status = DownloaderStatus::Indeterminate;
		}
	}
	if (std::wcsstr(event->desc, L"copied"))
	{
		g_symbolIsDownloading = false;
		g_progressCompleted = g_progressPreviouslyCompleted + 100;
		g_status = DownloaderStatus::None;
		g_symbolDownloadCompleted = true;
	}

	return false;
}

bool OpenGlass::InitializeProjectionBySymbols()
{
	const auto mainInstruction = Util::GetResourceStringView<IDS_STRING101>();
	const auto expandText = Util::GetResourceStringView<IDS_STRING103>();
	const auto collapseText = Util::GetResourceStringView<IDS_STRING104>();
	int result{ 0 };
	TASKDIALOGCONFIG config
	{
		sizeof(TASKDIALOGCONFIG),
		nullptr,
		nullptr,
		TDF_SIZE_TO_CONTENT | TDF_EXPAND_FOOTER_AREA | TDF_ALLOW_DIALOG_CANCELLATION,
		TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON,
		nullptr,
		{.pszMainIcon{TD_ERROR_ICON}},
		mainInstruction.data(),
		nullptr,
		0,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		nullptr,
		collapseText.data(),
		expandText.data(),
		{},
		nullptr,
		nullptr,
		0,
		0
	};

	{
		const auto asyncUICleanup = wil::scope_exit([]
		{
			if (g_symbolDownloaderHwnd)
			{
				SendMessageW(g_symbolDownloaderHwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
			}
		});

		const auto parser = std::make_unique<CSymbolParser>(SymEventCallback);
		HRESULT hr{ S_OK };

		do
		{
			g_progressPreviouslyCompleted = 0;
			g_symbolDownloadCompleted = false;
			g_symbolRequiresDownloading = false;
			g_detailsInfo.clear();
			hr = parser->LoadAndParse(L"uDWM.dll", uDWM::ParserCallback);
			if (FAILED(hr))
			{
				const auto contentTemplate = g_symbolRequiresDownloading && !g_symbolDownloadCompleted ? Util::GetResourceStringView<IDS_STRING106>() : Util::GetResourceStringView<IDS_STRING107>();
				const auto buffSize = contentTemplate.size() + 8ull;
				const auto content = std::make_unique_for_overwrite<WCHAR[]>(buffSize);
				swprintf_s(
					content.get(),
					buffSize,
					contentTemplate.data(),
					hr
				);

				g_status = DownloaderStatus::Error;
				config.hwndParent = g_symbolDownloaderHwnd;
				config.pszContent = content.get();
				config.pszExpandedInformation = g_detailsInfo.c_str();
				THROW_IF_FAILED(
					TaskDialogIndirect(
						&config,
						&result,
						nullptr,
						nullptr
					)
				);
				if (result != IDRETRY)
				{
					return false;
				}
			}
			else
			{
				break;
			}
		} while (true);

		do
		{
			g_progressPreviouslyCompleted = 100;
			g_symbolDownloadCompleted = false;
			g_symbolRequiresDownloading = false;
			g_detailsInfo.clear();
			hr = parser->LoadAndParse(L"dwmcore.dll", dwmcore::ParserCallback);
			if (FAILED(hr))
			{
				const auto contentTemplate = g_symbolRequiresDownloading && !g_symbolDownloadCompleted ? Util::GetResourceStringView<IDS_STRING106>() : Util::GetResourceStringView<IDS_STRING107>();
				const auto buffSize = contentTemplate.size() + 8ull;
				const auto content = std::make_unique_for_overwrite<WCHAR[]>(buffSize);
				swprintf_s(
					content.get(),
					buffSize,
					contentTemplate.data(),
					hr
				);

				g_status = DownloaderStatus::Error;
				config.hwndParent = g_symbolDownloaderHwnd;
				config.pszContent = content.get();
				config.pszExpandedInformation = g_detailsInfo.c_str();
				THROW_IF_FAILED(
					TaskDialogIndirect(
						&config,
						&result,
						nullptr,
						nullptr
					)
				);
				if (result != IDRETRY)
				{
					return false;
				}
			}
			else
			{
				break;
			}
		} while (true);
	}

	std::string missingFunctionsOrVariables{};
	uDWM::g_projectionArray.ReportMissing(missingFunctionsOrVariables, "[uDWM.dll] ");
	dwmcore::g_projectionArray.ReportMissing(missingFunctionsOrVariables, "[dwmcore.dll] ");
	if (!missingFunctionsOrVariables.empty())
	{
		missingFunctionsOrVariables.pop_back();
		missingFunctionsOrVariables.append(std::string_view{ "\0", 1 });
		const auto content = Util::GetResourceStringView<IDS_STRING108>();

		std::unique_ptr<WCHAR[]> convertedMissingInfo{};
		THROW_IF_FAILED(Util::MB2WC(convertedMissingInfo, missingFunctionsOrVariables.c_str(), static_cast<int>(missingFunctionsOrVariables.size())));
		
		config.hwndParent = nullptr;
		config.pszMainInstruction = mainInstruction.data();
		config.pszContent = content.data();
		config.pszExpandedInformation = convertedMissingInfo.get();
		config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
		THROW_IF_FAILED(
			TaskDialogIndirect(
				&config,
				&result,
				nullptr,
				nullptr
			)
		);

		return false;
	}

	return true;
}

DWORD WINAPI OpenGlass::InitializationThreadEntryPoint(PVOID)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	THROW_IF_FAILED(SetThreadDescription(GetCurrentThread(), L"OpenGlass Initialization Thread"));

	Startup();

	return S_OK;
}

DWORD WINAPI OpenGlass::UnInitializationThreadEntryPoint(PVOID)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	THROW_IF_FAILED(SetThreadDescription(GetCurrentThread(), L"OpenGlass UnInitialization Thread"));

	Shutdown();

	FreeLibraryAndExitThread(wil::GetModuleInstanceHandle(), S_OK);
}

void OpenGlass::Startup()
{
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		return;
	}

	wil::SetResultLoggingCallback([](const wil::FailureInfo& failure) noexcept
	{
		WCHAR logMessage[MAX_PATH]{};
		if (FAILED(wil::GetFailureLogString(logMessage, MAX_PATH, failure)))
		{
			return;
		}

		OutputDebugStringW(logMessage);
	});

	THROW_IF_FAILED(RoInitialize(RO_INIT_MULTITHREADED));
	wil::unique_rouninitialize_call wrtScope{};

	g_old = SetUnhandledExceptionFilter(TopLevelExceptionFilter);

	if (!os::IsOpenGlassSupported())
	{
		auto result = 0;
		THROW_IF_FAILED(
			Util::TaskDialog(
				nullptr,
				nullptr,
				nullptr,
				Util::GetResourceStringView<IDS_STRING101>().data(),
				Util::GetResourceStringView<IDS_STRING105>().data(),
				TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
				TD_WARNING_ICON,
				&result
			)
		);
		if (result != IDYES)
		{
			return;
		}
	}

	if (!InitializeProjectionBySymbols())
	{
		return;
	}

	// just wait patiently, in case the dwm notification window is not ready...
	while (!(g_notificationWindow = FindWindowW(L"DWM", nullptr))) { Sleep(5); }

	// make sure our third-party ui creators can send message to dwm
	THROW_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(g_notificationWindow, WM_DWMCOLORIZATIONCOLORCHANGED, MSGFLT_ALLOW, nullptr));

	THROW_IF_WIN32_BOOL_FALSE(WTSRegisterSessionNotification(g_notificationWindow, NOTIFY_FOR_THIS_SESSION));
	THROW_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(g_notificationWindow, WM_WTSSESSION_CHANGE, MSGFLT_ALLOW, nullptr));

	g_oldWndProc = reinterpret_cast<WNDPROC>(
		SetWindowLongPtrW(
			g_notificationWindow,
			GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(DwmNotificationWndProc)
		)
	);
	THROW_LAST_ERROR_IF(g_oldWndProc == 0);
	SendMessageW(g_notificationWindow, g_msgToggleHotKeyState, 0, 0);

	THROW_IF_FAILED(
		PowerRegisterForEffectivePowerModeNotifications(
			EFFECTIVE_POWER_MODE_V2,
			EffectivePowerModeCallback,
			nullptr,
			&g_powerNotify
		)
	);

	GlassEngine::LoadRegistry(false);
	GlassEngine::Startup();

#ifdef _DEBUG
	winrt::com_ptr<IDCompositionDeviceDebug> debugDevice{ nullptr };
	uDWM::CDesktopManager::GetInstance()->GetDCompDevice()->QueryInterface(
		debugDevice.put()
	);
	debugDevice->EnableDebugCounters();
#endif // _DEBUG

	GlassEngine::RedrawAll();

	g_startup = true;
	return;
}

void OpenGlass::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

#ifdef _DEBUG
	winrt::com_ptr<IDCompositionDeviceDebug> debugDevice{ nullptr };
	uDWM::CDesktopManager::GetInstance()->GetDCompDevice()->QueryInterface(
		debugDevice.put()
	);
	debugDevice->DisableDebugCounters();
#endif // _DEBUG

	g_startup = false;

	if (g_hotkeyRegistered)
	{
		SendMessageW(g_notificationWindow, g_msgToggleHotKeyState, 0, 0);
		g_hotkeyRegistered = false;
	}
	
	if (g_notificationWindow)
	{
		THROW_LAST_ERROR_IF(SetWindowLongPtrW(g_notificationWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_oldWndProc)) == 0);
		g_oldWndProc = nullptr;

		THROW_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(g_notificationWindow, WM_WTSSESSION_CHANGE, MSGFLT_DISALLOW, nullptr));
		THROW_IF_WIN32_BOOL_FALSE(WTSUnRegisterSessionNotification(g_notificationWindow));

		THROW_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(g_notificationWindow, WM_DWMCOLORIZATIONCOLORCHANGED, MSGFLT_DISALLOW, nullptr));

		g_notificationWindow = nullptr;
	}

	THROW_IF_FAILED(
		PowerUnregisterFromEffectivePowerModeNotifications(g_powerNotify)
	);
	g_powerNotify = nullptr;

	GlassEngine::Shutdown();
	GlassEngine::UnloadRegistry();
	GlassEngine::RedrawAll();

	SetUnhandledExceptionFilter(g_old);
}
