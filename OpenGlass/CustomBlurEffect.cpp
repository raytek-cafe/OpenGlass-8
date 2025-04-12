#include "pch.h"
#include "CustomBlurEffect.hpp"
#include "D2DPrivates.hpp"
#include "Util.hpp"

using namespace OpenGlass;
const float CCustomBlurEffect::k_optimizations[16]
{
	8.f, 6.f, 1.5f, 2.5f, D2D1_SCALE_INTERPOLATION_MODE_NEAREST_NEIGHBOR, 
	8.f, 6.f, 1.5f, 2.5f, D2D1_SCALE_INTERPOLATION_MODE_LINEAR, 
	12.f, 6.f, 2.f, 3.f, D2D1_SCALE_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR,
	0.f
};

float CCustomBlurEffect::DetermineOutputScale(
	float size
)
{
	float outputScale{ 1.f };
	if (size > 1.0)
	{
		const auto k = m_blurAmount <= k_optimizations[5 * m_optimization + 2] ? 1.f : 0.5f;
		outputScale = k * std::max(
			0.1f,
			std::min(
				1.f,
				#pragma warning(suppress:33011)
				k_optimizations[5 * m_optimization] / (m_blurAmount + k_optimizations[5 * m_optimization + 1])
			)
		);
		if (outputScale * size < 1.f)
		{
			return 1.f / size;
		}
	}
	return outputScale;
}

// Input -> (optional) ScaleDown -> CropInput -> Border -> DirectionalBlurX -> DirectionalBlurY -> (optional) Embedded -> (optional) ScaleUp
HRESULT CCustomBlurEffect::Initialize(ID2D1DeviceContext* context)
{
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Crop,
			m_cropInputEffect.put()
		)
	);
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Scale,
			m_scaleDownEffect.put()
		)
	);
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Border,
			m_borderEffect.put()
		)
	);
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1DirectionalBlurKernel,
			m_directionalBlurXEffect.put()
		)
	);
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1DirectionalBlurKernel,
			m_directionalBlurYEffect.put()
		)
	);
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Scale,
			m_scaleUpEffect.put()
		)
	);

	RETURN_IF_FAILED(
		m_scaleDownEffect->SetValue(
			D2D1_SCALE_PROP_BORDER_MODE,
			D2D1_BORDER_MODE_SOFT
		)
	);

	RETURN_IF_FAILED(
		m_cropInputEffect->SetValue(
			D2D1_CROP_PROP_BORDER_MODE,
			D2D1_BORDER_MODE_SOFT
		)
	);
	m_cropInputEffect->SetInputEffect(0, m_scaleDownEffect.get());

	m_borderEffect->SetInputEffect(0, m_cropInputEffect.get());
	RETURN_IF_FAILED(
		m_borderEffect->SetValue(
			D2D1_BORDER_PROP_EDGE_MODE_X, 
			D2D1_BORDER_EDGE_MODE_MIRROR
		)
	);
	RETURN_IF_FAILED(
		m_borderEffect->SetValue(
			D2D1_BORDER_PROP_EDGE_MODE_Y, 
			D2D1_BORDER_EDGE_MODE_MIRROR
		)
	);

	m_directionalBlurXEffect->SetInputEffect(0, m_borderEffect.get());
	RETURN_IF_FAILED(
		m_directionalBlurXEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_DIRECTION, 
			D2D1_DIRECTIONALBLURKERNEL_DIRECTION_X
		)
	);

	m_directionalBlurYEffect->SetInputEffect(0, m_directionalBlurXEffect.get());
	RETURN_IF_FAILED(
		m_directionalBlurYEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_DIRECTION, 
			D2D1_DIRECTIONALBLURKERNEL_DIRECTION_Y
		)
	);

	RETURN_IF_FAILED(
		m_scaleUpEffect->SetValue(
			D2D1_SCALE_PROP_BORDER_MODE,
			D2D1_BORDER_MODE_SOFT
		)
	);
	m_scaleUpEffect->SetInputEffect(0, m_directionalBlurYEffect.get());

	m_initialized = true;

	return S_OK;
}

