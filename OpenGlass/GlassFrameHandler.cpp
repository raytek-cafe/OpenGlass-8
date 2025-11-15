#include "pch.h"
#include "Shared.hpp"
#include "GlassFrameHandler.hpp"
#include "GlassKernel.hpp"
#include "uDWMProjection.hpp"
#include "dwmcoreProjection.hpp"
#include "GlassReflectionBrush.hpp"

using namespace OpenGlass;

namespace OpenGlass::GlassFrameHandler
{
	HRGN WINAPI MyCreateRoundRectRgn(int x1, int y1, int x2, int y2, int w, int h);
	HRGN WINAPI MyCreateRectRgn(int x1, int y1, int x2, int y2);
	HRGN WINAPI MyExtCreateRegion(const XFORM* lpx, DWORD nCount, const RGNDATA* lpData);

	HRESULT STDMETHODCALLTYPE MyCGlassColorizationParameters_AdjustWindowColorization(
		uDWM::CGlassColorizationParameters* This,
		uDWM::GpCC* colorUnused,
		float opacity,
		BYTE flag
	);
	HRESULT STDMETHODCALLTYPE MyResourceHelper_CreateGeometryFromHRGN(HRGN hrgn, uDWM::CRgnGeometryProxy** geometry);
	bool STDMETHODCALLTYPE MyCTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage(uDWM::CTopLevelAtlasedRectsVisual* This, const uDWM::CAtlasedImage* atlasedImage, UINT cloneOptions);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win10(uDWM::CTopLevelWindow* This, bool windowFramesOnly, bool unused1, bool unused2, uDWM::CTopLevelWindow** clonedWindow);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win11(uDWM::CTopLevelWindow* This, bool windowFramesOnly, uDWM::CTopLevelWindow** clonedWindow);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_UpdateNCAreaBackground(uDWM::CTopLevelWindow* This);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes(uDWM::CTopLevelWindow* This);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_UpdateClientBlur(uDWM::CTopLevelWindow* This);
	HRESULT STDMETHODCALLTYPE MyCButton_CloneVisualTree(uDWM::CButton* This, uDWM::CButton** clonedVisual, UINT cloneOption);
	void STDMETHODCALLTYPE MyCButton_SetSize(uDWM::CButton* This, const SIZE* size);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_ValidateVisual(uDWM::CTopLevelWindow* This);
	void STDMETHODCALLTYPE MyCTopLevelWindow_Destructor(uDWM::CTopLevelWindow* This);
	bool WINAPI MySetMargin(
		MARGINS* dstMargins,
		int cxLeftWidth,
		int cxRightWidth,
		int cyTopHeight,
		int cyBottomHeight,
		const MARGINS* srcMargins
	);

	decltype(&MyCreateRoundRectRgn) g_CreateRoundRectRgn_Org{ nullptr };
	decltype(&MyCreateRectRgn) g_CreateRectRgn_Org{ nullptr };
	decltype(&MyExtCreateRegion) g_ExtCreateRegion_Org{ nullptr };

	decltype(&MyCGlassColorizationParameters_AdjustWindowColorization) g_CGlassColorizationParameters_AdjustWindowColorization_Org{ nullptr };
	decltype(&MyResourceHelper_CreateGeometryFromHRGN) g_ResourceHelper_CreateGeometryFromHRGN_Org{ nullptr };
	decltype(&MyCTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage) g_CTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage_Org{ nullptr };
	PVOID g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org{ nullptr };
	decltype(&MyCTopLevelWindow_UpdateNCAreaBackground) g_CTopLevelWindow_UpdateNCAreaBackground_Org{ nullptr };
	decltype(&MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes) g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org{ nullptr };
	decltype(&MyCTopLevelWindow_UpdateClientBlur) g_CTopLevelWindow_UpdateClientBlur_Org{ nullptr };
	decltype(&MyCTopLevelWindow_ValidateVisual) g_CTopLevelWindow_ValidateVisual_Org{ nullptr };
	decltype(&MyCTopLevelWindow_Destructor) g_CTopLevelWindow_Destructor_Org{ nullptr };
	decltype(&MyCButton_CloneVisualTree) g_CButton_CloneVisualTree_Org{ nullptr };
	decltype(&MyCButton_CloneVisualTree)* g_CButton_CloneVisualTree_Org_Address{ nullptr };
	decltype(&MyCButton_SetSize) g_CButton_SetSize_Org{ nullptr };
	decltype(&MyCButton_SetSize)* g_CButton_SetSize_Org_Address{ nullptr };
	decltype(&MySetMargin) g_SetMargin_Org{ nullptr };
	
	UCHAR g_callCDesktopManager_IsHighContrastMode_Instructions[]
	{
		// call ???
		0xE8, 0x00, 0x00, 0x00, 0x00,
		// test al, al
		0x84, 0xC0
	};
	UCHAR g_callCDesktopManager_IsHighContrastMode_replacedInstruction[]
	{
		// move al, 0x01
		0xB0, 0x01,
		// nop
		// nop
		// nop
		0x90, 0x90, 0x90,
		// test al, al
		0x84, 0xC0
	};
	UCHAR g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_Instructions[]
	{
		// lea eax, [???-12h]
		0x8D, 0x00, 0xEE,
		// cmp eax, 3
		0x83, 0xF8, 0x03,
		// ja short loc_xxxxxxxx
		0x77, 
	};
	UCHAR g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_replacedInstructions[]
	{
		// lea eax, [???-12h]
		0x8D, 0x00, 0xEE,
		// nop
		// nop
		// nop
		0x90, 0x90, 0x90,
		// jmp
		0xEB,
	};
	UCHAR g_callCTopLevelWindow_IsShadowNCAreaPart_Instructions[]
	{
		// call ???
		0xE8, 0x00, 0x00, 0x00, 0x00,
		// test al, al
		0x84, 0xC0
	};
	UCHAR g_callCTopLevelWindow_IsShadowNCAreaPart_replacedInstructions[]
	{
		// move al, 0x00
		0xB0, 0x00,
		// nop
		// nop
		// nop
		0x90, 0x90, 0x90,
		// test al, al
		0x84, 0xC0
	};
	std::unordered_map<UCHAR*, std::vector<UCHAR>> g_instructionsToReplace{};
	std::unordered_map<UCHAR*, std::vector<UCHAR>> g_instructionsBackup{};
	
