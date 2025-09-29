#include "pch.h"
#include "GlassKernel.hpp"
#include "uDWMProjection.hpp"
#include "Shared.hpp"
#include "dwmcoreProjection.hpp"
#include "GlassReflectionBrush.hpp"
#include "GlassRenderer.hpp"
#include "GlassIntegrity.hpp"
#include "CaptionTextHandler.hpp"
#include "CustomThemeAtlasLoader.hpp"

using namespace OpenGlass;
namespace OpenGlass::GlassKernel
{
	HRESULT STDMETHODCALLTYPE MyIDCompositionDesktopDevice_WaitForCommitCompletion(IDCompositionDesktopDevice* This);
	HRESULT STDMETHODCALLTYPE MyCCachedVisualImage_CCachedTarget_Update(
		dwmcore::CCachedVisualImage::CCachedTarget* This,
		const D2D1_RECT_F& rect,
		DWM::MilStretch mode,
		const struct RenderTargetInfo& info
	);
	HRESULT STDMETHODCALLTYPE MyCDrawingContext_PreSubgraph(
		dwmcore::CDrawingContext* This,
		dwmcore::CVisualTree* visualTree,
		bool* conditionalBreak
	);
	HRESULT STDMETHODCALLTYPE MyCD2DContext_DestroyDeviceResources(dwmcore::CD2DContext* This);
	HRESULT STDMETHODCALLTYPE MyCXXX_ReleaseXXX(PVOID This);

	decltype(&MyIDCompositionDesktopDevice_WaitForCommitCompletion) g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org{ nullptr };
	decltype(&MyIDCompositionDesktopDevice_WaitForCommitCompletion)* g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address{ nullptr };
	decltype(&MyCCachedVisualImage_CCachedTarget_Update) g_CCachedVisualImage_CCachedTarget_Update_Org{ nullptr };
	decltype(&MyCDrawingContext_PreSubgraph) g_CDrawingContext_PreSubgraph_Org{ nullptr };
	decltype(&MyCD2DContext_DestroyDeviceResources) g_CD2DContext_DestroyDeviceResources_Org{ nullptr };
	decltype(&MyCXXX_ReleaseXXX) g_CXXX_ReleaseXXX_Org{ nullptr };

	ULONG g_old_dwOverlayTestMode{};
	size_t g_CVIHierarchy{};
	HWND g_hwnd{};

	void RedrawTopLevelWindow(uDWM::CTopLevelWindow* window)
	{
		if (window)
		{
			if (uDWM::g_versionInfo.build >= os::build_w11_22h2)
			{
				if (const auto dwriteTextVisual = window->GetDWriteTextVisual(); dwriteTextVisual)
				{
					// update text offset
					dwriteTextVisual->SetDirtyFlags(8);
					// update text color
					dwriteTextVisual->SetDirtyFlags(0x1000);
				}
			}
			// update text
			window->SetDirtyFlags(0x10000u);
			// update nc background
			window->SetDirtyFlags(0x100000u);
			// in case of fully transparent if color not changed
			if (const auto legacyVisual = window->GetLegacyVisual(); legacyVisual)
			{
				legacyVisual->ClearInstructions();
			}
			if (const auto colorizationParams = window->GetCaptionColorizationParameters(); colorizationParams)
			{
				if (const auto captionBrush = window->GetCaptionBrush(); captionBrush)
				{
					captionBrush->Update(1.0, colorizationParams->getArgbcolor());
				}
				if (const auto clientBlurBrush = window->GetClientBlurBrush(); clientBlurBrush)
				{
					clientBlurBrush->Update(1.0, colorizationParams->getArgbcolor());
				}
			}
			// update blur behind
			window->OnBlurBehindUpdated();
			if (const auto accentVisual = window->GetAccent(); accentVisual)
			{
				accentVisual->SetDirtyFlags(0x10000);
			}
			window->OnAccentPolicyUpdated();
			// update system backdrop
			if (uDWM::g_versionInfo.build >= os::build_w11_21h2)
			{
				window->SetDirtyFlags(0x4000u);
			}
		}
	}


