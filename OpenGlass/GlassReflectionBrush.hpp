#pragma once
#include "resource.h"
#include "uDWMProjection.hpp"
#include "dcompPrivates.hpp"
#include "Shared.hpp"

namespace OpenGlass::GlassReflectionBrush
{
	D2D1_RECT_F CalculateTargetViewport(
		const POINT& offset,
		float parallaxIntensity = 0.f,
		bool mirrored = false,
		LONG width = 0
	);

	winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy> GetOrCreate(
		void* resource,
		bool createIfNecessary = false
	);
	void Remove(void* resource);
	void Shutdown();
}