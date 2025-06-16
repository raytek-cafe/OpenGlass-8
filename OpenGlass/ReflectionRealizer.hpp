#pragma once
#include "resource.h"
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"

namespace OpenGlass
{
	struct CReflectionInput
	{
		float intensity;
		const D2D1_RECT_F* viewport;
		const dwmcore::CMILMatrix* worldTransform;

		std::span<D2D1_RECT_F> rectangles;
	};

	class CReflectionRealizer
	{
		winrt::com_ptr<ID2D1Bitmap1> m_reflectionBitmap{ nullptr };
		winrt::com_ptr<ID2D1SpriteBatch> m_spriteBatch{ nullptr };

		inline HRESULT LoadTexture(ID2D1DeviceContext* context);
	public:
		HRESULT Render(
			ID2D1DeviceContext* context,
			const CReflectionInput& input
		);
		void Reset()
		{
			m_reflectionBitmap = nullptr;
		}
	};
}
