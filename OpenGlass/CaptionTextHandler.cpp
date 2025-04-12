#include "pch.h"
#include "CaptionTextHandler.hpp"
#include "Shared.hpp"
#include "GlassKernel.hpp"
#include "uDWMProjection.hpp"
#include "dwmcoreProjection.hpp"
#include "CustomThemeAtlasLoader.hpp"

using namespace OpenGlass;
namespace OpenGlass::CaptionTextHandler
{
	int WINAPI MyDrawTextW(
		HDC hdc,
		LPCWSTR lpchText,
		int cchText,
		LPRECT lprc,
		UINT format
	);
	HBITMAP WINAPI MyCreateBitmap(
		int nWidth, 
		int nHeight, 
		UINT nPlanes, 
		UINT nBitCount, 
		const void* lpBits
	);
	HRESULT STDMETHODCALLTYPE MyIWICImagingFactory2_CreateBitmapFromHBITMAP(
		IWICImagingFactory2* This,
		HBITMAP hBitmap,
		HPALETTE hPalette,
		WICBitmapAlphaChannelOption options,
		IWICBitmap** ppIBitmap
	);
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_UpdateWindowVisuals(uDWM::CTopLevelWindow* This);
	HRESULT STDMETHODCALLTYPE MyCText_ValidateResources(uDWM::CText* This);
	HRESULT STDMETHODCALLTYPE MyCText_SetSize(uDWM::CText* This, const SIZE* size);
	HRESULT STDMETHODCALLTYPE MyCText_InitializeVisualTreeClone(uDWM::CText* This, uDWM::CText* clonedVisual, UINT cloneOption);
	HRESULT STDMETHODCALLTYPE MyCText_scalar_deleting_destructor(uDWM::CText* This, BYTE flag);
	HRESULT STDMETHODCALLTYPE MyCChannel_MatrixTransformUpdate(dwmcore::CChannel* This, UINT handleId, MilMatrix3x2D* matrix);

	decltype(&MyDrawTextW) g_DrawTextW_Org{ nullptr };
	decltype(&MyCreateBitmap) g_CreateBitmap_Org{ nullptr };
	decltype(&MyIWICImagingFactory2_CreateBitmapFromHBITMAP) g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org{ nullptr };
	decltype(&MyCTopLevelWindow_UpdateWindowVisuals) g_CTopLevelWindow_UpdateWindowVisuals_Org{ nullptr };
	decltype(&MyCText_ValidateResources) g_CText_ValidateResources_Org{ nullptr };
	decltype(&MyCText_SetSize) g_CText_SetSize_Org{ nullptr };
	decltype(&MyCText_InitializeVisualTreeClone) g_CText_InitializeVisualTreeClone_Org{ nullptr };
	decltype(&MyCText_scalar_deleting_destructor) g_CText_scalar_deleting_destructor_Org{ nullptr };
	decltype(&MyCChannel_MatrixTransformUpdate) g_CChannel_MatrixTransformUpdate_Org{ nullptr };
	PVOID* g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org_Address{ nullptr };

	uDWM::CText* g_textVisual{ nullptr };
	bool g_active{ false };

	// original sizes, no glow included
	SIZE g_textSize{};
	COLORREF g_captionActiveColor{};
	COLORREF g_captionInactiveColor{};
	MARGINS g_contentMargins{}, g_sizingMargins{};
	std::unordered_map<uDWM::CText*, bool> g_textVisualStateMap{};

	int g_textGlowSize{};
	BOOL g_centerCaption{ FALSE };
	bool g_disableTextHooks{ false };

	void CalculateRealizedTextGlowSize(int textGlowMode);
}

