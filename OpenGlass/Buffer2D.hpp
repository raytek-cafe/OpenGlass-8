#pragma once
#include "framework.hpp"
#include "wil.hpp"

namespace OpenGlass
{
	struct CBuffer2D
	{
		UINT m_width{};
		UINT m_height{};
		DXGI_FORMAT m_format{ DXGI_FORMAT_UNKNOWN };
		DXGI_SAMPLE_DESC m_sampleDesc{ 1, 0 };
		UINT m_bindFlags{};
		UINT m_miscFlags{};
		winrt::com_ptr<ID3D11Texture2D> m_texture{};
		winrt::com_ptr<ID3D11RenderTargetView> m_rtv{ nullptr };
		winrt::com_ptr<ID3D11ShaderResourceView> m_srv{ nullptr };

		D2D1_BITMAP_PROPERTIES1 m_properties
		{
			.pixelFormat = D2D1::PixelFormat(),
			.dpiX = 96.f,
			.dpiY = 96.f,
			.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE,
			.colorContext = nullptr
		};
		winrt::com_ptr<ID2D1Bitmap1> m_bitmap{};

		void Reset()
		{
			m_width = 0;
			m_height = 0;
			m_format = DXGI_FORMAT_UNKNOWN;
			m_sampleDesc = { 1, 0 };
			m_bindFlags = 0;
			m_miscFlags = 0;
			m_texture = nullptr;
			m_rtv = nullptr;
			m_srv = nullptr;

			m_properties =
			{
				.pixelFormat = D2D1::PixelFormat(),
				.dpiX = 96.f,
				.dpiY = 96.f,
				.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE,
				.colorContext = nullptr
			};
			m_bitmap = nullptr;
		}
		HRESULT Ensure(
			ID3D11Device* device,
			UINT width,
			UINT height,
			const D3D11_TEXTURE2D_DESC& baseDesc,
			UINT bindFlags,
			bool hwProtectionEnabled
		)
		{
			const UINT safeWidth = std::max(1u, width);
			const UINT safeHeight = std::max(1u, height);
			const UINT desiredMiscFlags =
				(baseDesc.MiscFlags & ~D3D11_RESOURCE_MISC_HW_PROTECTED) |
				(hwProtectionEnabled ? D3D11_RESOURCE_MISC_HW_PROTECTED : 0u);

			if (
				m_texture &&
				m_width == safeWidth &&
				m_height == safeHeight &&
				m_format == baseDesc.Format &&
				m_sampleDesc.Count == baseDesc.SampleDesc.Count &&
				m_sampleDesc.Quality == baseDesc.SampleDesc.Quality &&
				m_bindFlags == bindFlags &&
				m_miscFlags == desiredMiscFlags
			)
			{
				return S_OK;
			}

			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = safeWidth;
			desc.Height = safeHeight;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = baseDesc.Format;
			desc.SampleDesc = baseDesc.SampleDesc;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = bindFlags;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = desiredMiscFlags;

			winrt::com_ptr<ID3D11Texture2D> texture{};
			RETURN_IF_FAILED(device->CreateTexture2D(&desc, nullptr, texture.put()));

			m_width = safeWidth;
			m_height = safeHeight;
			m_format = baseDesc.Format;
			m_sampleDesc = baseDesc.SampleDesc;
			m_bindFlags = bindFlags;
			m_miscFlags = desiredMiscFlags;
			m_texture = std::move(texture);
			m_rtv = nullptr;
			m_srv = nullptr;
			m_bitmap = nullptr;
			return S_OK;
		}
		HRESULT EnsureRTV(ID3D11Device* device)
		{
			if (!m_texture)
			{
				return E_FAIL;
			}
			if (m_rtv)
			{
				return S_OK;
			}
			RETURN_IF_FAILED(device->CreateRenderTargetView(m_texture.get(), nullptr, m_rtv.put()));
			return S_OK;
		}
		HRESULT EnsureSRV(ID3D11Device* device)
		{
			if (!m_texture)
			{
				return E_FAIL;
			}
			if (m_srv)
			{
				return S_OK;
			}
			RETURN_IF_FAILED(device->CreateShaderResourceView(m_texture.get(), nullptr, m_srv.put()));
			return S_OK;
		}
		HRESULT EnsureD2D(
			ID2D1DeviceContext* d2dContext,
			const D2D1_BITMAP_PROPERTIES1& properties
		)
		{
			if (!m_texture)
			{
				return D2DERR_UNSUPPORTED_OPERATION;
			}

			if (
				m_bitmap &&
				m_properties.pixelFormat.format == properties.pixelFormat.format &&
				m_properties.pixelFormat.alphaMode == properties.pixelFormat.alphaMode &&
				m_properties.dpiX == properties.dpiX &&
				m_properties.dpiY == properties.dpiY &&
				m_properties.bitmapOptions == properties.bitmapOptions &&
				m_properties.colorContext == properties.colorContext
			)
			{
				return S_OK;
			}

			winrt::com_ptr<IDXGISurface> dxgiSurface{};
			RETURN_IF_FAILED(m_texture->QueryInterface(dxgiSurface.put()));
			RETURN_IF_FAILED(
				d2dContext->CreateBitmapFromDxgiSurface(
					dxgiSurface.get(),
					&properties,
					m_bitmap.put()
				)
			);
			return S_OK;
		}
	};
}
