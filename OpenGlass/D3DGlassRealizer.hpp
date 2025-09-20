#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"
#include "AeroEffect.hpp"
#include "Shared.hpp"

namespace OpenGlass
{
	struct CConstantBuffer
	{
		D2D1_VECTOR_2F viewportSize;
		D2D1_VECTOR_2F texelSize;
		D2D1_VECTOR_4F afterglow;
		D2D1_VECTOR_4F blurBalance;
		D2D1_VECTOR_4F color;
	};

	class CD3DGlassRealizer
	{
		bool m_initialized{ false };

		UINT										m_backBufferWidth{};
		UINT										m_backBufferHeight{};
		DXGI_FORMAT									m_dxgiFormat{};
		CConstantBuffer								m_cb{};
		winrt::com_ptr<ID3D11VertexShader>          m_vertexShader{ nullptr };
		winrt::com_ptr<ID3D11PixelShader>           m_pixelShaderBlurH{ nullptr };
		winrt::com_ptr<ID3D11PixelShader>           m_pixelShaderBlurV{ nullptr };
		winrt::com_ptr<ID3D11PixelShader>           m_pixelShaderPassthrough{ nullptr };
		winrt::com_ptr<ID3D11InputLayout>           m_vertexLayout{ nullptr };
		winrt::com_ptr<ID3D11Buffer>                m_vertexBuffer{ nullptr };
		winrt::com_ptr<ID3D11Buffer>                m_indexBuffer{ nullptr };
		// Dedicated buffers: blur passes use m_vertexBuffer/m_indexBuffer; compose pass uses the following
		winrt::com_ptr<ID3D11Buffer>                m_vertexBufferCompose{ nullptr };
		winrt::com_ptr<ID3D11Buffer>                m_indexBufferCompose{ nullptr };

		// Dynamic capacities (counts, not bytes)
		size_t                                     m_vertexCapacityBlur{ 128 };
		size_t                                     m_indexCapacityBlur{ 128 };
		size_t                                     m_vertexCapacityCompose{ 128 };
		size_t                                     m_indexCapacityCompose{ 128 };
		winrt::com_ptr<ID3D11Buffer>                m_constantBuffer{ nullptr };
		winrt::com_ptr<ID3D11SamplerState>          m_samplerLinear{ nullptr };

		winrt::com_ptr<ID3D11Texture2D>             m_texHalfRes1{ nullptr };
		winrt::com_ptr<ID3D11RenderTargetView>      m_rtvHalfRes1{ nullptr };
		winrt::com_ptr<ID3D11ShaderResourceView>    m_srvHalfRes1{ nullptr };

		winrt::com_ptr<ID3D11Texture2D>             m_texHalfRes2{ nullptr };
		winrt::com_ptr<ID3D11RenderTargetView>      m_rtvHalfRes2{ nullptr };
		winrt::com_ptr<ID3D11ShaderResourceView>    m_srvHalfRes2{ nullptr };

		winrt::com_ptr<ID3D11RasterizerState>       m_rasterizerStateScissor{ nullptr };

		static HRESULT CompileShader(
			LPCSTR sourceData,
			SIZE_T sourceSize,
			LPCSTR entryPoint,
			LPCSTR shaderModel,
			ID3DBlob** blob
		);

		HRESULT CreateDeviceResources(ID3D11Device* device);
		HRESULT CreateSizeDependentResources(ID3D11Device* device);
		HRESULT Initialize(ID3D11Device* device);
		HRESULT EnsureMeshBuffersCapacity(
			ID3D11Device* device,
			size_t requiredVertexCount,
			size_t requiredIndexCount,
			winrt::com_ptr<ID3D11Buffer>& vb,
			winrt::com_ptr<ID3D11Buffer>& ib,
			size_t& vertexCapacity,
			size_t& indexCapacity
		);
		HRESULT Tessellate(
			ID2D1Geometry* geometry,
			std::vector<D2D1_VECTOR_4F>& vertices,
			std::vector<WORD>& indices
		);
	public:
		HRESULT Render(
			ID3D11Device* device,
			ID3D11DeviceContext* context1,
			ID3D11Texture2D* backBuffer,
			ID3D11RenderTargetView* backBufferRTV,
			ID3D11ShaderResourceView* backBufferSRV,
			ID2D1Geometry* geometry,
			const D2D1_RECT_F& drawingWorldbounds,
			const D2D1_RECT_F& samplingWorldbounds,
			const CAeroParams& params,
			bool shapeIsRectangles
		);
	};
}