	float GetColorizationBlendingOpacity(bool active, bool maximized)
	{
		if (active && !maximized)
		{
			return Shared::g_colorizationBlendingOpacity;
		}
		else if (active && maximized)
		{
			return Shared::g_colorizationBlendingOpacityMaximized;
		}
		else if (!active && !maximized)
		{
			return Shared::g_colorizationBlendingOpacityInactive;
		}
		else if (!active && maximized)
		{
			return Shared::g_colorizationBlendingOpacityInactiveMaximized;
		}

		return 0.f;
	}
	D2D1_COLOR_F GetBlendingBaseColor(bool opaque, bool opaqueByMaximization)
	{
		D2D1_COLOR_F color{};

		if (Shared::g_opaqueBlendPriority == Shared::OpaqueBlendPriority::Vista)
		{
			if (opaqueByMaximization)
			{
				color = Shared::g_opaqueBlendColorMaximized;
				color.a = 1.f;
			}
			else if (opaque)
			{
				color = Shared::g_opaqueBlendColor;
				color.a = 1.f;
			}
			else
			{
				color = {};
			}
		}
		else
		{
			if (opaque)
			{
				color = Shared::g_opaqueBlendColor;
				color.a = 1.f;
			}
			else if (opaqueByMaximization)
			{
				color = Shared::g_opaqueBlendColorMaximized;
				color.a = 1.f;
			}
			else
			{
				color = {};
			}
		}

		return color;
	}
	D2D1_COLOR_F GetBlendingSourceColor(bool active)
	{
		if (Shared::g_type == Shared::GlassType::Blur)
		{
			const auto& color = active ? Shared::g_color : Shared::g_colorInactive;
			return
			{
				color.r,
				color.g,
				color.b,
				active ? Shared::g_glassOpacity : Shared::g_glassOpacityInactive
			};
		}
		#pragma warning(suppress:26813)
		else if (Shared::g_type == Shared::GlassType::Aero)
		{
			return
			{
				Shared::g_color.r,
				Shared::g_color.g,
				Shared::g_color.b,
				Shared::g_colorBalance
			};
		}

		return {};
	}
	CRealizedGlassColorizationParameters RealizeWindowColorization(
		const D2D1_COLOR_F& baseColor,
		const D2D1_COLOR_F& srcColor,
		float colorizationOpacity,
		bool opaque,
		bool livePreview
	)
	{
		CRealizedGlassColorizationParameters parameters{};

		if (Shared::g_type == Shared::GlassType::Blur)
		{
			parameters.color = srcColor;
			parameters.color.a *= colorizationOpacity;

			float a = (1.f - parameters.color.a) * baseColor.a + parameters.color.a;
			parameters.color.r = ((1.f - parameters.color.a) * baseColor.r * baseColor.a + parameters.color.r * parameters.color.a) / a;
			parameters.color.g = ((1.f - parameters.color.a) * baseColor.g * baseColor.a + parameters.color.g * parameters.color.a) / a;
			parameters.color.b = ((1.f - parameters.color.a) * baseColor.b * baseColor.a + parameters.color.b * parameters.color.a) / a;
			parameters.color.a = a;

			parameters.effectiveBlendColor = parameters.color;
		}
		#pragma warning(suppress:26813)
		else if (Shared::g_type == Shared::GlassType::Aero)
		{
			// uDWM!CGlassColorizationParameters::AdjustWindowColorization (Windows 7)
			parameters.afterglowBalance = Shared::g_afterglowBalance * (1.f - baseColor.a);
			parameters.blurBalance = Shared::g_blurBalance * (1.f - baseColor.a);

			parameters.afterglow = Shared::g_afterglow;
			parameters.color = srcColor;
			parameters.color.a *= colorizationOpacity;

			float a = (1.f - parameters.color.a) * baseColor.a + parameters.color.a;
			parameters.color.r = ((1.f - parameters.color.a) * baseColor.r * baseColor.a + parameters.color.r * parameters.color.a) / a;
			parameters.color.g = ((1.f - parameters.color.a) * baseColor.g * baseColor.a + parameters.color.g * parameters.color.a) / a;
			parameters.color.b = ((1.f - parameters.color.a) * baseColor.b * baseColor.a + parameters.color.b * parameters.color.a) / a;
			parameters.color.a = a;

			if (opaque)
			{
				parameters.blurBalance = 0.f;
			}
			else if (!livePreview)
			{
				parameters.blurBalance = 1.f - (1.f - parameters.blurBalance) * colorizationOpacity;
			}

			parameters.colorBalance = parameters.color.a;
			parameters.afterglowBalance = parameters.afterglowBalance;
			parameters.blurBalance = parameters.blurBalance;

			// dwmcore!CCapturedGlassColorizationParameters::GetEffectivescRGBBlendColor (Windows 7)
			parameters.effectiveBlendColor = parameters.color;
			if (opaque)
			{
				parameters.effectiveBlendColor.r *= parameters.colorBalance;
				parameters.effectiveBlendColor.g *= parameters.colorBalance;
				parameters.effectiveBlendColor.b *= parameters.colorBalance;
				parameters.effectiveBlendColor.a = 1.f;
			}
			else
			{
				const auto alpha = std::max(1.f - parameters.blurBalance, 0.1f);
				parameters.effectiveBlendColor =
				{
					std::clamp((parameters.effectiveBlendColor.r * parameters.colorBalance + parameters.afterglow.r * parameters.afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
					std::clamp((parameters.effectiveBlendColor.g * parameters.colorBalance + parameters.afterglow.g * parameters.afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
					std::clamp((parameters.effectiveBlendColor.b * parameters.colorBalance + parameters.afterglow.b * parameters.afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
					alpha
				};
			}
		}

		return parameters;
	}

	bool IsCurrentCVIFullyTransparent()
	{
		return g_CVIHierarchy && !g_hwnd;
	}
}

HRESULT STDMETHODCALLTYPE GlassKernel::MyIDCompositionDesktopDevice_WaitForCommitCompletion([[maybe_unused]] IDCompositionDesktopDevice* This)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE GlassKernel::MyCCachedVisualImage_CCachedTarget_Update(
	dwmcore::CCachedVisualImage::CCachedTarget* This,
	const D2D1_RECT_F& rect,
	DWM::MilStretch mode,
	const struct RenderTargetInfo& info
)
{
	g_hwnd = nullptr;
	g_CVIHierarchy += 1;
	const auto hr = g_CCachedVisualImage_CCachedTarget_Update_Org(
		This,
		rect,
		mode,
		info
	);
	g_CVIHierarchy -= 1;
	g_hwnd = nullptr;
	return hr;
}

HRESULT STDMETHODCALLTYPE GlassKernel::MyCDrawingContext_PreSubgraph(
	dwmcore::CDrawingContext* This,
	dwmcore::CVisualTree* visualTree,
	bool* conditionalBreak
)
{
	if (g_CVIHierarchy && !g_hwnd)
	{
		const auto visual = This->GetCurrentVisualHelper();
		if (visual)
		{
			const auto hwnd = visual->GetTopLevelWindow();
			if (hwnd)
			{
				g_hwnd = hwnd;
			}
		}
	}

	return g_CDrawingContext_PreSubgraph_Org(This, visualTree, conditionalBreak);
}

HRESULT STDMETHODCALLTYPE GlassKernel::MyCD2DContext_DestroyDeviceResources(dwmcore::CD2DContext* This)
{
	GlassRenderer::DestroyDeviceResources(This);
	GlassIntegrity::DestroyDeviceResources(This);
	return g_CD2DContext_DestroyDeviceResources_Org(This);
}

HRESULT STDMETHODCALLTYPE GlassKernel::MyCXXX_ReleaseXXX(PVOID This)
{
	CaptionTextHandler::DestroyDeviceResources();
	return g_CXXX_ReleaseXXX_Org(This);
}

void GlassKernel::RedrawAllTopLevelWindow()
{
	const auto lock = wil::EnterCriticalSection(uDWM::CDesktopManager::s_csDwmInstance);
	ULONG_PTR desktopID{ 0 };
	Util::GetDesktopID(1, &desktopID);
	const auto windowList = uDWM::CDesktopManager::GetInstance()->GetWindowList()->GetWindowListForDesktop(desktopID);
	for (auto i = windowList->Blink; i != windowList; i = i->Blink)
	{
		RedrawTopLevelWindow(
			reinterpret_cast<uDWM::CWindowData*>(i)->GetWindow()
		);
	}
}

float GlassKernel::GetBlurExtendedAmount()
{
	if (Shared::IsTransparencyDisabled())
	{
		return 0.f;
	}
	if (Shared::g_blurAmount == 0.f)
	{
		return 0.f;
	}
	if (Shared::g_useD3DRendering)
	{
		return 8.f;
	}

	return Shared::g_blurAmount * 3.f + 0.5f;
}

void GlassKernel::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Theme)
	{
		Shared::g_textGlowMode = GlassEngine::GetDwordFromRegistry(L"TextGlowMode", 1);

		WCHAR reflectionTexturePath[MAX_PATH]{};
		GlassEngine::GetStringFromRegistry(L"CustomThemeReflection", reflectionTexturePath);
		PathUnquoteSpacesW(reflectionTexturePath);
		Shared::g_reflectionTexturePath.assign(reflectionTexturePath);
	}
	if (type & GlassEngine::UpdateType::Framework)
	{
		DWORD value{ 0ul };
		if (
			FAILED(
				wil::reg::get_value_dword_nothrow(
					GlassEngine::GetDwmLocalMachineKey(),
					L"DisableGlassOnBattery",
					&value
				)
			)
		)
		{
			value = 1ul;
		}
		Shared::g_disableOnBattery = value;
	}
	if (type & GlassEngine::UpdateType::Backdrop)
	{
		auto value = GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionIntensity");
		Shared::g_reflectionIntensity = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);
		Shared::g_reflectionIntensityInactive = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionIntensityInactive", value)) / 100.f, 0.f, 1.f);
		Shared::g_reflectionParallaxIntensity = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionParallaxIntensity", 13)) / 100.f, 0.f, 1.f);

		Shared::g_type = static_cast<Shared::GlassType>(std::clamp(GlassEngine::GetDwordFromRegistry(L"GlassType", 0), 0ul, 1ul));
		Shared::g_reflectionPolicy = static_cast<Shared::ReflectionPolicy>(GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionPolicy", 0xFFFFFFFF));
		Shared::g_blurAmount = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"BlurDeviation", 30)) / 10.f * 3.f, 0.f, 250.f);
		Shared::g_blurOptimization = static_cast<D2D1_GAUSSIANBLUR_OPTIMIZATION>(std::clamp(GlassEngine::GetDwordFromRegistry(L"BlurOptimization", 0), 0ul, 2ul));
		Shared::g_roundRectRadius = static_cast<int>(GlassEngine::GetDwordFromRegistry(L"RoundRectRadius"));
		Shared::g_transparencyEnabled = GlassEngine::GetPersonalizeKey() ? Util::IsTransparencyEnabled(GlassEngine::GetPersonalizeKey()) : true;
		
		value = GlassEngine::GetDwordFromRegistry(L"ColorizationColorOverride", GlassEngine::GetDwordFromRegistry(L"ColorizationColor", 0xFFFFFFFF));
		Shared::g_color = Color::FromArgb(value);
		Shared::g_colorInactive = Color::FromArgb(GlassEngine::GetDwordFromRegistry(L"ColorizationColorInactive", value));

		Shared::g_opaqueBlend = static_cast<int>(GlassEngine::GetDwordFromRegistry(L"ColorizationOpaqueBlend"));
		Shared::g_opaqueBlendPriority = static_cast<Shared::OpaqueBlendPriority>(GlassEngine::GetDwordFromRegistry(L"ColorizationOpaqueBlendPriority"));
		Shared::g_opaqueBlendColor = Color::FromArgb(GlassEngine::GetDwordFromRegistry(L"ColorizationOpaqueBlendColor", 0xFFDFDFDF));
		Shared::g_opaqueBlendColorMaximized = Color::FromArgb(GlassEngine::GetDwordFromRegistry(L"ColorizationOpaqueBlendColorMaximized", 0), false);
		if (Shared::g_opaqueBlendColorMaximized.a > 0.f)
		{
			Shared::g_opaqueBlendColorMaximized.a = std::min(Shared::g_opaqueBlendColorMaximized.a, 1.f);
		}

		Shared::g_useD3DRendering = static_cast<bool>(GlassEngine::GetDwordFromRegistry(L"UseDirect3DRendering", 0));
	}

	if (type & GlassEngine::UpdateType::Backdrop || type & GlassEngine::UpdateType::Theme)
	{
		DWORD value{};

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationBlendingOpacity", 0xFFFFFFFD);
		if (value == 0xFFFFFFFD)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 100;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 100;
			}
		}
		else if (value == 0xFFFFFFFE)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 100;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 100;
			}
		}
		else if (value == 0xFFFFFFFF)
		{
			const auto themeHandle = CustomThemeAtlasLoader::GetThemeHandle();
			if (themeHandle)
			{
				value = 100;
				CustomThemeAtlasLoader::MyGetThemeInt(themeHandle, 46, 1, TMT_COLORIZATIONOPACITY, reinterpret_cast<int*>(&value));
			}
		}
		Shared::g_colorizationBlendingOpacity = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationBlendingOpacityInactive", 0xFFFFFFFD);
		if (value == 0xFFFFFFFD)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 100;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 40;
			}
		}
		else if (value == 0xFFFFFFFE)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 55;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 40;
			}
		}
		else if (value == 0xFFFFFFFF)
		{
			const auto themeHandle = CustomThemeAtlasLoader::GetThemeHandle();
			if (themeHandle)
			{
				value = 100;
				CustomThemeAtlasLoader::MyGetThemeInt(themeHandle, 46, 2, TMT_COLORIZATIONOPACITY, reinterpret_cast<int*>(&value));
			}
		}
		Shared::g_colorizationBlendingOpacityInactive = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationBlendingOpacityMaximized", 0xFFFFFFFD);
		if (value == 0xFFFFFFFD)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 100;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 100;
			}
		}
		else if (value == 0xFFFFFFFE)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 75;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 100;
			}
		}
		else if (value == 0xFFFFFFFF)
		{
			const auto themeHandle = CustomThemeAtlasLoader::GetThemeHandle();
			if (themeHandle)
			{
				value = 100;
				CustomThemeAtlasLoader::MyGetThemeInt(themeHandle, 46, 3, TMT_COLORIZATIONOPACITY, reinterpret_cast<int*>(&value));
			}
		}
		Shared::g_colorizationBlendingOpacityMaximized = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationBlendingOpacityInactiveMaximized", 0xFFFFFFFD);
		if (value == 0xFFFFFFFD)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 100;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 40;
			}
		}
		else if (value == 0xFFFFFFFE)
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				value = 75;
			}
			#pragma warning(suppress:26813)
			else if (Shared::g_type == Shared::GlassType::Aero)
			{
				value = 40;
			}
		}
		else if (value == 0xFFFFFFFF)
		{
			const auto themeHandle = CustomThemeAtlasLoader::GetThemeHandle();
			if (themeHandle)
			{
				value = 100;
				CustomThemeAtlasLoader::MyGetThemeInt(themeHandle, 46, 4, TMT_COLORIZATIONOPACITY, reinterpret_cast<int*>(&value));
			}
		}
		Shared::g_colorizationBlendingOpacityInactiveMaximized = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);
	}
}

