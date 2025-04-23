#include "pch.h"
#include "GlassKernel.hpp"
#include "GlassSafetyZoneLayer.hpp"

using namespace OpenGlass;

HRESULT CGlassSafetyZoneLayer::Push(
	ID2D1DeviceContext* context,
	ID2D1Bitmap1* renderTargetBitmap,
	const D2D1_MATRIX_3X2_F& deviceTransform,
	const D2D1_RECT_F& originalPixelRectangle,
	float extendedAmount
)
{
	m_renderTargetBitmap.copy_from(renderTargetBitmap);
	auto pushCleanupScope = wil::scope_exit([this]
	{
		m_renderTargetBitmap = nullptr;
	});
	const auto targetSize = renderTargetBitmap->GetSize();

	auto extendedDeviceRectangle = RectF::TransformRect(originalPixelRectangle, deviceTransform);
	auto originalDeviceRectangle = extendedDeviceRectangle;

	originalDeviceRectangle.left = std::max(std::floor(originalDeviceRectangle.left), 0.f);
	originalDeviceRectangle.top = std::max(std::floor(originalDeviceRectangle.top), 0.f);
	originalDeviceRectangle.right = std::min(std::ceil(originalDeviceRectangle.right), targetSize.width);
	originalDeviceRectangle.bottom = std::min(std::ceil(originalDeviceRectangle.bottom), targetSize.height);

	extendedDeviceRectangle.left = std::max(std::floor(extendedDeviceRectangle.left - extendedAmount), 0.f);
	extendedDeviceRectangle.top = std::max(std::floor(extendedDeviceRectangle.top - extendedAmount), 0.f);
	extendedDeviceRectangle.right = std::min(std::ceil(extendedDeviceRectangle.right + extendedAmount), targetSize.width);
	extendedDeviceRectangle.bottom = std::min(std::ceil(extendedDeviceRectangle.bottom + extendedAmount), targetSize.height);

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

	m_safetyZoneBufferVertical.Resize(
		D2D1::SizeU(
			static_cast<UINT32>(std::ceil(extendedAmount)) * 2,
			static_cast<UINT32>(std::ceil(targetSize.height + extendedAmount))
		)
	);
	m_safetyZoneBufferHorizon.Resize(
		D2D1::SizeU(
			static_cast<UINT32>(std::ceil(targetSize.width + extendedAmount)),
			static_cast<UINT32>(std::ceil(extendedAmount)) * 2
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
			m_safetyZoneBufferHorizon.CopyFrom(
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
			m_safetyZoneBufferHorizon.CopyFrom(
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
		m_safetyZoneBufferHorizon.CopyTo(
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
		m_safetyZoneBufferHorizon.CopyTo(
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
}

void CGlassSafetyZoneLayer::Reset()
{
	m_safetyZoneBufferVertical.Reset();
	m_safetyZoneBufferHorizon.Reset();
	memset(m_safetyZoneBounds, 0, sizeof(m_safetyZoneBounds));
}