#include "pch.h"
#include "HookHelper.hpp"
#include "uDWMProjection.hpp"
#include "dwmcoreProjection.hpp"
#include "Shared.hpp"
#include "GlassKernel.hpp"
#include "GlassRenderer.hpp"
#include "GlassRealizer.hpp"
#include "ReflectionRealizer.hpp"
#include "AeroColorizationEffect.hpp"
#include "D2DPrivates.hpp"
#include "GlassCoverageSet.hpp"

using namespace OpenGlass;

namespace OpenGlass::GlassRenderer
{
	template <typename T>
	HRESULT STDMETHODCALLTYPE MyCRenderData_TryDrawCommandAsDrawList(
		dwmcore::CRenderData* This,
		dwmcore::CDrawingContext* drawingContext,
		int commandType,
		DWM::span<dwmcore::CRenderCommand>* resources,
		bool* succeeded,
		T&& callback
	);
	HRESULT STDMETHODCALLTYPE MyCRenderData_TryDrawCommandAsDrawList_Win10(
		dwmcore::CRenderData* This,
		dwmcore::CDrawingContext* drawingContext,
		dwmcore::CDrawListCache* drawListCache,
		dwmcore::CResource* drawListEntryBuilder,
		bool unknwon,
		int commandType,
		DWM::span<dwmcore::CRenderCommand>* resources,
		bool* succeeded
	);
	HRESULT STDMETHODCALLTYPE MyCRenderData_TryDrawCommandAsDrawList_Win11(
		dwmcore::CRenderData* This,
		dwmcore::CDrawingContext* drawingContext,
		dwmcore::CDrawListCache* drawListCache,
		dwmcore::CResource* drawListEntryBuilder,
		int commandType,
		DWM::span<dwmcore::CRenderCommand>* resources,
		bool* succeeded
	);
	template <typename ...Args>
	HRESULT STDMETHODCALLTYPE MyCRenderData_DrawImageResource_FillMode(
		Args... args,
		const D2D1_RECT_F* srcRect,
		const D2D1_RECT_F* dstRect,
		float opacity
	);
	constexpr auto MyCRenderData_DrawImageResource_FillMode_Win10 = MyCRenderData_DrawImageResource_FillMode<
		dwmcore::CRenderData*,
		dwmcore::CDrawingContext*,
		dwmcore::CDrawListEntryBuilder*,
		bool,
		dwmcore::CImageSource*
	>;
	constexpr auto MyCRenderData_DrawImageResource_FillMode_Win11_Pre_24H2 = MyCRenderData_DrawImageResource_FillMode<
		dwmcore::CRenderData*,
		dwmcore::CDrawingContext*,
		dwmcore::CDrawListEntryBuilder*,
		dwmcore::CImageSource*
	>;
	constexpr auto MyCRenderData_DrawImageResource_FillMode_Win11_24H2 = MyCRenderData_DrawImageResource_FillMode<
		dwmcore::CDrawingContext*,
		dwmcore::CDrawListEntryBuilder*,
		dwmcore::CImageSource*
	>;
	HRESULT STDMETHODCALLTYPE MyCDrawingContext_DrawGeometry(
		dwmcore::IDrawingContext* This,
		dwmcore::CLegacyMilBrush* brush,
		dwmcore::CGeometry* geometry
	);
	void STDMETHODCALLTYPE MyID2D1DeviceContext_FillGeometry(
		ID2D1DeviceContext* This,
		ID2D1Geometry* geometry,
		ID2D1Brush* brush,
		ID2D1Brush* opacityBrush
	);

#ifdef BUILD_BETA
	constexpr bool c_enableWatermarkHook{ true };
#else
	constexpr bool c_enableWatermarkHook{ false };
#endif
	HRESULT STDMETHODCALLTYPE MyCWindowNode_RenderImage(
		dwmcore::CWindowNode* This,
		dwmcore::CDrawingContext* drawingContext,
		dwmcore::CWindowOcclusionInfo* occlusionInfo,
		dwmcore::IBitmapResource* bitmapSource,
		const dwmcore::CShape* shape,
		MARGINS* margins,
		UINT depth
	);
	
	PVOID g_CRenderData_TryDrawCommandAsDrawList_Org{ nullptr };
	PVOID g_CRenderData_DrawImageResource_FillMode_Org{ nullptr };
	decltype(&MyCDrawingContext_DrawGeometry) g_CDrawingContext_DrawGeometry_Org{ nullptr };
	decltype(&MyCDrawingContext_DrawGeometry)* g_CDrawingContext_DrawGeometry_Org_Address{ nullptr };
	decltype(&MyID2D1DeviceContext_FillGeometry) g_ID2D1DeviceContext_FillGeometry_Org{ nullptr };
	decltype(&MyID2D1DeviceContext_FillGeometry)* g_ID2D1DeviceContext_FillGeometry_Org_Address{ nullptr };

	decltype(&MyCWindowNode_RenderImage) g_CWindowNode_RenderImage_Org{ nullptr };
	
	enum RenderFlag
	{
		RenderFlag_Backdrop,
		RenderFlag_Reflection
	};

	struct CDeviceResources
	{
		winrt::com_ptr<ID2D1SolidColorBrush> m_brush{};
		CD2DBuffer m_buffer{};
		CGlassRealizer m_glassRealizer{};
		CReflectionRealizer m_reflectionRealizer{};
		winrt::com_ptr<ID2D1Effect> m_materialEffect{ nullptr };
#ifdef BUILD_BETA
		winrt::com_ptr<ID2D1Bitmap> m_textBitmap{};
		winrt::com_ptr<ID2D1Image> m_textGlowImage{};
#endif
	};
	bool g_shapeIsRectangles{};
	CGlassInput g_glassInput{};
	CReflectionInput g_reflectionInput{};
	D2D1_RECT_F g_drawingWorldBounds{};
	std::bitset<4> g_renderFlag{};