	uDWM::CTopLevelWindow* g_window{ nullptr };
	wil::unique_hrgn g_combinedRgn{ nullptr };
	RECT g_roundedBounds{};
	bool g_systemBackdrop{ false };
	std::optional<bool> g_redirectFirstCreateRectRgnCall{};
	bool g_windowFramesOnly{ false };
	enum CaptionButtons : UINT
	{
		Disabled = 0,
		WindowsVista,
		Windows7,
		Windows8,
		Windows8DP,
		Custom
	} g_captionButtons{ 0 };
	bool g_disableGlassHooks{ false };
	bool g_TopInsert{ false };

	SIZE CalculateButtonSize(int cySize, int buttonType);
	HRESULT UpdateReflectionViewport(uDWM::CTopLevelWindow* window);
}

SIZE GlassFrameHandler::CalculateButtonSize(int cySize, int buttonType)
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
	case CaptionButtons::Windows8DP:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.95454544f, 1.5428571f, 2.1344696f, 1.1800699f, 1.2763636f);
		break;
	case CaptionButtons::Custom:
		int customHeight = GlassEngine::GetDwordFromRegistry(L"CustomHeight", 33);
		int customLoneWidth = GlassEngine::GetDwordFromRegistry(L"CustomLoneWidth", 33);
		int customCloseWidth = GlassEngine::GetDwordFromRegistry(L"CustomCloseWidth", 33);
		int customMaxWidth = GlassEngine::GetDwordFromRegistry(L"CustomMaxWidth", 33);
		int customMinWidth = GlassEngine::GetDwordFromRegistry(L"CustomMinWidth", 33);
		int titleBarHeight = GlassEngine::GetDwordFromRegistry(L"CustomTitlebarHeight", 21);

		// Calculate ratios using title bar height; shoutouts to ImSwordQueen
		heightRatio = std::max(0.0f, static_cast<float>(customHeight) / static_cast<float>(titleBarHeight));
		loneWidthRatio = std::max(0.0f, static_cast<float>(customLoneWidth) / customHeight);
		closeWidthRatio = std::max(0.0f, static_cast<float>(customCloseWidth) / customHeight);
		maxWidthRatio = std::max(0.0f, static_cast<float>(customMaxWidth) / customHeight);
		minWidthRatio = std::max(0.0f, static_cast<float>(customMinWidth) / customHeight);
		break;
	} // funny defaults

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

HRESULT GlassFrameHandler::UpdateReflectionViewport(uDWM::CTopLevelWindow* window)
{
	const auto active = window->TreatAsActiveWindow();
	const auto maximized = window->TreatAsMaximized();
	const auto desktop = window->GetTransformParent();
	const auto opacity = 
	active ?
	(
		maximized ?
		Shared::g_reflectionIntensityMaximized :
		Shared::g_reflectionIntensity
	) :
	(
		maximized ?
		Shared::g_reflectionIntensityInactiveMaximized :
		Shared::g_reflectionIntensityInactive
	);
	if (
		const auto legacyVisual = window->GetLegacyVisual();
		legacyVisual &&
		legacyVisual->GetCount() == 3
	)
	{
		if (
			const auto brush = GlassReflectionBrush::GetOrCreate(window);
			brush &&
			!window->IsTrullyMinimized()
		)
		{
			RETURN_IF_FAILED(
				brush->Update(
					(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::NonClient) ?
					opacity :
					0.f,
					GlassReflectionBrush::CalculateTargetViewport(
						legacyVisual->GetLocalToParentVisualOffset(desktop),
						Shared::g_reflectionParallaxIntensity,
						window->IsRTLMirrored(),
						legacyVisual->GetWidth(),
						legacyVisual->GetScale()
					),
					D2D1::RectF(),
					nullptr,
					DWM::MilBrushMappingMode::Absolute,
					DWM::MilBrushMappingMode::Absolute,
					nullptr,
					nullptr,
					DWM::MilStretch::None,
					DWM::MilTileMode::Extend,
					DWM::MilHorizontalAlignment::Left,
					DWM::MilVerticalAlignment::Top,
					nullptr
				)
			);
		}
	}
	if (
		const auto clientBlurVisual = window->GetClientBlurVisual();
		clientBlurVisual &&
		clientBlurVisual->GetCount() == 2
	)
	{
		if (
			const auto brush = GlassReflectionBrush::GetOrCreate(window);
			brush &&
			!window->IsTrullyMinimized()
		)
		{
			RETURN_IF_FAILED(
				brush->Update(
					(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::NonClient) ?
					opacity :
					0.f,
					GlassReflectionBrush::CalculateTargetViewport(
						clientBlurVisual->GetLocalToParentVisualOffset(desktop),
						Shared::g_reflectionParallaxIntensity,
						window->IsRTLMirrored(),
						clientBlurVisual->GetWidth(),
						clientBlurVisual->GetScale()
					),
					D2D1::RectF(),
					nullptr,
					DWM::MilBrushMappingMode::Absolute,
					DWM::MilBrushMappingMode::Absolute,
					nullptr,
					nullptr,
					DWM::MilStretch::None,
					DWM::MilTileMode::Extend,
					DWM::MilHorizontalAlignment::Left,
					DWM::MilVerticalAlignment::Top,
					nullptr
				)
			);
		}
	}
	if (
		const auto accentVisual = window->GetAccent();
		accentVisual
	)
	{
		if (
			const auto brush = GlassReflectionBrush::GetOrCreate(window);
			brush &&
			!window->IsTrullyMinimized()
		)
		{
			RETURN_IF_FAILED(
				brush->Update(
					(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::NonClient) ?
					Shared::g_reflectionIntensity :
					0.f,
					GlassReflectionBrush::CalculateTargetViewport(
						accentVisual->GetLocalToParentVisualOffset(desktop),
						Shared::g_reflectionParallaxIntensity,
						window->IsRTLMirrored(),
						accentVisual->GetWidth(),
						accentVisual->GetScale()
					),
					D2D1::RectF(),
					nullptr,
					DWM::MilBrushMappingMode::Absolute,
					DWM::MilBrushMappingMode::Absolute,
					nullptr,
					nullptr,
					DWM::MilStretch::None,
					DWM::MilTileMode::Extend,
					DWM::MilHorizontalAlignment::Left,
					DWM::MilVerticalAlignment::Top,
					nullptr
				)
			);
		}
	}

	return S_OK;
}

