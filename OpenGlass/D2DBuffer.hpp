#pragma once
#include "framework.hpp"
#include "wil.hpp"
#include "D2DPrivates.hpp"

namespace OpenGlass
{
	class CD2DBuffer
	{
		D2D1_SIZE_U m_size{};
		winrt::com_ptr<ID2D1Bitmap1> m_buffer{};
		winrt::com_ptr<ID2D1Bitmap1> m_bufferAlphaPremultiplied{};

		HRESULT EnsureBitmapAndCompatibility(ID2D1DeviceContext* context, ID2D1Bitmap1* bitmap)
		{
			winrt::com_ptr<ID2D1ColorContext> colorContext;
			const auto pixelFormat = bitmap->GetPixelFormat();
			bitmap->GetColorContext(colorContext.put());

			if (m_buffer)
			{
				winrt::com_ptr<ID2D1ColorContext> bufferColorContext;
				const auto bufferPixelFormat = m_buffer->GetPixelFormat();
				m_buffer->GetColorContext(colorContext.put());

				if (
					bufferPixelFormat.format != pixelFormat.format ||
					bufferColorContext != colorContext
				)
				{
					Reset();
				}
			}

			auto bitmapProperties = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(
					pixelFormat.format,
					D2D1_ALPHA_MODE_IGNORE
				)
			);
			if (!m_buffer)
			{
				RETURN_IF_FAILED(
					context->CreateBitmap(
						m_size,
						nullptr,
						0,
						bitmapProperties,
						m_buffer.put()
					)
				);
			}
			if (!m_bufferAlphaPremultiplied)
			{
				winrt::com_ptr<ID2D1PrivateCompositorDeviceContext> compositorDeviceContext{};
				if (
					bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
					FAILED(context->QueryInterface(compositorDeviceContext.put())) ||
					FAILED(
						compositorDeviceContext->CreateSharedAtlasBitmap(
							m_buffer.get(),
							&bitmapProperties,
							m_bufferAlphaPremultiplied.put()
						)
					)
				)
				{
					winrt::com_ptr<ID2D1Bitmap> sharedBitmap{};
					context->CreateSharedBitmap(
						IID_ID2D1Bitmap,
						m_buffer.get(),
						reinterpret_cast<D2D1_BITMAP_PROPERTIES*>(&bitmapProperties),
						sharedBitmap.put()
					);
					RETURN_IF_FAILED(sharedBitmap->QueryInterface(m_bufferAlphaPremultiplied.put()));
				}
			}

			return S_OK;
		}
	public:
		void Reset()
		{
			m_size = {};
			m_buffer = nullptr;
			m_bufferAlphaPremultiplied = nullptr;
		}
		void Resize(const D2D1_SIZE_U& size)
		{
			if (
				m_size.width != size.width ||
				m_size.height != size.height
			)
			{
				Reset();
				m_size = size;
			}
		}
		void Reserve(const D2D1_SIZE_U& size)
		{
			if (
				m_size.width < size.width ||
				m_size.height < size.height
			)
			{
				Reset();
				m_size = size;
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
				(bitmap->GetPixelFormat().alphaMode == D2D1_ALPHA_MODE_IGNORE ? m_buffer : m_bufferAlphaPremultiplied)->CopyFromBitmap(
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
				(bitmap->GetPixelFormat().alphaMode == D2D1_ALPHA_MODE_IGNORE ? m_buffer : m_bufferAlphaPremultiplied).get(),
				&srcRect
			);
		}
		ID2D1Bitmap* GetCompatibleD2DBitmap(ID2D1DeviceContext* context, ID2D1Bitmap1* bitmap)
		{
			EnsureBitmapAndCompatibility(context, bitmap);
			return (bitmap->GetPixelFormat().alphaMode == D2D1_ALPHA_MODE_IGNORE ? m_buffer : m_bufferAlphaPremultiplied).get();
		}
	};
}
