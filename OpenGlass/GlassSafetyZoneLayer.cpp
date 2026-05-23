#include "pch.h"
#include "GlassKernel.hpp"
#include "GlassSafetyZoneLayer.hpp"
#include "DxgiPrivates.hpp"

using namespace OpenGlass;

HRESULT CGlassSafetyZoneLayer::Push(
	ID2D1DeviceContext* context,
	const D2D1_MATRIX_3X2_F& deviceTransform,
	const D2D1_RECT_F& originalPixelRectangle,
	float expansion,
	D2D1_RECT_F& extendedPixelRectangle
)
{
	winrt::com_ptr<ID2D1Bitmap1> renderTargetBitmap{ nullptr };
	RETURN_IF_FAILED(Util::GetTargetBitmapFromD2DContext(context, renderTargetBitmap));

	extendedPixelRectangle = originalPixelRectangle;
	auto pushCleanupScope = wil::scope_exit([this]
	{
		m_renderTargetTexture = nullptr;
	});
	const auto targetSize = renderTargetBitmap->GetSize();
	const D2D1_RECT_F targetRect
	{
		0.f,
		0.f,
		targetSize.width,
		targetSize.height
	};

	D2D1_RECT_F originalDeviceRectangle;
	D2D1_RECT_F extendedDeviceRectangle;

	originalDeviceRectangle = RectF::TransformRect(originalPixelRectangle, deviceTransform);
	originalDeviceRectangle.left = std::floor(originalDeviceRectangle.left);
	originalDeviceRectangle.top = std::floor(originalDeviceRectangle.top);
	originalDeviceRectangle.right = std::ceil(originalDeviceRectangle.right);
	originalDeviceRectangle.bottom = std::ceil(originalDeviceRectangle.bottom);
	RectF::IntersectUnsafe(originalDeviceRectangle, targetRect);

	extendedDeviceRectangle.left = originalPixelRectangle.left - expansion;
	extendedDeviceRectangle.top = originalPixelRectangle.top - expansion;
	extendedDeviceRectangle.right = originalPixelRectangle.right + expansion;
	extendedDeviceRectangle.bottom = originalPixelRectangle.bottom + expansion;
	extendedDeviceRectangle = RectF::TransformRect(extendedDeviceRectangle, deviceTransform);
	extendedDeviceRectangle.left = std::floor(extendedDeviceRectangle.left);
	extendedDeviceRectangle.top = std::floor(extendedDeviceRectangle.top);
	extendedDeviceRectangle.right = std::ceil(extendedDeviceRectangle.right);
	extendedDeviceRectangle.bottom = std::ceil(extendedDeviceRectangle.bottom);
	RectF::IntersectUnsafe(extendedDeviceRectangle, targetRect);

	D2D1_MATRIX_3X2_F invertedDeviceTransform = deviceTransform;
	if (D2D1InvertMatrix(&invertedDeviceTransform)) [[unlikely]]
	{
		extendedPixelRectangle = RectF::TransformRect(extendedDeviceRectangle, invertedDeviceTransform);
	}
	else
	{
		extendedPixelRectangle.left -= expansion;
		extendedPixelRectangle.top -= expansion;
		extendedPixelRectangle.right += expansion;
		extendedPixelRectangle.bottom += expansion;
	}

	if (
		extendedPixelRectangle.left == originalPixelRectangle.left &&
		extendedPixelRectangle.top == originalPixelRectangle.top &&
		extendedPixelRectangle.right == originalPixelRectangle.right &&
		extendedPixelRectangle.bottom == originalPixelRectangle.bottom
	)
	{
		return S_OK;
	}

	BOOL hwProtectionEnabled = FALSE;
	D3D11_TEXTURE2D_DESC renderTargetDesc{};

	RETURN_IF_FAILED(
		Util::GetTextureFromD2DBitmap(
			renderTargetBitmap.get(),
			m_renderTargetTexture
		)
	);
	RETURN_IF_FAILED(
		Util::GetDescAndHwProtectionStateFromTexture(
			m_renderTargetTexture.get(),
			renderTargetDesc,
			hwProtectionEnabled
		)
	);

	winrt::com_ptr<ID3D11Device> d3dDevice{};
	m_renderTargetTexture->GetDevice(d3dDevice.put());
	winrt::com_ptr<ID3D11DeviceContext> d3dContext{};
	d3dDevice->GetImmediateContext(d3dContext.put());

	m_safetyZoneBounds[0] =
	{
		static_cast<UINT32>(extendedDeviceRectangle.left),
		static_cast<UINT32>(extendedDeviceRectangle.top),
		static_cast<UINT32>(originalDeviceRectangle.left),
		static_cast<UINT32>(originalDeviceRectangle.bottom)
	};
	m_safetyZoneBounds[1] =
	{
		static_cast<UINT32>(originalDeviceRectangle.left),
		static_cast<UINT32>(extendedDeviceRectangle.top),
		static_cast<UINT32>(extendedDeviceRectangle.right),
		static_cast<UINT32>(originalDeviceRectangle.top)
	};
	m_safetyZoneBounds[2] =
	{
		static_cast<UINT32>(originalDeviceRectangle.right),
		static_cast<UINT32>(originalDeviceRectangle.top),
		static_cast<UINT32>(extendedDeviceRectangle.right),
		static_cast<UINT32>(extendedDeviceRectangle.bottom)
	};
	m_safetyZoneBounds[3] =
	{
		static_cast<UINT32>(extendedDeviceRectangle.left),
		static_cast<UINT32>(originalDeviceRectangle.bottom),
		static_cast<UINT32>(originalDeviceRectangle.right),
		static_cast<UINT32>(extendedDeviceRectangle.bottom)
	};

	D2D1_RECT_F extendedRectangle
	{
		0.f,
		0.f,
		expansion,
		expansion
	};
	extendedRectangle = RectF::TransformRect(extendedRectangle, deviceTransform);
	const auto actualExtendedAmountX = wil::rect_width(extendedRectangle);
	const auto actualExtendedAmountY = wil::rect_height(extendedRectangle);

	const UINT verticalWidth = static_cast<UINT>(std::ceil(actualExtendedAmountX)) * 2u;
	const UINT verticalHeight = static_cast<UINT>(std::ceil(targetSize.height + actualExtendedAmountY));
	const UINT horizontalWidth = static_cast<UINT>(std::ceil(targetSize.width + actualExtendedAmountX));
	const UINT horizontalHeight = static_cast<UINT>(std::ceil(actualExtendedAmountY)) * 2u;

	RETURN_IF_FAILED(m_safetyZoneBufferVertical.Ensure(d3dDevice.get(), verticalWidth, verticalHeight, renderTargetDesc, 0, hwProtectionEnabled));
	RETURN_IF_FAILED(m_safetyZoneBufferHorizontal.Ensure(d3dDevice.get(), horizontalWidth, horizontalHeight, renderTargetDesc, 0, hwProtectionEnabled));

	if (!wil::rect_is_empty(m_safetyZoneBounds[0]))
	{
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_renderTargetTexture.get(),
			m_safetyZoneBufferVertical.m_texture.get(),
			m_safetyZoneBounds[0],
			0,
			0
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[2]))
	{
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_renderTargetTexture.get(),
			m_safetyZoneBufferVertical.m_texture.get(),
			m_safetyZoneBounds[2],
			wil::rect_width(m_safetyZoneBounds[0]),
			0
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[1]))
	{
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_renderTargetTexture.get(),
			m_safetyZoneBufferHorizontal.m_texture.get(),
			m_safetyZoneBounds[1],
			0,
			0
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[3]))
	{
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_renderTargetTexture.get(),
			m_safetyZoneBufferHorizontal.m_texture.get(),
			m_safetyZoneBounds[3],
			0,
			wil::rect_height(m_safetyZoneBounds[1])
		);
	}

	d3dContext->Flush();
	pushCleanupScope.release();
	return S_OK;
}