void GlassKernel::Startup()
{
	g_old_dwOverlayTestMode = *dwmcore::CCommonRegistryData::m_dwOverlayTestMode;
	*dwmcore::CCommonRegistryData::m_dwOverlayTestMode = 0x5;
	g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address = reinterpret_cast<decltype(g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address)>(&(HookHelper::vftbl_of(uDWM::CDesktopManager::GetInstance()->GetDCompDevice())[4]));
	HookHelper::WritePointer(g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address, MyIDCompositionDesktopDevice_WaitForCommitCompletion, &g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org);

	dwmcore::g_projectionArray.ApplyToVariable("CCachedVisualImage::CCachedTarget::Update", g_CCachedVisualImage_CCachedTarget_Update_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CDrawingContext::PreSubgraph", g_CDrawingContext_PreSubgraph_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CD2DContext::DestroyDeviceResources", g_CD2DContext_DestroyDeviceResources_Org);
	if (uDWM::g_versionInfo.build < os::build_w11_21h2)
	{
		uDWM::g_projectionArray.ApplyToVariable("CDesktopManager::ReleaseDXGIAdapter", g_CXXX_ReleaseXXX_Org);
	}
	else
	{
		uDWM::g_projectionArray.ApplyToVariable("CGraphicsDeviceManager::ReleaseGraphicsDevice", g_CXXX_ReleaseXXX_Org);
	}

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]() static
		{
			HookHelper::Detours::Attach(&g_CCachedVisualImage_CCachedTarget_Update_Org, MyCCachedVisualImage_CCachedTarget_Update);
			HookHelper::Detours::Attach(&g_CDrawingContext_PreSubgraph_Org, MyCDrawingContext_PreSubgraph);
			HookHelper::Detours::Attach(&g_CD2DContext_DestroyDeviceResources_Org, MyCD2DContext_DestroyDeviceResources);
			HookHelper::Detours::Attach(&g_CXXX_ReleaseXXX_Org, MyCXXX_ReleaseXXX);
		})
	);
}

void GlassKernel::Shutdown()
{
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]() static
		{
			HookHelper::Detours::Detach(&g_CCachedVisualImage_CCachedTarget_Update_Org, MyCCachedVisualImage_CCachedTarget_Update);
			HookHelper::Detours::Detach(&g_CDrawingContext_PreSubgraph_Org, MyCDrawingContext_PreSubgraph);
			HookHelper::Detours::Detach(&g_CD2DContext_DestroyDeviceResources_Org, MyCD2DContext_DestroyDeviceResources);
			HookHelper::Detours::Detach(&g_CXXX_ReleaseXXX_Org, MyCXXX_ReleaseXXX);
		})
	);

	HookHelper::WritePointer(g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address, g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org);
	
	*dwmcore::CCommonRegistryData::m_dwOverlayTestMode = g_old_dwOverlayTestMode;
	GlassReflectionBrush::Shutdown();
}