	wil::srwlock g_lock{};
	bool g_disabled{};
	std::unordered_map<dwmcore::CD2DContext*, CDeviceResources> g_deviceResources{};
	CDeviceResources* g_currentDeviceResources{};

	bool g_fixLivePreviewRendering{ false };
	int g_drawGeometryCommandType{ 0 };

	D2D1_RECT_F g_textLayoutBox{};
#ifdef BUILD_BETA
	winrt::com_ptr<IDWriteFactory> g_dwriteFactory{ nullptr };
	winrt::com_ptr<IDWriteTextFormat> g_dwriteTextFormat{ nullptr };
	winrt::com_ptr<IDWriteTextLayout> g_dwriteTextLayout{ nullptr };
#endif

	HRESULT LoadMaterialEffect(ID2D1DeviceContext* context);
	dwmcore::CShapePtr GetShapeRenderingData(
		const dwmcore::CDrawingContext* drawingContext,
		const dwmcore::CShape* shape, 
		const dwmcore::CMILMatrix* matrix,
		D2D1_RECT_F& drawingWorldBounds,
		std::unique_ptr<D2D1_RECT_F[]>& rectangles,
		UINT& rectanglesCount,
		bool& shapeIsRectangles
	)
	{
		shapeIsRectangles = false;
		dwmcore::CShapePtr renderingShape{};
		if (
			FAILED(
				drawingContext->GetUnOccludedWorldShape(
					shape,
					drawingContext->GetD2DContextOwner()->GetCurrentZ(),
					renderingShape.put()
				)
			) ||
			!renderingShape
		) [[unlikely]]
		{
			shape->CopyShape(matrix, renderingShape.put());
		}

		if (renderingShape && !renderingShape->IsEmpty()) [[likely]]
		{
			D2D1_RECT_F shapeWorldBounds;
			shape->GetTightBounds(&shapeWorldBounds, matrix);
			drawingContext->GetClipBoundsWorld(&drawingWorldBounds);
			RectF::IntersectUnsafe(drawingWorldBounds, shapeWorldBounds);
			
			if (renderingShape->IsRectangles(&rectanglesCount)) [[likely]]
			{
				rectangles = std::make_unique_for_overwrite<D2D1_RECT_F[]>(rectanglesCount);
				if (renderingShape->GetRectangles(rectangles.get(), rectanglesCount)) [[likely]]
				{
					shapeIsRectangles = true;
				}
			}

			if (!shapeIsRectangles) [[unlikely]]
			{
				rectanglesCount = 1;
				rectangles = std::make_unique_for_overwrite<D2D1_RECT_F[]>(rectanglesCount);
				renderingShape->GetTightBounds(rectangles.get(), nullptr);
			}
		}
		else
		{
			drawingWorldBounds = {};
			rectangles.reset();
			rectanglesCount = 0;
		}

		return renderingShape;
	}
}

template <typename T>
HRESULT STDMETHODCALLTYPE GlassRenderer::MyCRenderData_TryDrawCommandAsDrawList(
	dwmcore::CRenderData* This,
	dwmcore::CDrawingContext* drawingContext,
	int commandType,
	DWM::span<dwmcore::CRenderCommand>* resources,
	bool* succeeded,
	T&& callback
)
{
	if (
		commandType == g_drawGeometryCommandType &&
		resources->length == sizeof(dwmcore::CDrawGeometryCommand) &&
		HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex]) == dwmcore::CImageLegacyMilBrush::vftable &&
		static_cast<dwmcore::CImageLegacyMilBrush*>(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex])->GetImageSource() &&
		HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->geometryIndex]) == dwmcore::CRectangleGeometry::vftable &&
		static_cast<dwmcore::CImageLegacyMilBrush*>(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex])->GetFloatResource()
	)
	{
		g_fixLivePreviewRendering = true;
	}
	const auto cleanup = wil::scope_exit([]
	{
		g_fixLivePreviewRendering = false;
	});
	if (
		commandType == g_drawGeometryCommandType &&
		resources->length == sizeof(dwmcore::CDrawGeometryCommand) &&
		(
			(
				HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex]) == dwmcore::CSolidColorLegacyMilBrush::vftable
			) ||
			(
				HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex]) == dwmcore::CImageLegacyMilBrush::vftable &&
				static_cast<dwmcore::CImageLegacyMilBrush*>(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex])->GetImageSource() == nullptr
			)
		) &&
		(
			HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->geometryIndex]) == dwmcore::CRegionGeometry::vftable ||
			HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->geometryIndex]) == dwmcore::CRectangleGeometry::vftable ||
			HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->geometryIndex]) == dwmcore::CCombinedGeometry::vftable
		)
	)
	{
		if (!g_CDrawingContext_DrawGeometry_Org)
		{
			g_CDrawingContext_DrawGeometry_Org_Address = reinterpret_cast<decltype(g_CDrawingContext_DrawGeometry_Org_Address)>(&(HookHelper::vftbl_of(drawingContext->GetInterface())[4]));
			HookHelper::WritePointer(
				g_CDrawingContext_DrawGeometry_Org_Address, 
				MyCDrawingContext_DrawGeometry,
				&g_CDrawingContext_DrawGeometry_Org
			);
		}

		*succeeded = false;
		return S_OK;
	}

	return callback();
}
HRESULT STDMETHODCALLTYPE GlassRenderer::MyCRenderData_TryDrawCommandAsDrawList_Win10(
	dwmcore::CRenderData* This,
	dwmcore::CDrawingContext* drawingContext,
	dwmcore::CDrawListCache* drawListCache,
	dwmcore::CResource* drawListEntryBuilder,
	bool unknwon,
	int commandType,
	DWM::span<dwmcore::CRenderCommand>* resources,
	bool* succeeded
)
{
	return MyCRenderData_TryDrawCommandAsDrawList(
		This,
		drawingContext,
		commandType,
		resources,
		succeeded,
		[=]()
		{
			return (reinterpret_cast<decltype(&MyCRenderData_TryDrawCommandAsDrawList_Win10)>(g_CRenderData_TryDrawCommandAsDrawList_Org))(
				This,
				drawingContext,
				drawListCache,
				drawListEntryBuilder,
				unknwon,
				commandType,
				resources,
				succeeded
			);
		}
	);
}
HRESULT STDMETHODCALLTYPE GlassRenderer::MyCRenderData_TryDrawCommandAsDrawList_Win11(
	dwmcore::CRenderData* This,
	dwmcore::CDrawingContext* drawingContext,
	dwmcore::CDrawListCache* drawListCache,
	dwmcore::CResource* drawListEntryBuilder,
	int commandType,
	DWM::span<dwmcore::CRenderCommand>* resources,
	bool* succeeded
)
{
	return MyCRenderData_TryDrawCommandAsDrawList(
		This,
		drawingContext,
		commandType,
		resources,
		succeeded,
		[=]()
		{
			return (reinterpret_cast<decltype(&MyCRenderData_TryDrawCommandAsDrawList_Win11)>(g_CRenderData_TryDrawCommandAsDrawList_Org))(
				This,
				drawingContext,
				drawListCache,
				drawListEntryBuilder,
				commandType,
				resources,
				succeeded
			);
		}
	);
}