void CGlassSafetyZoneLayer::Pop()
{
	if (!m_renderTargetTexture)
	{
		ZeroMemory(m_safetyZoneBounds, sizeof(m_safetyZoneBounds));
		return;
	}

	winrt::com_ptr<ID3D11Device> d3dDevice{};
	m_renderTargetTexture->GetDevice(d3dDevice.put());
	winrt::com_ptr<ID3D11DeviceContext> d3dContext{};
	d3dDevice->GetImmediateContext(d3dContext.put());

	if (!wil::rect_is_empty(m_safetyZoneBounds[0]))
	{
		const D2D1_RECT_U srcRect
		{
			0u,
			0u,
			wil::rect_width(m_safetyZoneBounds[0]),
			wil::rect_height(m_safetyZoneBounds[0])
		};
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_safetyZoneBufferVertical.m_texture.get(),
			m_renderTargetTexture.get(),
			srcRect,
			m_safetyZoneBounds[0].left,
			m_safetyZoneBounds[0].top
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[2]))
	{
		const D2D1_RECT_U srcRect
		{
			wil::rect_width(m_safetyZoneBounds[0]),
			0u,
			wil::rect_width(m_safetyZoneBounds[0]) + wil::rect_width(m_safetyZoneBounds[2]),
			wil::rect_height(m_safetyZoneBounds[2])
		};
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_safetyZoneBufferVertical.m_texture.get(),
			m_renderTargetTexture.get(),
			srcRect,
			m_safetyZoneBounds[2].left,
			m_safetyZoneBounds[2].top
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[1]))
	{
		const D2D1_RECT_U srcRect
		{
			0u,
			0u,
			wil::rect_width(m_safetyZoneBounds[1]),
			wil::rect_height(m_safetyZoneBounds[1])
		};
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_safetyZoneBufferHorizontal.m_texture.get(),
			m_renderTargetTexture.get(),
			srcRect,
			m_safetyZoneBounds[1].left,
			m_safetyZoneBounds[1].top
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[3]))
	{
		const D2D1_RECT_U srcRect
		{
			0u,
			wil::rect_height(m_safetyZoneBounds[1]),
			wil::rect_width(m_safetyZoneBounds[3]),
			wil::rect_height(m_safetyZoneBounds[1]) + wil::rect_height(m_safetyZoneBounds[3])
		};
		Util::CopyTextureRegion(
			d3dContext.get(),
			m_safetyZoneBufferHorizontal.m_texture.get(),
			m_renderTargetTexture.get(),
			srcRect,
			m_safetyZoneBounds[3].left,
			m_safetyZoneBounds[3].top
		);
	}

	d3dContext->Flush();
	m_renderTargetTexture = nullptr;
	d3dContext = nullptr;
	ZeroMemory(m_safetyZoneBounds, sizeof(m_safetyZoneBounds));
}
