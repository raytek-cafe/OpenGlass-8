#include "pch.h"
#include "Util.hpp"
#include "uDWMProjection.hpp"
#include "GlassRealizer.hpp"
#include "AeroEffect.hpp"
#include "BlurEffect.hpp"

using namespace OpenGlass;

bool CGlassRealizer::EnsureGlassEffect(Shared::GlassType type)
{
	if (m_glassType != type)
	{
		m_glassType = type;
		switch (m_glassType)
		{
		case Shared::GlassType::Blur:
		{
			m_glassEffect = winrt::make<CBlurEffect>();
			break;
		}
		case Shared::GlassType::Aero:
		{
			m_glassEffect = winrt::make<CAeroEffect>();
			break;
		}
		default:
			m_glassEffect = nullptr;
			break;
		}
	}
	if (!m_glassEffect)
	{
		return false;
	}

	return true;
}

HRESULT CGlassRealizer::Render(
	ID2D1DeviceContext* context,
	const GlassInput& input
)
{
	if (m_glassType != input.params.type)
	{
		m_glassType = input.params.type;
		switch (m_glassType)
		{
		case Shared::GlassType::Blur:
		{
			m_glassEffect = winrt::make<CBlurEffect>();
			break;
		}
		case Shared::GlassType::Aero:
		{
			m_glassEffect = winrt::make<CAeroEffect>();
			break;
		}
		default:
			m_glassEffect = nullptr;
			break;
		}
	}
	if (!m_glassEffect)
	{
		return S_OK;
	}

	const auto targetSize = input.renderTargetBitmap->GetSize();
	const D2D1_RECT_F drawingWorldBounds
	{
		std::max(0.f, input.drawingWorldBounds->left),
		std::max(0.f, input.drawingWorldBounds->top),
		std::min(targetSize.width, input.drawingWorldBounds->right),
		std::min(targetSize.height, input.drawingWorldBounds->bottom),
	};
	const D2D1_RECT_F imageBounds
	{
		0.f,
		0.f,
		targetSize.width,
		targetSize.height
	};

	input.buffer->Reserve(input.renderTargetBitmap->GetPixelSize());
	RETURN_IF_FAILED(
		input.buffer->CopyFrom(
			context,
			D2D1::Point2U(
				static_cast<UINT32>(drawingWorldBounds.left),
				static_cast<UINT32>(drawingWorldBounds.top)
			),
			input.renderTargetBitmap,
			D2D1::RectU(
				static_cast<UINT32>(drawingWorldBounds.left),
				static_cast<UINT32>(drawingWorldBounds.top),
				static_cast<UINT32>(drawingWorldBounds.right),
				static_cast<UINT32>(drawingWorldBounds.bottom)
			)
		)
	);

	const auto buffer = input.buffer->GetBitmapForEffectInputNoRef(context, input.renderTargetBitmap->GetPixelFormat());
	const auto cleanupCustomEffect = wil::scope_exit([this]
	{
		m_glassEffect->Reset();
	});
	RETURN_IF_FAILED(
		m_glassEffect->Build(
			context,
			buffer,
			drawingWorldBounds,
			imageBounds,
			static_cast<const void*>(&input.params)
		)
	);

	winrt::com_ptr<ID2D1Image> outputImage{};
	m_glassEffect->GetOutput(outputImage.put());
	RETURN_HR_IF_NULL(D2DERR_INVALID_CALL, outputImage);

	D2D1_MATRIX_3X2_F originalMatrix, outputMatrix = m_glassEffect->GetOutputMatrix();
	context->GetTransform(&originalMatrix);
	context->SetTransform(m_glassEffect->GetOutputMatrix());

	D2D1InvertMatrix(&outputMatrix);
	const auto transformedImageRect = RectF::TransformRect(drawingWorldBounds, outputMatrix);
	for (const auto rectangle : input.rectangles)
	{
		auto transformedSubRectangle = RectF::TransformRect(rectangle, outputMatrix);
		RectF::IntersectUnsafe(transformedSubRectangle, transformedImageRect);

		context->DrawImage(
			outputImage.get(),
			D2D1::Point2F(
				transformedSubRectangle.left,
				transformedSubRectangle.top
			),
			transformedSubRectangle,
			(
				input.params.optimization == D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED ?
				D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR :
				(
					input.params.optimization == D2D1_GAUSSIANBLUR_OPTIMIZATION_QUALITY ?
					D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR :
					D2D1_INTERPOLATION_MODE_LINEAR
				)
			),
			D2D1_COMPOSITE_MODE_BOUNDED_SOURCE_COPY
		);
	}

	context->SetTransform(originalMatrix);

	return S_OK;
}