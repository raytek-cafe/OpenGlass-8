#pragma once
#include "D2DBuffer.hpp"
#include "dwmcoreProjection.hpp"

namespace OpenGlass
{
	class CGlassSafetyZoneLayer : public winrt::implements<CGlassSafetyZoneLayer, IUnknown, winrt::non_agile, winrt::no_weak_ref>
	{
		winrt::com_ptr<CD2DBuffer> m_safetyZoneBuffers[4]
		{
			winrt::make_self<CD2DBuffer>(),
			winrt::make_self<CD2DBuffer>(),
			winrt::make_self<CD2DBuffer>(),
			winrt::make_self<CD2DBuffer>()
		};
		D2D1_RECT_U m_safetyZoneBounds[4]{};
		winrt::com_ptr<ID2D1Bitmap1> m_renderTargetBitmap{};
	public:
		HRESULT Push(
			ID2D1DeviceContext* context,
			ID2D1Bitmap1* renderTargetBitmap,
			const D2D1_MATRIX_3X2_F& deviceTransform,
			const D2D1_RECT_F& originalPixelRectangle,
			float extendedAmount
		);
		void Pop();
		void Reset();
		ID2D1Bitmap1* GetOwner() const { return m_renderTargetBitmap.get(); }
	};
}