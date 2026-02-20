#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::GlassFrameHandler
{
	SIZE CalculateButtonSize(int cySize, int buttonType);
	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
}