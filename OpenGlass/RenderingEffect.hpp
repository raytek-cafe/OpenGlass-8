#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass
{
	// [Guid("B83196A0-84DA-4723-B44E-ECCDA0D26B97")]
	DECLARE_INTERFACE_IID_(IRenderingEffect, IUnknown, "B83196A0-84DA-4723-B44E-ECCDA0D26B97")
	{
		virtual HRESULT STDMETHODCALLTYPE Build(
			ID2D1DeviceContext * context,
			ID2D1Image * inputImage,
			const D2D1_RECT_F & imageRectangle,
			const void* additionalParams
		) = 0;
		virtual D2D1_POINT_2F STDMETHODCALLTYPE GetOutputOffset() const = 0;
		virtual D2D1_MATRIX_3X2_F STDMETHODCALLTYPE GetOutputMatrix() const = 0;
		virtual void STDMETHODCALLTYPE GetOutput(ID2D1Image **output) const = 0;
		virtual void STDMETHODCALLTYPE Reset() = 0;
	};
}