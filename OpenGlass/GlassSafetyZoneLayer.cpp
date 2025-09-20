#include "pch.h"
#include "GlassKernel.hpp"
#include "GlassSafetyZoneLayer.hpp"

using namespace OpenGlass;

HRESULT CGlassSafetyZoneLayer::Push(
	ID2D1DeviceContext* context,
	ID2D1Bitmap1* renderTargetBitmap,
	const D2D1_MATRIX_3X2_F& deviceTransform,
	const D2D1_RECT_F& originalPixelRectangle,
	float extendedAmount,
	D2D1_RECT_F& extendedPixelRectangle
)
{
	extendedPixelRectangle = originalPixelRectangle;
	m_renderTargetBitmap.copy_from(renderTargetBitmap);
	auto pushCleanupScope = wil::scope_exit([this]
	{
		m_renderTargetBitmap = nullptr;
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

	extendedDeviceRectangle.left = originalPixelRectangle.left - extendedAmount;
	extendedDeviceRectangle.top = originalPixelRectangle.top - extendedAmount;
	extendedDeviceRectangle.right = originalPixelRectangle.right + extendedAmount;
	extendedDeviceRectangle.bottom = originalPixelRectangle.bottom + extendedAmount;
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
		extendedPixelRectangle.left -= extendedAmount;
		extendedPixelRectangle.top -= extendedAmount;
		extendedPixelRectangle.right += extendedAmount;
		extendedPixelRectangle.bottom += extendedAmount;
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
		extendedAmount,
		extendedAmount
	};
	extendedRectangle = RectF::TransformRect(extendedRectangle, deviceTransform);
	const auto actualExtendedAmountX = wil::rect_width(extendedRectangle);
	const auto actualExtendedAmountY = wil::rect_height(extendedRectangle);

	m_safetyZoneBufferVertical.Resize(
		D2D1::SizeU(
			static_cast<UINT32>(std::ceil(actualExtendedAmountX)) * 2,
			static_cast<UINT32>(std::ceil(targetSize.height + actualExtendedAmountY))
		)
	);
	m_safetyZoneBufferHorizontal.Resize(
		D2D1::SizeU(
			static_cast<UINT32>(std::ceil(targetSize.width + actualExtendedAmountX)),
			static_cast<UINT32>(std::ceil(actualExtendedAmountY)) * 2
		)
	);

	if (!wil::rect_is_empty(m_safetyZoneBounds[0]))
	{
		RETURN_IF_FAILED(
			m_safetyZoneBufferVertical.CopyFrom(
				context,
				D2D1::Point2U(0, 0),
				m_renderTargetBitmap.get(),
				m_safetyZoneBounds[0]
			)
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[2]))
	{
		RETURN_IF_FAILED(
			m_safetyZoneBufferVertical.CopyFrom(
				context,
				D2D1::Point2U(wil::rect_width(m_safetyZoneBounds[0]), 0),
				m_renderTargetBitmap.get(),
				m_safetyZoneBounds[2]
			)
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[1]))
	{
		RETURN_IF_FAILED(
			m_safetyZoneBufferHorizontal.CopyFrom(
				context,
				D2D1::Point2U(0, 0),
				m_renderTargetBitmap.get(),
				m_safetyZoneBounds[1]
			)
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[3]))
	{
		RETURN_IF_FAILED(
			m_safetyZoneBufferHorizontal.CopyFrom(
				context,
				D2D1::Point2U(0, wil::rect_height(m_safetyZoneBounds[1])),
				m_renderTargetBitmap.get(),
				m_safetyZoneBounds[3]
			)
		);
	}

	pushCleanupScope.release();
	return S_OK;
}

void CGlassSafetyZoneLayer::Pop()
{
	if (!m_renderTargetBitmap)
	{
		return;
	}

	if (!wil::rect_is_empty(m_safetyZoneBounds[0]))
	{
		m_safetyZoneBufferVertical.CopyTo(
			D2D1::Point2U(m_safetyZoneBounds[0].left, m_safetyZoneBounds[0].top),
			m_renderTargetBitmap.get(),
			D2D1::RectU(
				0u,
				0u,
				wil::rect_width(m_safetyZoneBounds[0]),
				wil::rect_height(m_safetyZoneBounds[0])
			)
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[2]))
	{
		m_safetyZoneBufferVertical.CopyTo(
			D2D1::Point2U(m_safetyZoneBounds[2].left, m_safetyZoneBounds[2].top),
			m_renderTargetBitmap.get(),
			D2D1::RectU(
				wil::rect_width(m_safetyZoneBounds[0]),
				0,
				wil::rect_width(m_safetyZoneBounds[0]) + wil::rect_width(m_safetyZoneBounds[2]),
				wil::rect_height(m_safetyZoneBounds[2])
			)
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[1]))
	{
		m_safetyZoneBufferHorizontal.CopyTo(
			D2D1::Point2U(m_safetyZoneBounds[1].left, m_safetyZoneBounds[1].top),
			m_renderTargetBitmap.get(),
			D2D1::RectU(
				0u,
				0u,
				wil::rect_width(m_safetyZoneBounds[1]),
				wil::rect_height(m_safetyZoneBounds[1])
			)
		);
	}
	if (!wil::rect_is_empty(m_safetyZoneBounds[3]))
	{
		m_safetyZoneBufferHorizontal.CopyTo(
			D2D1::Point2U(m_safetyZoneBounds[3].left, m_safetyZoneBounds[3].top),
			m_renderTargetBitmap.get(),
			D2D1::RectU(
				0,
				wil::rect_height(m_safetyZoneBounds[1]),
				wil::rect_width(m_safetyZoneBounds[3]),
				wil::rect_height(m_safetyZoneBounds[1]) + wil::rect_height(m_safetyZoneBounds[3])
			)
		);
	}
	
	m_renderTargetBitmap = nullptr;
	ZeroMemory(m_safetyZoneBounds, sizeof(m_safetyZoneBounds));
}
