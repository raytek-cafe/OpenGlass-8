#pragma once
#include "CustomBlurEffect.hpp"

namespace OpenGlass
{
	struct CBlurParams : CCustomBlurParams
	{
		D2D1_COLOR_F color;
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
		HRESULT Build(
			ID2D1DeviceContext* context,
			ID2D1Image* inputImage,
			const D2D1_RECT_F& imageRectangle,
			const void* additionalParams
		) override;
		D2D1_POINT_2F GetOutputOffset() const override { return m_customBlurEffect->GetOutputOffset(); }
		D2D1_MATRIX_3X2_F GetOutputMatrix() const override;
		void GetOutput(ID2D1Image** output) const override;
		void Reset() override;
	};
}