template <typename ...Args>
HRESULT STDMETHODCALLTYPE GlassRenderer::MyCRenderData_DrawImageResource_FillMode(
	Args... args,
	const D2D1_RECT_F* srcRect,
	const D2D1_RECT_F* dstRect,
	float opacity
)
{
	// The DWM team changed the implementation of dwmcore!CRenderData::TryDrawCommandAsDrawList, 
	// which is why Aero Peek is glitching since Windows 10 1803. 
	// 
	// https://github.com/microsoft/Windows-Dev-Performance/issues/12
	// 
	// I still can't really figure out 
	// why they're passing the geometry's bounding rectangle to srcRect parameter.
	// 
	// But since Windows 11 24H2, 
	// they stop this buggy behavior when MilStretch is set to Uniform or UniformToFill.
	if (g_fixLivePreviewRendering)
	{
		srcRect = nullptr;
	}

	return (reinterpret_cast<decltype(&MyCRenderData_DrawImageResource_FillMode<Args...>)>(g_CRenderData_DrawImageResource_FillMode_Org))(
		args...,
		srcRect,
		dstRect,
		opacity
	);
}

HRESULT STDMETHODCALLTYPE GlassRenderer::MyCDrawingContext_DrawGeometry(
	dwmcore::IDrawingContext* This,
	dwmcore::CLegacyMilBrush* brush,
	dwmcore::CGeometry* geometry
)
{
	dwmcore::CShapePtr geometryShape{};
	if (
		FAILED(geometry->GetShapeData(nullptr, &geometryShape)) ||
		!geometryShape ||
		geometryShape->IsEmpty()
	)
	{
		return S_OK;
	}

	const auto drawingContext = This->GetDrawingContext();
	const auto occlusionConctext = drawingContext->GetOcclusionContext();
	const auto d2dContext = drawingContext->GetD2DContext();
	const auto context = d2dContext->GetDeviceContext();
	const auto matrix = drawingContext->GetWorldTransform();
	const auto currentVisual = drawingContext->GetCurrentVisualHelper();
	const auto currentVisualTree = drawingContext->GetCurrentVisualTree();
	const auto desktopTree = HookHelper::vftbl_of(currentVisualTree) != dwmcore::CDesktopTree::vftable ? currentVisual->GetDesktopTree() : currentVisualTree;
	
	if (!context)
	{
		return S_OK;
	}

	UINT rectanglesCount;
	std::unique_ptr<D2D1_RECT_F[]> rectangles;

	const auto renderingShape = GetShapeRenderingData(
		drawingContext,
		geometryShape.get(),
		matrix,
		g_drawingWorldBounds,
		rectangles,
		rectanglesCount,
		g_shapeIsRectangles
	);

	if (
		occlusionConctext &&
		occlusionConctext->GetArrayBasedCoverageSet()->IsCovered(
			g_drawingWorldBounds,
			drawingContext->GetD2DContextOwner()->GetCurrentZ()
		)
	)
	{
		return S_OK;
	}

	d2dContext->EnsureBeginDraw();
	const auto lockScope = g_lock.lock_exclusive();
	if (g_disabled)
	{
		return S_OK;
	}

	g_currentDeviceResources = &g_deviceResources.try_emplace(d2dContext).first->second;
	RETURN_IF_FAILED(
		drawingContext->PushTransformInternal(
			nullptr,
			dwmcore::CMILMatrix::Identity,
			false,
			true
		)
	);
	const auto transformScope = wil::scope_exit([drawingContext]
	{
		drawingContext->PopTransformInternal(true);
		g_currentDeviceResources = nullptr;
	});
	
	if (HookHelper::vftbl_of(brush) == dwmcore::CImageLegacyMilBrush::vftable)
	{
		const auto imageBrush = static_cast<dwmcore::CImageLegacyMilBrush*>(brush);
		const auto opacity = imageBrush->GetOpacityValue();

		if (opacity == 0.f)
		{
			return S_OK;
		}
		
		g_reflectionInput.intensity = opacity;
		g_reflectionInput.worldTransform = matrix;
		g_reflectionInput.viewport = &imageBrush->GetViewport();
		g_reflectionInput.rectangles = std::span{ rectangles.get(), rectanglesCount };
		g_renderFlag.set(RenderFlag_Reflection, true);
	}

	const auto renderTargetBitmap = Util::GetTargetBitmapFromDeviceContext(context);

	if (HookHelper::vftbl_of(brush) == dwmcore::CSolidColorLegacyMilBrush::vftable)
	{
		const auto solidColorBrush = static_cast<dwmcore::CSolidColorLegacyMilBrush*>(brush);
		const auto opacity = solidColorBrush->GetRealizedOpacity();
		auto color = Color::scRGBTosRGB(solidColorBrush->GetRealizedColor());

		if (color.a == 0.f || opacity == 0.f)
		{
			return S_OK;
		}

		const auto extendedAmount = !desktopTree || GlassKernel::IsCurrentCVIFullyTransparent() ? 0.f : GlassKernel::GetBlurExtendedAmount();
		const auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(occlusionConctext);
		
		// try render glass
		if (
			opacity == 1.f &&
			(color.a == 0.50f || color.a == 0.25f)
		)
		{
			const auto active = (color.a == 0.50f);
			
			D2D1_RECT_F shapeWorldBounds;
			RETURN_IF_FAILED(geometryShape->GetTightBounds(&shapeWorldBounds, matrix));

			// uDWM!CGlassColorizationParameters::AdjustWindowColorization (Windows 7)
			float glassOpacity, blurBalance, afterglowBalance, colorBalance;
			GlassKernel::CalculateRealizedAeroParams(
				active,
				extendedAmount,
				glassOpacity,
				blurBalance,
				afterglowBalance,
				&colorBalance
			);
			if (
				const auto opaque = Shared::IsGlassFullyOpaque(
					glassOpacity,
					blurBalance,
					afterglowBalance
				);
				!opaque &&
				extendedAmount &&
				renderTargetBitmap &&
				glassCoverageSet
			)
			{
				RETURN_IF_FAILED(drawingContext->FlushD2D());
				if (
					glassCoverageSet->IsFullyCovered(
						shapeWorldBounds,
						drawingContext->GetD2DContextOwner()->GetCurrentZ()
					) ||
					GlassKernel::IsCVIPresent()
				)
				{
					g_glassInput.params.extraScaleAmount = 0.8f;
					g_glassInput.params.prescaleInteroplation = D2D1_SCALE_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
					g_glassInput.params.optimization = D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED;
					g_glassInput.drawImageInterpolationMode = D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
				}
				else
				{
					g_glassInput.params.extraScaleAmount = 1.f;
					g_glassInput.params.prescaleInteroplation = D2D1_SCALE_INTERPOLATION_MODE_FORCE_DWORD;
					g_glassInput.params.optimization = Shared::g_blurOptimization;
					g_glassInput.drawImageInterpolationMode = D2D1_INTERPOLATION_MODE_LINEAR;
				}

				g_glassInput.sourceBitmap = renderTargetBitmap.get();
				g_glassInput.rectangles = std::span{ rectangles.get(), rectanglesCount };
				g_glassInput.buffer = &g_currentDeviceResources->m_buffer;
				// effect settings
				g_glassInput.params.blurAmount = Shared::g_blurAmount;
				g_glassInput.params.color = color;
				g_glassInput.params.color.a = 1.f;
				g_glassInput.params.colorBalance = (Shared::g_type == Shared::GlassType::Blur) ? glassOpacity : colorBalance;
				g_glassInput.params.afterglow = Shared::g_afterglow;
				g_glassInput.params.afterglowBalance = afterglowBalance;
				g_glassInput.params.blurBalance = blurBalance;

				g_renderFlag.set(RenderFlag_Backdrop, true);
			}
			else
			{
				if (Shared::g_type == Shared::GlassType::Blur)
				{
					color.a = opaque ? 1.f : glassOpacity;
				}

				#pragma warning(suppress:26813)
				if (Shared::g_type == Shared::GlassType::Aero)
				{
					// dwmcore!CCapturedGlassColorizationParameters::GetEffectivescRGBBlendColor (Windows 7)
					if (opaque)
					{
						color.r *= colorBalance;
						color.g *= colorBalance;
						color.b *= colorBalance;
						color.a = 1.f;
					}
					else
					{
						const auto alpha = std::max(1.f - blurBalance, 0.1f);
						color =
						{
							std::clamp((color.r * colorBalance + Shared::g_afterglow.r * afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
							std::clamp((color.g * colorBalance + Shared::g_afterglow.g * afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
							std::clamp((color.b * colorBalance + Shared::g_afterglow.b * afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
							alpha
						};
					}
				}
			}
		}

		if (!g_renderFlag.any())
		{
			color.a *= opacity;
			if (FAILED(drawingContext->FillShapeWithSolidColor(renderingShape.get(), color)))
			{
				if (!g_currentDeviceResources->m_brush)
				{
					context->CreateSolidColorBrush({}, g_currentDeviceResources->m_brush.put());
				}
				g_currentDeviceResources->m_brush->SetColor(color);
				drawingContext->FillShapeWithBrush(renderingShape.get(), g_currentDeviceResources->m_brush.get());
			}
		}
	}

	if (g_renderFlag.any())
	{
		if (!g_ID2D1DeviceContext_FillGeometry_Org)
		{
			g_ID2D1DeviceContext_FillGeometry_Org_Address = reinterpret_cast<decltype(g_ID2D1DeviceContext_FillGeometry_Org_Address)>(&HookHelper::vftbl_of(context)[0x17]);
			HookHelper::WritePointer(
				g_ID2D1DeviceContext_FillGeometry_Org_Address, 
				MyID2D1DeviceContext_FillGeometry,
				&g_ID2D1DeviceContext_FillGeometry_Org
			);
		}

		drawingContext->FillShapeWithBrush(renderingShape.get(), nullptr);
	}

	return S_OK;
}

void STDMETHODCALLTYPE GlassRenderer::MyID2D1DeviceContext_FillGeometry(
	ID2D1DeviceContext* This,
	ID2D1Geometry* geometry,
	ID2D1Brush* brush,
	ID2D1Brush* opacityBrush
)
{
	if (brush || !g_currentDeviceResources)
	{
		return g_ID2D1DeviceContext_FillGeometry_Org(This, geometry, brush, opacityBrush);
	}

	const auto primitiveBlend = This->GetPrimitiveBlend();
	const auto antialiasMode = This->GetAntialiasMode();
	This->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
	This->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

	bool ignoreLayer{ g_shapeIsRectangles };
	if (!ignoreLayer)
	{
		This->PushLayer(
			D2D1::LayerParameters1(
				g_drawingWorldBounds,
				geometry,
				D2D1_ANTIALIAS_MODE_ALIASED,
				D2D1::IdentityMatrix(),
				1.f,
				nullptr,
				D2D1_LAYER_OPTIONS1_INITIALIZE_FROM_BACKGROUND |
				(
					g_renderFlag.test(RenderFlag_Backdrop) ?
					D2D1_LAYER_OPTIONS1_IGNORE_ALPHA :
					D2D1_LAYER_OPTIONS1_NONE
				)
			),
			nullptr
		);
	}
	if (g_renderFlag.test(RenderFlag_Backdrop))
	{
		LOG_IF_FAILED(
			g_currentDeviceResources->m_glassRealizer.Render(
				This,
				g_glassInput
			)
		);

		if (Shared::g_materialIntensity)
		{
			winrt::com_ptr<ID2D1DeviceContext6> contextForBlending{};
			THROW_IF_FAILED(This->QueryInterface(contextForBlending.put()));

			HRESULT hr{ S_OK };
			if (!g_currentDeviceResources->m_materialEffect)
			{
				hr = LoadMaterialEffect(This);
			}

			if (SUCCEEDED(hr))
			{
				LOG_IF_FAILED(
					g_currentDeviceResources->m_materialEffect->SetValue(
						D2D1_OPACITY_PROP_OPACITY,
						Shared::g_materialIntensity
					)
				);
				winrt::com_ptr<ID2D1Image> materialImage{};
				g_currentDeviceResources->m_materialEffect->GetOutput(materialImage.put());

				for (const auto& subRectangle : g_glassInput.rectangles)
				{
					D2D1_POINT_2F targetOffset
					{
						subRectangle.left,
						subRectangle.top
					};
					contextForBlending->BlendImage(
						materialImage.get(),
						D2D1_BLEND_MODE_MULTIPLY,
						&targetOffset,
						&subRectangle,
						D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
					);
				}
			}
			else
			{
				LOG_HR(hr);
			}
		}
	}
	if (g_renderFlag.test(RenderFlag_Reflection))
	{
		const auto primitiveBlendPreReflection = This->GetPrimitiveBlend();
		This->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
		LOG_IF_FAILED(
			g_currentDeviceResources->m_reflectionRealizer.Render(
				This,
				g_reflectionInput
			)
		);
		This->SetPrimitiveBlend(primitiveBlendPreReflection);
	}
	if (!ignoreLayer)
	{
		This->PopLayer();
	}

	This->SetAntialiasMode(antialiasMode);
	This->SetPrimitiveBlend(primitiveBlend);

	g_renderFlag.reset();
}

HRESULT STDMETHODCALLTYPE GlassRenderer::MyCWindowNode_RenderImage(
	dwmcore::CWindowNode* This,
	dwmcore::CDrawingContext* drawingContext,
	dwmcore::CWindowOcclusionInfo* occlusionInfo,
	dwmcore::IBitmapResource* bitmapSource,
	const dwmcore::CShape* shape,
	MARGINS* margins,
	UINT depth
)
{
	const auto hr = g_CWindowNode_RenderImage_Org(
		This,
		drawingContext,
		occlusionInfo,
		bitmapSource,
		shape,
		margins,
		depth
	);

#ifdef BUILD_BETA
	const auto hwnd = This->GetHwnd();
	if (
		(
			RegisterWindowMessageW(L"WorkerW") == GetClassWord(hwnd, GCW_ATOM) ||
			RegisterWindowMessageW(L"Progman") == GetClassWord(hwnd, GCW_ATOM)
		) &&
		FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr)
	)
	{
		LOG_IF_FAILED(drawingContext->FlushD2D());

		const auto d2dContext = drawingContext->GetD2DContext();
		const auto context = d2dContext->GetDeviceContext();
		const auto lockScope = g_lock.lock_exclusive();
		if (g_disabled)
		{
			return S_OK;
		}

		winrt::com_ptr<ID2D1Effect> textGlowEffect{};
		winrt::com_ptr<ID2D1Effect> textMorphologyEffect{};
		auto& deviceResources = g_deviceResources.try_emplace(d2dContext).first->second;
		if (!deviceResources.m_textBitmap || !deviceResources.m_textGlowImage)
		{
			THROW_IF_FAILED(
				context->CreateEffect(
					CLSID_D2D1Morphology,
					textMorphologyEffect.put()
				)
			);
			THROW_IF_FAILED(
				textMorphologyEffect->SetValue(
					D2D1_MORPHOLOGY_PROP_MODE,
					D2D1_MORPHOLOGY_MODE_DILATE
				)
			);
			THROW_IF_FAILED(
				textMorphologyEffect->SetValue(
					D2D1_MORPHOLOGY_PROP_WIDTH,
					3
				)
			);
			THROW_IF_FAILED(
				textMorphologyEffect->SetValue(
					D2D1_MORPHOLOGY_PROP_HEIGHT,
					3
				)
			);

			THROW_IF_FAILED(
				context->CreateEffect(
					CLSID_D2D1Shadow,
					textGlowEffect.put()
				)
			);
			THROW_IF_FAILED(
				textGlowEffect->SetValue(
					D2D1_PROPERTY_CACHED,
					TRUE
				)
			);
			THROW_IF_FAILED(
				textGlowEffect->SetValue(
					D2D1_SHADOW_PROP_OPTIMIZATION,
					D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED
				)
			);
			THROW_IF_FAILED(
				textGlowEffect->SetValue(
					D2D1_SHADOW_PROP_COLOR,
					D2D1::ColorF(D2D1::ColorF::Black)
				)
			);
			THROW_IF_FAILED(
				textGlowEffect->SetValue(
					D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION,
					4.f
				)
			);
			textGlowEffect->SetInputEffect(0, textMorphologyEffect.get());

			winrt::com_ptr<ID2D1BitmapRenderTarget> bitmapRT{};
			THROW_IF_FAILED(
				context->CreateCompatibleRenderTarget(
					D2D1::SizeF(
						wil::rect_width(g_textLayoutBox) + 24.f,
						wil::rect_height(g_textLayoutBox) + 24.f
					),
					bitmapRT.put()
				)
			);
			winrt::com_ptr<ID2D1SolidColorBrush> watermarkBrush{};
			THROW_IF_FAILED(bitmapRT->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), watermarkBrush.put()));

			bitmapRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
			bitmapRT->BeginDraw();
			bitmapRT->Clear();
			bitmapRT->DrawTextLayout(
				D2D1::Point2F(12.f, 12.f),
				g_dwriteTextLayout.get(),
				watermarkBrush.get(),
				D2D1_DRAW_TEXT_OPTIONS_NONE
			);

			THROW_IF_FAILED(bitmapRT->EndDraw());
			THROW_IF_FAILED(bitmapRT->GetBitmap(deviceResources.m_textBitmap.put()));
			textMorphologyEffect->SetInput(0, deviceResources.m_textBitmap.get());
			textGlowEffect->GetOutput(deviceResources.m_textGlowImage.put());
		}

		MONITORINFO monitorInfo{ sizeof(monitorInfo) };
		THROW_IF_WIN32_BOOL_FALSE(GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitorInfo));
		D2D1_POINT_2F origin
		{
			static_cast<float>(monitorInfo.rcWork.right) - 9.f - wil::rect_width(g_textLayoutBox) - 24.f,
			static_cast<float>(monitorInfo.rcWork.bottom) - 9.f - wil::rect_height(g_textLayoutBox) - 24.f,
		};
		context->DrawImage(
			deviceResources.m_textGlowImage.get(),
			&origin,
			nullptr,
			D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
		);
		context->DrawImage(
			deviceResources.m_textBitmap.get(),
			&origin,
			nullptr,
			D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
		);
	}
#endif

	return hr;
}

HRESULT GlassRenderer::LoadMaterialEffect(ID2D1DeviceContext* context)
{
	winrt::com_ptr<IStream> stream{ nullptr };
	if (
		Shared::g_materialTexturePath.empty() ||
		!PathFileExistsW(Shared::g_materialTexturePath.data())
	)
	{
		wil::unique_hmodule wucModule
		{ 
			LoadLibraryExW(
				L"Windows.UI.Xaml.Controls.dll",
				nullptr,
				LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_SEARCH_SYSTEM32
			) 
		};
		const auto resourceHandle = FindResourceW(wucModule.get(), MAKEINTRESOURCE(2000), RT_RCDATA);
		RETURN_LAST_ERROR_IF_NULL(resourceHandle);
		const auto globalHandle = LoadResource(wucModule.get(), resourceHandle);
		RETURN_LAST_ERROR_IF_NULL(globalHandle);
		const auto cleanup = wil::scope_exit([=]
		{
			if (globalHandle)
			{
				UnlockResource(globalHandle);
				FreeResource(globalHandle);
			}
		});
		const auto resourceSize = SizeofResource(wucModule.get(), resourceHandle);
		RETURN_LAST_ERROR_IF(resourceSize == 0);
		const auto resourceAddress = reinterpret_cast<PBYTE>(LockResource(globalHandle));
		stream = { SHCreateMemStream(resourceAddress, resourceSize), winrt::take_ownership_from_abi };
	}
	else
	{
		wil::unique_hfile file{ CreateFileW(Shared::g_materialTexturePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0) };
		RETURN_LAST_ERROR_IF(!file.is_valid());

		LARGE_INTEGER fileSize{};
		RETURN_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &fileSize));

		auto buffer{ std::make_unique<BYTE[]>(static_cast<size_t>(fileSize.QuadPart)) };
		RETURN_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), buffer.get(), static_cast<DWORD>(fileSize.QuadPart), nullptr, nullptr));
		stream = { SHCreateMemStream(buffer.get(), static_cast<UINT>(fileSize.QuadPart)), winrt::take_ownership_from_abi };
	}
	RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);

	winrt::com_ptr<IWICImagingFactory2> wicFactory{ nullptr };
	wicFactory.copy_from(uDWM::CDesktopManager::GetInstance()->GetWICFactory());
	winrt::com_ptr<IWICBitmapDecoder> wicDecoder{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateDecoderFromStream(stream.get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, wicDecoder.put()));
	winrt::com_ptr<IWICBitmapFrameDecode> wicFrame{ nullptr };
	RETURN_IF_FAILED(wicDecoder->GetFrame(0, wicFrame.put()));
	winrt::com_ptr<IWICFormatConverter> wicConverter{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
	RETURN_IF_FAILED(
		wicConverter->Initialize(
			wicFrame.get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0, 
			WICBitmapPaletteTypeCustom
		)
	);

	winrt::com_ptr<ID2D1Effect> bitmapSourceEffect{ nullptr };
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1BitmapSource,
			bitmapSourceEffect.put()
		)
	);
	RETURN_IF_FAILED(
		bitmapSourceEffect->SetValue(
			D2D1_BITMAPSOURCE_PROP_ALPHA_MODE,
			D2D1_ALPHA_MODE_PREMULTIPLIED
		)
	);
	RETURN_IF_FAILED(
		bitmapSourceEffect->SetValue(
			D2D1_PROPERTY_CACHED,
			TRUE
		)
	);
	RETURN_IF_FAILED(
		bitmapSourceEffect->SetValue(
			D2D1_BITMAPSOURCE_PROP_WIC_BITMAP_SOURCE,
			wicConverter.get()
		)
	);

	winrt::com_ptr<ID2D1Effect> tileEffect{ nullptr };
	RETURN_IF_FAILED(
		context->CreateEffect(
			CLSID_D2D1Border,
			tileEffect.put()
		)
	);
	RETURN_IF_FAILED(
		tileEffect->SetValue(
			D2D1_BORDER_PROP_EDGE_MODE_X,
			D2D1_BORDER_EDGE_MODE_WRAP
		)
	);
	RETURN_IF_FAILED(
		tileEffect->SetValue(
			D2D1_BORDER_PROP_EDGE_MODE_Y,
			D2D1_BORDER_EDGE_MODE_WRAP
		)
	);
	tileEffect->SetInputEffect(0, bitmapSourceEffect.get());

	if (!g_currentDeviceResources->m_materialEffect)
	{
		RETURN_IF_FAILED(
			context->CreateEffect(
				CLSID_D2D1Opacity,
				g_currentDeviceResources->m_materialEffect.put()
			)
		);
		RETURN_IF_FAILED(
			g_currentDeviceResources->m_materialEffect->SetValue(
				D2D1_OPACITY_PROP_OPACITY,
				Shared::g_materialIntensity
			)
		);
		g_currentDeviceResources->m_materialEffect->SetInputEffect(0, tileEffect.get());
	}

	return S_OK;
}

