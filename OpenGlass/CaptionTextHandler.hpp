#pragma once
#include "framework.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::CaptionTextHandler
{
	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
}