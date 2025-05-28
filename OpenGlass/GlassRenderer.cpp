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
	void STDMETHODCALLTYPE MyCGeometry_Destructor(
		dwmcore::CGeometry* This
	);
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
	
	PVOID g_CRenderData_TryDrawCommandAsDrawList_Org{ nullptr };
	decltype(&MyCGeometry_Destructor) g_CGeometry_Destructor_Org{ nullptr };
	decltype(&MyCDrawingContext_DrawGeometry) g_CDrawingContext_DrawGeometry_Org{ nullptr };
	decltype(&MyCDrawingContext_DrawGeometry)* g_CDrawingContext_DrawGeometry_Org_Address{ nullptr };
	decltype(&MyID2D1DeviceContext_FillGeometry) g_ID2D1DeviceContext_FillGeometry_Org{ nullptr };
	decltype(&MyID2D1DeviceContext_FillGeometry)* g_ID2D1DeviceContext_FillGeometry_Org_Address{ nullptr };
	
	enum RenderFlag
	{
		RenderFlag_Backdrop,
		RenderFlag_Reflection
	};

	bool g_shapeIsRectangles{};
	GlassInput g_glassInput{};
	ReflectionInput g_reflectionInput{};
	std::bitset<2> g_renderFlag{};
	ID2D1Device* g_deviceNoRef{};
	winrt::com_ptr<ID2D1SolidColorBrush> g_brush{};
	D2D1_RECT_F g_drawingWorldBounds{};

	int g_drawGeometryCommandType{ 0 };
	CD2DBuffer g_buffer{};
	winrt::com_ptr<CGlassRealizer> g_glassRealizer{ nullptr };
	winrt::com_ptr<CReflectionRealizer> g_reflectionRealizer{ nullptr };
	std::unordered_map<dwmcore::CGeometry*, dwmcore::CMILMatrix> g_geometryMap{};

	bool g_disableBlurRendering{ false };
	bool ControlBlurRendering(bool disable)
	{
		bool previousState = g_disableBlurRendering;
		g_disableBlurRendering = disable;
		return previousState;
	}

	dwmcore::CShapePtr GetShapeRenderingData(
		const dwmcore::CDrawingContext* drawingContext,
		const dwmcore::CShape* shape, 
		const dwmcore::CMILMatrix* matrix,
		D2D1_RECT_F& drawingWorldBounds,
		std::unique_ptr<D2D1_RECT_F[]>& rectangles,
		UINT& rectanglesCount,
		bool& shapeIsRectangles,
		bool& zeroCopyAllowed
	)
	{
		zeroCopyAllowed = shapeIsRectangles = false;
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

			if (rectanglesCount == 1)
			{
				zeroCopyAllowed = true;
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
			g_CDrawingContext_DrawGeometry_Org = HookHelper::WritePointer(g_CDrawingContext_DrawGeometry_Org_Address, MyCDrawingContext_DrawGeometry);
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

void STDMETHODCALLTYPE GlassRenderer::MyCGeometry_Destructor(
	dwmcore::CGeometry* This
)
{
	g_geometryMap.erase(This);
	return g_CGeometry_Destructor_Org(This);
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
	const auto currentVisual = drawingContext->GetCurrentVisualHelper();
	const auto d2dContext = drawingContext->GetD2DContext();
	const auto context = d2dContext->GetDeviceContext();
	const auto matrix = drawingContext->GetWorldTransform();
	const auto currentVisualTree = drawingContext->GetCurrentVisualTree();
	const auto desktopTree = HookHelper::vftbl_of(currentVisualTree) != dwmcore::CDesktopTree::vftable ? currentVisual->GetDesktopTree() : currentVisualTree;
	
	UINT rectanglesCount;
	std::unique_ptr<D2D1_RECT_F[]> rectangles;

	const auto renderingShape = GetShapeRenderingData(
		drawingContext,
		geometryShape.get(),
		matrix,
		g_drawingWorldBounds,
		rectangles,
		rectanglesCount,
		g_shapeIsRectangles,
		g_glassInput.zeroCopyAllowed
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
	});

	winrt::com_ptr<ID2D1Device> device{ nullptr };
	context->GetDevice(device.put());
	// device lost
	if (g_deviceNoRef != device.get())
	{
		g_deviceNoRef = device.get();
		g_brush = nullptr;
		g_glassRealizer = nullptr;
		g_reflectionRealizer = nullptr;
		g_buffer.Reset();
	}
	
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
		g_renderFlag.set(RenderFlag_Reflection, true);
	}

	winrt::com_ptr<ID2D1Bitmap1> sharedAtlasBitmap;
	winrt::com_ptr<ID2D1Bitmap1> renderTargetBitmap;

	if (HookHelper::vftbl_of(brush) == dwmcore::CSolidColorLegacyMilBrush::vftable)
	{
		const auto solidColorBrush = static_cast<dwmcore::CSolidColorLegacyMilBrush*>(brush);
		const auto opacity = solidColorBrush->GetRealizedOpacity();
		auto color = Color::scRGBTosRGB(solidColorBrush->GetRealizedColor());

		if (color.a == 0.f || opacity == 0.f)
		{
			return S_OK;
		}

		const auto extendedAmount = (g_disableBlurRendering || !desktopTree) ? 0.f : GlassKernel::GetBlurExtendedAmount();
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
				(
					renderTargetBitmap = drawingContext->AcquireRenderTargetBitmap(true),
					renderTargetBitmap
				) &&
				!(
					(
						!desktopTree ||
						HookHelper::vftbl_of(currentVisualTree) != dwmcore::CDesktopTree::vftable
					) &&
					GlassKernel::IsInCVIHierarchy() &&
					(
						renderTargetBitmap = drawingContext->AcquireRenderTargetBitmap(true),
						renderTargetBitmap->GetPixelFormat().alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED
					)
				) &&
				glassCoverageSet
			)
			{
				if (
					glassCoverageSet->IsFullyCovered(
						shapeWorldBounds,
						drawingContext->GetD2DContextOwner()->GetCurrentZ()
					)
				)
				{
					g_glassInput.params.optimization = D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED;
					g_glassInput.nearestNeighborFinalScale = true;
					g_glassInput.zeroCopyAllowed = true;
				}
				else
				{
					g_glassInput.nearestNeighborFinalScale = false;
					g_glassInput.params.optimization = Shared::g_blurOptimization;
				}

				// temporary disable zero copy
				// 2025/5/26
				g_glassInput.zeroCopyAllowed = false;
				if (g_glassInput.zeroCopyAllowed)
				{
					winrt::com_ptr<ID2D1ColorContext> colorContext;
					renderTargetBitmap->GetColorContext(colorContext.put());
					const auto bitmapProperties = D2D1::BitmapProperties1(
						renderTargetBitmap->GetOptions(),
						renderTargetBitmap->GetPixelFormat(),
						0.f,
						0.f,
						colorContext.get()
					);

					if (
						FAILED(
							d2dContext->GetPrivateCompositorDeviceContext()->CreateSharedAtlasBitmap(
								renderTargetBitmap.get(),
								&bitmapProperties,
								sharedAtlasBitmap.put()
							)
						)
					)
					{
						g_glassInput.zeroCopyAllowed = false;
					}
				}

				g_glassInput.sourceBitmap = sharedAtlasBitmap.get() ? sharedAtlasBitmap.get() : renderTargetBitmap.get();
				g_glassInput.rectangles = std::span{ rectangles.get(), rectanglesCount };
				// effect settings
				g_glassInput.params.blurAmount = Shared::g_blurAmount;
				g_glassInput.params.color = color;
				g_glassInput.params.color.a = 1.f;
				g_glassInput.params.colorBalance = (Shared::g_type == Shared::GlassType::Blur) ? glassOpacity : colorBalance;
				g_glassInput.params.afterglow = Shared::g_afterglow;
				g_glassInput.params.afterglowBalance = afterglowBalance;
				g_glassInput.params.blurBalance = blurBalance;
				g_glassInput.params.type = Shared::g_type;

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
				if (!g_brush)
				{
					context->CreateSolidColorBrush({}, g_brush.put());
				}
				g_brush->SetColor(color);
				drawingContext->FillShapeWithBrush(renderingShape.get(), g_brush.get());
			}
		}
	}

	if (g_renderFlag.any())
	{
		if (!g_ID2D1DeviceContext_FillGeometry_Org)
		{
			g_ID2D1DeviceContext_FillGeometry_Org_Address = reinterpret_cast<decltype(g_ID2D1DeviceContext_FillGeometry_Org_Address)>(&HookHelper::vftbl_of(context)[0x17]);
			g_ID2D1DeviceContext_FillGeometry_Org = HookHelper::WritePointer(g_ID2D1DeviceContext_FillGeometry_Org_Address, MyID2D1DeviceContext_FillGeometry);
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
	if (brush)
	{
		return g_ID2D1DeviceContext_FillGeometry_Org(This, geometry, brush, opacityBrush);
	}

	bool ignoreLayer
	{
		g_renderFlag.test(RenderFlag_Backdrop) &&
		!g_renderFlag.test(RenderFlag_Reflection) &&
		g_shapeIsRectangles
	};
	const auto primitiveBlend = This->GetPrimitiveBlend();
	if (!ignoreLayer)
	{
		bool ignoreAlpha
		{
			g_renderFlag.test(RenderFlag_Backdrop) &&
			g_glassInput.sourceBitmap->GetPixelFormat().alphaMode == D2D1_ALPHA_MODE_IGNORE
		};
		This->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
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
					ignoreAlpha ?
					D2D1_LAYER_OPTIONS1_IGNORE_ALPHA :
					D2D1_LAYER_OPTIONS1_NONE
				)
			),
			nullptr
		);
	}
	if (g_renderFlag.test(RenderFlag_Backdrop))
	{
		auto glassRealizer = g_glassRealizer;
		if (!glassRealizer)
		{
			glassRealizer = winrt::make_self<CGlassRealizer>();
			g_glassRealizer = glassRealizer;
		}
		glassRealizer->Render(
			This,
			g_glassInput
		);
	}
	if (!ignoreLayer)
	{
		if (g_renderFlag.test(RenderFlag_Reflection))
		{
			This->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
			auto reflectionRealizer = g_reflectionRealizer;
			if (!reflectionRealizer)
			{
				reflectionRealizer = winrt::make_self<CReflectionRealizer>();
				g_reflectionRealizer = reflectionRealizer;
			}
			reflectionRealizer->Render(
				This,
				g_reflectionInput
			);
			This->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
		}
		This->PopLayer();
		This->SetPrimitiveBlend(primitiveBlend);
	}

	g_renderFlag.reset();
}

