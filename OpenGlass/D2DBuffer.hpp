#pragma once
#include "framework.hpp"
#include "wil.hpp"

namespace OpenGlass
{
	class CD2DBuffer : public winrt::implements<CD2DBuffer, IUnknown, winrt::non_agile, winrt::no_weak_ref>
	{
		winrt::com_ptr<ID2D1Bitmap1> m_bufferAlphaPremultiplied{};
		winrt::com_ptr<ID2D1Bitmap> m_bufferAlphaIgnored{};
		D2D1_SIZE_U m_size{};

		void EnsurePixelFormatCompatible(DXGI_FORMAT format)
		{
			if (
				m_bufferAlphaPremultiplied &&
				m_bufferAlphaPremultiplied->GetPixelFormat().format != format
			)
			{
				Reset();
			}
		}
		HRESULT EnsureBitmap(ID2D1DeviceContext* context, DXGI_FORMAT format)
		{
			if (!m_bufferAlphaPremultiplied)
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
							D2D1_BITMAP_OPTIONS_NONE | D2D1_BITMAP_OPTIONS_TARGET,
							D2D1::PixelFormat(
								format,
								D2D1_ALPHA_MODE_PREMULTIPLIED
							)
						),
						m_bufferAlphaPremultiplied.put()
					)
				);
				auto bitmapProperties = D2D1::BitmapProperties(D2D1::PixelFormat(format, D2D1_ALPHA_MODE_IGNORE));
				RETURN_IF_FAILED(
					context->CreateSharedBitmap(
						IID_ID2D1Bitmap1,
						m_bufferAlphaPremultiplied.get(),
						&bitmapProperties,
						m_bufferAlphaIgnored.put()
					)
				);
			}

			return S_OK;
		}
	public:
		void Reset()
		{
			m_bufferAlphaPremultiplied = nullptr;
			m_bufferAlphaIgnored = nullptr;
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
			ID2D1Bitmap* bitmap,
			const D2D1_RECT_U& srcRect
		)
		{
			const auto pixelFormat = bitmap->GetPixelFormat();
			EnsurePixelFormatCompatible(pixelFormat.format);
			RETURN_IF_FAILED(EnsureBitmap(context, pixelFormat.format));
			RETURN_IF_FAILED(
				(pixelFormat.alphaMode == D2D1_ALPHA_MODE_IGNORE ? m_bufferAlphaIgnored.get() : m_bufferAlphaPremultiplied.get())->CopyFromBitmap(
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
			const auto pixelFormat = bitmap->GetPixelFormat();
			EnsurePixelFormatCompatible(pixelFormat.format);
			if (!m_bufferAlphaPremultiplied)
			{
				return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
			}

			RETURN_IF_FAILED(
				bitmap->CopyFromBitmap(
					&destPoint,
					(pixelFormat.alphaMode == D2D1_ALPHA_MODE_IGNORE ? m_bufferAlphaIgnored.get() : m_bufferAlphaPremultiplied.get()),
					&srcRect
				)
			);

			return S_OK;
		}
		ID2D1Bitmap* GetBitmapForEffectInputNoRef(ID2D1DeviceContext* context, D2D1_PIXEL_FORMAT pixelFormat)
		{
			EnsurePixelFormatCompatible(pixelFormat.format);
			EnsureBitmap(context, pixelFormat.format);
			return pixelFormat.alphaMode == D2D1_ALPHA_MODE_IGNORE ? m_bufferAlphaIgnored.get() : m_bufferAlphaPremultiplied.get();
		}
	};
}