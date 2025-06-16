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
		D2D1_INTERPOLATION_MODE drawImageInterpolationMode;

		const D2D1_RECT_F* drawingWorldBounds;
		std::span<D2D1_RECT_F> rectangles;

		ID2D1Bitmap1* sourceBitmap;
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
