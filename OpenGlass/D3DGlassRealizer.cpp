#include "pch.h"
#include "Util.hpp"
#include "D3DGlassRealizer.hpp"
#include "Shared.hpp"

using namespace OpenGlass;

namespace OpenGlass
{
	constexpr int c_maxTriangle = 2048;

	// https://learn.microsoft.com/en-us/archive/msdn-magazine/2014/march/directx-factor-triangles-and-tessellation
	class CTessellationSink : public winrt::implements<CTessellationSink, ID2D1TessellationSink, winrt::non_agile, winrt::no_weak_ref>
	{
		std::vector<D2D1_TRIANGLE> m_triangles{};
	public:
		STDMETHOD_(void, AddTriangles)(_In_reads_(trianglesCount) CONST D2D1_TRIANGLE* triangles, UINT32 trianglesCount) override
		{
			const auto triangleViews = std::span{ triangles, trianglesCount };
			m_triangles.insert(m_triangles.end(), triangleViews.begin(), triangleViews.end());
		}
		STDMETHOD(Close)() override
		{
			return S_OK;
		}
		const std::vector<D2D1_TRIANGLE>& GetTriangles() const
		{
			return m_triangles;
		}
	};

	const std::string_view g_blurShaders =
	#include "BlurShaders.hpp"
	;
}

HRESULT CD3DGlassRealizer::CompileShader(
	LPCSTR sourceData,
	SIZE_T sourceSize,
	LPCSTR entryPoint,
	LPCSTR shaderModel,
	ID3DBlob** blob
)
{
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	winrt::com_ptr<ID3DBlob> errorBlob{ nullptr };
	
	HRESULT hr
	{
		D3DCompile(
			sourceData,
			sourceSize,
			nullptr,
			nullptr,
			nullptr,
			entryPoint,
			shaderModel,
			dwShaderFlags,
			0,
			blob,
			errorBlob.put()
		)
	};
	if (FAILED(hr))
	{
		if (errorBlob)
		{
			int outLength{};
			std::unique_ptr<WCHAR[]> errorInfo{};
			Util::MB2WC(errorInfo, reinterpret_cast<const char*>(errorBlob->GetBufferPointer()), static_cast<int>(errorBlob->GetBufferSize()), &outLength);
			RETURN_HR_MSG(hr, "%s", std::wstring(errorInfo.get(), outLength).c_str());
		}
		else
		{
			RETURN_HR(hr);
		}
	}

	return S_OK;
}

HRESULT CD3DGlassRealizer::CreateDeviceResources(ID3D11Device* device)
{
	winrt::com_ptr<ID3DBlob> vsBlob{ nullptr };
	RETURN_IF_FAILED(CompileShader(g_blurShaders.data(), g_blurShaders.length(), "VS", "vs_4_0", vsBlob.put()));

	RETURN_IF_FAILED(
		device->CreateVertexShader(
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			nullptr,
			m_vertexShader.put()
		)
	);

	D3D11_INPUT_ELEMENT_DESC layout[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements{ ARRAYSIZE(layout) };

	RETURN_IF_FAILED(
		device->CreateInputLayout(
			layout,
			numElements,
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			m_vertexLayout.put()
		)
	);
	vsBlob = nullptr;

	winrt::com_ptr<ID3DBlob> psBlobH{ nullptr };
	RETURN_IF_FAILED(CompileShader(g_blurShaders.data(), g_blurShaders.length(), "PS_BlurH", "ps_4_0", psBlobH.put()));

	RETURN_IF_FAILED(
		device->CreatePixelShader(
			psBlobH->GetBufferPointer(),
			psBlobH->GetBufferSize(),
			nullptr,
			m_pixelShaderBlurH.put()
		)
	);
	psBlobH = nullptr;

	winrt::com_ptr<ID3DBlob> psBlobV{ nullptr };
	RETURN_IF_FAILED(CompileShader(g_blurShaders.data(), g_blurShaders.length(), "PS_BlurV", "ps_4_0", psBlobV.put()));

	RETURN_IF_FAILED(
		device->CreatePixelShader(
			psBlobV->GetBufferPointer(),
			psBlobV->GetBufferSize(),
			nullptr,
			m_pixelShaderBlurV.put()
		)
	);
	psBlobV = nullptr;

	winrt::com_ptr<ID3DBlob> psBlobPassthrough{ nullptr };
	RETURN_IF_FAILED(CompileShader(g_blurShaders.data(), g_blurShaders.length(), "PS_Passthrough", "ps_4_0", psBlobPassthrough.put()));

	RETURN_IF_FAILED(
		device->CreatePixelShader(
			psBlobPassthrough->GetBufferPointer(),
			psBlobPassthrough->GetBufferSize(),
			nullptr,
			m_pixelShaderPassthrough.put()
		)
	);
	psBlobPassthrough = nullptr;

	// Create small initial buffers (128) and grow dynamically on demand
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(D2D1_VECTOR_4F) * m_vertexCapacityBlur);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, m_vertexBuffer.put()));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(WORD) * m_indexCapacityBlur);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, m_indexBuffer.put()));

	// Compose buffers
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(D2D1_VECTOR_4F) * m_vertexCapacityCompose);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, m_vertexBufferCompose.put()));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(WORD) * m_indexCapacityCompose);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, m_indexBufferCompose.put()));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, m_constantBuffer.put()));

	D3D11_SAMPLER_DESC sampDesc{};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	RETURN_IF_FAILED(device->CreateSamplerState(&sampDesc, m_samplerLinear.put()));

	D3D11_RASTERIZER_DESC rasterDesc{};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = FALSE;
	rasterDesc.ScissorEnable = TRUE;
	RETURN_IF_FAILED(device->CreateRasterizerState(&rasterDesc, m_rasterizerStateScissor.put()));

	return S_OK;
}

