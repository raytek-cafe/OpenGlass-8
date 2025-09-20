#include "pch.h"
#include "Util.hpp"
#include "uDWMProjection.hpp"
#include "GlassRealizer.hpp"
#include "AeroEffect.hpp"
#include "BlurEffect.hpp"
#include "Shared.hpp"

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
	const CGlassInput& input
)
{
	if (!EnsureGlassEffect(Shared::g_type))
	{
		return S_OK;
	}

	const auto renderTargetBitmap = Util::GetTargetBitmapFromDeviceContext(context);
	if (!renderTargetBitmap)
	{
		return S_OK;
	}

	const D2D1_RECT_F alignedSamplingWorldBounds
	{
		std::floor(input.samplingWorldBoundsShapeClipped->left),
		std::floor(input.samplingWorldBoundsShapeClipped->top),
		std::ceil(input.samplingWorldBoundsShapeClipped->right),
		std::ceil(input.samplingWorldBoundsShapeClipped->bottom)
	};
	const D2D1_RECT_F alignedDrawingWorldBounds
	{
		std::floor(input.drawingWorldBounds->left),
		std::floor(input.drawingWorldBounds->top),
		std::ceil(input.drawingWorldBounds->right),
		std::ceil(input.drawingWorldBounds->bottom)
	};

	winrt::com_ptr<ID2D1PrivateCompositorDeviceContext> compositorDeviceContext{};
	winrt::com_ptr<ID2D1Bitmap1> sharedAtlasBitmap{};
	
	if (
		const auto bitmapProperties = D2D1::BitmapProperties1(
			renderTargetBitmap->GetOptions(),
			renderTargetBitmap->GetPixelFormat()
		);
		input.zeroCopyOptimization &&
		SUCCEEDED(context->QueryInterface(compositorDeviceContext.put()))
	)
	{
		LOG_IF_FAILED(
			compositorDeviceContext->CreateSharedAtlasBitmap(
				renderTargetBitmap.get(),
				&bitmapProperties,
				sharedAtlasBitmap.put()
			)
		);
	}

	const auto buffer = input.buffer;
	if (!sharedAtlasBitmap)
	{
		buffer->Reserve(renderTargetBitmap->GetPixelSize());
		RETURN_IF_FAILED(
			buffer->CopyFrom(
				context,
				D2D1::Point2U(
					static_cast<UINT32>(alignedSamplingWorldBounds.left),
					static_cast<UINT32>(alignedSamplingWorldBounds.top)
				),
				renderTargetBitmap.get(),
				D2D1::RectU(
					static_cast<UINT32>(alignedSamplingWorldBounds.left),
					static_cast<UINT32>(alignedSamplingWorldBounds.top),
					static_cast<UINT32>(alignedSamplingWorldBounds.right),
					static_cast<UINT32>(alignedSamplingWorldBounds.bottom)
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
			sharedAtlasBitmap ? sharedAtlasBitmap.get() : buffer->GetCompatibleD2DBitmap(context, renderTargetBitmap.get()),
			alignedSamplingWorldBounds,
			static_cast<const void*>(&input.params)
		)
	);
	winrt::com_ptr<ID2D1Image> outputImage{};
	m_glassEffect->GetOutput(outputImage.put());

	RETURN_HR_IF_NULL(D2DERR_UNSUPPORTED_PIXEL_FORMAT, outputImage);

	D2D1_MATRIX_3X2_F originalMatrix, outputMatrix = m_glassEffect->GetOutputMatrix();
	context->GetTransform(&originalMatrix);
	context->SetTransform(outputMatrix);

	D2D1InvertMatrix(&outputMatrix);

	const auto offset = m_glassEffect->GetOutputOffset();
	for (auto subRectangle : input.rectangles)
	{
		if (RectF::IntersectUnsafe(subRectangle, alignedDrawingWorldBounds))
		{
			auto transformedSubRectangle = RectF::TransformRect(subRectangle, outputMatrix);

			context->DrawImage(
				outputImage.get(),
				D2D1::Point2F(
					transformedSubRectangle.left,
					transformedSubRectangle.top
				),
				D2D1::RectF(
					transformedSubRectangle.left + offset.x,
					transformedSubRectangle.top + offset.y,
					transformedSubRectangle.right + offset.x,
					transformedSubRectangle.bottom + offset.y
				),
				D2D1_INTERPOLATION_MODE_LINEAR,
				D2D1_COMPOSITE_MODE_BOUNDED_SOURCE_COPY
			);
		}
	}

	context->SetTransform(originalMatrix);

	return S_OK;
}
