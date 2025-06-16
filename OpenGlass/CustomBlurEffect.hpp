#pragma once
#include "RenderingEffect.hpp"

// this is actually a full mirror implementation of dwmcore!CCustomBlur, 
// in order to produce a high performance blur effect than raw direct2d gaussian blur effect
// no reason for touching it

// i have to apologize to all other contributors since i completely refactor this part of the code
namespace OpenGlass
{
	struct CCustomBlurParams
	{
		float blurAmount;
		float extraScaleAmount;
		D2D1_SCALE_INTERPOLATION_MODE prescaleInteroplation;
		D2D1_GAUSSIANBLUR_OPTIMIZATION optimization;
		bool cachePrescaledImage;
	};

	class CCustomBlurEffect : public winrt::implements<CCustomBlurEffect, IRenderingEffect, winrt::non_agile, winrt::no_weak_ref>
	{
		bool m_initialized{ false };

		float m_blurAmount{ 0.f };
		float m_extraScaleAmount{ 1.f };
		D2D1_GAUSSIANBLUR_OPTIMIZATION m_optimization{ D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED };
		D2D1_SCALE_INTERPOLATION_MODE m_prescaleInteroplation{ D2D1_SCALE_INTERPOLATION_MODE_FORCE_DWORD };
		D2D1_VECTOR_2F m_prescaleAmount{};
		D2D1_POINT_2F m_offset{};

		winrt::com_ptr<ID2D1Effect> m_cropInputEffect{};
		winrt::com_ptr<ID2D1Effect> m_scaleDownEffect{};
		winrt::com_ptr<ID2D1Effect> m_cropAlignEffect{};
		winrt::com_ptr<ID2D1Effect> m_borderEffect{};
		winrt::com_ptr<ID2D1Effect> m_directionalBlurXEffect{};
		winrt::com_ptr<ID2D1Effect> m_directionalBlurYEffect{};

		static const float k_optimizations[16];
		float DetermineOutputScale(
			float size
		);
		HRESULT Initialize(ID2D1DeviceContext* context);
		HRESULT CalculateAndSetEffectParams(const D2D1_RECT_F& imageRectangle);
	public:
		HRESULT STDMETHODCALLTYPE Build(
			ID2D1DeviceContext* context,
			ID2D1Image* inputImage,
			const D2D1_RECT_F& imageRectangle,
			const void* additionalParams
		) override;
		D2D1_MATRIX_3X2_F STDMETHODCALLTYPE GetOutputMatrix() const override;
		D2D1_POINT_2F STDMETHODCALLTYPE GetOutputOffset() const override { return m_offset; }
		void STDMETHODCALLTYPE GetOutput(ID2D1Image** output) const override;
		void STDMETHODCALLTYPE Reset() override;
	};
}
