#pragma once
#include "resource.h"
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"

namespace OpenGlass
{
	struct ReflectionInput
	{
		float intensity;
		const dwmcore::CMILMatrix* worldTransform;
		const D2D1_RECT_F* viewport;
	};

	class CReflectionRealizer : public winrt::implements<CReflectionRealizer, IUnknown, winrt::non_agile, winrt::no_weak_ref>
	{
		winrt::com_ptr<ID2D1Bitmap1> m_reflectionBitmap{ nullptr };

		inline HRESULT LoadTexture(ID2D1DeviceContext* context);
	public:
		HRESULT Render(
			ID2D1DeviceContext* context,
			const ReflectionInput& input
		);
		void Reset()
		{
			m_reflectionBitmap = nullptr;
		}
	};
}