void GlassRenderer::DestroyDeviceResources(dwmcore::CD2DContext* d2dContext)
{
	g_deviceResources.erase(d2dContext);
}

void GlassRenderer::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Theme)
	{
		WCHAR materialTexturePath[MAX_PATH]{};
		GlassEngine::GetStringFromRegistry(L"CustomThemeMaterial", materialTexturePath);
		PathUnquoteSpacesW(materialTexturePath);
		Shared::g_materialTexturePath.assign(materialTexturePath);

		const auto lockScope = g_lock.lock_exclusive();
		for (auto& [_, deviceResources] : g_deviceResources)
		{
			deviceResources.m_reflectionRealizer.Reset();
			deviceResources.m_materialEffect = nullptr;
		}
	}
	if (type & GlassEngine::UpdateType::Backdrop)
	{
		auto value = GlassEngine::GetDwordFromRegistry(L"GlassOpacity", 63);
		Shared::g_glassOpacity = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);
		Shared::g_glassOpacityInactive = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"GlassOpacityInactive", value)) / 100.f, 0.f, 1.f);
		
		value = GlassEngine::GetDwordFromRegistry(L"ColorizationAfterglowOverride", GlassEngine::GetDwordFromRegistry(L"ColorizationAfterglow"));
		Shared::g_afterglow = Color::FromArgb(value);

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationBlurBalanceOverride", GlassEngine::GetDwordFromRegistry(L"ColorizationBlurBalance"));
		Shared::g_blurBalance = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationColorBalanceOverride", GlassEngine::GetDwordFromRegistry(L"ColorizationColorBalance"));
		Shared::g_colorBalance = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);

		value = GlassEngine::GetDwordFromRegistry(L"ColorizationAfterglowBalanceOverride", GlassEngine::GetDwordFromRegistry(L"ColorizationAfterglowBalance"));
		Shared::g_afterglowBalance = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);
	
		value = GlassEngine::GetDwordFromRegistry(L"MaterialOpacity");
		Shared::g_materialIntensity = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);

		value = GlassEngine::GetDwordFromRegistry(L"ZeroCopyOptimization", 1);
		g_glassInput.zeroCopyOptimization = static_cast<bool>(value);
		g_glassInput.params.cachePrescaledImage = static_cast<bool>(value);
	}
}

