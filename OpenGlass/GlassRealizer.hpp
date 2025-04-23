#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"
#include "AeroEffect.hpp"
#include "D2DBuffer.hpp"
#include "Shared.hpp"

namespace OpenGlass
{
	struct GlassParams : AeroParams
	{
		Shared::GlassType type;
	};
	struct GlassInput
	{
		GlassParams params;

		ID2D1Bitmap1* sourceBitmap;
		D2D1_RECT_F sampleWorldBounds;
		const D2D1_RECT_F* drawingWorldBounds;
		std::span<D2D1_RECT_F> rectangles;
		CD2DBuffer* buffer;
		bool zeroCopyAllowed;
		bool nearestNeighborFinalScale;
	};

	class CGlassRealizer : public winrt::implements<CGlassRealizer, IUnknown, winrt::non_agile, winrt::no_weak_ref>
	{
		Shared::GlassType m_glassType{ Shared::GlassType::Invalid };
		winrt::com_ptr<IRenderingEffect> m_glassEffect{ nullptr };

		bool EnsureGlassEffect(Shared::GlassType type);
	public:
		HRESULT Render(
			ID2D1DeviceContext* context,
			const GlassInput& input
		);
		void Reset() 
		{ 
			m_glassEffect = nullptr; 
			m_glassType = Shared::GlassType::Invalid;
		}
	};
}