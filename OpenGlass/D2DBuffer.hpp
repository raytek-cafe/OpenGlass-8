#pragma once
#include "framework.hpp"
#include "wil.hpp"

namespace OpenGlass
{
	class CD2DBuffer
	{
		winrt::com_ptr<ID2D1Bitmap1> m_buffer{};
		D2D1_SIZE_U m_size{};

		void EnsurePixelFormatCompatible(D2D1_PIXEL_FORMAT pixelFormat)
		{
			if (m_buffer)
			{
				if (
					const auto bufferPixelFormat = m_buffer->GetPixelFormat();
					bufferPixelFormat.format != pixelFormat.format ||
					bufferPixelFormat.alphaMode != pixelFormat.alphaMode
				)
				{
					Reset();
				}
			}
		}
		HRESULT EnsureBitmap(ID2D1DeviceContext* context, D2D1_PIXEL_FORMAT pixelFormat)
		{
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
							D2D1_BITMAP_OPTIONS_NONE,
							pixelFormat
						),
						m_buffer.put()
					)
				);
				/*OutputDebugStringW(
					std::format(
						L"created d2d buffer {} x {} [{}, {}]\n",
						m_size.width,
						m_size.height,
						(UINT)pixelFormat.format,
						(UINT)pixelFormat.alphaMode
					).c_str()
				);*/
			}

			return S_OK;
		}
	public:
		void Reset()
		{
			/*if (m_buffer)
			{
				const auto pixelFormat = m_buffer->GetPixelFormat();
				OutputDebugStringW(
					std::format(
						L"destroyed d2d buffer {} x {} [{}, {}]\n",
						m_size.width,
						m_size.height,
						(UINT)pixelFormat.format,
						(UINT)pixelFormat.alphaMode
					).c_str()
				);
			}*/
			m_buffer = nullptr;
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
		HRESULT CopyFrom(
			ID2D1DeviceContext* context,
			const D2D1_POINT_2U& destPoint,
			ID2D1Bitmap* bitmap,
			const D2D1_RECT_U& srcRect
		)
		{
			const auto pixelFormat = bitmap->GetPixelFormat();
			EnsurePixelFormatCompatible(pixelFormat);
			RETURN_IF_FAILED(EnsureBitmap(context, pixelFormat));
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
			const auto pixelFormat = bitmap->GetPixelFormat();
			EnsurePixelFormatCompatible(pixelFormat);
			if (!m_buffer)
			{
				return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
			}

			RETURN_IF_FAILED(
				bitmap->CopyFromBitmap(
					&destPoint,
					m_buffer.get(),
					&srcRect
				)
			);

			return S_OK;
		}
		ID2D1Bitmap* GetD2DBitmap(ID2D1DeviceContext* context, D2D1_PIXEL_FORMAT pixelFormat)
		{
			EnsurePixelFormatCompatible(pixelFormat);
			EnsureBitmap(context, pixelFormat);
			return m_buffer.get();
		}
		D2D1_SIZE_U& GetSize()
		{
			return m_size;
		}
	};
}