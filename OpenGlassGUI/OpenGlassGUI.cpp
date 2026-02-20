#include "pch.h"
#include "OpenGlassGUI.hpp"
#include "MainFrame.hpp"

// IMPLEMENT_APP must be in global scope
IMPLEMENT_APP(OpenGlass::OpenGlassApp)

namespace OpenGlass
{
	bool OpenGlassApp::OnInit()
	{
		MSWEnableDarkMode(wxApp::DarkMode_Auto);

		m_singleInstanceChecker.Create(L"OpenGlassGUI.SingleInstance");
		if (m_singleInstanceChecker.IsAnotherRunning())
			return false;

		if (!wxApp::OnInit())
			return false;

		MainFrame* frame = new MainFrame(L"Aero Glass for Win10+");
		if (frame->IsInitializationCanceled())
		{
			frame->Destroy();
			return false;
		}
		frame->Show(true);
		return true;
	}
}