void GlassRenderer::Startup()
{
	THROW_IF_FAILED(CAeroColorizationEffect::Register(*dwmcore::g_DeviceManager));

	dwmcore::g_projectionArray.ApplyToVariable("CRenderData::TryDrawCommandAsDrawList", g_CRenderData_TryDrawCommandAsDrawList_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CRenderData::DrawImageResource_FillMode", g_CRenderData_DrawImageResource_FillMode_Org);
	if constexpr (c_enableWatermarkHook)
	{
		dwmcore::g_projectionArray.ApplyToVariable("CWindowNode::RenderImage", g_CWindowNode_RenderImage_Org);
	}

	if (!g_drawGeometryCommandType)
	{
		switch (dwmcore::g_buildNumber)
		{
		case os::build_w10_2004:
		{
			g_drawGeometryCommandType = 461;
			break;
		}
		case os::build_w11_21h2:
		{
			g_drawGeometryCommandType = 456;
			break;
		}
		case os::build_w11_22h2:
		{
			g_drawGeometryCommandType = 444;
			break;
		}
		// Microsoft has been changing things so frequently since 24H2 that
		// the command types can be completely different under the same build number, 
		// they are absolutely insane
		case os::build_w11_24h2:
		{
			break;
		}
		default:
			break;
		}
	}
	if (!g_drawGeometryCommandType)
	{
		UCHAR* CRenderDataBuilder_DrawGeometry_Instructions{ nullptr };
		dwmcore::g_projectionArray.ApplyToVariable("CRenderDataBuilder::DrawGeometry", CRenderDataBuilder_DrawGeometry_Instructions);

		UCHAR g_movDrawGeometryCommandType_Instructions[]
		{
			// mov dword ptr [rdx+rcx+4], ???
			0xC7, 0x44, 0x0A, 0x04, // 0x00, 0x00, 0x00, 0x00
		};
		auto i = 128;
		do
		{
			
			if (
				memcmp(
					CRenderDataBuilder_DrawGeometry_Instructions,
					g_movDrawGeometryCommandType_Instructions,
					sizeof(g_movDrawGeometryCommandType_Instructions)
				) == 0
			)
			{
				g_drawGeometryCommandType = *reinterpret_cast<int*>(CRenderDataBuilder_DrawGeometry_Instructions + sizeof(g_movDrawGeometryCommandType_Instructions));
				break;
			}

			CRenderDataBuilder_DrawGeometry_Instructions += 1;
			i--;
		} while (i);
	}
	g_glassInput.drawingWorldBounds = &g_drawingWorldBounds;

#ifdef BUILD_BETA
	THROW_IF_FAILED(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(g_dwriteFactory.put())
		)
	);
	THROW_IF_FAILED(
		g_dwriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			18.f,
			L"en-us",
			g_dwriteTextFormat.put()
		)
	);
	THROW_IF_FAILED(g_dwriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));
	THROW_IF_FAILED(g_dwriteTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
	std::wstring watermarkText
	{
		L"Aero Glass For Windows 10+ (Beta 01) \n"
		L"For testing purposes only. Do not distribute. \n"
		L"https://github.com/ALTaleX531/OpenGlass "
	};
	THROW_IF_FAILED(
		g_dwriteFactory->CreateTextLayout(
			watermarkText.c_str(),
			static_cast<UINT32>(watermarkText.size()),
			g_dwriteTextFormat.get(),
			0.f,
			0.f,
			g_dwriteTextLayout.put()
		)
	);
	DWRITE_TEXT_METRICS metrics{};
	THROW_IF_FAILED(g_dwriteTextLayout->GetMetrics(&metrics));
	g_textLayoutBox = 
	{
		0.f, 
		0.f,
		metrics.widthIncludingTrailingWhitespace,
		metrics.height
	};
	THROW_IF_FAILED(g_dwriteTextLayout->SetMaxWidth(wil::rect_width(g_textLayoutBox)));
#endif
	
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win10);
			}
			else
			{
				HookHelper::Detours::Attach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win11);
			}

			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(
					&g_CRenderData_DrawImageResource_FillMode_Org,
					MyCRenderData_DrawImageResource_FillMode_Win10
				);
			}
			else if (dwmcore::g_buildNumber < os::build_w11_24h2)
			{
				HookHelper::Detours::Attach(
					&g_CRenderData_DrawImageResource_FillMode_Org,
					MyCRenderData_DrawImageResource_FillMode_Win11_Pre_24H2
				);
			}
			else
			{
				HookHelper::Detours::Attach(
					&g_CRenderData_DrawImageResource_FillMode_Org,
					MyCRenderData_DrawImageResource_FillMode_Win11_24H2
				);
			}

			if constexpr (c_enableWatermarkHook)
			{
				HookHelper::Detours::Attach(&g_CWindowNode_RenderImage_Org, MyCWindowNode_RenderImage);
			}
		})
	);
}