void GlassRenderer::Update(GlassEngine::UpdateType type)
{
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
	}

	g_reflectionRealizer = nullptr;
	g_glassRealizer = nullptr;
}

void GlassRenderer::Startup()
{
	THROW_IF_FAILED(CAeroColorizationEffect::Register(*dwmcore::g_DeviceManager));

	dwmcore::g_projectionArray.ApplyToVariable("CGeometry::~CGeometry", g_CGeometry_Destructor_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CRenderData::TryDrawCommandAsDrawList", g_CRenderData_TryDrawCommandAsDrawList_Org);
	
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
	g_glassInput.buffer = &g_buffer;
	g_glassInput.drawingWorldBounds = &g_drawingWorldBounds;
	
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Attach(&g_CGeometry_Destructor_Org, MyCGeometry_Destructor);
			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win10);
			}
			else
			{
				HookHelper::Detours::Attach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win11);
			}
		})
	);
}

void GlassRenderer::Shutdown()
{
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_CGeometry_Destructor_Org, MyCGeometry_Destructor);
			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win10);
			}
			else
			{
				HookHelper::Detours::Detach(&g_CRenderData_TryDrawCommandAsDrawList_Org, MyCRenderData_TryDrawCommandAsDrawList_Win11);
			}
		})
	);

	Sleep(1);

	if (g_ID2D1DeviceContext_FillGeometry_Org)
	{
		HookHelper::WritePointer(g_ID2D1DeviceContext_FillGeometry_Org_Address, g_ID2D1DeviceContext_FillGeometry_Org);
	}
	if (g_CDrawingContext_DrawGeometry_Org)
	{
		HookHelper::WritePointer(g_CDrawingContext_DrawGeometry_Org_Address, g_CDrawingContext_DrawGeometry_Org);
	}

	g_geometryMap.clear();
	g_buffer.Reset();
	g_reflectionRealizer = nullptr;
	g_glassRealizer = nullptr;
	g_brush = nullptr;
	THROW_IF_FAILED(CAeroColorizationEffect::UnRegister(*dwmcore::g_DeviceManager));
}
