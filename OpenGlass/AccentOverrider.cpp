#include "pch.h"
#include "AccentOverrider.hpp"
#include "GlassKernel.hpp"
#include "uDWMProjection.hpp"
#include "Shared.hpp"

using namespace OpenGlass;
namespace OpenGlass::AccentOverrider
{
	HRESULT STDMETHODCALLTYPE MyCAccent_UpdateAccentPolicy(
		uDWM::CAccent* This,
		LPCRECT rect,
		uDWM::ACCENT_POLICY* policy,
		uDWM::CBaseGeometryProxy* geometry
	);
	HRESULT STDMETHODCALLTYPE MyCAccent__UpdateSolidFill(
		uDWM::CAccentBlurBehind* This,
		uDWM::CRenderDataVisual* visual,
		UINT color,
		const D2D1_RECT_F& rect,
		float opacity
	);
	HRESULT STDMETHODCALLTYPE MyCAccent__UpdateAccentBlurBehind(
		uDWM::CAccent* This
	);
	bool STDMETHODCALLTYPE MyCAccentBlurBehind_IsBlurBehindDirty(
		uDWM::CAccentBlurBehind* This,
		const uDWM::CWindowData* data,
		LPCRECT rectangle,
		ULONG_PTR desktopId,
		HWND hwnd
	);
	decltype(&MyCAccent_UpdateAccentPolicy) g_CAccent_UpdateAccentPolicy_Org{ nullptr };
	decltype(&MyCAccent__UpdateSolidFill) g_CAccent__UpdateSolidFill_Org{ nullptr };
	decltype(&MyCAccent__UpdateAccentBlurBehind) g_CAccent__UpdateAccentBlurBehind_Org{ nullptr };
	decltype(&MyCAccentBlurBehind_IsBlurBehindDirty) g_CAccentBlurBehind_IsBlurBehindDirty_Org{ nullptr };
}

HRESULT STDMETHODCALLTYPE AccentOverrider::MyCAccent_UpdateAccentPolicy(
	uDWM::CAccent* This,
	LPCRECT rect,
	uDWM::ACCENT_POLICY* policy,
	uDWM::CBaseGeometryProxy* geometry
)
{
	if (!Shared::g_overrideAccent)
	{
		return g_CAccent_UpdateAccentPolicy_Org(This, rect, policy, geometry);
	}

	auto accentPolicy = *policy;
	if (
		accentPolicy.AccentState != 2 &&
		accentPolicy.AccentState != 3 &&
		accentPolicy.AccentState != 4
	)
	{
		return g_CAccent_UpdateAccentPolicy_Org(This, rect, policy, geometry);
	}

	accentPolicy.AccentState = 2;
	return g_CAccent_UpdateAccentPolicy_Org(This, rect, &accentPolicy, geometry);
}
HRESULT STDMETHODCALLTYPE AccentOverrider::MyCAccent__UpdateSolidFill(
	uDWM::CAccentBlurBehind* This,
	uDWM::CRenderDataVisual* visual,
	UINT color,
	const D2D1_RECT_F& rect,
	float opacity
)
{
	if (!Shared::g_overrideAccent)
	{
		return g_CAccent__UpdateSolidFill_Org(
			This,
			visual,
			color,
			rect,
			opacity
		);
	}

	RETURN_IF_FAILED(visual->ClearInstructions());
	winrt::com_ptr<uDWM::CSolidColorLegacyMilBrushProxy> brush{ nullptr };
	RETURN_IF_FAILED(
		uDWM::CDesktopManager::GetInstance()->GetCompositor()->CreateSolidColorLegacyMilBrushProxy(
			brush.put()
		)
	);
	RETURN_IF_FAILED(brush->Update(0.5, Color::sRGBToscRGB(GlassKernel::CalculateWindowColorization(true))));
	winrt::com_ptr<uDWM::CRgnGeometryProxy> geometry{ nullptr };
	RETURN_IF_FAILED(
		uDWM::ResourceHelper::CreateGeometryFromHRGN(
			wil::unique_hrgn
			{
				CreateRectRgn(
					static_cast<LONG>(rect.left),
					static_cast<LONG>(rect.top),
					static_cast<LONG>(rect.right),
					static_cast<LONG>(rect.bottom)
				)
			}.get(),
			geometry.put()
		)
	);
	winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
	RETURN_IF_FAILED(
		uDWM::CDrawGeometryInstruction::Create(
			brush.get(),
			geometry.get(),
			instruction.put()
		)
	);
	RETURN_IF_FAILED(visual->AddInstruction(instruction.get()));
	return S_OK;
}
bool STDMETHODCALLTYPE AccentOverrider::MyCAccentBlurBehind_IsBlurBehindDirty(
	[[maybe_unused]] uDWM::CAccentBlurBehind* This,
	[[maybe_unused]] const uDWM::CWindowData* data,
	[[maybe_unused]] LPCRECT rectangle,
	[[maybe_unused]] ULONG_PTR desktopId,
	[[maybe_unused]] HWND hwnd
)
{
	return false;
}

void AccentOverrider::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Backdrop)
	{
		Shared::g_overrideAccent = static_cast<bool>(GlassEngine::GetDwordFromRegistry(L"GlassOverrideAccent"));
	}
}

void AccentOverrider::Startup()
{
	uDWM::g_projectionArray.ApplyToVariable("CAccent::UpdateAccentPolicy", g_CAccent_UpdateAccentPolicy_Org);
	uDWM::g_projectionArray.ApplyToVariable("CAccent::_UpdateSolidFill", g_CAccent__UpdateSolidFill_Org);
	uDWM::g_projectionArray.ApplyToVariable("CAccentBlurBehind::IsBlurBehindDirty", g_CAccentBlurBehind_IsBlurBehindDirty_Org);

	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Attach(&g_CAccent_UpdateAccentPolicy_Org, MyCAccent_UpdateAccentPolicy);
			HookHelper::Detours::Attach(&g_CAccent__UpdateSolidFill_Org, MyCAccent__UpdateSolidFill);
			if (uDWM::g_buildNumber < os::build_w11_22h2)
			{
				HookHelper::Detours::Attach(&g_CAccentBlurBehind_IsBlurBehindDirty_Org, MyCAccentBlurBehind_IsBlurBehindDirty);
			}
		})
	);
}

void AccentOverrider::Shutdown()
{
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_CAccent_UpdateAccentPolicy_Org, MyCAccent_UpdateAccentPolicy);
			HookHelper::Detours::Detach(&g_CAccent__UpdateSolidFill_Org, MyCAccent__UpdateSolidFill);
			if (uDWM::g_buildNumber < os::build_w11_22h2)
			{
				HookHelper::Detours::Detach(&g_CAccentBlurBehind_IsBlurBehindDirty_Org, MyCAccentBlurBehind_IsBlurBehindDirty);
			}
		})
	);
}