int WINAPI CaptionTextHandler::MyDrawTextW(
	HDC hdc,
	LPCWSTR lpchText,
	int cchText,
	LPRECT lprc,
	UINT format
)
{
	int result{ 0 };
	auto drawTextCallback = [](HDC hdc, LPWSTR pszText, int cchText, LPRECT prc, UINT dwFlags, LPARAM lParam) -> int
	{
		return *reinterpret_cast<int*>(lParam) = g_DrawTextW_Org(hdc, pszText, cchText, prc, dwFlags);
	};

	if ((format & DT_CALCRECT))
	{
		result = g_DrawTextW_Org(hdc, lpchText, cchText, lprc, format);

		g_textSize.cx = wil::rect_width(*lprc);
		g_textSize.cy = wil::rect_height(*lprc);

		return result;
	}
	// clear the background, so the text can be shown transparent
	// with this hack, we don't need to hook FillRect any more
	BITMAP bmp{};
	if (GetObjectW(GetCurrentObject(hdc, OBJ_BITMAP), sizeof(bmp), &bmp) && bmp.bmBits)
	{
		memset(bmp.bmBits, 0, 4 * bmp.bmWidth * bmp.bmHeight);
	}

	OffsetRect(lprc, g_textGlowSize, g_textGlowSize);
	
	auto textColor = GetTextColor(hdc);
	auto textColorOverride = g_active ? g_captionActiveColor : g_captionInactiveColor;
	auto useThemeColor = textColorOverride == 0xFFFFFFFE;
	DTTOPTS options
	{
		sizeof(DTTOPTS),
		DTT_TEXTCOLOR | DTT_COMPOSITED | DTT_CALLBACK | DTT_GLOWSIZE,
		textColorOverride != 0xFFFFFFFF ? textColorOverride : textColor,
		0,
		0,
		0,
		{},
		0,
		0,
		0,
		0,
		FALSE,
		0,
		drawTextCallback,
		(LPARAM)&result
	};
	if (useThemeColor)
	{
		options.dwFlags &= ~DTT_TEXTCOLOR;
		options.crText = 0;
	}
	if (LOWORD(Shared::g_textGlowMode) == 3)
	{
		options.iGlowSize = g_textGlowSize;
	}
	wil::unique_htheme hTheme{ OpenThemeData(nullptr, L"CompositedWindow::Window") };

	if (Shared::g_textGlowMode == 1 || Shared::g_textGlowMode == 2)
	{
		const auto opacity = Shared::g_textGlowMode == 2 ? static_cast<int>((g_active ? Shared::g_textOpacity : Shared::g_textOpacityInactive) * 255.f) : 255;
		RECT glowDrawRect
		{
			lprc->left - g_contentMargins.cxLeftWidth,
			lprc->top - g_contentMargins.cyTopHeight,
			lprc->right + g_contentMargins.cxRightWidth,
			lprc->bottom + g_contentMargins.cyBottomHeight
		};

		Util::DrawNineGridBitmap(
			hdc,
			Shared::g_textGlowBitmap.get(),
			glowDrawRect,
			g_sizingMargins,
			opacity
		);
	}
	if (hTheme)
	{
		THROW_IF_FAILED(
			DrawThemeTextEx(
				hTheme.get(), 
				hdc, 
				useThemeColor ? WP_CAPTION : 0,
				useThemeColor ? (g_active ? CS_ACTIVE : CS_INACTIVE) : 0,
				lpchText, 
				cchText, 
				format, 
				lprc, 
				&options
			)
		);
	}
	else
	{
		THROW_HR_IF_NULL(E_FAIL, hTheme);
		result = g_DrawTextW_Org(hdc, lpchText, cchText, lprc, format);
	}

	// override that so we can use the correct param in CDrawImageInstruction::Create
	lprc->left -= g_textGlowSize;
	lprc->top -= g_textGlowSize;
	lprc->right += g_textGlowSize;
	lprc->bottom += g_textGlowSize;

	return result;
}
HBITMAP WINAPI CaptionTextHandler::MyCreateBitmap(
	int nWidth,
	int nHeight,
	[[maybe_unused]] UINT nPlanes,
	[[maybe_unused]] UINT nBitCount,
	[[maybe_unused]] const void* lpBits
)
{
	if (nWidth == 1 && nHeight == 1)
	{
		return g_CreateBitmap_Org(nWidth, nHeight, nPlanes, nBitCount, lpBits);
	}

	nWidth = g_textSize.cx + g_textGlowSize * 2;
	nHeight = g_textSize.cy + g_textGlowSize * 2;

	PVOID bits{ nullptr };
	BITMAPINFO bitmapInfo{ {sizeof(bitmapInfo.bmiHeader), nWidth, -nHeight, 1, 32, BI_RGB} };
	HBITMAP bitmap{ CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0) };
	memset(bits, 0, sizeof(nWidth * nHeight * 4));

	return bitmap;
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyIWICImagingFactory2_CreateBitmapFromHBITMAP(
	IWICImagingFactory2* This,
	HBITMAP hBitmap,
	HPALETTE hPalette,
	[[maybe_unused]] WICBitmapAlphaChannelOption options,
	IWICBitmap** ppIBitmap
)
{
	return g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org(
		This,
		hBitmap,
		hPalette,
		WICBitmapAlphaChannelOption::WICBitmapUsePremultipliedAlpha,
		ppIBitmap
	);
}

HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCTopLevelWindow_UpdateWindowVisuals(uDWM::CTopLevelWindow* This)
{
	const auto hr = g_CTopLevelWindow_UpdateWindowVisuals_Org(This);
	if (const auto textVisual = This->GetTextVisual(); textVisual)
	{
		g_textVisualStateMap[textVisual] = This->TreatAsActiveWindow();
	}
	return hr;
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCText_ValidateResources(uDWM::CText* This)
{
	// redraw the text to get the width and height
	This->SetDirtyFlags(0x1000);
	g_active = g_textVisualStateMap[This];
	g_textVisual = This;
	auto hr = g_CText_ValidateResources_Org(This);
	g_textVisual = nullptr;
	g_textSize = {};

	return hr;
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCText_SetSize(uDWM::CText* This, const SIZE* size)
{
	if (!g_centerCaption)
	{
		return g_CText_SetSize_Org(This, size);
	}

	auto oldWidth = This->GetWidth();
	auto hr = g_CText_SetSize_Org(This, size);

	// update align transform
	if (oldWidth != size->cx)
	{
		This->SetDirtyFlags(0x8000);
	}

	return hr;
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCText_InitializeVisualTreeClone(uDWM::CText* This, uDWM::CText* clonedVisual, UINT cloneOption)
{
	g_textVisualStateMap[clonedVisual] = g_textVisualStateMap[This];
	return g_CText_InitializeVisualTreeClone_Org(This, clonedVisual, cloneOption);
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCText_scalar_deleting_destructor(uDWM::CText* This, BYTE flag)
{
	g_textVisualStateMap.erase(This);
	return g_CText_scalar_deleting_destructor_Org(This, flag);
}
HRESULT STDMETHODCALLTYPE CaptionTextHandler::MyCChannel_MatrixTransformUpdate(dwmcore::CChannel* This, UINT handleId, MilMatrix3x2D* matrix)
{
	if (g_textVisual)
	{
		matrix->DX -= static_cast<DOUBLE>(g_textGlowSize);
		if (g_centerCaption)
		{
			auto offset = std::floor(
				std::min(
					static_cast<DOUBLE>(
						(
							g_centerCaption == 2 ?
							g_textVisual->GetTransformParent()->GetWidth() :
							g_textVisual->GetWidth()
						) - g_textSize.cx
					) / 2.
					-
					(
						g_centerCaption == 2 ?
						static_cast<DOUBLE>(g_textVisual->GetX()) :
						0.
					),
					static_cast<DOUBLE>(g_textVisual->GetWidth()) - 
					static_cast<DOUBLE>(g_textSize.cx)
				)
			);
			matrix->DX += g_textVisual->IsRTL() ? -offset : offset;
		}
		matrix->DY = std::floor(
			static_cast<DOUBLE>(
				g_textVisual->GetHeight() -
				(g_textSize.cy + g_textGlowSize * 2)
			) / 2.
		);
	}

	return g_CChannel_MatrixTransformUpdate_Org(This, handleId, matrix);
}

void CaptionTextHandler::CalculateRealizedTextGlowSize(int textGlowMode)
{
	if (textGlowMode == 0)
	{
		g_textGlowSize = 0;
	}
	else if (textGlowMode == 1 || textGlowMode == 2)
	{
		const auto themeHandle = CustomThemeAtlasLoader::GetThemeHandle();
		if (themeHandle)
		{
			CustomThemeAtlasLoader::MyGetThemeMargins(
				themeHandle,
				nullptr,
				45,
				0,
				TMT_SIZINGMARGINS,
				nullptr,
				&g_sizingMargins
			);
			CustomThemeAtlasLoader::MyGetThemeMargins(
				themeHandle,
				nullptr,
				45,
				0,
				TMT_CONTENTMARGINS,
				nullptr,
				&g_contentMargins
			);
			g_textGlowSize = std::max(
				{
					g_contentMargins.cxLeftWidth,
					g_contentMargins.cxRightWidth,
					g_contentMargins.cyTopHeight,
					g_contentMargins.cyBottomHeight
				}
			);
		}
		else
		{
			g_sizingMargins = {};
			g_contentMargins = {};
		}
	}
	else
	{
		g_textGlowSize = HIWORD(textGlowMode);
		if (!g_textGlowSize)
		{
			wil::unique_htheme themeHandle{ OpenThemeData(nullptr, L"CompositedWindow::Window") };
			if (themeHandle)
			{
				CustomThemeAtlasLoader::MyGetThemeInt(themeHandle.get(), 0, 0, TMT_TEXTGLOWSIZE, &g_textGlowSize);
			}
			else
			{
				g_textGlowSize = 12;
			}
		}
	}
}

void CaptionTextHandler::Update(GlassEngine::UpdateType type)
{
	if (g_disableTextHooks)
	{
		return;
	}

	if (uDWM::g_buildNumber >= os::build_w11_22h2)
	{
		return;
	}

	if (type & GlassEngine::UpdateType::Theme)
	{
		CalculateRealizedTextGlowSize(Shared::g_textGlowMode);
	}
	if (type & GlassEngine::UpdateType::Backdrop)
	{
		g_centerCaption = std::clamp(static_cast<int>(GlassEngine::GetDwordFromRegistry(L"CenterCaption", FALSE)), 0, 2);
		g_captionActiveColor = GlassEngine::GetDwordFromRegistry(L"ColorizationColorCaption", 0xFFFFFFFF);
		g_captionInactiveColor = GlassEngine::GetDwordFromRegistry(L"ColorizationColorCaptionInactive", g_captionActiveColor);
	}
}

void CaptionTextHandler::Startup()
{
	DWORD value{ 0ul };
	wil::reg::get_value_dword_nothrow(
		GlassEngine::GetDwmKey(),
		L"DisabledHooks",
		&value
	);
	g_disableTextHooks = (value & 1) != 0;

	if (g_disableTextHooks)
	{
		return;
	}

	if (uDWM::g_buildNumber >= os::build_w11_22h2)
	{
		return;
	}

	dwmcore::g_projectionArray.ApplyToVariable("CChannel::MatrixTransformUpdate", g_CChannel_MatrixTransformUpdate_Org);
	uDWM::g_projectionArray.ApplyToVariable("CText::ValidateResources", g_CText_ValidateResources_Org);
	uDWM::g_projectionArray.ApplyToVariable("CText::SetSize", g_CText_SetSize_Org);
	uDWM::g_projectionArray.ApplyToVariable("CText::InitializeVisualTreeClone", g_CText_InitializeVisualTreeClone_Org);
	uDWM::g_projectionArray.ApplyToVariable("CText::`scalar deleting destructor'", g_CText_scalar_deleting_destructor_Org);
	uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::UpdateWindowVisuals", g_CTopLevelWindow_UpdateWindowVisuals_Org);

	wil::unique_hmodule wincodecsMoudle{ LoadLibraryExW(L"WindowsCodecs.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) };
	THROW_LAST_ERROR_IF_NULL(wincodecsMoudle);
	const auto WICCreateImagingFactory_Proxy_fn = reinterpret_cast<HRESULT(WINAPI*)(UINT, IWICImagingFactory2**)>(
		GetProcAddress(wincodecsMoudle.get(), "WICCreateImagingFactory_Proxy")
	);
	THROW_LAST_ERROR_IF_NULL(WICCreateImagingFactory_Proxy_fn);
	winrt::com_ptr<IWICImagingFactory2> wicFactory{ nullptr };
	THROW_IF_FAILED(WICCreateImagingFactory_Proxy_fn(WINCODEC_SDK_VERSION2, wicFactory.put()));
	g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org_Address = &HookHelper::vftbl_of(wicFactory.get())[21];
	g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org = HookHelper::WritePointer(
		g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org_Address,
		MyIWICImagingFactory2_CreateBitmapFromHBITMAP
	);
	g_DrawTextW_Org = reinterpret_cast<decltype(g_DrawTextW_Org)>(HookHelper::WriteIAT(uDWM::g_moduleHandle, "user32.dll", "DrawTextW", MyDrawTextW));
	g_CreateBitmap_Org = reinterpret_cast<decltype(g_CreateBitmap_Org)>(HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "CreateBitmap", MyCreateBitmap));

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Attach(&g_CChannel_MatrixTransformUpdate_Org, MyCChannel_MatrixTransformUpdate);
			HookHelper::Detours::Attach(&g_CText_ValidateResources_Org, MyCText_ValidateResources);
			HookHelper::Detours::Attach(&g_CText_SetSize_Org, MyCText_SetSize);
			HookHelper::Detours::Attach(&g_CText_InitializeVisualTreeClone_Org, MyCText_InitializeVisualTreeClone);
			HookHelper::Detours::Attach(&g_CText_scalar_deleting_destructor_Org, MyCText_scalar_deleting_destructor);
			HookHelper::Detours::Attach(&g_CTopLevelWindow_UpdateWindowVisuals_Org, MyCTopLevelWindow_UpdateWindowVisuals);
		})
	);

	const auto lock = wil::EnterCriticalSection(uDWM::CDesktopManager::s_csDwmInstance);
	ULONG_PTR desktopID{ 0 };
	Util::GetDesktopID(1, &desktopID);
	const auto windowList = uDWM::CDesktopManager::GetInstance()->GetWindowList()->GetWindowListForDesktop(desktopID);
	for (auto i = windowList->Blink; i != windowList; i = i->Blink)
	{
		const auto window = reinterpret_cast<uDWM::CWindowData*>(i)->GetWindow();
		if (!window)
		{
			continue;
		}
		const auto textVisual = window->GetTextVisual();
		if (!textVisual)
		{
			continue;
		}
		g_textVisualStateMap[textVisual] = window->TreatAsActiveWindow();
	}
}
void CaptionTextHandler::Shutdown()
{
	if (g_disableTextHooks)
	{
		return;
	}

	if (uDWM::g_buildNumber >= os::build_w11_22h2)
	{
		return;
	}

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_CChannel_MatrixTransformUpdate_Org, MyCChannel_MatrixTransformUpdate);
			HookHelper::Detours::Detach(&g_CText_ValidateResources_Org, MyCText_ValidateResources);
			HookHelper::Detours::Detach(&g_CText_SetSize_Org, MyCText_SetSize);
			HookHelper::Detours::Detach(&g_CText_InitializeVisualTreeClone_Org, MyCText_InitializeVisualTreeClone);
			HookHelper::Detours::Detach(&g_CText_scalar_deleting_destructor_Org, MyCText_scalar_deleting_destructor);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_UpdateWindowVisuals_Org, MyCTopLevelWindow_UpdateWindowVisuals);
		})
	);

	Sleep(1);

	HookHelper::WritePointer(
		g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org_Address,
		g_IWICImagingFactory2_CreateBitmapFromHBITMAP_Org
	);
	HookHelper::WriteIAT(uDWM::g_moduleHandle, "user32.dll", "DrawTextW", g_DrawTextW_Org);
	HookHelper::WriteIAT(uDWM::g_moduleHandle, "gdi32.dll", "CreateBitmap", g_CreateBitmap_Org);

	g_textVisualStateMap.clear();
	g_textVisual = nullptr;
	g_textSize = {};
}