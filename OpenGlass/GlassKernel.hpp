#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::GlassKernel
{
	void RedrawAllTopLevelWindow();
	float GetBlurExtendedAmount();
	D2D1_COLOR_F CalculateWindowColorization(bool active);
	void CalculateRealizedAeroParams(
		bool active,
		float extendedAmount,
		float& glassOpacity,
		float& blurBalance,
		float& afterglowBalance,
		float* colorBalance
	);
	bool IsInCVIHierarchy();

	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
}