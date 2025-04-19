#pragma once
#include "CustomBlurEffect.hpp"

namespace OpenGlass
{
	struct BlurParams : CustomBlurParams
	{
		D2D1_COLOR_F color;
		float colorBalance;
	};

	class CBlurEffect : public winrt::implements<CBlurEffect, IRenderingEffect, winrt::non_agile, winrt::no_weak_ref>
	{
		bool m_initialized{ false };

		winrt::com_ptr<IRenderingEffect> m_customBlurEffect{};

		winrt::com_ptr<ID2D1Effect> m_compositeEffect{};
		winrt::com_ptr<ID2D1Effect> m_colorEffect{};
		winrt::com_ptr<ID2D1Effect> m_outputEffect{};

		HRESULT Initialize(ID2D1DeviceContext* context);
	public:
		HRESULT STDMETHODCALLTYPE Build(
			ID2D1DeviceContext* context,
			ID2D1Image* inputImage,
			const D2D1_RECT_F& imageRectangle,
			const D2D1_RECT_F& imageBounds,
			const void* additionalParams
		) override;
		D2D1_MATRIX_3X2_F STDMETHODCALLTYPE GetOutputMatrix() const override;
		void STDMETHODCALLTYPE GetOutput(ID2D1Image** output) const override;
		void STDMETHODCALLTYPE Reset() override;
	};
}