#include "pch.h"
#include "GlassEngine.hpp"
#include "CaptionTextHandler.hpp"
#include "ButtonGlowHandler.hpp"
#include "GlassKernel.hpp"
#include "AccentOverrider.hpp"
#include "GlassFrameHandler.hpp"
#include "GlassReflectionHandler.hpp"
#include "GlassRenderer.hpp"
#include "GlassIntegrity.hpp"
#include "CustomThemeAtlasLoader.hpp"
#include "GlassService.hpp"

using namespace OpenGlass;
namespace OpenGlass::GlassEngine
{
	wil::unique_hkey g_dwmKey{ nullptr };
	wil::unique_hkey g_personalizeKey{ nullptr };
	wil::unique_hkey g_dwmLocalMachineKey{ nullptr };
}

HKEY GlassEngine::GetDwmKey()
{
	return g_dwmKey.get();
}

HKEY GlassEngine::GetPersonalizeKey()
{
	return g_personalizeKey.get();
}

HKEY GlassEngine::GetDwmLocalMachineKey()
{
	return g_dwmLocalMachineKey.get();
}

void GlassEngine::LoadRegistry(bool redrawNow)
{
	UnloadRegistry();

	GlassService::RequestBuffer content{ GlassService::RequestType::OpenUserRegistry };
	const auto hr = GlassService::SendRequest(content);
	if (SUCCEEDED(hr))
	{
		if (content.dwmKey)
		{
			g_dwmKey.reset(content.dwmKey);
		}
		if (content.personalizeKey)
		{
			g_personalizeKey.reset(content.personalizeKey);
		}
		Update(UpdateType::All, redrawNow);
	}
	wil::reg::open_unique_key_nothrow(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\DWM", g_dwmLocalMachineKey);
}
void GlassEngine::UnloadRegistry()
{
	wil::reg::open_unique_key_nothrow(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\DWM", g_dwmKey);
	wil::reg::open_unique_key_nothrow(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", g_personalizeKey);
	wil::reg::open_unique_key_nothrow(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\DWM", g_dwmLocalMachineKey);
}

void GlassEngine::RedrawAll()
{
	GlassKernel::RedrawAllTopLevelWindow();
}

void GlassEngine::Update(UpdateType type, bool redrawNow)
{
	{
		const auto lock = wil::EnterCriticalSection(uDWM::CDesktopManager::s_csDwmInstance);
		GlassKernel::Update(type);
		CustomThemeAtlasLoader::Update(type);
		CaptionTextHandler::Update(type);
		ButtonGlowHandler::Update(type);
		AccentOverrider::Update(type);
		GlassFrameHandler::Update(type);
		GlassReflectionHandler::Update(type);
		GlassRenderer::Update(type);
		GlassIntegrity::Update(type);
	}
	if (redrawNow)
	{
		GlassKernel::RedrawAllTopLevelWindow();
		//DwmFlush();
	}
}

void GlassEngine::Startup()
{
	const auto lock = wil::EnterCriticalSection(uDWM::CDesktopManager::s_csDwmInstance);
	GlassKernel::Startup();
	CustomThemeAtlasLoader::Startup();
	CaptionTextHandler::Startup();
	ButtonGlowHandler::Startup();
	AccentOverrider::Startup();
	GlassFrameHandler::Startup();
	GlassReflectionHandler::Startup();
	GlassRenderer::Startup();
	GlassIntegrity::Startup();
}
void GlassEngine::Shutdown()
{
	const auto lock = wil::EnterCriticalSection(uDWM::CDesktopManager::s_csDwmInstance);
	GlassIntegrity::Shutdown();
	GlassRenderer::Shutdown();
	GlassReflectionHandler::Shutdown();
	GlassFrameHandler::Shutdown();
	AccentOverrider::Shutdown();
	ButtonGlowHandler::Shutdown();
	CaptionTextHandler::Shutdown();
	CustomThemeAtlasLoader::Shutdown();
	GlassKernel::Shutdown();
}
