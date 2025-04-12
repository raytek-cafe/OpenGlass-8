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

	GlassInput g_glassInput{};
	ReflectionInput g_reflectionInput{};
	std::bitset<2> g_renderFlag{};
	ID2D1Device* g_deviceNoRef{};
	winrt::com_ptr<ID2D1SolidColorBrush> g_brush{};

	int g_drawGeometryCommandType{ 0 };
	winrt::com_ptr<CGlassRealizer> g_glassRealizer{ nullptr };
	winrt::com_ptr<CReflectionRealizer> g_reflectionRealizer{ nullptr };
	winrt::com_ptr<CD2DBuffer> g_blurBuffer{ nullptr };
	std::unordered_map<dwmcore::CGeometry*, dwmcore::CMILMatrix> g_geometryMap{};

	bool g_disableBlurRendering{ false };
	bool ControlBlurRendering(bool disable)
	{
		bool previousState = g_disableBlurRendering;
		g_disableBlurRendering = disable;
		return previousState;
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
		HookHelper::vftbl_of(This->GetResources()->data[static_cast<dwmcore::CDrawGeometryCommand*>(resources->data)->brushIndex]) == dwmcore::CSolidColorLegacyMilBrush::vftable
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
	const auto solidColorBrush = static_cast<dwmcore::CSolidColorLegacyMilBrush*>(brush);
	const auto color = Color::scRGBTosRGB(solidColorBrush->GetRealizedColor());
	const auto opacity = solidColorBrush->GetRealizedOpacity();
	auto finalColor = color;
	finalColor.a *= opacity;

	// shape is nullptr or empty
	dwmcore::CShapePtr geometryShape{};
	if (
		FAILED(geometry->GetShapeData(nullptr, &geometryShape)) ||
		!geometryShape ||
		geometryShape->IsEmpty() ||
		opacity == 0.f
	)
	{
		return S_OK;
	}

	const auto drawingContext = This->GetDrawingContext();
	const auto currentVisual = drawingContext->GetCurrentVisualHelper();
	const auto d2dContext = drawingContext->GetD2DContext();
	const auto context = d2dContext->GetDeviceContext();
	const auto matrix = drawingContext->GetWorldTransform(); 
	const auto currentVisualTree = drawingContext->GetCurrentVisualTree();
	const auto desktopTree = HookHelper::vftbl_of(currentVisualTree) != dwmcore::CDesktopTree::vftable ? currentVisual->GetDesktopTree() : currentVisualTree;
	auto desktopTreeInvalid = !desktopTree;
	const auto extendedAmount = (g_disableBlurRendering || desktopTreeInvalid) ? 0.f : GlassKernel::GetBlurExtendedAmount();
	
	D2D1_RECT_F drawingWorldBounds{};
	drawingContext->GetClipBoundsWorld(&drawingWorldBounds);
	D2D1_RECT_F glassWorldBounds{};
	geometryShape->GetTightBounds(&glassWorldBounds, matrix);
	RectF::IntersectUnsafe(drawingWorldBounds, glassWorldBounds);

	dwmcore::CShapePtr renderingShape{};
	if (
		FAILED(
			drawingContext->GetUnOccludedWorldShape(
				geometryShape.get(),
				drawingContext->GetD2DContextOwner()->GetCurrentZ(),
				renderingShape.put()
			)
		) ||
		!renderingShape
	)
	{
		geometryShape->CopyShape(matrix, renderingShape.put());
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
		g_blurBuffer = nullptr;
	}
	if (!g_brush)
	{
		context->CreateSolidColorBrush({}, g_brush.put());
	}

	auto blurBuffer = g_blurBuffer;
	if (!blurBuffer)
	{
		blurBuffer = winrt::make_self<CD2DBuffer>();
		g_blurBuffer = blurBuffer;
	}

	// check if the shape is consisted of rectangles
	UINT rectanglesCount;
	renderingShape->IsRectangles(&rectanglesCount);

	winrt::com_ptr<ID2D1Bitmap1> renderTargetBitmap;
	std::unique_ptr<dwmcore::CMILMatrix> visualWorldTransform;
	D2D1_RECT_F shapeLocalBounds;
	if (opacity != 1.f)
	{
		visualWorldTransform = std::make_unique_for_overwrite<dwmcore::CMILMatrix>();

		if (desktopTree)
		{
			RETURN_IF_FAILED(
				currentVisual->GetWorldTransform(
					desktopTree,
					3ul,
					visualWorldTransform.get(),
					nullptr,
					nullptr
				)
			);
			g_geometryMap[geometry] = *visualWorldTransform;
		}
		else
		{
			auto it = g_geometryMap.find(geometry);
			if (it != g_geometryMap.end())
			{
				*visualWorldTransform = it->second;
			}
			else
			{
				RETURN_IF_FAILED(
					currentVisual->GetWorldTransform(
						currentVisualTree,
						3ul,
						visualWorldTransform.get(),
						nullptr,
						nullptr
					)
				);
				g_geometryMap[geometry] = *visualWorldTransform;
			}

			desktopTreeInvalid = false;
		}

		RETURN_IF_FAILED(geometryShape->GetTightBounds(&shapeLocalBounds, nullptr));

		const auto active = (opacity == 0.5f);
		const auto reflectionIntensity = active ? Shared::g_reflectionIntensity : Shared::g_reflectionIntensityInactive;
		// reflection only and not parallax
		if (
			color.r == 0.f &&
			color.g == 0.f &&
			color.b == 0.f &&
			color.a == 0.f
		)
		{
			if (
				reflectionIntensity &&
				!desktopTreeInvalid
			)
			{
				g_reflectionInput.reflectionIntensity = reflectionIntensity;
				g_reflectionInput.reflectionParallaxIntensity = 0.f;
				g_reflectionInput.shapeLocalBounds = &shapeLocalBounds;
				g_reflectionInput.worldTransform = matrix;
				g_reflectionInput.visualWorldTransform = visualWorldTransform.get();
				g_renderFlag.set(RenderFlag_Reflection, true);
			}
		}
		else
		{
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
				extendedAmount
			)
			{
				renderTargetBitmap = drawingContext->AcquireRenderTargetBitmap(true);

				g_glassInput.d2dContext = d2dContext;
				g_glassInput.drawingWorldBounds = &drawingWorldBounds;
				g_glassInput.renderTargetBitmap = renderTargetBitmap.get();
				g_glassInput.extendedAmount = extendedAmount;
				// effect settings
				g_glassInput.params.blurAmount = Shared::g_blurAmount;
				g_glassInput.params.optimization = Shared::g_blurOptimization;
				g_glassInput.params.color = color;
				g_glassInput.params.color.a = 1.f;
				g_glassInput.params.colorBalance = (Shared::g_type == Shared::GlassType::Blur) ? glassOpacity : colorBalance;
				g_glassInput.params.afterglow = Shared::g_afterglow;
				g_glassInput.params.afterglowBalance = afterglowBalance;
				g_glassInput.params.blurBalance = blurBalance;
				g_glassInput.params.type = Shared::g_type;
				g_glassInput.buffer = blurBuffer.get();

				g_renderFlag.set(RenderFlag_Backdrop, true);
			}
			else
			{
				if (Shared::g_type == Shared::GlassType::Blur)
				{
					finalColor.a = opaque ? 1.f : glassOpacity;
				}

				#pragma warning(suppress:26813)
				if (Shared::g_type == Shared::GlassType::Aero)
				{
					// dwmcore!CCapturedGlassColorizationParameters::GetEffectivescRGBBlendColor (Windows 7)
					if (opaque)
					{
						finalColor.r *= colorBalance;
						finalColor.g *= colorBalance;
						finalColor.b *= colorBalance;
						finalColor.a = 1.f;
					}
					else
					{
						const auto alpha = std::max(1.f - blurBalance, 0.1f);
						finalColor =
						{
							std::clamp((color.r * colorBalance + Shared::g_afterglow.r * afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
							std::clamp((color.g * colorBalance + Shared::g_afterglow.g * afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
							std::clamp((color.b * colorBalance + Shared::g_afterglow.b * afterglowBalance * 0.6f) / alpha, 0.f, 1.f),
							alpha
						};
					}
				}

				if (rectanglesCount)
				{
					drawingContext->FillShapeWithSolidColor(renderingShape.get(), finalColor);
				}
				else
				{
					g_brush->SetColor(finalColor);
					drawingContext->FillShapeWithBrush(renderingShape.get(), g_brush.get());
				}
			}

			if (
				reflectionIntensity && 
				!desktopTreeInvalid &&
				(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::NonClient)
			)
			{
				g_reflectionInput.reflectionIntensity = reflectionIntensity;
				g_reflectionInput.reflectionParallaxIntensity = Shared::g_reflectionParallaxIntensity;
				g_reflectionInput.shapeLocalBounds = &shapeLocalBounds;
				g_reflectionInput.worldTransform = matrix;
				g_reflectionInput.visualWorldTransform = visualWorldTransform.get();
				g_renderFlag.set(RenderFlag_Reflection, true);
			}
		}
	}
	else
	{
		if (rectanglesCount)
		{
			drawingContext->FillShapeWithSolidColor(renderingShape.get(), finalColor);
		}
		else
		{
			g_brush->SetColor(finalColor);
			drawingContext->FillShapeWithBrush(renderingShape.get(), g_brush.get());
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

	bool ignoreAlpha
	{
		g_renderFlag.test(RenderFlag_Backdrop) &&
		g_glassInput.renderTargetBitmap->GetPixelFormat().alphaMode == D2D1_ALPHA_MODE_IGNORE
	};
	const auto primitiveBlend = This->GetPrimitiveBlend();
	This->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
	This->PushLayer(
		D2D1::LayerParameters1(
			D2D1::InfiniteRect(),
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
	g_renderFlag.reset();
}

void GlassRenderer::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Backdrop)
	{
		auto value = GlassEngine::GetDwordFromRegistry(L"GlassOpacity", 63);
		Shared::g_glassOpacity = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);
		Shared::g_glassOpacityInactive = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"GlassOpacityInactive", value)) / 100.f, 0.f, 1.f);
		
		value = GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionIntensity");
		Shared::g_reflectionIntensity = std::clamp(static_cast<float>(value) / 100.f, 0.f, 1.f);
		Shared::g_reflectionIntensityInactive = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionIntensityInactive", value)) / 100.f, 0.f, 1.f);
		Shared::g_reflectionParallaxIntensity = std::clamp(static_cast<float>(GlassEngine::GetDwordFromRegistry(L"ColorizationGlassReflectionParallaxIntensity", 13)) / 100.f, 0.f, 1.f);

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
	g_blurBuffer = nullptr;
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
	g_reflectionRealizer = nullptr;
	g_glassRealizer = nullptr;
	g_blurBuffer = nullptr;
	g_brush = nullptr;
	THROW_IF_FAILED(CAeroColorizationEffect::UnRegister(*dwmcore::g_DeviceManager));
}
