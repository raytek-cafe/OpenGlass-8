#pragma once
#include "framework.hpp"
#include "dwmcoreProjection.hpp"
#include "Buffer2D.hpp"

namespace OpenGlass
{
	class CGlassSafetyZoneLayer
	{
		CBuffer2D m_safetyZoneBufferVertical{};
		CBuffer2D m_safetyZoneBufferHorizontal{};
		D2D1_RECT_U m_safetyZoneBounds[4]{};
		winrt::com_ptr<ID3D11Texture2D> m_renderTargetTexture{};
	public:
		HRESULT Push(
			ID2D1DeviceContext* context,
			const D2D1_MATRIX_3X2_F& deviceTransform,
			const D2D1_RECT_F& originalPixelRectangle,
			float expansion,
			D2D1_RECT_F& extendedPixelRectangle
		);
		void Pop();
		ID3D11Texture2D* GetOwner() const { return m_renderTargetTexture.get(); }
	};
}
