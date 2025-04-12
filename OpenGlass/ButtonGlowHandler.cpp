#include "pch.h"
#include "ButtonGlowHandler.hpp"

using namespace OpenGlass;
namespace OpenGlass::ButtonGlowHandler
{
	constexpr int MARGIN_OFFSET = 0x30; //offset is the same as w7, so it shouldnt change

	/*
	* https://imgur.com/a/kHFLBON
	* To find these offsets, look in CTopLevelWindow::UpdateButtonVisuals in uDWM.dll
	* where offset can be found inside, used to get the image to pass into CButton::SetVisualStates
	* there will be many calls, but they all get at the same 2 offset, which correspond to these
	*/
	constexpr int MINMAXBUTTONGLOWIMAGE = 0xD0; 
	constexpr int CLOSEBUTTONGLOWIMAGE = 0xC8; 

	static int MINMAXBUTTONGLOW = 93; //16 in windows 7
	static int CLOSEBUTTONGLOW = 92; //11 in windows 7
	static int TOOLCLOSEBUTTONGLOW = 94; //47 in windows 7

	HRESULT(WINAPI* CTopLevelWindow__CreateBitmapFromAtlas)(HTHEME hTheme, int iPartId, MARGINS* outMargins, void** outBitmapSource);
	
	//not 1 to 1 to the one in windows 7 udwm, however it achieves the same outcome whilst being simpler
	HRESULT CTopLevelWindow__CreateButtonGlowsFromAtlas(HTHEME hTheme)
	{
		MARGINS margins{};
		void* OutBitmapSourceBlue = nullptr;
		void* OutBitmapSourceRed = nullptr;
		void* OutBitmapSourceTool = nullptr;

		RETURN_IF_FAILED(CTopLevelWindow__CreateBitmapFromAtlas(hTheme, MINMAXBUTTONGLOW, &margins, &OutBitmapSourceBlue));
		*(MARGINS*)(__int64(OutBitmapSourceBlue) + MARGIN_OFFSET) = margins;

		RETURN_IF_FAILED(CTopLevelWindow__CreateBitmapFromAtlas(hTheme, CLOSEBUTTONGLOW, &margins, &OutBitmapSourceRed));
		*(MARGINS*)(__int64(OutBitmapSourceRed) + MARGIN_OFFSET) = margins;

		RETURN_IF_FAILED(CTopLevelWindow__CreateBitmapFromAtlas(hTheme, TOOLCLOSEBUTTONGLOW, &margins, &OutBitmapSourceTool));
		*(MARGINS*)(__int64(OutBitmapSourceTool) + MARGIN_OFFSET) = margins;

		for (int i = 0; i < 4; ++i)
		{
			auto frame = uDWM::CTopLevelWindow::GetWindowFrames()[i];
			*(void**)(__int64(frame) + MINMAXBUTTONGLOWIMAGE) = OutBitmapSourceBlue;
			*(void**)(__int64(frame) + CLOSEBUTTONGLOWIMAGE) = OutBitmapSourceRed;
		}
		for (int i = 4; i < 6; ++i)
		{
			auto frame = uDWM::CTopLevelWindow::GetWindowFrames()[i];
			*(void**)(__int64(frame) + MINMAXBUTTONGLOWIMAGE) = OutBitmapSourceTool;
			*(void**)(__int64(frame) + CLOSEBUTTONGLOWIMAGE) = OutBitmapSourceTool;
		}
		return S_OK;
	}

	HRESULT (WINAPI* CTopLevelWindow_CreateGlyphsFromAtlas)(HTHEME hTheme);
	HRESULT WINAPI CTopLevelWindow_CreateGlyphsFromAtlas_Hook(HTHEME hTheme)
	{
		CTopLevelWindow__CreateButtonGlowsFromAtlas(hTheme);
		return CTopLevelWindow_CreateGlyphsFromAtlas(hTheme);
	}

	void Update([[maybe_unused]] GlassEngine::UpdateType type)
	{
		MINMAXBUTTONGLOW = GlassEngine::GetDwordFromRegistry(L"MINMAXBUTTONGLOWid", 93);
		CLOSEBUTTONGLOW = GlassEngine::GetDwordFromRegistry(L"CLOSEBUTTONGLOWid", 92);
		TOOLCLOSEBUTTONGLOW = GlassEngine::GetDwordFromRegistry(L"TOOLCLOSEBUTTONGLOWid", 94);
	}

	void Startup()
	{
		if (uDWM::g_buildNumber < os::build_w11_21h2)
		{
			uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::CreateBitmapFromAtlas", CTopLevelWindow__CreateBitmapFromAtlas);
			uDWM::g_projectionArray.ApplyToVariable("CTopLevelWindow::CreateGlyphsFromAtlas", CTopLevelWindow_CreateGlyphsFromAtlas);

			THROW_IF_FAILED(
				HookHelper::Detours::Write([]()
				{
					HookHelper::Detours::Attach(&CTopLevelWindow_CreateGlyphsFromAtlas, CTopLevelWindow_CreateGlyphsFromAtlas_Hook);
				})
			);
		}
	}

	void Shutdown()
	{
		if (uDWM::g_buildNumber < os::build_w11_21h2)
		{
			THROW_IF_FAILED(
				HookHelper::Detours::Write([]()
				{
					HookHelper::Detours::Detach(&CTopLevelWindow_CreateGlyphsFromAtlas, CTopLevelWindow_CreateGlyphsFromAtlas_Hook);
				})
			);
		}
	}
}
