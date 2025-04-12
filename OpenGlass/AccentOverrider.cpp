#include "pch.h"
#include "AccentOverrider.hpp"
#include "GlassKernel.hpp"
#include "uDWMProjection.hpp"
#include "Shared.hpp"

using namespace OpenGlass;
namespace OpenGlass::AccentOverrider
{
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
	decltype(&MyCAccent__UpdateAccentBlurBehind) g_CAccent__UpdateAccentBlurBehind_Org{ nullptr };
	decltype(&MyCAccentBlurBehind_IsBlurBehindDirty) g_CAccentBlurBehind_IsBlurBehindDirty_Org{ nullptr };
}

HRESULT STDMETHODCALLTYPE AccentOverrider::MyCAccent__UpdateAccentBlurBehind(
	uDWM::CAccent* This
)
{
	if (!Shared::g_overrideAccent)
	{
		return g_CAccent__UpdateAccentBlurBehind_Org(
			This
		);
	}

	if (This->IsVisibleAndUncloaked())
	{
		THROW_IF_FAILED(This->ClearInstructions());
		THROW_IF_FAILED(This->GetVisualCollection()->RemoveAll());
		winrt::com_ptr<uDWM::CSolidColorLegacyMilBrushProxy> brush{ nullptr };
		THROW_IF_FAILED(
			uDWM::CDesktopManager::GetInstance()->GetCompositor()->CreateSolidColorLegacyMilBrushProxy(
				brush.put()
			)
		);
		THROW_IF_FAILED(brush->Update(0.5, Color::sRGBToscRGB(GlassKernel::CalculateWindowColorization(true))));
		winrt::com_ptr<uDWM::CRgnGeometryProxy> geometry{ nullptr };
		THROW_IF_FAILED(
			uDWM::ResourceHelper::CreateGeometryFromHRGN(
				wil::unique_hrgn
				{
					CreateRectRgn(
						0,
						0,
						This->GetWidth(),
						This->GetHeight()
					)
				}.get(),
				geometry.put()
			)
		);
		winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
		THROW_IF_FAILED(
			uDWM::CDrawGeometryInstruction::Create(
				brush.get(),
				geometry.get(),
				instruction.put()
			)
		);
		THROW_IF_FAILED(This->AddInstruction(instruction.get()));
	}
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
	if (uDWM::g_buildNumber < os::build_w11_22h2) 
	{
		uDWM::g_projectionArray.ApplyToVariable("CAccent::_UpdateAccentBlurBehind", g_CAccent__UpdateAccentBlurBehind_Org);
		uDWM::g_projectionArray.ApplyToVariable("CAccentBlurBehind::IsBlurBehindDirty", g_CAccentBlurBehind_IsBlurBehindDirty_Org);

		THROW_IF_FAILED(
			HookHelper::Detours::Write([]()
			{
				HookHelper::Detours::Attach(&g_CAccent__UpdateAccentBlurBehind_Org, MyCAccent__UpdateAccentBlurBehind);
				HookHelper::Detours::Attach(&g_CAccentBlurBehind_IsBlurBehindDirty_Org, MyCAccentBlurBehind_IsBlurBehindDirty);
			})
		);
	}
}

void AccentOverrider::Shutdown()
{
	if (uDWM::g_buildNumber < os::build_w11_22h2) 
	{
		THROW_IF_FAILED(
			HookHelper::Detours::Write([]()
			{
				HookHelper::Detours::Detach(&g_CAccent__UpdateAccentBlurBehind_Org, MyCAccent__UpdateAccentBlurBehind);
				HookHelper::Detours::Detach(&g_CAccentBlurBehind_IsBlurBehindDirty_Org, MyCAccentBlurBehind_IsBlurBehindDirty);	
			})
		);
	}
}