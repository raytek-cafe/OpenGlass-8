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

	class CReflectionRealizer : public winrt::implements<CReflectionRealizer, IUnknown, winrt::non_agile, winrt::no_weak_ref>
	{
		winrt::com_ptr<ID2D1Bitmap1> m_reflectionBitmap{ nullptr };

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
