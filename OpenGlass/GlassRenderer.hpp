#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::GlassRenderer
{
	void DestroyDeviceResources();

	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
}
