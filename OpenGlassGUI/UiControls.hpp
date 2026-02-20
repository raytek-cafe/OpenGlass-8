#pragma once
#include "pch.h"

namespace OpenGlass
{
	// Helper class - now uses wxWidgets tooltips for better control
	class NativeSlider : public wxSlider
	{
	public:
		NativeSlider(wxWindow* parent,
			wxWindowID id,
			int value,
			int minValue,
			int maxValue,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = wxSL_HORIZONTAL,
			const wxValidator& validator = wxDefaultValidator,
			const wxString& name = wxSliderNameStr)
		{
			Create(parent, id, value, minValue, maxValue, pos, size, style, validator, name);
		}

		bool Create(wxWindow* parent,
			wxWindowID id,
			int value,
			int minValue,
			int maxValue,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = wxSL_HORIZONTAL,
			const wxValidator& validator = wxDefaultValidator,
			const wxString& name = wxSliderNameStr)
		{
			return wxSlider::Create(parent, id, value, minValue, maxValue, pos, size, style, validator, name);
		}
	};
}
