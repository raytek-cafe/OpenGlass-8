#include "pch.h"
#include "AeroEffect.hpp"
#include "AeroColorizationEffect.hpp"

using namespace OpenGlass;

HRESULT CAeroEffect::Initialize(ID2D1DeviceContext* context)
{
	m_customBlurEffect = winrt::make<CCustomBlurEffect>();

	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_AeroColorizationEffect,
			m_colorizationEffect.put()
		)
	);

	m_initialized = true;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAeroEffect::Build(
	ID2D1DeviceContext* context,
	ID2D1Image* inputImage,
	const D2D1_RECT_F& imageRectangle,
	const void* additionalParams
)
{
	if (!m_initialized)
	{
		RETURN_IF_FAILED(Initialize(context));
	}
	const auto params = static_cast<const AeroParams*>(additionalParams);

	RETURN_IF_FAILED(
		m_customBlurEffect->Build(
			context,
			inputImage,
			imageRectangle,
			additionalParams
		)
	);
	const auto fullyTransparent =
		params->afterglowBalance == 0.f &&
		params->colorBalance == 0.f &&
		params->blurBalance == 1.f;
	if (!fullyTransparent)
	{
		RETURN_IF_FAILED(
			m_colorizationEffect->SetValue(
				AEROCOLORIZATION_PROP_AFTERGLOW,
				D2D1::Vector4F(
					params->afterglow.r * params->afterglowBalance,
					params->afterglow.g * params->afterglowBalance,
					params->afterglow.b * params->afterglowBalance,
					1.f
				)
			)
		);
		RETURN_IF_FAILED(
			m_colorizationEffect->SetValue(
				AEROCOLORIZATION_PROP_BLURBALANCE,
				D2D1::Vector4F(
					params->blurBalance
				)
			)
		);
		RETURN_IF_FAILED(
			m_colorizationEffect->SetValue(
				AEROCOLORIZATION_PROP_COLOR,
				D2D1::Vector4F(
					params->color.r * params->colorBalance,
					params->color.g * params->colorBalance,
					params->color.b * params->colorBalance,
					1.f
				)
			)
		);

		winrt::com_ptr<ID2D1Image> image{};
		m_customBlurEffect->GetOutput(image.put());
		m_colorizationEffect->SetInput(0, image.get());
		m_outputEffect = m_colorizationEffect;
	}
	else
	{
		m_outputEffect = nullptr;
	}

	return S_OK;
}

D2D1_MATRIX_3X2_F STDMETHODCALLTYPE CAeroEffect::GetOutputMatrix() const
{
	return m_customBlurEffect->GetOutputMatrix();
}

void STDMETHODCALLTYPE CAeroEffect::GetOutput(ID2D1Image** output) const
{
	if (m_outputEffect)
	{
		m_outputEffect->GetOutput(output);
	}
	else
	{
		m_customBlurEffect->GetOutput(output);
	}
}

void STDMETHODCALLTYPE CAeroEffect::Reset()
{
	if (m_customBlurEffect)
	{
		m_customBlurEffect->Reset();
	}
	if (m_colorizationEffect)
	{
		m_colorizationEffect->SetInput(0, nullptr);
	}
}