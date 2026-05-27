#include "pch.h"
#include "Shared.hpp"
#include "CaptionMetricsTweaker.hpp"

using namespace OpenGlass;

namespace OpenGlass::CaptionMetricsTweaker
{
	void MyCButton_SetSize(uDWM::CButton* This, const SIZE* size);
	HRESULT MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes(uDWM::CTopLevelWindow* This);

	decltype(&MyCButton_SetSize) g_CButton_SetSize_Org{ nullptr };
	decltype(&MyCButton_SetSize)* g_CButton_SetSize_Org_Address{ nullptr };
	decltype(&MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes) g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org{ nullptr };

	enum CaptionButtons : UINT
	{
		Disabled = 0,
		WindowsVista,
		Windows7,
		Windows8,
		Custom
	} g_captionButtons{ 0 };

	struct CustomCaptionButtonsMetrics
	{
		int titlebarHeight{ 21 };
		int height{ 20 };
		int loneWidth{ 47 };
		int closeWidth{ 47 };
		int maxWidth{ 26 };
		int minWidth{ 28 };
		int groupOffsetX{ 0 };
		int groupOffsetY{ 0 };
		int buttonSpacing{ 0 };
		int closeOffsetX{ 0 };
		int closeOffsetY{ 0 };
		int maxOffsetX{ 0 };
		int maxOffsetY{ 0 };
		int minOffsetX{ 0 };
		int minOffsetY{ 0 };
		int toolWidth{ 0 };
		int toolHeight{ 0 };
		int toolOffsetX{ 0 };
		int toolOffsetY{ 0 };
		bool topInsert{ false };
	} g_customCaptionButtonsMetrics{};

	SIZE CalculateButtonSize(int cySize, int buttonType);
}

SIZE CaptionMetricsTweaker::CalculateButtonSize(int cySize, int buttonType)
{
	enum
	{
		LoneButton = 0,
		MinButton,
		MaxButton,
		CloseButton,
	};

	auto [heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio] = std::make_tuple(0.f, 0.f, 0.f, 0.f, 0.f);

	switch (g_captionButtons)
	{
	case CaptionButtons::WindowsVista:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.94736844f, 2.3157895f, 2.3157895f, 1.3157895f, 1.3684211f);
		break;
	case CaptionButtons::Windows7:
	default:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.95238096f, 2.3333333f, 2.3333333f, 1.2857143f, 1.3809524f);
		break;
	case CaptionButtons::Windows8:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.95454544f, 1.6363636f, 2.2272727f, 1.2272727f, 1.3181819f);
		break;
	case CaptionButtons::Custom:
	{
		const auto clampRatio = [](float value, float minimum, float maximum)
		{
			return std::clamp(value, minimum, maximum);
		};

		heightRatio = clampRatio(
			static_cast<float>(g_customCaptionButtonsMetrics.height) / static_cast<float>(g_customCaptionButtonsMetrics.titlebarHeight),
			0.5f,
			2.0f
		);
		loneWidthRatio = clampRatio(
			static_cast<float>(g_customCaptionButtonsMetrics.loneWidth) / static_cast<float>(g_customCaptionButtonsMetrics.height),
			0.5f,
			3.0f
		);
		closeWidthRatio = clampRatio(
			static_cast<float>(g_customCaptionButtonsMetrics.closeWidth) / static_cast<float>(g_customCaptionButtonsMetrics.height),
			0.5f,
			3.0f
		);
		maxWidthRatio = clampRatio(
			static_cast<float>(g_customCaptionButtonsMetrics.maxWidth) / static_cast<float>(g_customCaptionButtonsMetrics.height),
			0.5f,
			2.0f
		);
		minWidthRatio = clampRatio(
			static_cast<float>(g_customCaptionButtonsMetrics.minWidth) / static_cast<float>(g_customCaptionButtonsMetrics.height),
			0.5f,
			2.0f
		);
		break;
	}
	}

	SIZE buttonSize = { 0, static_cast<LONG>(std::round(static_cast<float>(cySize) * heightRatio)) };

	switch (buttonType)
	{
	case CloseButton:
		buttonSize.cx = static_cast<LONG>(std::round(static_cast<float>(cySize) * closeWidthRatio));
		break;
	case MaxButton:
		buttonSize.cx = static_cast<LONG>(std::round(static_cast<float>(cySize) * maxWidthRatio));
		break;
	case MinButton:
		buttonSize.cx = static_cast<LONG>(std::round(static_cast<float>(cySize) * minWidthRatio));
		break;
	case LoneButton:
		buttonSize.cx = static_cast<LONG>(std::round(static_cast<float>(cySize) * loneWidthRatio));
		break;
	default:
		break;
	}

	return buttonSize;
}

