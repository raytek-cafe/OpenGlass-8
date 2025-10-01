#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::CustomThemeAtlasLoader
{
	HRESULT STDMETHODCALLTYPE MyGetThemeStream(
		HTHEME hTheme,
		int iPartId,
		int iStateId,
		int iPropId,
		VOID** ppvStream,
		DWORD* pcbStream,
		HINSTANCE hInst
	);
	HRESULT STDMETHODCALLTYPE MyGetThemeMargins(
		HTHEME hTheme,
		HDC hdc,
		int iPartId,
		int iStateId,
		int iPropId,
		LPCRECT prc,
		MARGINS* pMargins
	);
	HRESULT STDMETHODCALLTYPE MyGetThemeRect(
		HTHEME hTheme,
		int iPartId,
		int iStateId,
		int iPropId,
		LPRECT pRect
	);
	HRESULT STDMETHODCALLTYPE MyGetThemeInt(
		HTHEME hTheme,
		int iPartId,
		int iStateId,
		int iPropId,
		int* piVal
	);
	HRESULT STDMETHODCALLTYPE MyGetThemeColor(
		HTHEME hTheme,
		int iPartId,
		int iStateId,
		int iPropId,
		COLORREF* pColor
	);
	HTHEME GetThemeHandle();

	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
}
