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
	LONG width,
	const DWM::MilSizeD& scale
)
{
	D2D1_RECT_F viewport
	{
		-1.f * 
		(
			mirrored ? 
			(
				static_cast<float>(GetSystemMetrics(SM_XVIRTUALSCREEN)) +
				static_cast<float>(GetSystemMetrics(SM_CXVIRTUALSCREEN))

				-

				(
					static_cast<float>(offset.x) +
					static_cast<float>(width)
				)
				
			) : 
			static_cast<float>(
				offset.x - 
				GetSystemMetrics(SM_XVIRTUALSCREEN)
			)
		) * 
		(
			1.f - 
			parallaxIntensity
		),

		-static_cast<float>(
			offset.y - 
			GetSystemMetrics(SM_YVIRTUALSCREEN)
		)
	};
	viewport.right = viewport.left + static_cast<float>(GetSystemMetrics(SM_CXVIRTUALSCREEN));
	viewport.bottom = viewport.top + static_cast<float>(GetSystemMetrics(SM_CYVIRTUALSCREEN));

	viewport.left /= static_cast<float>(scale.width);
	viewport.top /= static_cast<float>(scale.height);
	viewport.right /= static_cast<float>(scale.width);
	viewport.bottom /= static_cast<float>(scale.height);

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