HRESULT CaptionMetricsTweaker::MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes(uDWM::CTopLevelWindow* This)
{
	const auto hr = g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org(This);

	if (!g_captionButtons)
	{
		return hr;
	}

	auto data = This->GetData();
	if (!data)
	{
		return hr;
	}

	auto maximized = This->IsWindowMaximized();
	auto toolWindow = This->IsToolWindow();
	auto loneButton = This->IsLoneButton();

	auto& visibleMargins = This->GetFrameOutsideMargins(maximized);
	auto& borderMargins = This->GetBorderMargins();

	int cxLeft = borderMargins.cxLeftWidth ? borderMargins.cxLeftWidth : data->GetFrameThickness();

	auto UpdateButton = [&](int buttonType, int offsetRight, int offsetTop, SIZE buttonSize)
	{
		if (auto button = This->GetButton(buttonType); button)
		{
			MARGINS inset = { 0x7FFFFFFF, offsetRight, offsetTop, 0x7FFFFFFF };

			g_CButton_SetSize_Org(button, &buttonSize);
			button->SetInsetFromParent(inset);
			button->GetGlyphOpacity() = 1.f;

			return true;
		}
		return false;
	};

	int cySize = GetSystemMetricsForDpi(SM_CYSIZE, data->GetWindowDPI());

	int offsetRight = maximized ? borderMargins.cxRightWidth + 2 : (borderMargins.cxRightWidth ? borderMargins.cxRightWidth - 2 : data->GetFrameThickness() - 2);
	int offsetTop =
		g_customCaptionButtonsMetrics.topInsert && g_captionButtons == CaptionButtons::Custom ?
		(maximized ? visibleMargins.cyTopHeight : visibleMargins.cyTopHeight + 1) :
		(maximized ? visibleMargins.cyTopHeight - 1 : visibleMargins.cyTopHeight + 1);

	auto closeButtonSize = loneButton ? CalculateButtonSize(cySize, 0) : CalculateButtonSize(cySize, 3);
	auto maxButtonSize = CalculateButtonSize(cySize, 2);
	auto minButtonSize = CalculateButtonSize(cySize, 1);
	if (g_captionButtons == CaptionButtons::Custom)
	{
		offsetRight += g_customCaptionButtonsMetrics.groupOffsetX;
		offsetTop += g_customCaptionButtonsMetrics.groupOffsetY;

		if (UpdateButton(3, offsetRight + g_customCaptionButtonsMetrics.closeOffsetX, offsetTop + g_customCaptionButtonsMetrics.closeOffsetY, closeButtonSize) && !toolWindow)
		{
			offsetRight += closeButtonSize.cx + g_customCaptionButtonsMetrics.buttonSpacing;
		}

		if (UpdateButton(2, offsetRight + g_customCaptionButtonsMetrics.maxOffsetX, offsetTop + g_customCaptionButtonsMetrics.maxOffsetY, maxButtonSize))
		{
			offsetRight += maxButtonSize.cx + g_customCaptionButtonsMetrics.buttonSpacing;
		}

		if (UpdateButton(1, offsetRight + g_customCaptionButtonsMetrics.minOffsetX, offsetTop + g_customCaptionButtonsMetrics.minOffsetY, minButtonSize))
		{
			offsetRight += minButtonSize.cx + g_customCaptionButtonsMetrics.buttonSpacing;
		}
	}
	else
	{
		if (UpdateButton(3, offsetRight, offsetTop, closeButtonSize) && !toolWindow)
		{
			offsetRight = closeButtonSize.cx + offsetRight;
		}

		if (UpdateButton(2, offsetRight, offsetTop, maxButtonSize))
		{
			offsetRight += maxButtonSize.cx;
		}

		if (UpdateButton(1, offsetRight, offsetTop, minButtonSize))
		{
			offsetRight += minButtonSize.cx;
		}
	}

	UpdateButton(0, offsetRight, offsetTop, minButtonSize);

	if (toolWindow)
	{
		int cySmSize = GetSystemMetricsForDpi(SM_CYSMSIZE, data->GetWindowDPI());
		SIZE toolButtonSize =
			g_captionButtons == CaptionButtons::Custom ?
			SIZE
			{
				std::max(1, cySmSize + g_customCaptionButtonsMetrics.toolWidth),
				std::max(1, cySmSize + g_customCaptionButtonsMetrics.toolHeight)
			} :
			SIZE
			{
				cySmSize,
				cySmSize
			};

		offsetTop = (borderMargins.cyTopHeight - toolButtonSize.cy - 4 > offsetTop) ? borderMargins.cyTopHeight - toolButtonSize.cy - 4 : offsetTop;
		UpdateButton(
			3,
			offsetRight + (g_captionButtons == CaptionButtons::Custom ? g_customCaptionButtonsMetrics.toolOffsetX : 0),
			offsetTop + (g_captionButtons == CaptionButtons::Custom ? g_customCaptionButtonsMetrics.toolOffsetY : 0),
			toolButtonSize
		);
		offsetRight = toolButtonSize.cx + offsetRight;
	}

	if (auto iconVisual = This->GetIconVisual(); iconVisual && cxLeft > 0)
	{
		iconVisual->SetInsetFromParentLeft(cxLeft);
	}

	if (auto textVisual = This->GetTextVisual(); textVisual)
	{
		if (auto iconVisual = This->GetIconVisual(); iconVisual && cxLeft > 0)
		{
			cxLeft += iconVisual->GetWidth() ? iconVisual->GetWidth() + 5 : 0;
		}
		MARGINS inset = { cxLeft, offsetRight, visibleMargins.cyTopHeight, 0x7FFFFFFF };
		textVisual->SetInsetFromParent(inset);
	}

	return hr;
}

