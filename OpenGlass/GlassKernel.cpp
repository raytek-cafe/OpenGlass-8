#include "pch.h"
#include "GlassKernel.hpp"
#include "uDWMProjection.hpp"
#include "Shared.hpp"
#include "dwmcoreProjection.hpp"
#include "GlassReflectionBrush.hpp"
#include "GlassRenderer.hpp"
#include "GlassIntegrity.hpp"
#include "CaptionTextHandler.hpp"

using namespace OpenGlass;
namespace OpenGlass::GlassKernel
{
	HRESULT STDMETHODCALLTYPE MyIDCompositionDesktopDevice_WaitForCommitCompletion(IDCompositionDesktopDevice* This);
	HRESULT STDMETHODCALLTYPE MyCCachedVisualImage_CCachedTarget_Update(
		dwmcore::CCachedVisualImage::CCachedTarget* This,
		const D2D1_RECT_F& rect,
		DWM::MilStretch mode,
		const dwmcore::RenderTargetInfo& info
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
	ULONGLONG g_old_BackdropBlurCachingThrottleQPCTimeDelta{};
	size_t g_CVIHierarchy{};
	HWND g_hwnd{};

	void RedrawTopLevelWindow(uDWM::CTopLevelWindow* window)
	{
		if (window)
		{
			if (uDWM::g_buildNumber >= os::build_w11_22h2)
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
			// update blur behind
			window->OnBlurBehindUpdated();
			if (const auto accentVisual = window->GetAccent(); accentVisual)
			{
				accentVisual->SetDirtyFlags(0x10000);
			}
			window->OnAccentPolicyUpdated();
			// update system backdrop
			if (uDWM::g_buildNumber >= os::build_w11_21h2)
			{
				window->SetDirtyFlags(0x4000u);
			}
		}
	}

	D2D1_COLOR_F CalculateWindowColorization(bool active)
	{
		auto color = active ? Shared::g_color : Shared::g_colorInactive;
		if (Shared::IsTransparencyDisabled())
		{
			if (Shared::g_type == Shared::GlassType::Blur)
			{
				const auto glassOpacity = active ? Shared::g_glassOpacity : Shared::g_glassOpacityInactive;
				color.r = (1.f - glassOpacity) * Shared::g_opaqueBlendColor.r + color.r * glassOpacity;
				color.g = (1.f - glassOpacity) * Shared::g_opaqueBlendColor.g + color.g * glassOpacity;
				color.b = (1.f - glassOpacity) * Shared::g_opaqueBlendColor.b + color.b * glassOpacity;
			}
			#pragma warning(suppress:26813)
			if (Shared::g_type == Shared::GlassType::Aero)
			{
				const auto magicFactor = active ? 1.f : 0.4f;
				color.r = (1.f - Shared::g_colorBalance * magicFactor) * Shared::g_opaqueBlendColor.r + color.r * Shared::g_colorBalance * magicFactor;
				color.g = (1.f - Shared::g_colorBalance * magicFactor) * Shared::g_opaqueBlendColor.g + color.g * Shared::g_colorBalance * magicFactor;
				color.b = (1.f - Shared::g_colorBalance * magicFactor) * Shared::g_opaqueBlendColor.b + color.b * Shared::g_colorBalance * magicFactor;
			}
		}

		return color;
	}

	void CalculateRealizedAeroParams(
		bool active,
		float extendedAmount,
		float& glassOpacity,
		float& blurBalance,
		float& afterglowBalance,
		float* colorBalance
	)
	{
		const auto magicFactor = active ? 1.f : 0.4f;
		const auto translucent = !Shared::IsTransparencyDisabled();
		glassOpacity = active ? Shared::g_glassOpacity : Shared::g_glassOpacityInactive;
		blurBalance = !translucent ? 0.f : (extendedAmount ? (1.f - ((1.f - Shared::g_blurBalance) * magicFactor)) : Shared::g_blurBalance);
		afterglowBalance = !translucent ? 0.f : Shared::g_afterglowBalance;
		if (colorBalance)
		{
			*colorBalance = (1.f - Shared::g_colorBalance * magicFactor) * (!translucent ? 1.f : 0.f) + Shared::g_colorBalance * magicFactor;
		}
	}

	bool IsCVIPresent()
	{
		return g_CVIHierarchy;
	}
	bool IsCurrentCVIFullyTransparent()
	{
		return IsCVIPresent() && !g_hwnd;
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
	const dwmcore::RenderTargetInfo& info
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
	GlassRenderer::DestroyDeviceResources();
	GlassIntegrity::DestroyDeviceResources();
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
	if (Shared::g_blurAmount == 0.f)
	{
		return 0.f;
	}
	return !Shared::IsGlassFullyOpaque() ? Shared::g_blurAmount * 3.f + 0.5f : 0.f;
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
		Shared::g_disableOnBattery = static_cast<bool>(GlassEngine::GetDwordFromRegistry(L"DisableGlassOnBattery", TRUE));
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
		Shared::g_transparencyEnabled = Util::IsTransparencyEnabled(GlassEngine::GetPersonalizeKey());
		
		value = GlassEngine::GetDwordFromRegistry(L"ColorizationColorOverride", GlassEngine::GetDwordFromRegistry(L"ColorizationColor"));
		Shared::g_color = Color::FromArgb(value);
		Shared::g_colorInactive = Color::FromArgb(GlassEngine::GetDwordFromRegistry(L"ColorizationColorInactive", value));

		Shared::g_opaqueBlend = static_cast<int>(GlassEngine::GetDwordFromRegistry(L"ColorizationOpaqueBlend"));
		Shared::g_opaqueBlendColor = Color::FromArgb(GlassEngine::GetDwordFromRegistry(L"ColorizationOpaqueBlendColor", 0xFFDFDFDF));
	}
}

void GlassKernel::Startup()
{
	g_old_dwOverlayTestMode = *dwmcore::CCommonRegistryData::m_dwOverlayTestMode;
	g_old_BackdropBlurCachingThrottleQPCTimeDelta = *dwmcore::CCommonRegistryData::m_backdropBlurCachingThrottleQPCTimeDelta;
	*dwmcore::CCommonRegistryData::m_dwOverlayTestMode = 0x5;
	*dwmcore::CCommonRegistryData::m_backdropBlurCachingThrottleQPCTimeDelta = 0;

	g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address = reinterpret_cast<decltype(g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address)>(&(HookHelper::vftbl_of(uDWM::CDesktopManager::GetInstance()->GetDCompDevice())[4]));
	HookHelper::WritePointer(g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address, MyIDCompositionDesktopDevice_WaitForCommitCompletion, &g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org);

	dwmcore::g_projectionArray.ApplyToVariable("CCachedVisualImage::CCachedTarget::Update", g_CCachedVisualImage_CCachedTarget_Update_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CDrawingContext::PreSubgraph", g_CDrawingContext_PreSubgraph_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CD2DContext::DestroyDeviceResources", g_CD2DContext_DestroyDeviceResources_Org);
	if (uDWM::g_buildNumber < os::build_w11_21h2)
	{
		uDWM::g_projectionArray.ApplyToVariable("CDesktopManager::ReleaseDXGIAdapter", g_CXXX_ReleaseXXX_Org);
	}
	else
	{
		uDWM::g_projectionArray.ApplyToVariable("CGraphicsDeviceManager::ReleaseGraphicsDevice", g_CXXX_ReleaseXXX_Org);
	}

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
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
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_CCachedVisualImage_CCachedTarget_Update_Org, MyCCachedVisualImage_CCachedTarget_Update);
			HookHelper::Detours::Detach(&g_CDrawingContext_PreSubgraph_Org, MyCDrawingContext_PreSubgraph);
			HookHelper::Detours::Detach(&g_CD2DContext_DestroyDeviceResources_Org, MyCD2DContext_DestroyDeviceResources);
			HookHelper::Detours::Detach(&g_CXXX_ReleaseXXX_Org, MyCXXX_ReleaseXXX);
		})
	);

	GlassReflectionBrush::Shutdown();
	Sleep(20);

	HookHelper::WritePointer(g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org_Address, g_IDCompositionDesktopDevice_WaitForCommitCompletion_Org);
	
	*dwmcore::CCommonRegistryData::m_backdropBlurCachingThrottleQPCTimeDelta = g_old_BackdropBlurCachingThrottleQPCTimeDelta;
	*dwmcore::CCommonRegistryData::m_dwOverlayTestMode = g_old_dwOverlayTestMode;
}
