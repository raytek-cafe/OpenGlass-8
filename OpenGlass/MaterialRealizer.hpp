#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass
{
	struct MaterialContext
	{
		float opacity;
	};

	class CMaterialRealizer
	{
		winrt::com_ptr<ID2D1Effect> m_materialEffect{ nullptr };

		HRESULT LoadEffect(ID2D1DeviceContext* context);
	public:
		HRESULT Render(
			ID2D1DeviceContext* context,
			const std::span<const D2D1_RECT_F>& rectangles,
			const MaterialContext& materialContext
		);
		void Reset()
		{
			m_materialEffect = nullptr;
		}
	};
}