void GlassRenderer::Shutdown()
{
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win10);
			}
			else
			{
				HookHelper::Detours::Detach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win11);
			}

			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(
					&g_CRenderData_DrawImageResource_FillMode_Org,
					MyCRenderData_DrawImageResource_FillMode_Win10
				);
			}
			else if (dwmcore::g_buildNumber < os::build_w11_24h2)
			{
				HookHelper::Detours::Detach(
					&g_CRenderData_DrawImageResource_FillMode_Org,
					MyCRenderData_DrawImageResource_FillMode_Win11_Pre_24H2
				);
			}
			else
			{
				HookHelper::Detours::Detach(
					&g_CRenderData_DrawImageResource_FillMode_Org,
					MyCRenderData_DrawImageResource_FillMode_Win11_24H2
				);
			}

			if constexpr (c_enableWatermarkHook)
			{
				HookHelper::Detours::Detach(&g_CWindowNode_RenderImage_Org, MyCWindowNode_RenderImage);
			}
		})
	);

#ifdef BUILD_BETA
	g_dwriteTextLayout = nullptr;
	g_dwriteTextFormat = nullptr;
	g_dwriteFactory = nullptr;
#endif

	if (g_ID2D1DeviceContext_FillGeometry_Org)
	{
		HookHelper::WritePointer(
			g_ID2D1DeviceContext_FillGeometry_Org_Address, 
			g_ID2D1DeviceContext_FillGeometry_Org
		);
	}
	if (g_CDrawingContext_DrawGeometry_Org)
	{
		HookHelper::WritePointer(
			g_CDrawingContext_DrawGeometry_Org_Address, 
			g_CDrawingContext_DrawGeometry_Org
		);
	}

	const auto lockScope = g_lock.lock_exclusive();
	g_disabled = true;
	g_deviceResources.clear();
	THROW_IF_FAILED(CAeroColorizationEffect::UnRegister(*dwmcore::g_DeviceManager));
}