HRGN WINAPI GlassFrameHandler::MyCreateRoundRectRgn(int x1, int y1, int x2, int y2, int w, int h)
{
	if (Shared::g_roundRectRadius == -1)
	{
		g_roundedBounds = {};
		return g_CreateRoundRectRgn_Org(x1, y1, x2, y2, w, h);
	}

	g_roundedBounds = { x1, y1, x2, y2 };
	return g_CreateRoundRectRgn_Org(x1, y1, x2, y2, Shared::g_roundRectRadius * 2, Shared::g_roundRectRadius * 2);
}
HRGN WINAPI GlassFrameHandler::MyCreateRectRgn(int x1, int y1, int x2, int y2)
{
	if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
	{
		return g_CreateRectRgn_Org(x1, y1, x2, y2);
	}

	if (g_redirectFirstCreateRectRgnCall)
	{
		if (g_redirectFirstCreateRectRgnCall.value() == true)
		{
			g_redirectFirstCreateRectRgnCall = false;
			return MyCreateRoundRectRgn(x1, y1, x2, y2, 0, 0);
		}
	}
	else
	{
		return MyCreateRoundRectRgn(x1, y1, x2, y2, 0, 0);
	}

	return g_CreateRectRgn_Org(x1, y1, x2, y2);
}

HRGN WINAPI GlassFrameHandler::MyExtCreateRegion(const XFORM* lpx, DWORD nCount, const RGNDATA* lpData)
{
	const auto rectangles = reinterpret_cast<LPRECT>(const_cast<RGNDATA*>(lpData)->Buffer);
	if (lpData->rdh.nCount == 4)
	{
		// impl for glass8 .layout propertie DontDeflateInactiveFrameGeometry
		if (
			Shared::g_dontDeflateInactiveFrameGeometry &&
			rectangles[0].top == static_cast<LONG>(uDWM::CDesktopManager::GetInstance()->GetDPIValue())
		)
		{
			// top
			rectangles[0].top = 0;
			rectangles[0].left--;
			rectangles[0].right++;
			// left
			rectangles[1].left--;
			// right
			rectangles[2].right++;
			// bottom
			rectangles[3].bottom++;
			rectangles[3].left--;
			rectangles[3].right++;
		}
	}

	// glass8: KNOWN WIN10 LIMITATION: 
	//     - due to the way how Win10 composes windows frames, 
	//       it is not possible to fully apply corner radius
	// but i fixed it
	if (lpData->rdh.nCount == 1)
	{
		if (!IsRectEmpty(&g_roundedBounds))
		{
			const auto cleanupRoundedBounds = wil::scope_exit([] { g_roundedBounds = {}; });
			const HRGN captionRgn = g_ExtCreateRegion_Org(lpx, nCount, lpData);
			if (!captionRgn)
			{
				return captionRgn;
			}
			const wil::unique_hrgn roundedRgn{ g_CreateRoundRectRgn_Org(g_roundedBounds.left, g_roundedBounds.top, g_roundedBounds.right, g_roundedBounds.bottom, Shared::g_roundRectRadius, Shared::g_roundRectRadius) };
			if (!roundedRgn)
			{
				return captionRgn;
			}
			CombineRgn(captionRgn, captionRgn, roundedRgn.get(), RGN_AND);

			return captionRgn;
		}
	}
	return g_ExtCreateRegion_Org(lpx, nCount, lpData);
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCGlassColorizationParameters_AdjustWindowColorization(
	uDWM::CGlassColorizationParameters* This,
	[[maybe_unused]] uDWM::GpCC* colorUnused,
	[[maybe_unused]] float opacity,
	BYTE flag
)
{
	const auto active = (flag & 1) != 0;
	const auto maximized = g_window ? g_window->TreatAsMaximized() : false;

	This->color = Color::ToArgb(
		GlassKernel::RealizeWindowColorization(
			GlassKernel::GetBlendingBaseColor(Shared::IsTransparencyDisabled(), Shared::IsOpaqueOnMaximized(maximized)),
			GlassKernel::GetBlendingSourceColor(active),
			GlassKernel::GetColorizationBlendingOpacity(active, maximized),
			Shared::IsTransparencyDisabled() || Shared::IsOpaqueOnMaximized(maximized),
			false
		).effectiveBlendColor
	);
	//This->color = Color::ToArgb(GlassKernel::CalculateWindowColorization(active, maximized));
	This->afterglow = 0;
	This->colorBalance = 100;
	This->afterglowBalance = 0;
	This->blurBalance = 0;
	This->windowColorization = TRUE;
	This->glassAttribute = 0;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyResourceHelper_CreateGeometryFromHRGN(HRGN hrgn, uDWM::CRgnGeometryProxy** geometry)
{
	if (g_combinedRgn)
	{
		CombineRgn(
			g_combinedRgn.get(),
			g_combinedRgn.get(),
			hrgn,
			RGN_OR
		);
	}

	return g_ResourceHelper_CreateGeometryFromHRGN_Org(
		hrgn,
		geometry
	);
}

bool STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage(uDWM::CTopLevelAtlasedRectsVisual* This, const uDWM::CAtlasedImage* atlasedImage, UINT cloneOptions)
{
	const auto partId = atlasedImage->GetPartId();

	bool isSqueegeePart = (partId - 9) <= 8;
	bool isButtonPart = (partId == 22);
	bool isWindowPart = (partId) <= 7;

	// hide button image
	if (isButtonPart)
	{
		return false;
	}

	// hide live preview images in close/minimize animation and focused live preview window
	if ((isSqueegeePart && (cloneOptions == 4)) || (isSqueegeePart && (cloneOptions != 4) && !g_windowFramesOnly))
	{
		return false;
	}

	// show window frames in focused live preview window
	if (isWindowPart && (cloneOptions != 4) && !g_windowFramesOnly)
	{
		return true;
	}

	return g_CTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage_Org(This, atlasedImage, cloneOptions);
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win10(uDWM::CTopLevelWindow* This, bool windowFramesOnly, bool unused1, bool unused2, uDWM::CTopLevelWindow** clonedWindow)
{
	g_windowFramesOnly = windowFramesOnly;
	const auto hr = reinterpret_cast<decltype(&MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win10)>(g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org)(This, windowFramesOnly, unused1, unused2, clonedWindow);
	g_windowFramesOnly = !windowFramesOnly;

	for (int i = 0; i < 4; i++)
	{
		if (auto button = This->GetButton(i))
		{
			// HACK: we need to allow the cloning of buttons after this function for close/minimize
			if (uDWM::g_versionInfo.build < os::build_w11_21h2)
			{
				button->SetExcludeSubtree(false);
			}
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win11(uDWM::CTopLevelWindow* This, bool windowFramesOnly, uDWM::CTopLevelWindow** clonedWindow)
{
	g_windowFramesOnly = windowFramesOnly;
	const auto hr = reinterpret_cast<decltype(&MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win11)>(g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org)(This, windowFramesOnly, clonedWindow);
	g_windowFramesOnly = !windowFramesOnly;

	return hr;
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_UpdateNCAreaBackground(uDWM::CTopLevelWindow* This)
{
	HRESULT hr{ S_OK };

	auto& highContrastMode = uDWM::CDesktopManager::GetInstance()->GetIsHighContrastMode();
	const auto old_highContrastMode = highContrastMode;
	if (uDWM::g_versionInfo.build >= os::build_w11_21h2)
	{
		highContrastMode = true;
	}
	const auto highContrastFakeScope = wil::scope_exit([&highContrastMode, old_highContrastMode]
	{
		if (uDWM::g_versionInfo.build >= os::build_w11_21h2)
		{
			highContrastMode = old_highContrastMode;
		}
	});

	{
		const auto active = This->TreatAsActiveWindow();

		g_redirectFirstCreateRectRgnCall = true;
		g_combinedRgn.reset(CreateRectRgn(0, 0, 0, 0));
		const auto combinedRgnScope = wil::scope_exit([] { g_redirectFirstCreateRectRgnCall = std::nullopt; g_combinedRgn.reset(); });
		hr = g_CTopLevelWindow_UpdateNCAreaBackground_Org(This);

		if (
			const auto legacyVisual = This->GetLegacyVisual();
			legacyVisual &&
			SUCCEEDED(legacyVisual->_ValidateVisual())
		)
		{
			if (auto captionGeometry = This->GetCaptionGeometry(); captionGeometry)
			{
				RETURN_IF_FAILED(
					uDWM::ResourceHelper::CreateGeometryFromHRGN(
						g_combinedRgn.get(),
						&captionGeometry
					)
				);

				if (legacyVisual->GetCount() == 2)
				{
					if (
						const auto brush = GlassReflectionBrush::GetOrCreate(
							This,
							true
						);
						brush &&
						!This->IsTrullyMinimized()
					)
					{
						RETURN_IF_FAILED(
							brush->Update(
								(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::NonClient) ?
								(
									active ?
									Shared::g_reflectionIntensity :
									Shared::g_reflectionIntensityInactive
								) :
								0.f,
								GlassReflectionBrush::CalculateTargetViewport(
									legacyVisual->GetLocalToParentVisualOffset(This->GetTransformParent()),
									Shared::g_reflectionParallaxIntensity,
									This->IsRTLMirrored(),
									legacyVisual->GetWidth(),
									legacyVisual->GetScale()
								),
								D2D1::RectF(),
								nullptr,
								DWM::MilBrushMappingMode::Absolute,
								DWM::MilBrushMappingMode::Absolute,
								nullptr,
								nullptr,
								DWM::MilStretch::None,
								DWM::MilTileMode::Extend,
								DWM::MilHorizontalAlignment::Left,
								DWM::MilVerticalAlignment::Top,
								nullptr
							)
						);
						winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
						RETURN_IF_FAILED(
							uDWM::CDrawGeometryInstruction::Create(
								brush.get(),
								captionGeometry,
								instruction.put()
							)
						);
						RETURN_IF_FAILED(legacyVisual->AddInstruction(instruction.get()));
					}
				}
			}
			if (auto borderGeometry = This->GetBorderGeometry(); borderGeometry)
			{
				RETURN_IF_FAILED(
					uDWM::ResourceHelper::CreateGeometryFromHRGN(
						wil::unique_hrgn{ CreateRectRgn(0, 0, 0, 0) }.get(),
						&borderGeometry
					)
				);
			}
		}

		auto color = This->GetCaptionColorizationParameters()->getArgbcolor();
		color.a = GlassKernel::AlphaChannelReinterpreter(active, This->TreatAsMaximized()).ToFloat();
		if (auto captionBrush = This->GetCaptionBrush(); captionBrush)
		{
			RETURN_IF_FAILED(captionBrush->Update(1.0, color));
		}
		if (auto clientBlurBrush = This->GetClientBlurBrush(); clientBlurBrush)
		{
			RETURN_IF_FAILED(clientBlurBrush->Update(1.0, color));
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes(uDWM::CTopLevelWindow* This)
{
	auto hr = g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org(This);

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

	auto& visibleMargins = This->GetMarginsVisibleOutside(maximized);
	auto& borderMargins = This->GetBorderMargins();

	int cxLeft = borderMargins.cxLeftWidth ? borderMargins.cxLeftWidth : This->GetFrameThickness();

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

	int offsetRight = maximized ? borderMargins.cxRightWidth + 2 : (borderMargins.cxRightWidth ? borderMargins.cxRightWidth - 2 : This->GetFrameThickness() - 2);

	int offsetTop;
	if (g_TopInsert == true)
	{
		offsetTop = maximized ? visibleMargins.cyTopHeight : visibleMargins.cyTopHeight + 1;
	}
	else
	{
		offsetTop = maximized ? visibleMargins.cyTopHeight - 1 : visibleMargins.cyTopHeight + 1;
	}

	auto closeButtonSize = loneButton ? CalculateButtonSize(cySize, 0) : CalculateButtonSize(cySize, 3);
	auto maxButtonSize = CalculateButtonSize(cySize, 2);
	auto minButtonSize = CalculateButtonSize(cySize, 1);
	if (g_captionButtons == 5)
	{
		int offsetGroupX = GlassEngine::GetDwordFromRegistry(L"ButtonGroupOffsetX", 0);
		int offsetGroupY = GlassEngine::GetDwordFromRegistry(L"ButtonGroupOffsetY", 0);
		int buttonSpacing = GlassEngine::GetDwordFromRegistry(L"ButtonSpacing", 0);

		int offsetCloseX = GlassEngine::GetDwordFromRegistry(L"CloseOffsetX", 0);
		int offsetCloseY = GlassEngine::GetDwordFromRegistry(L"CloseOffsetY", 0);
		int offsetMaxX = GlassEngine::GetDwordFromRegistry(L"MaxOffsetX", 0);
		int offsetMaxY = GlassEngine::GetDwordFromRegistry(L"MaxOffsetY", 0);
		int offsetMinX = GlassEngine::GetDwordFromRegistry(L"MinOffsetX", 0);
		int offsetMinY = GlassEngine::GetDwordFromRegistry(L"MinOffsetY", 0);

		offsetRight += offsetGroupX;
		offsetTop += offsetGroupY;

		if (UpdateButton(3, offsetRight + offsetCloseX, offsetTop + offsetCloseY, closeButtonSize) && !toolWindow)
			offsetRight += closeButtonSize.cx + buttonSpacing;

		if (UpdateButton(2, offsetRight + offsetMaxX, offsetTop + offsetMaxY, maxButtonSize))
			offsetRight += maxButtonSize.cx + buttonSpacing;

		if (UpdateButton(1, offsetRight + offsetMinX, offsetTop + offsetMinY, minButtonSize))
			offsetRight += minButtonSize.cx + buttonSpacing;
	}
	else
	{
		if (UpdateButton(3, offsetRight, offsetTop, closeButtonSize) && !toolWindow)
			offsetRight += closeButtonSize.cx;

		if (UpdateButton(2, offsetRight, offsetTop, maxButtonSize))
			offsetRight += maxButtonSize.cx;

		if (UpdateButton(1, offsetRight, offsetTop, minButtonSize))
			offsetRight += minButtonSize.cx;
	}

	UpdateButton(0, offsetRight, offsetTop, minButtonSize);

	if (toolWindow)
	{
		int cySmSize = GetSystemMetricsForDpi(SM_CYSMSIZE, data->GetWindowDPI());

		int customToolWidth = GlassEngine::GetDwordFromRegistry(L"CustomToolWidth", 0);
		int customToolHeight = GlassEngine::GetDwordFromRegistry(L"CustomToolHeight", 0);

		int offsetToolX = GlassEngine::GetDwordFromRegistry(L"ToolOffsetX", 0);
		int offsetToolY = GlassEngine::GetDwordFromRegistry(L"ToolOffsetY", 0);

		if (g_captionButtons == 5)
		{
			SIZE toolButtonSize = { cySmSize + customToolWidth , cySmSize + customToolHeight };
			offsetTop = (borderMargins.cyTopHeight - toolButtonSize.cy - 4 > offsetTop) ? borderMargins.cyTopHeight - toolButtonSize.cy - 4 : offsetTop;
			UpdateButton(3, offsetRight + offsetToolX, offsetTop + offsetToolY, toolButtonSize);
			offsetRight = toolButtonSize.cx + offsetRight;
		}
		else
		{
			SIZE toolButtonSize = { cySmSize , cySmSize };
			offsetTop = (borderMargins.cyTopHeight - toolButtonSize.cy - 4 > offsetTop) ? borderMargins.cyTopHeight - toolButtonSize.cy - 4 : offsetTop;
			UpdateButton(3, offsetRight, offsetTop, toolButtonSize);
			offsetRight = toolButtonSize.cx + offsetRight;
		}
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

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_UpdateClientBlur(uDWM::CTopLevelWindow* This)
{
	const auto hr = g_CTopLevelWindow_UpdateClientBlur_Org(This);
	const auto active = This->TreatAsActiveWindow();
	auto color = This->GetCaptionColorizationParameters()->getArgbcolor();
	color.a = GlassKernel::AlphaChannelReinterpreter(active, This->TreatAsMaximized()).ToFloat();

	if (auto clientBlurBrush = This->GetClientBlurBrush(); clientBlurBrush)
	{
		RETURN_IF_FAILED(clientBlurBrush->Update(1.0, color));
	}

	if (
		const auto clientBlurVisual = This->GetClientBlurVisual(); 
		clientBlurVisual &&
		clientBlurVisual->GetCount() == 1
	)
	{
		if (
			const auto brush = GlassReflectionBrush::GetOrCreate(
				This,
				true
			);
			brush &&
			!This->IsTrullyMinimized()
		)
		{
			RETURN_IF_FAILED(
				brush->Update(
					(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::NonClient) ?
					(
						active ?
						Shared::g_reflectionIntensity :
						Shared::g_reflectionIntensityInactive
					) :
					0.f,
					GlassReflectionBrush::CalculateTargetViewport(
						clientBlurVisual->GetLocalToParentVisualOffset(This->GetTransformParent()),
						Shared::g_reflectionParallaxIntensity,
						This->IsRTLMirrored(),
						clientBlurVisual->GetWidth(),
						clientBlurVisual->GetScale()
					),
					D2D1::RectF(),
					nullptr,
					DWM::MilBrushMappingMode::Absolute,
					DWM::MilBrushMappingMode::Absolute,
					nullptr,
					nullptr,
					DWM::MilStretch::None,
					DWM::MilTileMode::Extend,
					DWM::MilHorizontalAlignment::Left,
					DWM::MilVerticalAlignment::Top,
					nullptr
				)
			);
			winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
			RETURN_IF_FAILED(
				uDWM::CDrawGeometryInstruction::Create(
					brush.get(),
					static_cast<uDWM::CDrawGeometryInstruction*>(
						clientBlurVisual->GetInstructions().views()[0]
					)->GetGeometry(),
					instruction.put()
				)
			);
			RETURN_IF_FAILED(clientBlurVisual->AddInstruction(instruction.get()));
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCButton_CloneVisualTree(uDWM::CButton* This, uDWM::CButton** clonedVisual, UINT cloneOption)
{
	auto cleanup = wil::scope_exit([clonedVisual]
	{
		if (clonedVisual)
		{
			(*clonedVisual)->Release();
			*clonedVisual = nullptr;
		}
	});

	// CButton::CancelCrossfade
	if (This->GetTimeline())
	{
		*This->GetButtonState() |= 0x40u;
		This->SetDirtyFlags(0x10000);
		RETURN_IF_FAILED(This->RenderRecursive());
	}

	RETURN_IF_FAILED(uDWM::CButton::Create(clonedVisual));
	RETURN_IF_FAILED(This->InitializeVisualTreeClone(*clonedVisual, cloneOption));
	RETURN_IF_FAILED(
		(*clonedVisual)->SetVisualStates(
			This->GetGlyphBitmapArray(),
			This->GetButtonBitmapArray(),
			This->GetGlyphOpacity()
		)
	);

	*(*clonedVisual)->GetButtonState() = *This->GetButtonState();
	cleanup.release();

	return S_OK;
}

void STDMETHODCALLTYPE GlassFrameHandler::MyCButton_SetSize(uDWM::CButton* This, const SIZE* size)
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

HRESULT STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_ValidateVisual(uDWM::CTopLevelWindow* This)
{
	auto data = This->GetData();
	if (!data)
	{
		return g_CTopLevelWindow_ValidateVisual_Org(This);
	}

	g_window = This;
	const auto updateReflectionBeforeLeave = wil::scope_exit([This]
	{
		g_window = nullptr;
		LOG_IF_FAILED(UpdateReflectionViewport(This));
	});

	if (uDWM::g_versionInfo.build < os::build_w11_21h2)
	{
		return g_CTopLevelWindow_ValidateVisual_Org(This);
	}

	auto& systemBackdropType = data->GetSystemBackdropType();
	auto& captionColorOverride = data->GetCaptionColorOverride();
	auto& borderColorOverride = data->GetBorderColorOverride();
	auto& textColorOverride = data->GetTextColorOverride();
	auto& extendedFrameMargins = data->GetExtendedFrameMargins();
	auto& borderUpdatesSuppressed = This->GetIsBorderUpdatesSuppressed();
	const auto old_systemBackdropType = systemBackdropType;
	const auto old_captionColorOverride = captionColorOverride;
	const auto old_borderColorOverride = borderColorOverride;
	const auto old_textColorOverride = textColorOverride;
	const auto old_extendedFrameMargins = extendedFrameMargins;
	const auto old_borderUpdatesSuppressed = borderUpdatesSuppressed;
	const auto disableModernFrames = Shared::g_disableModernBorders && This->HasNonClientBackground(data);
	const auto windowBorder = This->GetWindowBorder();
	
	g_systemBackdrop = (uDWM::g_versionInfo.build == os::build_w11_21h2 && old_systemBackdropType) || (uDWM::g_versionInfo.build > os::build_w11_21h2 && old_systemBackdropType >= DWMSBT_MAINWINDOW);
	systemBackdropType = (uDWM::g_versionInfo.build == os::build_w11_21h2 ? DWMSBT_AUTO : DWMSBT_NONE);
	captionColorOverride = 0;
	borderColorOverride = 0;
	textColorOverride = 0;
	if (g_systemBackdrop)
	{
		// known issue: 
		// set cyTopHeight to any non zero value will cause compatibility issue with Outlook (new)
		// Outlook (new) has drawn its own titlebar buttons with native titlebar buttons hidden,
		// this will make the native titlebar buttons visible
		// 
		if (disableModernFrames)
		{
			extendedFrameMargins.cyTopHeight = 0x7FFFFFFF;
		}
		extendedFrameMargins.cxLeftWidth = 0x7FFFFFFF;
		extendedFrameMargins.cxRightWidth = 0x7FFFFFFF;
		extendedFrameMargins.cyBottomHeight = 0x7FFFFFFF;
	}
	if (disableModernFrames)
	{
		borderUpdatesSuppressed = true;
		if (windowBorder)
		{
			RETURN_IF_FAILED(windowBorder->EnableBorder(false));
		}
	}
	for (const auto& [address, instructions] : g_instructionsToReplace)
	{
		HookHelper::PatchInstructions(
			address,
			instructions.data(),
			instructions.size()
		);
	}

	const auto scope = wil::scope_exit([&, old_textColorOverride, old_borderColorOverride, old_captionColorOverride, old_systemBackdropType, old_extendedFrameMargins, disableModernFrames]
	{
		for (const auto& [address, instructions] : g_instructionsBackup)
		{
			HookHelper::PatchInstructions(
				address,
				instructions.data(),
				instructions.size()
			);
		}
		if (disableModernFrames)
		{
			if (g_systemBackdrop)
			{
				extendedFrameMargins = old_extendedFrameMargins;
			}
			borderUpdatesSuppressed = old_borderUpdatesSuppressed;
		}
		g_systemBackdrop = false;
		textColorOverride = old_textColorOverride;
		borderColorOverride = old_borderColorOverride;
		captionColorOverride = old_captionColorOverride;
		systemBackdropType = old_systemBackdropType;
	});


	return g_CTopLevelWindow_ValidateVisual_Org(This);
}

void STDMETHODCALLTYPE GlassFrameHandler::MyCTopLevelWindow_Destructor(uDWM::CTopLevelWindow* This)
{
	GlassReflectionBrush::Remove(This);
	return g_CTopLevelWindow_Destructor_Org(This);
}

bool WINAPI GlassFrameHandler::MySetMargin(
	MARGINS* dstMargins,
	int cxLeftWidth,
	int cxRightWidth,
	int cyTopHeight,
	int cyBottomHeight,
	const MARGINS* srcMargins
)
{
	return g_SetMargin_Org(
		dstMargins,
		cxLeftWidth,
		cxRightWidth,
		Shared::g_disableModernBorders ? std::max(0, cyTopHeight) : cyTopHeight,
		cyBottomHeight,
		srcMargins
	);
}

void GlassFrameHandler::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Theme)
	{
		Shared::g_disableModernBorders = static_cast<bool>(GlassEngine::GetDwordFromRegistry(L"DisableModernBorders", FALSE));
		g_captionButtons = static_cast<CaptionButtons>(GlassEngine::GetDwordFromRegistry(L"CaptionButtons", 0));
		g_TopInsert = static_cast<bool>(GlassEngine::GetDwordFromRegistry(L"TopInsert", 0));
	}
}

void GlassFrameHandler::Startup()
{
	DWORD value{ 0ul };
	wil::reg::get_value_dword_nothrow(
		GlassEngine::GetDwmLocalMachineKey(),
		L"DisabledHooks",
		&value
	);
	g_disableGlassHooks = (value & 4) != 0;

	if (g_disableGlassHooks)
	{
		return;
	}

	uDWM::g_projectionArray.ApplyToVariable("CGlassColorizationParameters::AdjustWindowColorization", g_CGlassColorizationParameters_AdjustWindowColorization_Org);
	uDWM::g_projectionArray.ApplyToVariable("ResourceHelper::CreateGeometryFromHRGN", g_ResourceHelper_CreateGeometryFromHRGN_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelAtlasedRectsVisual::ShouldCloneAtlasImage", g_CTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::UpdateNCAreaBackground", g_CTopLevelWindow_UpdateNCAreaBackground_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::UpdateNCAreaPositionsAndSizes", g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::UpdateClientBlur", g_CTopLevelWindow_UpdateClientBlur_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::ValidateVisual", g_CTopLevelWindow_ValidateVisual_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::~CTopLevelWindow", g_CTopLevelWindow_Destructor_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::CloneVisualTreeForLivePreview", g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org);
	uDWM::g_projectionArray.ApplyToVariable("SetMargin", g_SetMargin_Org);
	
	PVOID CVisual_SetSize_Org{ nullptr };
	PVOID CAtlasedRectsVisual_CloneVisualTree_Org{ nullptr };
	uDWM::g_projectionArray.ApplyToVariable("CVisual::SetSize", CVisual_SetSize_Org);
	uDWM::g_projectionArray.ApplyToVariable("CAtlasedRectsVisual::CloneVisualTree", CAtlasedRectsVisual_CloneVisualTree_Org);
	for (auto& vf : std::span{ uDWM::CButton::vftable, 20 })
	{
		if (vf == CVisual_SetSize_Org)
		{
			g_CButton_SetSize_Org_Address = reinterpret_cast<decltype(g_CButton_SetSize_Org_Address)>(&vf);
			HookHelper::WritePointer(g_CButton_SetSize_Org_Address, MyCButton_SetSize, &g_CButton_SetSize_Org);
		}
		if (vf == CAtlasedRectsVisual_CloneVisualTree_Org && uDWM::g_versionInfo.build <= os::build_w11_21h2)
		{
			g_CButton_CloneVisualTree_Org_Address = reinterpret_cast<decltype(g_CButton_CloneVisualTree_Org_Address)>(&vf);
			HookHelper::WritePointer(g_CButton_CloneVisualTree_Org_Address, MyCButton_CloneVisualTree, &g_CButton_CloneVisualTree_Org);
		}
	}


	if (uDWM::g_versionInfo.build >= os::build_w11_21h2)
	{
		UCHAR* CTopLevelWindow_UpdateWindowVisuals_Instructions{ nullptr };
		UCHAR* CDesktopManager_IsHighContrastMode_Instructions{ nullptr };
		UCHAR* CTopLevelWindow_IsShadowNCAreaPart_Instructions{ nullptr };
		uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::UpdateWindowVisuals", CTopLevelWindow_UpdateWindowVisuals_Instructions);
		uDWM::g_projectionArray.ApplyToVariable("CDesktopManager::IsHighContrastMode", CDesktopManager_IsHighContrastMode_Instructions);
		uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::IsShadowNCAreaPart", CTopLevelWindow_IsShadowNCAreaPart_Instructions);

		auto i = 1'200;
		const auto CTopLevelWindow_UpdateWindowVisuals_Instructions_Previous = CTopLevelWindow_UpdateWindowVisuals_Instructions;
		bool callCDesktopManager_IsHighContrastMode_SecondTime{};
		do
		{
			*reinterpret_cast<DWORD*>(&g_callCDesktopManager_IsHighContrastMode_Instructions[1]) = static_cast<DWORD>(CDesktopManager_IsHighContrastMode_Instructions - (CTopLevelWindow_UpdateWindowVisuals_Instructions + 5));
			if (
				memcmp(
					CTopLevelWindow_UpdateWindowVisuals_Instructions,
					g_callCDesktopManager_IsHighContrastMode_Instructions,
					sizeof(g_callCDesktopManager_IsHighContrastMode_Instructions)
				) == 0
			)
			{
				// in case we touched the inlined call part of CTopLevelWindow::GetBorderRect
				if (callCDesktopManager_IsHighContrastMode_SecondTime)
				{
					g_instructionsBackup.clear();
					g_instructionsToReplace.clear();
				}
				std::vector<UCHAR> backup(sizeof(g_callCDesktopManager_IsHighContrastMode_Instructions), 0);
				memcpy_s(
					backup.data(),
					backup.size(),
					CTopLevelWindow_UpdateWindowVisuals_Instructions,
					sizeof(g_callCDesktopManager_IsHighContrastMode_Instructions)
				);
				g_instructionsBackup.insert_or_assign(
					CTopLevelWindow_UpdateWindowVisuals_Instructions,
					backup
				);
				g_instructionsToReplace.insert_or_assign(
					CTopLevelWindow_UpdateWindowVisuals_Instructions,
					std::vector(std::begin(g_callCDesktopManager_IsHighContrastMode_replacedInstruction), std::end(g_callCDesktopManager_IsHighContrastMode_replacedInstruction))
				);
				callCDesktopManager_IsHighContrastMode_SecondTime = true;
			}

			CTopLevelWindow_UpdateWindowVisuals_Instructions += 1;
			i--;
		} while (i);

		CTopLevelWindow_UpdateWindowVisuals_Instructions = CTopLevelWindow_UpdateWindowVisuals_Instructions_Previous;
		if (uDWM::g_versionInfo.build < os::build_w11_24h2)
		{
			i = 450'000;
			do
			{
				g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_replacedInstructions[1] = g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_Instructions[1] = CTopLevelWindow_UpdateWindowVisuals_Instructions[1];
				if (
					memcmp(
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_Instructions,
						sizeof(g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_Instructions)
					) == 0
				)
				{
					std::vector<UCHAR> backup(sizeof(g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_Instructions), 0);
					memcpy_s(
						backup.data(),
						backup.size(),
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						sizeof(g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_Instructions)
					);
					g_instructionsBackup.insert_or_assign(
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						backup
					);
					g_instructionsToReplace.insert_or_assign(
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						std::vector(std::begin(g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_replacedInstructions), std::end(g_callCTopLevelWindow_IsShadowNCAreaPart_inlined_replacedInstructions))	
					);
					break;
				}

				CTopLevelWindow_UpdateWindowVisuals_Instructions += 1;
				i--;
			} while (i);
		}
		else
		{
			i = 1500;
			do
			{
				*reinterpret_cast<DWORD*>(&g_callCTopLevelWindow_IsShadowNCAreaPart_Instructions[1]) = static_cast<DWORD>(CTopLevelWindow_IsShadowNCAreaPart_Instructions - (CTopLevelWindow_UpdateWindowVisuals_Instructions + 5));
				if (
					memcmp(
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						g_callCTopLevelWindow_IsShadowNCAreaPart_Instructions,
						sizeof(g_callCTopLevelWindow_IsShadowNCAreaPart_Instructions)
					) == 0
				)
				{
					std::vector<UCHAR> backup(sizeof(g_callCTopLevelWindow_IsShadowNCAreaPart_Instructions), 0);
					memcpy_s(
						backup.data(),
						backup.size(),
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						sizeof(g_callCTopLevelWindow_IsShadowNCAreaPart_Instructions)
					);
					g_instructionsBackup.insert_or_assign(
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						backup
					);
					g_instructionsToReplace.insert_or_assign(
						CTopLevelWindow_UpdateWindowVisuals_Instructions,
						std::vector(std::begin(g_callCTopLevelWindow_IsShadowNCAreaPart_replacedInstructions), std::end(g_callCTopLevelWindow_IsShadowNCAreaPart_replacedInstructions))
					);
					break;
				}

				CTopLevelWindow_UpdateWindowVisuals_Instructions += 1;
				i--;
			} while (i);
		}
	}

	if (uDWM::g_versionInfo.build < os::build_w11_24h2)
	{
		g_CreateRoundRectRgn_Org = reinterpret_cast<decltype(g_CreateRoundRectRgn_Org)>(HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "CreateRoundRectRgn", MyCreateRoundRectRgn));
	}
	else
	{
		g_CreateRoundRectRgn_Org = CreateRoundRectRgn;
		g_CreateRectRgn_Org = reinterpret_cast<decltype(g_CreateRectRgn_Org)>(HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "CreateRectRgn", MyCreateRectRgn));
	}
	g_ExtCreateRegion_Org = reinterpret_cast<decltype(g_ExtCreateRegion_Org)>(HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "ExtCreateRegion", MyExtCreateRegion));

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]() static
		{
			HookHelper::Detours::Attach(&g_CGlassColorizationParameters_AdjustWindowColorization_Org, MyCGlassColorizationParameters_AdjustWindowColorization);
			HookHelper::Detours::Attach(&g_ResourceHelper_CreateGeometryFromHRGN_Org, MyResourceHelper_CreateGeometryFromHRGN);
			HookHelper::Detours::Attach(&g_CTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage_Org, MyCTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage);
			HookHelper::Detours::Attach(&g_CTopLevelWindow_UpdateNCAreaBackground_Org, MyCTopLevelWindow_UpdateNCAreaBackground);
			HookHelper::Detours::Attach(&g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org, MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes);
			HookHelper::Detours::Attach(&g_CTopLevelWindow_UpdateClientBlur_Org, MyCTopLevelWindow_UpdateClientBlur);
			
			if (uDWM::g_versionInfo.build <= os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(&g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org, MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win10);
			}
			else
			{
				HookHelper::Detours::Attach(&g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org, MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win11);
			}

			HookHelper::Detours::Attach(&g_CTopLevelWindow_Destructor_Org, MyCTopLevelWindow_Destructor);
			HookHelper::Detours::Attach(&g_CTopLevelWindow_ValidateVisual_Org, MyCTopLevelWindow_ValidateVisual);

			if (uDWM::g_versionInfo.build >= os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(&g_SetMargin_Org, MySetMargin);
			}
		})
	);
}

void GlassFrameHandler::Shutdown()
{
	if (g_disableGlassHooks)
	{
		return;
	}

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]() static
		{
			HookHelper::Detours::Detach(&g_CGlassColorizationParameters_AdjustWindowColorization_Org, MyCGlassColorizationParameters_AdjustWindowColorization);
			HookHelper::Detours::Detach(&g_ResourceHelper_CreateGeometryFromHRGN_Org, MyResourceHelper_CreateGeometryFromHRGN);
			HookHelper::Detours::Detach(&g_CTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage_Org, MyCTopLevelAtlasedRectsVisual_ShouldCloneAtlasImage);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_UpdateNCAreaBackground_Org, MyCTopLevelWindow_UpdateNCAreaBackground);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org, MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_UpdateClientBlur_Org, MyCTopLevelWindow_UpdateClientBlur);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_Destructor_Org, MyCTopLevelWindow_Destructor);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_ValidateVisual_Org, MyCTopLevelWindow_ValidateVisual);

			if (uDWM::g_versionInfo.build <= os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(&g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org, MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win10);
			}
			else
			{
				HookHelper::Detours::Detach(&g_CTopLevelWindow_CloneVisualTreeForLivePreview_Org, MyCTopLevelWindow_CloneVisualTreeForLivePreview_Win11);
			}

			if (uDWM::g_versionInfo.build >= os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(&g_SetMargin_Org, MySetMargin);
			}
		})
	);

	SwitchToThread();

	HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "ExtCreateRegion", g_ExtCreateRegion_Org);
	if (uDWM::g_versionInfo.build < os::build_w11_24h2)
	{
		HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "CreateRoundRectRgn", g_CreateRoundRectRgn_Org);
	}
	else
	{
		HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "CreateRectRgn", g_CreateRectRgn_Org);
	}

	if (g_CButton_SetSize_Org)
	{
		HookHelper::WritePointer(g_CButton_SetSize_Org_Address, g_CButton_SetSize_Org);
	}
	if (g_CButton_CloneVisualTree_Org)
	{
		HookHelper::WritePointer(g_CButton_CloneVisualTree_Org_Address, g_CButton_CloneVisualTree_Org);
	}

	g_combinedRgn.reset();
	g_instructionsBackup.clear();
}
