#pragma once
#include "framework.hpp"
#include "wil.hpp"

namespace OpenGlass
{
	class CD2DBuffer
	{
		winrt::com_ptr<ID2D1Bitmap1> m_buffer{};
		D2D1_SIZE_U m_size{};

		HRESULT EnsureBitmapAndCompatibility(ID2D1DeviceContext* context, ID2D1Bitmap1* bitmap)
		{
			winrt::com_ptr<ID2D1ColorContext> colorContext;
			const auto pixelFormat = bitmap->GetPixelFormat();
			const auto options = bitmap->GetOptions();
			bitmap->GetColorContext(colorContext.put());

			if (m_buffer)
			{
				winrt::com_ptr<ID2D1ColorContext> bufferColorContext;
				const auto bufferPixelFormat = m_buffer->GetPixelFormat();
				const auto bufferOptions = m_buffer->GetOptions();
				m_buffer->GetColorContext(colorContext.put());

				if (
					bufferPixelFormat.format != pixelFormat.format ||
					bufferPixelFormat.alphaMode != pixelFormat.alphaMode ||
					bufferOptions != options ||
					bufferColorContext != colorContext
				)
				{
					Reset();
				}
			}
			if (!m_buffer)
			{
				RETURN_IF_FAILED(
					context->CreateBitmap(
						D2D1::SizeU(
							m_size.width,
							m_size.height
						),
						nullptr,
						0,
						D2D1::BitmapProperties1(
							options,
							pixelFormat
						),
						m_buffer.put()
					)
				);
			}

			return S_OK;
		}
	public:
		void Reset()
		{
			m_buffer = nullptr;
		}
		void Resize(const D2D1_SIZE_U& size)
		{
			if (
				m_size.width != size.width ||
				m_size.height != size.height
			)
			{
				m_size = size;
				Reset();
			}
		}
		HRESULT CopyFrom(
			ID2D1DeviceContext* context,
			const D2D1_POINT_2U& destPoint,
			ID2D1Bitmap1* bitmap,
			const D2D1_RECT_U& srcRect
		)
		{
			RETURN_IF_FAILED(EnsureBitmapAndCompatibility(context, bitmap));
			RETURN_IF_FAILED(
				m_buffer->CopyFromBitmap(
					&destPoint,
					bitmap,
					&srcRect
				)
			);

			return S_OK;
		}
		HRESULT CopyTo(
			const D2D1_POINT_2U& destPoint,
			ID2D1Bitmap* bitmap,
			const D2D1_RECT_U& srcRect
		)
		{
			return bitmap->CopyFromBitmap(
				&destPoint,
				m_buffer.get(),
				&srcRect
			);
		}
		ID2D1Bitmap* GetCompatibleD2DBitmap(ID2D1DeviceContext* context, ID2D1Bitmap1* bitmap)
		{
			EnsureBitmapAndCompatibility(context, bitmap);
			return m_buffer.get();
		}
	};
}