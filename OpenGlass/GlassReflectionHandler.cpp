#include "pch.h"
#include "GlassReflectionHandler.hpp"
#include "uDWMProjection.hpp"
#include "dwmcoreProjection.hpp"
#include "Shared.hpp"
#include "GlassReflectionBrush.hpp"

using namespace OpenGlass;

namespace OpenGlass::GlassReflectionHandler
{
	void STDMETHODCALLTYPE MyCAnimatedGlassSheet_OnRectUpdated(uDWM::CAnimatedGlassSheet* This, LPCRECT lprc);
	void STDMETHODCALLTYPE MyCAnimatedGlassSheet_Destructor(uDWM::CAnimatedGlassSheet* This);

	HRESULT STDMETHODCALLTYPE MyCLivePreview__FadeOutToGlass(uDWM::CLivePreview* This);
	HRESULT STDMETHODCALLTYPE MyCLivePreview__UpdateInstructions(uDWM::CLivePreview* This);
	HRESULT STDMETHODCALLTYPE MyCLivePreview__UpdateResourcesForMonitor(uDWM::CLivePreview* This, uDWM::LivePreviewResource* livepreviewResource);
	HRESULT STDMETHODCALLTYPE MyCCachedVisualImageProxy_Update(
		uDWM::CCachedVisualImageProxy* This,
		const D2D1_RECT_F& viewbox,
		const DWM::MilSizeD& realizationSize,
		const uDWM::CRectResourceProxy* rectProxy,
		const uDWM::CSizeResourceProxy* sizeProxy,
		const uDWM::CVisualProxy* visualProxy,
		DWM::MilBrushMappingMode viewboxUnits
	);

	decltype(&MyCAnimatedGlassSheet_OnRectUpdated) g_CAnimatedGlassSheet_OnRectUpdated_Org{ nullptr };
	decltype(&MyCAnimatedGlassSheet_Destructor) g_CAnimatedGlassSheet_Destructor_Org{ nullptr };

	decltype(&MyCLivePreview__FadeOutToGlass) g_CLivePreview__FadeOutToGlass_Org{ nullptr };
	decltype(&MyCLivePreview__UpdateInstructions) g_CLivePreview__UpdateInstructions_Org{ nullptr };
	decltype(&MyCLivePreview__UpdateResourcesForMonitor) g_CLivePreview__UpdateResourcesForMonitor_Org{ nullptr };
	decltype(&MyCCachedVisualImageProxy_Update) g_CCachedVisualImageProxy_Update_Org{ nullptr };

