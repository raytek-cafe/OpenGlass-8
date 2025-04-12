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
	const auto buffer = input.buffer->GetBitmapForEffectInputNoRef(context, input.renderTargetBitmap->GetPixelFormat());
	/*RETURN_IF_FAILED(
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
	);*/

	{
		const auto compositorContext = input.d2dContext->GetPrivateCompositorDeviceContext();

		winrt::com_ptr<ID2D1PrivateDepthBuffer> depthBuffer{};
		RETURN_IF_FAILED(compositorContext->GetDepthBuffer(depthBuffer.put()));
		RETURN_IF_FAILED(
			compositorContext->SetTargetAndDepthBuffer(
				buffer,
				depthBuffer.get()
			)
		);
		const auto targetScope = wil::scope_exit([&]
		{
			compositorContext->SetTargetAndDepthBuffer(
				input.renderTargetBitmap,
				depthBuffer.get()
			);
		});

		{
			const auto cleanupCustomEffect = wil::scope_exit([this]
			{
				m_glassEffect->Reset();
			});
			winrt::com_ptr<ID2D1Image> outputImage{};
			RETURN_IF_FAILED(
				m_glassEffect->Build(
					context,
					input.renderTargetBitmap,
					drawingWorldBounds,
					imageBounds,
					static_cast<const void*>(&input.params)
				)
			);

			m_glassEffect->GetOutput(outputImage.put());
			if (outputImage)
			{
				context->DrawImage(
					outputImage.get(),
					D2D1::Point2F(
						drawingWorldBounds.left,
						drawingWorldBounds.top
					),
					drawingWorldBounds,
					D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
					D2D1_COMPOSITE_MODE_BOUNDED_SOURCE_COPY
				);
			}
		}
	}

	context->DrawImage(
		buffer,
		D2D1::Point2F(
			drawingWorldBounds.left,
			drawingWorldBounds.top
		),
		drawingWorldBounds,
		D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
		D2D1_COMPOSITE_MODE_BOUNDED_SOURCE_COPY
	);
	/*RETURN_IF_FAILED(
		input.buffer->CopyTo(
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
	);*/

	return S_OK;
}