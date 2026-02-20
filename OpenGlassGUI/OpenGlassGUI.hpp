#pragma once
#include "pch.h"

namespace OpenGlass
{
	class OpenGlassApp : public wxApp
	{
	public:
		virtual bool OnInit();
	private:
		wxSingleInstanceChecker m_singleInstanceChecker;
	};
}

DECLARE_APP(OpenGlass::OpenGlassApp)