	class CAnimatedReflectionSheet : public winrt::implements<CAnimatedReflectionSheet, IUnknown, winrt::non_agile, winrt::no_weak_ref>
	{
		uDWM::CAnimatedGlassSheet* m_sheet{ nullptr };
		winrt::com_ptr<uDWM::CRenderDataVisual> m_visual{ nullptr };
		winrt::com_ptr<uDWM::CRgnGeometryProxy> m_geometry{ nullptr };
	public:
		CAnimatedReflectionSheet(uDWM::CAnimatedGlassSheet* sheet) : m_sheet{ sheet } {};
		virtual ~CAnimatedReflectionSheet()
		{
			if (m_visual)
			{
				m_sheet->GetVisualCollection()->Remove(m_visual.get());
				GlassReflectionBrush::Remove(m_visual.get());
			}
		}
		static HRESULT STDMETHODCALLTYPE Create(uDWM::CAnimatedGlassSheet* glassSheet, CAnimatedReflectionSheet** outputSheet)
		{
			auto reflectionSheet = winrt::make_self<CAnimatedReflectionSheet>(glassSheet);
			RETURN_IF_FAILED(reflectionSheet->Initialize());
			*outputSheet = reflectionSheet.detach();
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Initialize()
		{
			RETURN_IF_FAILED(
				uDWM::CRenderDataVisual::Create(
					m_visual.put()
				)
			);
			m_visual->SetInsetFromParent({});
			const auto brush = GlassReflectionBrush::GetOrCreate(m_visual.get(), true);
			if (!brush)
			{
				return E_FAIL;
			}
			
			wil::unique_hrgn emptyRegion{ CreateRectRgn(0, 0, 0, 0) };
			RETURN_LAST_ERROR_IF_NULL(emptyRegion);

			RETURN_IF_FAILED(
				uDWM::ResourceHelper::CreateGeometryFromHRGN(
					emptyRegion.get(),
					m_geometry.put()
				)
			);
			winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
			RETURN_IF_FAILED(
				uDWM::CDrawGeometryInstruction::Create(
					brush.get(),
					m_geometry.get(),
					instruction.put()
				)
			);
			RETURN_IF_FAILED(m_visual->AddInstruction(instruction.get()));
			RETURN_IF_FAILED(
				m_sheet->GetVisualCollection()->InsertRelative(
					m_visual.get(),
					nullptr,
					false,
					true
				)
			);

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE OnRectUpdated(LPCRECT lprc)
		{
			const auto brush = GlassReflectionBrush::GetOrCreate(m_visual.get());
			RETURN_IF_FAILED(
				brush->Update(
					(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::AnimatedGlassSheet) ? 
					Shared::g_reflectionIntensity : 
					0.f,
					GlassReflectionBrush::CalculateTargetViewport(
						{ lprc->left, lprc->top }
					),
					D2D1::RectF(),
					nullptr,
					DWM::MilBrushMappingMode::Absolute,
					DWM::MilBrushMappingMode::Absolute,
					nullptr,
					nullptr,
					DWM::MilStretch::None,
					DWM::MilTileMode::Extend,
					DWM::MilHorizontalAlignment::Left,
					DWM::MilVerticalAlignment::Top,
					nullptr
				)
			);
			m_visual->_ValidateVisual();

			RETURN_IF_FAILED(
				uDWM::ResourceHelper::CreateGeometryFromHRGN(
					wil::unique_hrgn
					{
						CreateRoundRectRgn(
							0 - m_sheet->GetAtlasPaddingLeft(),
							0 - m_sheet->GetAtlasPaddingTop(),
							wil::rect_width(*lprc) - m_sheet->GetAtlasPaddingRight(),
							wil::rect_height(*lprc) - m_sheet->GetAtlasPaddingBottom(),
							Shared::g_roundRectRadius,
							Shared::g_roundRectRadius
						)
					}.get(),
					reinterpret_cast<uDWM::CRgnGeometryProxy**>(&m_geometry)
				)
			);
			return S_OK;
		}
	};
	std::unordered_map<uDWM::CAnimatedGlassSheet*, winrt::com_ptr<CAnimatedReflectionSheet>> g_sheetMap{};

	uDWM::LivePreviewResource* g_livepreviewResource{ nullptr };
}

void STDMETHODCALLTYPE GlassReflectionHandler::MyCAnimatedGlassSheet_OnRectUpdated(uDWM::CAnimatedGlassSheet* This, LPCRECT lprc)
{
	winrt::com_ptr<CAnimatedReflectionSheet> reflectionSheet{};
	if (const auto it = g_sheetMap.find(This); it != g_sheetMap.end())
	{
		reflectionSheet = it->second;
	}
	else
	{
		if (SUCCEEDED(CAnimatedReflectionSheet::Create(This, reflectionSheet.put())))
		{
			g_sheetMap.emplace(This, reflectionSheet);
		}
	}

	if (reflectionSheet)
	{
		reflectionSheet->OnRectUpdated(lprc);
	}

	return g_CAnimatedGlassSheet_OnRectUpdated_Org(This, lprc);
}
void STDMETHODCALLTYPE GlassReflectionHandler::MyCAnimatedGlassSheet_Destructor(uDWM::CAnimatedGlassSheet* This)
{
	g_sheetMap.erase(This);
	return g_CAnimatedGlassSheet_Destructor_Org(This);
}

// here restores
// CLivePreview::_UpdateGlassVisual
// CLivePreview::_UpdateInstructions
HRESULT STDMETHODCALLTYPE GlassReflectionHandler::MyCLivePreview__FadeOutToGlass(uDWM::CLivePreview* This)
{
	RETURN_IF_FAILED(This->_UpdateResources());

	for (auto& visual : This->GetLivePreviewVisualArray()->views())
	{
		const auto data = visual.data;
		if (uDWM::CLivePreview::IsWindowIncluded(data))
		{
			auto windowFrames = visual.windowFrames;
			if (visual.unknown0 || !uDWM::CLivePreview::IsWindowCloneAsWindowFrames(data))
			{
				if (windowFrames)
				{
					windowFrames->GetTransformParent()->GetVisualCollection()->Remove(
						windowFrames
					);
					windowFrames->Release();
					visual.windowFrames = nullptr;
				}
			}
			else if (!windowFrames)
			{
				visual.data->GetWindow()->CloneVisualTreeForLivePreview(true, &visual.windowFrames);
				This->GetGlassVisual()->GetVisualCollection()->InsertRelative(
					visual.windowFrames,
					nullptr,
					false,
					true
				);
			}
		}
	}
	RETURN_IF_FAILED(This->ClearInstructions());
	RETURN_IF_FAILED(This->GetGlassVisual()->ClearInstructions());
	for (const auto& resource : This->GetLivePreviewResourceArray()->views())
	{
		//if (resource.IsWindowBoundingRectNotEmpty())
		if (
			!IsRectEmpty(resource.GetWindowBoundingRect())
		)
		{
			winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
			if (
				resource.GetWindowVisualBrush() &&
				resource.GetWindowBoundingGeometry()
			)
			{
				RETURN_IF_FAILED(
					uDWM::CDrawGeometryInstruction::Create(
						resource.GetWindowVisualBrush(),
						resource.GetWindowBoundingGeometry(),
						instruction.put()
					)
				);
				RETURN_IF_FAILED(This->AddInstruction(instruction.get()));
			}

			if (
				resource.GetGlassVisualBrush() &&
				resource.GetGlassBoundingGeometry()
			)
			{
				RETURN_IF_FAILED(
					uDWM::CDrawGeometryInstruction::Create(
						resource.GetGlassVisualBrush(),
						resource.GetGlassBoundingGeometry(),
						instruction.put()
					)
				);
				RETURN_IF_FAILED(This->AddInstruction(instruction.get()));
			}
		}
		//if (resource.IsGlassBoundingRectNotEmpty())
		if (
			!IsRectEmpty(resource.GetGlassBoundingRect()) && 
			resource.GetReflectionGeometry()
		)
		{
			if (
				const auto brush = GlassReflectionBrush::GetOrCreate(
					This,
					true
				);
				brush
			)
			{
				RETURN_IF_FAILED(
					brush->Update(
						(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::LivePreview) ? 
						Shared::g_reflectionIntensity : 
						0.f,
						GlassReflectionBrush::CalculateTargetViewport(
							This->GetGlassVisual()->GetLocalToParentVisualOffset(This->GetTransformParent())
						),
						D2D1::RectF(),
						nullptr,
						DWM::MilBrushMappingMode::Absolute,
						DWM::MilBrushMappingMode::Absolute,
						nullptr,
						nullptr,
						DWM::MilStretch::None,
						DWM::MilTileMode::Extend,
						DWM::MilHorizontalAlignment::Left,
						DWM::MilVerticalAlignment::Top,
						nullptr
					)
				);

				winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
				RETURN_IF_FAILED(
					uDWM::CDrawGeometryInstruction::Create(
						brush.get(),
						resource.GetReflectionGeometry(),
						instruction.put()
					)
				);
				RETURN_IF_FAILED(This->GetGlassVisual()->AddInstruction(instruction.get()));
			}
		}
	}

	return g_CLivePreview__FadeOutToGlass_Org(This);
}
HRESULT STDMETHODCALLTYPE GlassReflectionHandler::MyCLivePreview__UpdateInstructions(uDWM::CLivePreview* This)
{
	auto hr = g_CLivePreview__UpdateInstructions_Org(This);

	for (const auto& resource : This->GetLivePreviewResourceArray()->views())
	{
		//if (resource.IsGlassBoundingRectNotEmpty())
		if (
			!IsRectEmpty(resource.GetGlassBoundingRect()) && 
			resource.GetReflectionGeometry()
		)
		{
			if (
				const auto brush = GlassReflectionBrush::GetOrCreate(
					This,
					true
				);
				brush
			)
			{
				RETURN_IF_FAILED(
					brush->Update(
						(Shared::g_reflectionPolicy & Shared::ReflectionPolicy::LivePreview) ? 
						Shared::g_reflectionIntensity : 
						0.f,
						GlassReflectionBrush::CalculateTargetViewport(
							This->GetGlassVisual()->GetLocalToParentVisualOffset(This->GetTransformParent())
						),
						D2D1::RectF(),
						nullptr,
						DWM::MilBrushMappingMode::Absolute,
						DWM::MilBrushMappingMode::Absolute,
						nullptr,
						nullptr,
						DWM::MilStretch::None,
						DWM::MilTileMode::Extend,
						DWM::MilHorizontalAlignment::Left,
						DWM::MilVerticalAlignment::Top,
						nullptr
					)
				);

				winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction{ nullptr };
				RETURN_IF_FAILED(
					uDWM::CDrawGeometryInstruction::Create(
						brush.get(),
						resource.GetReflectionGeometry(),
						instruction.put()
					)
				);
				RETURN_IF_FAILED(This->GetGlassVisual()->AddInstruction(instruction.get()));
			}
		}
	}

	return hr;
}
HRESULT STDMETHODCALLTYPE GlassReflectionHandler::MyCLivePreview__UpdateResourcesForMonitor(uDWM::CLivePreview* This, uDWM::LivePreviewResource* livepreviewResource)
{
	g_livepreviewResource = livepreviewResource;
	auto hr = g_CLivePreview__UpdateResourcesForMonitor_Org(This, livepreviewResource);
	g_livepreviewResource = nullptr;

	return hr;
}
HRESULT STDMETHODCALLTYPE GlassReflectionHandler::MyCCachedVisualImageProxy_Update(
	uDWM::CCachedVisualImageProxy* This,
	const D2D1_RECT_F& viewbox,
	const DWM::MilSizeD& realizationSize,
	const uDWM::CRectResourceProxy* rectProxy,
	const uDWM::CSizeResourceProxy* sizeProxy,
	const uDWM::CVisualProxy* visualProxy,
	DWM::MilBrushMappingMode viewboxUnits
)
{
	// The DWM team changed the implementation of dwmcore!CRenderData::TryDrawCommandAsDrawList, 
	// which is why Aero Peek is glitching since Windows 10 1803. 
	// 
	// https://github.com/microsoft/Windows-Dev-Performance/issues/12
	if (g_livepreviewResource)
	{
		auto fixedViewBox = viewbox;
		fixedViewBox.left = static_cast<float>(g_livepreviewResource->GetMonitorRect()->left);
		fixedViewBox.top = static_cast<float>(g_livepreviewResource->GetMonitorRect()->top);
		fixedViewBox.right = static_cast<float>(g_livepreviewResource->GetMonitorRect()->right);
		fixedViewBox.bottom = static_cast<float>(g_livepreviewResource->GetMonitorRect()->bottom);
		return g_CCachedVisualImageProxy_Update_Org(This, fixedViewBox, realizationSize, rectProxy, sizeProxy, visualProxy, viewboxUnits);
	}
	return g_CCachedVisualImageProxy_Update_Org(This, viewbox, realizationSize, rectProxy, sizeProxy, visualProxy, viewboxUnits);
}

void GlassReflectionHandler::Update([[maybe_unused]] GlassEngine::UpdateType type)
{
}

void GlassReflectionHandler::Startup()
{
	uDWM::g_projectionArray.ApplyToVariable("CAnimatedGlassSheet::OnRectUpdated", g_CAnimatedGlassSheet_OnRectUpdated_Org);
	uDWM::g_projectionArray.ApplyToVariable("CAnimatedGlassSheet::~CAnimatedGlassSheet", g_CAnimatedGlassSheet_Destructor_Org);
	
	uDWM::g_projectionArray.ApplyToVariable("CLivePreview::_FadeOutToGlass", g_CLivePreview__FadeOutToGlass_Org);
	uDWM::g_projectionArray.ApplyToVariable("CLivePreview::_UpdateInstructions", g_CLivePreview__UpdateInstructions_Org);
	uDWM::g_projectionArray.ApplyToVariable("CLivePreview::_UpdateResourcesForMonitor", g_CLivePreview__UpdateResourcesForMonitor_Org);
	uDWM::g_projectionArray.ApplyToVariable("CCachedVisualImageProxy::Update", g_CCachedVisualImageProxy_Update_Org);
	
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Attach(&g_CLivePreview__UpdateResourcesForMonitor_Org, MyCLivePreview__UpdateResourcesForMonitor);
			HookHelper::Detours::Attach(&g_CCachedVisualImageProxy_Update_Org, MyCCachedVisualImageProxy_Update);
			if (uDWM::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(&g_CAnimatedGlassSheet_OnRectUpdated_Org, MyCAnimatedGlassSheet_OnRectUpdated);
				HookHelper::Detours::Attach(&g_CAnimatedGlassSheet_Destructor_Org, MyCAnimatedGlassSheet_Destructor);

				HookHelper::Detours::Attach(&g_CLivePreview__UpdateInstructions_Org, MyCLivePreview__UpdateInstructions);
			}
			else
			{
				HookHelper::Detours::Attach(&g_CLivePreview__FadeOutToGlass_Org, MyCLivePreview__FadeOutToGlass);
			}
		})
	);
}

void GlassReflectionHandler::Shutdown()
{
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_CLivePreview__UpdateResourcesForMonitor_Org, MyCLivePreview__UpdateResourcesForMonitor);
			HookHelper::Detours::Detach(&g_CCachedVisualImageProxy_Update_Org, MyCCachedVisualImageProxy_Update);
			if (uDWM::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(&g_CAnimatedGlassSheet_OnRectUpdated_Org, MyCAnimatedGlassSheet_OnRectUpdated);
				HookHelper::Detours::Detach(&g_CAnimatedGlassSheet_Destructor_Org, MyCAnimatedGlassSheet_Destructor);

				HookHelper::Detours::Detach(&g_CLivePreview__UpdateInstructions_Org, MyCLivePreview__UpdateInstructions);
			}
			else
			{
				HookHelper::Detours::Detach(&g_CLivePreview__FadeOutToGlass_Org, MyCLivePreview__FadeOutToGlass);
			}
		})
	);

	g_sheetMap.clear();
}