HRESULT CD3DGlassRealizer::EnsureMeshBuffersCapacity(
	ID3D11Device* device,
	size_t requiredVertexCount,
	size_t requiredIndexCount,
	winrt::com_ptr<ID3D11Buffer>& vb,
	winrt::com_ptr<ID3D11Buffer>& ib,
	size_t& vertexCapacity,
	size_t& indexCapacity
)
{
	auto growTo = [](size_t cur, size_t req) noexcept -> size_t 
	{
		if (req <= cur) return cur;
		size_t cap = (cur == 0 ? 128ull : cur);
		while (cap < req) cap <<= 1; // power-of-two growth
		return cap;
	};

	bool needVB = requiredVertexCount > vertexCapacity;
	bool needIB = requiredIndexCount > indexCapacity;
	if (!needVB && !needIB)
	{
		return S_OK;
	}

	D3D11_BUFFER_DESC bd{};
	if (needVB)
	{
		vertexCapacity = growTo(vertexCapacity, requiredVertexCount);
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = static_cast<UINT>(sizeof(D2D1_VECTOR_4F) * vertexCapacity);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, vb.put()));
	}
	if (needIB)
	{
		indexCapacity = growTo(indexCapacity, requiredIndexCount);
		if (indexCapacity > std::numeric_limits<WORD>::max())
		{
			indexCapacity = std::numeric_limits<WORD>::max();
		}
		bd = {};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = static_cast<UINT>(sizeof(WORD) * indexCapacity);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		RETURN_IF_FAILED(device->CreateBuffer(&bd, nullptr, ib.put()));
	}
	return S_OK;
}

HRESULT CD3DGlassRealizer::CreateSizeDependentResources(ID3D11Device* device)
{
	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = m_dxgiFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	texDesc.Width = static_cast<UINT>(std::ceil(m_backBufferWidth * 0.5f));
	texDesc.Height = m_backBufferHeight;
	// Pass 1 output
	RETURN_IF_FAILED(device->CreateTexture2D(&texDesc, nullptr, m_texHalfRes1.put()));
	RETURN_IF_FAILED(device->CreateRenderTargetView(m_texHalfRes1.get(), nullptr, m_rtvHalfRes1.put()));
	RETURN_IF_FAILED(device->CreateShaderResourceView(m_texHalfRes1.get(), nullptr, m_srvHalfRes1.put()));

	texDesc.Width = static_cast<UINT>(std::ceil(m_backBufferWidth * 0.5f));
	texDesc.Height = static_cast<UINT>(std::ceil(m_backBufferHeight * 0.5f));
	// Pass 2 output
	RETURN_IF_FAILED(device->CreateTexture2D(&texDesc, nullptr, m_texHalfRes2.put()));
	RETURN_IF_FAILED(device->CreateRenderTargetView(m_texHalfRes2.get(), nullptr, m_rtvHalfRes2.put()));
	RETURN_IF_FAILED(device->CreateShaderResourceView(m_texHalfRes2.get(), nullptr, m_srvHalfRes2.put()));

	return S_OK;
}