void CaptionMetricsTweaker::MyCButton_SetSize(uDWM::CButton* This, const SIZE* size)
{
	if (Shared::g_captionHeight.has_value())
	{
		const SIZE replacedSize
		{
			size->cx,
			std::min(size->cy, static_cast<LONG>(Shared::g_captionHeight.value() * uDWM::CDesktopManager::GetInstance()->GetDPIValue()))
		};
		return g_CButton_SetSize_Org(This, &replacedSize);
	}

	return g_CButton_SetSize_Org(This, size);
}

void CaptionMetricsTweaker::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Theme)
	{
		const auto readInt = [](PCWSTR name, DWORD defaultValue)
		{
			return static_cast<int>(static_cast<LONG>(GlassEngine::GetDwordFromRegistry(name, defaultValue)));
		};
		const auto readClampedInt = [readInt](PCWSTR name, DWORD defaultValue, int minimum, int maximum)
		{
			return std::clamp(readInt(name, defaultValue), minimum, maximum);
		};

		g_captionButtons = static_cast<CaptionButtons>(std::clamp(GlassEngine::GetDwordFromRegistry(L"CaptionButtons", 0), 0ul, 4ul));
		g_customCaptionButtonsMetrics.titlebarHeight = readClampedInt(L"CustomTitlebarHeight", 21, 1, 100);
		g_customCaptionButtonsMetrics.height = readClampedInt(L"CustomHeight", 20, 1, 100);
		g_customCaptionButtonsMetrics.loneWidth = readClampedInt(L"CustomLoneWidth", 47, 1, 200);
		g_customCaptionButtonsMetrics.closeWidth = readClampedInt(L"CustomCloseWidth", 47, 1, 200);
		g_customCaptionButtonsMetrics.maxWidth = readClampedInt(L"CustomMaxWidth", 26, 1, 200);
		g_customCaptionButtonsMetrics.minWidth = readClampedInt(L"CustomMinWidth", 28, 1, 200);
		g_customCaptionButtonsMetrics.groupOffsetX = readClampedInt(L"ButtonGroupOffsetX", 0, 0, 200);
		g_customCaptionButtonsMetrics.groupOffsetY = readClampedInt(L"ButtonGroupOffsetY", 0, 0, 5);
		g_customCaptionButtonsMetrics.buttonSpacing = readClampedInt(L"ButtonSpacing", 0, 0, 200);
		g_customCaptionButtonsMetrics.closeOffsetX = readClampedInt(L"CloseOffsetX", 0, 0, 200);
		g_customCaptionButtonsMetrics.closeOffsetY = readClampedInt(L"CloseOffsetY", 0, -5, 5);
		g_customCaptionButtonsMetrics.maxOffsetX = readClampedInt(L"MaxOffsetX", 0, 0, 200);
		g_customCaptionButtonsMetrics.maxOffsetY = readClampedInt(L"MaxOffsetY", 0, -5, 5);
		g_customCaptionButtonsMetrics.minOffsetX = readClampedInt(L"MinOffsetX", 0, 0, 200);
		g_customCaptionButtonsMetrics.minOffsetY = readClampedInt(L"MinOffsetY", 0, -5, 5);
		g_customCaptionButtonsMetrics.toolWidth = readClampedInt(L"CustomToolWidth", 0, 1, 100);
		g_customCaptionButtonsMetrics.toolHeight = readClampedInt(L"CustomToolHeight", 0, -10, 100);
		g_customCaptionButtonsMetrics.toolOffsetX = readClampedInt(L"ToolOffsetX", 0, -20, 200);
		g_customCaptionButtonsMetrics.toolOffsetY = readClampedInt(L"ToolOffsetY", 0, -5, 5);
		g_customCaptionButtonsMetrics.topInsert = GlassEngine::GetDwordFromRegistry(L"TopInsert", 0) != 0;
	}
}

