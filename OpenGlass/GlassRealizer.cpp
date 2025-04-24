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

	const auto targetSize = input.sourceBitmap->GetSize();
	const D2D1_RECT_F imageBounds
	{
		0.f,
		0.f,
		targetSize.width,
		targetSize.height
	};

	if (!input.zeroCopyAllowed)
	{
		input.buffer->Resize(input.sourceBitmap->GetPixelSize());
		RETURN_IF_FAILED(
			input.buffer->CopyFrom(
				context,
				D2D1::Point2U(
					static_cast<UINT32>(input.drawingWorldBounds->left),
					static_cast<UINT32>(input.drawingWorldBounds->top)
				),
				input.sourceBitmap,
				D2D1::RectU(
					static_cast<UINT32>(input.drawingWorldBounds->left),
					static_cast<UINT32>(input.drawingWorldBounds->top),
					static_cast<UINT32>(input.drawingWorldBounds->right),
					static_cast<UINT32>(input.drawingWorldBounds->bottom)
				)
			)
		);
	}

	const auto cleanupCustomEffect = wil::scope_exit([this]
	{
		m_glassEffect->Reset();
	});

	RETURN_IF_FAILED(
		m_glassEffect->Build(
			context,
			input.zeroCopyAllowed ? input.sourceBitmap : input.buffer->GetD2DBitmap(context, input.sourceBitmap->GetPixelFormat()),
			*input.drawingWorldBounds,
			static_cast<const void*>(&input.params)
		)
	);
	winrt::com_ptr<ID2D1Image> outputImage{};
	m_glassEffect->GetOutput(outputImage.put());

	RETURN_HR_IF_NULL(D2DERR_UNSUPPORTED_PIXEL_FORMAT, outputImage);

	D2D1_MATRIX_3X2_F originalMatrix, outputMatrix = m_glassEffect->GetOutputMatrix();
	D2D1_RENDERING_CONTROLS originalRenderingControls, renderingControls;
	context->GetRenderingControls(&originalRenderingControls);
	renderingControls = originalRenderingControls;
	renderingControls.tileSize = D2D1::SizeU(4096, 4096);
	context->SetRenderingControls(renderingControls);
	context->GetTransform(&originalMatrix);
	context->SetTransform(outputMatrix);

	D2D1InvertMatrix(&outputMatrix);
	const auto transformedDrawingRect = RectF::TransformRect(*input.drawingWorldBounds, outputMatrix);

	for (const auto& rectangle : input.rectangles)
	{
		auto transformedSubRectangle = RectF::TransformRect(rectangle, outputMatrix);

		if (RectF::IntersectUnsafe(transformedSubRectangle, transformedDrawingRect))
		{
			context->DrawImage(
				outputImage.get(),
				D2D1::Point2F(
					transformedSubRectangle.left,
					transformedSubRectangle.top
				),
				transformedSubRectangle,
				input.nearestNeighborFinalScale ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_INTERPOLATION_MODE_LINEAR,
				D2D1_COMPOSITE_MODE_BOUNDED_SOURCE_COPY
			);
		}
	}

	context->SetRenderingControls(originalRenderingControls);
	context->SetTransform(originalMatrix);

	return S_OK;
}