HRESULT CD3DGlassRealizer::Initialize(ID3D11Device* device)
{
	RETURN_IF_FAILED(CreateDeviceResources(device));
	RETURN_IF_FAILED(CreateSizeDependentResources(device));

	m_initialized = true;
	return S_OK;
}

HRESULT CD3DGlassRealizer::Tessellate(
	ID2D1Geometry* geometry,
	std::vector<D2D1_VECTOR_4F>& vertices,
	std::vector<WORD>& indices
)
{
	auto tessellationSink = winrt::make_self<CTessellationSink>();
	RETURN_IF_FAILED(
		geometry->Tessellate(
			nullptr,
			1.f,
			tessellationSink.get()
		)
	);

	const auto capacity = tessellationSink->GetTriangles().size() * 3;
	vertices.reserve(capacity);
	indices.reserve(capacity);

	WORD baseVertexIndex{ 0 };
	for (const auto& triangles : tessellationSink->GetTriangles())
	{
		vertices.emplace_back(
			triangles.point1.x,
			triangles.point1.y,
			triangles.point1.x / m_backBufferWidth,
			triangles.point1.y / m_backBufferHeight
		);
		vertices.emplace_back(
			triangles.point2.x,
			triangles.point2.y,
			triangles.point2.x / m_backBufferWidth,
			triangles.point2.y / m_backBufferHeight
		);
		vertices.emplace_back(
			triangles.point3.x,
			triangles.point3.y,
			triangles.point3.x / m_backBufferWidth,
			triangles.point3.y / m_backBufferHeight
		);

		indices.push_back(baseVertexIndex++);
		indices.push_back(baseVertexIndex++);
		indices.push_back(baseVertexIndex++);
	}

	return S_OK;
}