void CaptionMetricsTweaker::Startup()
{
	if (Shared::g_disabledHooks.test(Shared::DisabledHooks_CaptionMetricsTweaker))
	{
		return;
	}

	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::UpdateNCAreaPositionsAndSizes", g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org);

	PVOID CVisual_SetSize_Org{ nullptr };
	uDWM::g_projectionArray.ApplyToVariable("CVisual::SetSize", CVisual_SetSize_Org);
	for (auto& vf : std::span{ uDWM::CButton::vftable, 20 })
	{
		if (vf == CVisual_SetSize_Org)
		{
			g_CButton_SetSize_Org_Address = reinterpret_cast<decltype(g_CButton_SetSize_Org_Address)>(&vf);
			HookHelper::PatchPointerT(g_CButton_SetSize_Org_Address, MyCButton_SetSize, &g_CButton_SetSize_Org);
		}
	}

	HookHelper::PatchFunctions(
		std::initializer_list<HookHelper::DetourInfo>
		{
			{ &g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org, &MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes },
		},
		true
	);
}

void CaptionMetricsTweaker::Shutdown()
{
	if (Shared::g_disabledHooks.test(Shared::DisabledHooks_CaptionMetricsTweaker))
	{
		return;
	}

	HookHelper::PatchFunctions(
		std::initializer_list<HookHelper::DetourInfo>
		{
			{ &g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org, &MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes },
		},
		false
	);

	SwitchToThread();

	if (g_CButton_SetSize_Org)
	{
		HookHelper::PatchPointerT(g_CButton_SetSize_Org_Address, g_CButton_SetSize_Org);
	}
}
