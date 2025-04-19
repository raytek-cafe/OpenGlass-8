#include "pch.h"
#include "BlurEffect.hpp"

using namespace OpenGlass;

HRESULT CBlurEffect::Initialize(ID2D1DeviceContext* context)
{
	m_customBlurEffect = winrt::make<CCustomBlurEffect>();

	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Flood,
			m_colorEffect.put()
		)
	);
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Composite,
			m_compositeEffect.put()
		)
	);

	RETURN_IF_FAILED(
		m_compositeEffect->SetValue(
			D2D1_COMPOSITE_PROP_MODE,
			D2D1_COMPOSITE_MODE_SOURCE_OVER
		)
	);
	m_compositeEffect->SetInputEffect(1, m_colorEffect.get());

	m_initialized = true;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CBlurEffect::Build(
	ID2D1DeviceContext* context,
	ID2D1Image* inputImage,
	const D2D1_RECT_F& imageRectangle,
	const D2D1_RECT_F& imageBounds,
	const void* additionalParams
)
{
	if (!m_initialized)
	{
		RETURN_IF_FAILED(Initialize(context));
	}
	const auto params = static_cast<const BlurParams*>(additionalParams);

	RETURN_IF_FAILED(
		m_customBlurEffect->Build(
			context,
			inputImage,
			imageRectangle,
			imageBounds,
			additionalParams
		)
	);
	const auto fullyTransparent = params->colorBalance == 0.f;
	if (!fullyTransparent)
	{
		RETURN_IF_FAILED(
			m_colorEffect->SetValue(
				D2D1_FLOOD_PROP_COLOR,
				D2D1::Vector4F(
					params->color.r * params->colorBalance,
					params->color.g * params->colorBalance,
					params->color.b * params->colorBalance,
					params->colorBalance
				)
			)
		);

		winrt::com_ptr<ID2D1Image> image{};
		m_customBlurEffect->GetOutput(image.put());
		m_compositeEffect->SetInput(0, image.get());
		m_outputEffect = m_compositeEffect;
	}
	else
	{
		m_outputEffect = nullptr;
	}

	return S_OK;
}

D2D1_MATRIX_3X2_F STDMETHODCALLTYPE CBlurEffect::GetOutputMatrix() const
{
	return m_customBlurEffect->GetOutputMatrix();
}

void STDMETHODCALLTYPE CBlurEffect::GetOutput(ID2D1Image** output) const
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

void STDMETHODCALLTYPE CBlurEffect::Reset()
{
	if (m_customBlurEffect)
	{
		m_customBlurEffect->Reset();
	}
	if (m_compositeEffect)
	{
		m_compositeEffect->SetInput(0, nullptr);
	}
}