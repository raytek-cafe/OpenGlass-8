#include "pch.h"
#include "GlassReflectionBrush.hpp"

using namespace OpenGlass;
namespace OpenGlass::GlassReflectionBrush
{
	std::unordered_map<void*, winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy>> g_glassReflectionBrushMap{};
}

D2D1_RECT_F GlassReflectionBrush::CalculateTargetViewport(
	const POINT& offset,
	float parallaxIntensity,
	bool mirrored,
	LONG width
)
{
	D2D1_RECT_F viewport
	{
		(mirrored ? 1.f : -1.f) * static_cast<float>(mirrored ? (offset.x + width - (GetSystemMetrics(SM_CXVIRTUALSCREEN))) : offset.x) * (1.f - parallaxIntensity),
		-static_cast<float>(offset.y),
		(mirrored ? 1.f : -1.f) * static_cast<float>(mirrored ? (offset.x + width - (GetSystemMetrics(SM_CXVIRTUALSCREEN))) : offset.x) * (1.f - parallaxIntensity) + static_cast<float>(GetSystemMetrics(SM_CXVIRTUALSCREEN)),
		-static_cast<float>(offset.y) + static_cast<float>(GetSystemMetrics(SM_CYVIRTUALSCREEN))
	};

	return viewport;
}

winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy> GlassReflectionBrush::GetOrCreate(void* resource, bool createIfNecessary)
{
	auto it = g_glassReflectionBrushMap.find(resource);

	if (createIfNecessary)
	{
		if (it == g_glassReflectionBrushMap.end())
		{
			winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy> brush{ nullptr };
			THROW_IF_FAILED(
				uDWM::CDesktopManager::GetInstance()->GetCompositor()->CreateImageLegacyMilBrushProxy(
					brush.put()
				)
			);
			auto result = g_glassReflectionBrushMap.emplace(resource, brush);
			if (result.second == true)
			{
				it = result.first;
			}
		}
	}

	return it == g_glassReflectionBrushMap.end() ? nullptr : it->second;
}
void GlassReflectionBrush::Remove(void* resource)
{
	auto it = g_glassReflectionBrushMap.find(resource);

	if (it != g_glassReflectionBrushMap.end())
	{
		g_glassReflectionBrushMap.erase(it);
	}
}
void GlassReflectionBrush::Shutdown()
{
	g_glassReflectionBrushMap.clear();
}