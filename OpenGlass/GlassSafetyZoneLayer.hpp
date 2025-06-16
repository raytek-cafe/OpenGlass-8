#pragma once
#include "D2DBuffer.hpp"
#include "dwmcoreProjection.hpp"

namespace OpenGlass
{
	class CGlassSafetyZoneLayer
	{
		CD2DBuffer m_safetyZoneBufferVertical{};
		CD2DBuffer m_safetyZoneBufferHorizon{};
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
		ID2D1Bitmap1* GetOwner() const { return m_renderTargetBitmap.get(); }
	};
}