HRESULT CCustomBlurEffect::CalculateAndSetEffectParams()
{
	D2D1_VECTOR_2F prescaleAmount
	{
		DetermineOutputScale(wil::rect_width(m_imageBounds)),
		DetermineOutputScale(wil::rect_height(m_imageBounds))
	};
	D2D1_VECTOR_2F finalBlurAmount{ m_blurAmount, m_blurAmount };
	auto finalPrescaleAmount = prescaleAmount;

	#pragma warning(suppress:33011)
	if (prescaleAmount.x != 1.f && finalBlurAmount.x > k_optimizations[5 * m_optimization + 2])
	{
		if (prescaleAmount.x <= 0.5f)
		{
			finalPrescaleAmount.x *= 2.f;
		}
	}
	#pragma warning(suppress:33011)
	if (prescaleAmount.y != 1.f && finalBlurAmount.y > k_optimizations[5 * m_optimization + 2])
	{
		if (prescaleAmount.y <= 0.5f)
		{
			finalPrescaleAmount.y *= 2.f;
		}
	}

	if (prescaleAmount.x == 1.f && prescaleAmount.y == 1.f)
	{
		m_outputEffect = m_directionalBlurYEffect;
	}
	else
	{
		m_outputEffect = m_scaleUpEffect;
		RETURN_IF_FAILED(
			m_scaleUpEffect->SetValue(
				D2D1_SCALE_PROP_SCALE,
				D2D1::Point2F(
					1.f / prescaleAmount.x,
					1.f / prescaleAmount.y
				)
			)
		);
	}

	RETURN_IF_FAILED(
		m_scaleDownEffect->SetValue(
			D2D1_SCALE_PROP_INTERPOLATION_MODE,
			static_cast<UINT32>(k_optimizations[5 * m_optimization + 4])
		)
	);
	RETURN_IF_FAILED(
		m_scaleDownEffect->SetValue(
			D2D1_SCALE_PROP_SCALE,
			finalPrescaleAmount
		)
	);
	if (finalPrescaleAmount.x == 1.f && finalPrescaleAmount.y == 1.f)
	{
		m_inputEffect = m_cropInputEffect;
	}
	else
	{
		m_inputEffect = m_scaleDownEffect;
		m_cropInputEffect->SetInputEffect(0, m_scaleDownEffect.get());

		finalBlurAmount = D2D1::Vector2F(
			finalBlurAmount.x * finalPrescaleAmount.x,
			finalBlurAmount.y * finalPrescaleAmount.y
		);
	}

	RETURN_IF_FAILED(
		m_directionalBlurXEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_STANDARD_DEVIATION,
			finalBlurAmount.x
		)
	);
	RETURN_IF_FAILED(
		m_directionalBlurXEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_KERNEL_RANGE_FACTOR,
			k_optimizations[5 * m_optimization + 3]
		)
	);
	RETURN_IF_FAILED(
		m_directionalBlurXEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_OPTIMIZATION_TRANSFORM,
			(prescaleAmount.x != finalPrescaleAmount.x) ? D2D1_DIRECTIONALBLURKERNEL_OPTIMIZATION_TRANSFORM_SCALE : D2D1_DIRECTIONALBLURKERNEL_OPTIMIZATION_TRANSFORM_IDENDITY
		)
	);
	RETURN_IF_FAILED(
		m_directionalBlurYEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_STANDARD_DEVIATION,
			finalBlurAmount.y
		)
	);
	RETURN_IF_FAILED(
		m_directionalBlurYEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_KERNEL_RANGE_FACTOR,
			k_optimizations[5 * m_optimization + 3]
		)
	);
	RETURN_IF_FAILED(
		m_directionalBlurYEffect->SetValue(
			D2D1_DIRECTIONALBLURKERNEL_PROP_OPTIMIZATION_TRANSFORM,
			(prescaleAmount.y != finalPrescaleAmount.y) ? D2D1_DIRECTIONALBLURKERNEL_OPTIMIZATION_TRANSFORM_SCALE : D2D1_DIRECTIONALBLURKERNEL_OPTIMIZATION_TRANSFORM_IDENDITY
		)
	);
	RETURN_IF_FAILED(
		m_scaleUpEffect->SetValue(
			D2D1_SCALE_PROP_INTERPOLATION_MODE,
			static_cast<UINT32>(k_optimizations[5 * m_optimization + 4])
		)
	);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CCustomBlurEffect::Build(
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

	bool recalculateParams{ true };
	
	do
	{
		const auto params = static_cast<const CustomBlurParams*>(additionalParams);
		const auto blurAmount = params->blurAmount;
		const auto optimization = params->optimization;
		
		if (m_blurAmount != blurAmount)
		{
			m_blurAmount = blurAmount;
			break;
		}
		if (m_optimization != optimization)
		{
			m_optimization = optimization;
			break;
		}
		if (memcmp(&m_imageBounds, &imageBounds, sizeof(D2D1_RECT_F)) != 0)
		{
			m_imageBounds = imageBounds;
			break;
		}

		recalculateParams = false;
	} while (false);
	
	if (recalculateParams)
	{
		RETURN_IF_FAILED(
			CalculateAndSetEffectParams()
		);
	}

	m_inputEffect->SetInput(0, inputImage);

	D2D1_VECTOR_2F finalPrescaleAmount{ 1.f, 1.f };
	RETURN_IF_FAILED(
		m_scaleDownEffect->GetValue(
			D2D1_SCALE_PROP_SCALE,
			&finalPrescaleAmount
		)
	);

	D2D1_RECT_F scaledImageRect{};
	scaledImageRect.left = std::floor(imageRectangle.left * finalPrescaleAmount.x);
	scaledImageRect.top = std::floor(imageRectangle.top * finalPrescaleAmount.y);
	scaledImageRect.right = std::ceil(imageRectangle.right * finalPrescaleAmount.x);
	scaledImageRect.bottom = std::ceil(imageRectangle.bottom * finalPrescaleAmount.y);
	RETURN_IF_FAILED(
		m_cropInputEffect->SetValue(
			D2D1_CROP_PROP_RECT,
			scaledImageRect
		)
	);

	return S_OK;
}

void STDMETHODCALLTYPE CCustomBlurEffect::GetOutput(ID2D1Image** output) const
{
	m_outputEffect->GetOutput(output);
}

void STDMETHODCALLTYPE CCustomBlurEffect::Reset()
{
	if (m_scaleDownEffect)
	{ 
		m_scaleDownEffect->SetInput(0, nullptr);
	}
}