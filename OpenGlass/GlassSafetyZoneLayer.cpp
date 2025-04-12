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
		Reset();
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

	D2D1_SIZE_U safetyZoneBufferSizes[4]
	{
		{
			static_cast<UINT32>(std::ceil(extendedAmount)), 
			static_cast<UINT32>(std::ceil(targetSize.height + extendedAmount))
		},
		{
			static_cast<UINT32>(std::ceil(targetSize.width + extendedAmount)), 
			static_cast<UINT32>(std::ceil(extendedAmount))
		},
		{
			static_cast<UINT32>(std::ceil(extendedAmount)), 
			static_cast<UINT32>(std::ceil(targetSize.height + extendedAmount))
		},
		{
			static_cast<UINT32>(std::ceil(targetSize.width + extendedAmount)), 
			static_cast<UINT32>(std::ceil(extendedAmount))
		}
	};

	for (int i = 0; i < std::size(m_safetyZoneBounds); i++)
	{
		if (wil::rect_is_empty(m_safetyZoneBounds[i]))
		{
			continue;
		}

		m_safetyZoneBuffers[i]->Reserve(safetyZoneBufferSizes[i]);
		D2D1_POINT_2U dest
		{
			0u,
			0u
		};
		RETURN_IF_FAILED(
			m_safetyZoneBuffers[i]->CopyFrom(
				context,
				dest,
				m_renderTargetBitmap.get(),
				m_safetyZoneBounds[i]
			)
		);
	}

	pushCleanupScope.release();
	return S_OK;
}

void CGlassSafetyZoneLayer::Pop()
{
	for (int i = 0; i < std::size(m_safetyZoneBounds); i++)
	{
		if (wil::rect_is_empty(m_safetyZoneBounds[i]))
		{
			continue;
		}

		D2D1_POINT_2U dest
		{
			m_safetyZoneBounds[i].left,
			m_safetyZoneBounds[i].top
		};
		D2D1_RECT_U src
		{
			0u,
			0u,
			wil::rect_width(m_safetyZoneBounds[i]),
			wil::rect_height(m_safetyZoneBounds[i])
		};
		m_safetyZoneBuffers[i]->CopyTo(
			dest,
			m_renderTargetBitmap.get(),
			src
		);
		m_safetyZoneBounds[i] = {};
	}
	m_renderTargetBitmap = nullptr;
}

void CGlassSafetyZoneLayer::Reset()
{
	for (int i = 0; i < std::size(m_safetyZoneBounds); i++)
	{
		m_safetyZoneBuffers[i]->Reset();
		m_safetyZoneBounds[i] = {};
	}
}