#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"
#include "AeroEffect.hpp"
#include "D2DBuffer.hpp"
#include "Shared.hpp"

namespace OpenGlass
{
	struct CGlassInput
	{
		CAeroParams params;

		const D2D1_RECT_F* drawingWorldBounds;
		const D2D1_RECT_F* samplingWorldBoundsShapeClipped;
		std::span<D2D1_RECT_F> rectangles;

		CD2DBuffer* buffer;
		bool zeroCopyOptimization;
	};

	class CGlassRealizer
	{
		Shared::GlassType m_glassType{ Shared::GlassType::Invalid };
		winrt::com_ptr<IRenderingEffect> m_glassEffect{ nullptr };

		bool EnsureGlassEffect(Shared::GlassType type);
	public:
		HRESULT Render(
			ID2D1DeviceContext* context,
			const CGlassInput& input
		);
	};
}