HRESULT CD3DGlassRealizer::Render(
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
)
{
	D3D11_TEXTURE2D_DESC backBufferDesc{};
	backBuffer->GetDesc(&backBufferDesc);

	if (!m_initialized)
	{
		m_backBufferWidth = backBufferDesc.Width;
		m_backBufferHeight = backBufferDesc.Height;
		m_dxgiFormat = backBufferDesc.Format;
		RETURN_IF_FAILED(Initialize(device));
	}
	if (
		m_backBufferWidth != backBufferDesc.Width ||
		m_backBufferHeight != backBufferDesc.Height ||
		m_dxgiFormat != backBufferDesc.Format
	)
	{
		m_backBufferWidth = backBufferDesc.Width;
		m_backBufferHeight = backBufferDesc.Height;
		m_dxgiFormat = backBufferDesc.Format;
		RETURN_IF_FAILED(CreateSizeDependentResources(device));
	}

	std::vector<D2D1_VECTOR_4F> vertices1{};
	std::vector<D2D1_VECTOR_4F> vertices2{};
	std::vector<WORD> indices1{};
	std::vector<WORD> indices2{};

	RETURN_IF_FAILED(
		Tessellate(
			geometry,
			vertices1,
			indices1
		)
	);
	if (!shapeIsRectangles)
	{
		vertices2.reserve(4);
		indices2.reserve(6);

		vertices2.emplace_back(
			samplingWorldbounds.left,
			samplingWorldbounds.top,
			samplingWorldbounds.left / m_backBufferWidth,
			samplingWorldbounds.top / m_backBufferHeight
		);
		vertices2.emplace_back(
			samplingWorldbounds.right,
			samplingWorldbounds.top,
			samplingWorldbounds.right / m_backBufferWidth,
			samplingWorldbounds.top / m_backBufferHeight
		);
		vertices2.emplace_back(
			samplingWorldbounds.right,
			samplingWorldbounds.bottom,
			samplingWorldbounds.right / m_backBufferWidth,
			samplingWorldbounds.bottom / m_backBufferHeight
		);
		vertices2.emplace_back(
			samplingWorldbounds.left,
			samplingWorldbounds.bottom,
			samplingWorldbounds.left / m_backBufferWidth,
			samplingWorldbounds.bottom / m_backBufferHeight
		);

		indices2.push_back(0);
		indices2.push_back(1);
		indices2.push_back(2);
		indices2.push_back(0);
		indices2.push_back(2);
		indices2.push_back(3);
	}
	else
	{
		winrt::com_ptr<ID2D1Factory> factory{};
		winrt::com_ptr<ID2D1PathGeometry> widenedGeometry{};
		geometry->GetFactory(factory.put());
		RETURN_IF_FAILED(factory->CreatePathGeometry(widenedGeometry.put()));
		winrt::com_ptr<ID2D1GeometrySink> sink{};
		RETURN_IF_FAILED(widenedGeometry->Open(sink.put()));
		RETURN_IF_FAILED(
			geometry->Widen(
				7.f,
				nullptr,
				nullptr,
				1.f,
				sink.get()
			)
		);
		RETURN_IF_FAILED(sink->Close());
		ID2D1Geometry* geometries[]{ geometry, widenedGeometry.get() };
		winrt::com_ptr<ID2D1GeometryGroup> geometryGroup{};
		RETURN_IF_FAILED(
			factory->CreateGeometryGroup(
				D2D1_FILL_MODE_WINDING,
				geometries,
				std::size(geometries),
				geometryGroup.put()
			)
		);
		RETURN_IF_FAILED(
			Tessellate(
				geometryGroup.get(),
				vertices2,
				indices2
			)
		);
	}
	if (vertices1.empty())
	{
		return S_OK;
	}

	// Ensure buffer capacities (blur uses vertices2/indices2; compose uses vertices1/indices1)
	RETURN_IF_FAILED(EnsureMeshBuffersCapacity(device, vertices2.size(), indices2.size(), m_vertexBuffer, m_indexBuffer, m_vertexCapacityBlur, m_indexCapacityBlur));
	RETURN_IF_FAILED(EnsureMeshBuffersCapacity(device, vertices1.size(), indices1.size(), m_vertexBufferCompose, m_indexBufferCompose, m_vertexCapacityCompose, m_indexCapacityCompose));

	Util::D3D11ContextStateGuard contextStateGuard(context1);

	UINT stride{ sizeof(D2D1_VECTOR_4F) };
	UINT offset{ 0 };
	context1->IASetInputLayout(m_vertexLayout.get());
	context1->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//----------------------------------------------------------------------------------
	// Update Constant Buffer
	//----------------------------------------------------------------------------------
	CConstantBuffer cb;
	cb.color.x = params.color.r * params.colorBalance;
	cb.color.y = params.color.g * params.colorBalance;
	cb.color.z = params.color.b * params.colorBalance;
	cb.color.w = 1.f;
	cb.afterglow.x = params.afterglow.r * params.afterglowBalance;
	cb.afterglow.y = params.afterglow.g * params.afterglowBalance;
	cb.afterglow.z = params.afterglow.b * params.afterglowBalance;
	cb.afterglow.w = 1.f;
	cb.blurBalance.x = params.blurBalance;

	cb.texelSize.x = 1.f / static_cast<float>(m_backBufferWidth) * 2.f;
	cb.texelSize.y = 1.f / static_cast<float>(m_backBufferHeight) * 2.f;
	cb.viewportSize.x = static_cast<float>(m_backBufferWidth);
	cb.viewportSize.y = static_cast<float>(m_backBufferHeight);
	if (memcmp(&m_cb, &cb, sizeof(cb)) != 0)
	{
		m_cb = cb;
		context1->UpdateSubresource(m_constantBuffer.get(), 0, nullptr, &cb, 0, 0);
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	RETURN_IF_FAILED(context1->Map(m_vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, vertices2.data(), sizeof(D2D1_VECTOR_4F) * vertices2.size());
	context1->Unmap(m_vertexBuffer.get(), 0);

	RETURN_IF_FAILED(context1->Map(m_indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, indices2.data(), sizeof(WORD) * indices2.size());
	context1->Unmap(m_indexBuffer.get(), 0);

	RETURN_IF_FAILED(context1->Map(m_vertexBufferCompose.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, vertices1.data(), sizeof(D2D1_VECTOR_4F) * vertices1.size());
	context1->Unmap(m_vertexBufferCompose.get(), 0);

	RETURN_IF_FAILED(context1->Map(m_indexBufferCompose.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, indices1.data(), sizeof(WORD) * indices1.size());
	context1->Unmap(m_indexBufferCompose.get(), 0);

	D3D11_VIEWPORT vp{};
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	D3D11_RECT clipRect;
	context1->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	context1->VSSetShader(m_vertexShader.get(), nullptr, 0);
	context1->VSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer* const*>(&m_constantBuffer));
	context1->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer* const*>(&m_constantBuffer));
	context1->PSSetSamplers(0, 1, reinterpret_cast<ID3D11SamplerState* const*>(&m_samplerLinear));
	context1->RSSetState(m_rasterizerStateScissor.get());
	
	//----------------------------------------------------------------------------------
	// Pass 1
	//----------------------------------------------------------------------------------
	context1->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer* const*>(&m_vertexBuffer), &stride, &offset);
	context1->IASetIndexBuffer(m_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
	context1->OMSetRenderTargets(1, reinterpret_cast<ID3D11RenderTargetView* const*>(&m_rtvHalfRes1), nullptr);

	vp.Width = static_cast<float>(m_backBufferWidth) * 0.5f;
	vp.Height = static_cast<float>(m_backBufferHeight);
	context1->RSSetViewports(1, &vp);

	context1->PSSetShader(m_pixelShaderBlurH.get(), nullptr, 0);
	context1->PSSetShaderResources(0, 1, reinterpret_cast<ID3D11ShaderResourceView* const*>(&backBufferSRV));

	clipRect = RectF::ToRectL(
		{
			samplingWorldbounds.left * 0.5f,
			samplingWorldbounds.top,
			samplingWorldbounds.right * 0.5f,
			samplingWorldbounds.bottom
		}
	);
	context1->RSSetScissorRects(1, &clipRect);
	context1->DrawIndexed(static_cast<UINT>(indices2.size()), 0, 0);
	{ ID3D11ShaderResourceView* nullSRV[1] = { nullptr }; context1->PSSetShaderResources(0, 1, nullSRV); }


	//----------------------------------------------------------------------------------
	// Pass 2
	//----------------------------------------------------------------------------------
	context1->OMSetRenderTargets(1, reinterpret_cast<ID3D11RenderTargetView* const*>(&m_rtvHalfRes2), nullptr);

	vp.Width = static_cast<float>(m_backBufferWidth) * 0.5f;
	vp.Height = static_cast<float>(m_backBufferHeight) * 0.5f;
	context1->RSSetViewports(1, &vp);

	context1->PSSetShader(m_pixelShaderBlurV.get(), nullptr, 0);
	context1->PSSetShaderResources(0, 1, reinterpret_cast<ID3D11ShaderResourceView* const*>(&m_srvHalfRes1));

	clipRect = RectF::ToRectL(
		{
			drawingWorldbounds.left * 0.5f - 0.5f,
			drawingWorldbounds.top * 0.5f - 0.5f,
			drawingWorldbounds.right * 0.5f + 0.5f,
			drawingWorldbounds.bottom * 0.5f + 0.5f
		}
	);
	context1->RSSetScissorRects(1, &clipRect);
	context1->DrawIndexed(static_cast<UINT>(indices2.size()), 0, 0);
	{ ID3D11ShaderResourceView* nullSRV[1] = { nullptr }; context1->PSSetShaderResources(0, 1, nullSRV); }


	//----------------------------------------------------------------------------------
	// Pass 3
	//----------------------------------------------------------------------------------
	context1->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer* const*>(&m_vertexBufferCompose), &stride, &offset);
	context1->IASetIndexBuffer(m_indexBufferCompose.get(), DXGI_FORMAT_R16_UINT, 0);
	context1->OMSetRenderTargets(1, reinterpret_cast<ID3D11RenderTargetView* const*>(&backBufferRTV), nullptr);

	vp.Width = static_cast<float>(m_backBufferWidth);
	vp.Height = static_cast<float>(m_backBufferHeight);
	context1->RSSetViewports(1, &vp);

	context1->PSSetShader(m_pixelShaderPassthrough.get(), nullptr, 0);
	context1->PSSetShaderResources(0, 1, reinterpret_cast<ID3D11ShaderResourceView* const*>(&m_srvHalfRes2));
	
	clipRect = RectF::ToRectL(drawingWorldbounds);
	context1->RSSetScissorRects(1, &clipRect);
	context1->DrawIndexed(static_cast<UINT>(indices1.size()), 0, 0);

	return S_OK;
}
