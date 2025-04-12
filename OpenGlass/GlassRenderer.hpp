#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::GlassRenderer
{	
	bool ControlBlurRendering(bool disable);

	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
}