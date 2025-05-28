#include "pch.h"
#include "HookHelper.hpp"
#include "uDWMProjection.hpp"
#include "dwmcoreProjection.hpp"
#include "Shared.hpp"
#include "GlassKernel.hpp"
#include "GlassSafetyZone.hpp"
#include "GlassSafetyZoneLayer.hpp"
#include "GlassCoverageSet.hpp"
#include "GlassRenderer.hpp"

using namespace OpenGlass;

namespace OpenGlass::GlassSafetyZone
{
	HRESULT STDMETHODCALLTYPE MyCOcclusionContext_Compute(
		dwmcore::COcclusionContext* This,
		const dwmcore::CVisualTree* visualTree,
		const DWM::span<D2D1_RECT_F>& rectangles,
		float unknown,
		const DWM::span<dwmcore::COcclusionContext*>& overlays
	);
	HRESULT STDMETHODCALLTYPE MyCOcclusionContext_DrawGeometry(
		dwmcore::IDrawingContext* This,
		dwmcore::CLegacyMilBrush* brush,
		dwmcore::CGeometry* geometry
	);
	HRESULT STDMETHODCALLTYPE MyCOcclusionContext_SetDeviceTransform(
		dwmcore::COcclusionContext* This,
		const dwmcore::CMILMatrix* matrix
	);
	void STDMETHODCALLTYPE MyCOcclusionContext_Destructor(dwmcore::COcclusionContext* This);

	bool STDMETHODCALLTYPE MyCArrayBasedCoverageSet_IsCovered(
		dwmcore::CArrayBasedCoverageSet* This,
		const D2D1_RECT_F& coverage,
		int depth
	);
	bool STDMETHODCALLTYPE MyCOcclusionContext_IsOccluded(
		dwmcore::COcclusionContext* This,
		const D2D1_RECT_F& coverage,
		int depth,
		bool ignoreDeviceTransform
	);
	HRESULT STDMETHODCALLTYPE MyCOcclusionContext_PageInPixelsRectToDeviceRect(
		dwmcore::COcclusionContext* This,
		const D2D1_RECT_F& src,
		D2D1_RECT_F* dst
	);
	HRESULT STDMETHODCALLTYPE MyCDirtyRegion_GetUnOccludedDirtyRect(
		dwmcore::CDirtyRegion* This,
		D2D1_RECT_F* dirtyRect,
		int i,
		const D2D1_RECT_F& bounds,
		bool useSuperSample,
		const DWM::span<dwmcore::CVisual>& visuals,
		dwmcore::COcclusionContext* occlusionContext
	);
	HRESULT STDMETHODCALLTYPE MyCDirtyRegion_GetOptimizedRect(
		dwmcore::CDirtyRegion* This,
		D2D1_RECT_F* dirtyRect,
		int i,
		const D2D1_RECT_F& bounds,
		const dwmcore::CRegion* region,
		const dwmcore::CMILMatrix* matrix,
		bool useSuperSample,
		const DWM::span<dwmcore::CVisual>& visuals,
		dwmcore::COcclusionContext* occlusionContext
	);
	HRESULT STDMETHODCALLTYPE MyCTreeDirty_GetOptimizedRect(
		dwmcore::CTreeDirty* This,
		D2D1_RECT_F* dirtyRect,
		UINT i,
		const D2D1_RECT_F& bounds,
		dwmcore::COcclusionContext* occlusionContext,
		const dwmcore::CRegion* region,
		const dwmcore::CMILMatrix* matrix,
		bool useSuperSample,
		const DWM::span<dwmcore::CVisual>& visuals
	);

	template <typename T>
	HRESULT STDMETHODCALLTYPE MyCDrawingContext_DrawVisualTree(
		dwmcore::CDrawingContext* This,
		const D2D1_RECT_F& rectangle,
		dwmcore::COcclusionContext* occlusionContext,
		bool useSuperSample,
		T&& callback
	);
	HRESULT STDMETHODCALLTYPE MyCDrawingContext_DrawVisualTree_Win10(
		dwmcore::CDrawingContext* This,
		dwmcore::CVisualTree* tree,
		const D2D1_RECT_F& rectangle,
		dwmcore::COcclusionContext* occlusionContext,
		int clearMode,
		bool useSuperSample
	);
	HRESULT STDMETHODCALLTYPE MyCDrawingContext_DrawVisualTree_Win11(
		dwmcore::CDrawingContext* This,
		dwmcore::CVisualTree* tree,
		const D2D1_RECT_F& rectangle,
		dwmcore::COcclusionContext* occlusionContext,
		int clearMode,
		bool useSuperSample,
		dwmcore::CVisual* visualOverride
	);

	decltype(&MyCOcclusionContext_Compute) g_COcclusionContext_Compute_Org{ nullptr };
	decltype(&MyCOcclusionContext_DrawGeometry) g_COcclusionContext_DrawGeometry_Org{ nullptr };
	decltype(&MyCOcclusionContext_DrawGeometry)* g_COcclusionContext_DrawGeometry_Org_Address{ nullptr };
	decltype(&MyCOcclusionContext_SetDeviceTransform) g_COcclusionContext_SetDeviceTransform_Org{ nullptr };
	decltype(&MyCOcclusionContext_Destructor) g_COcclusionContext_Destructor_Org{ nullptr };

	decltype(&MyCArrayBasedCoverageSet_IsCovered) g_CArrayBasedCoverageSet_IsCovered_Org{ nullptr };
	decltype(&MyCOcclusionContext_IsOccluded) g_COcclusionContext_IsOccluded_Org{ nullptr };
	decltype(&MyCOcclusionContext_PageInPixelsRectToDeviceRect) g_COcclusionContext_PageInPixelsRectToDeviceRect_Org{ nullptr };
	decltype(&MyCDirtyRegion_GetUnOccludedDirtyRect) g_CDirtyRegion_GetUnOccludedDirtyRect_Org{ nullptr };
	decltype(&MyCDirtyRegion_GetOptimizedRect) g_CDirtyRegion_GetOptimizedRect_Org{ nullptr };
	decltype(&MyCTreeDirty_GetOptimizedRect) g_CTreeDirty_GetOptimizedRect_Org{ nullptr };
	PVOID g_CDrawingContext_DrawVisualTree_Org{ nullptr };

	IGlassCoverageSet* g_glassCoverageSetNoRef{ nullptr };
	dwmcore::CDrawingContext* g_drawingContextNoRef{ nullptr };
	ID2D1Device* g_deviceNoRef{ nullptr };
	CGlassSafetyZoneLayer g_glassSafetyZoneLayer{};

	std::unordered_map<dwmcore::COcclusionContext*, ULONGLONG> g_shrunkCoverageSetMap{};
	void ShrinkOccluderGlassAboved(dwmcore::COcclusionContext* occlusionContext);

	struct UnoccludedDirtyRegionCalculationContext
	{
		dwmcore::COcclusionContext* occlusionContext;
		D2D1_RECT_F* dirtyRect;
		dwmcore::CMILMatrix deviceTransform;
		UINT deviceTransformFlag;

		void Enter(dwmcore::COcclusionContext* context)
		{
			if (context)
			{
				occlusionContext = context;
				if (dwmcore::g_buildNumber < os::build_w11_24h2)
				{
					dirtyRect = nullptr;
					deviceTransformFlag = *occlusionContext->GetDeviceTransformFlag();
					if (!(deviceTransformFlag & 0x1))
					{
						deviceTransform = *occlusionContext->GetDeviceTransform();
						*occlusionContext->GetDeviceTransformFlag() |= 0x1;
						*const_cast<dwmcore::CMILMatrix*>(occlusionContext->GetDeviceTransform()) = *dwmcore::CMILMatrix::Identity;
					}
				}
			}
		}
		void Leave()
		{
			if (occlusionContext)
			{
				if (dwmcore::g_buildNumber < os::build_w11_24h2)
				{
					if (!(deviceTransformFlag & 0x1))
					{
						*occlusionContext->GetDeviceTransformFlag() = deviceTransformFlag;
						*const_cast<dwmcore::CMILMatrix*>(occlusionContext->GetDeviceTransform()) = deviceTransform;
					}
					dirtyRect = nullptr;
				}
				occlusionContext = nullptr;
			}
		}
		bool IsActive()
		{
			return occlusionContext != nullptr;
		}
	};
	using udrcc_leave_scope_exit = wil::unique_any<UnoccludedDirtyRegionCalculationContext*, decltype(&UnoccludedDirtyRegionCalculationContext::Leave), &UnoccludedDirtyRegionCalculationContext::Leave, wil::details::pointer_access_none>;
	[[nodiscard]] inline udrcc_leave_scope_exit EnterUnoccludedDirtyRegionCalculationContext(UnoccludedDirtyRegionCalculationContext* pudrcc, dwmcore::COcclusionContext* context) noexcept
	{
		pudrcc->Enter(context);
		return udrcc_leave_scope_exit{ pudrcc };
	}
	UnoccludedDirtyRegionCalculationContext g_calculationContext{};
}

void GlassSafetyZone::ShrinkOccluderGlassAboved(dwmcore::COcclusionContext* occlusionContext)
{
	const auto frameId = occlusionContext->GetFrameId();
	const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();

	if (!extendedAmount)
	{
		return;
	}
	const auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(occlusionContext);
	if (!glassCoverageSet)
	{
		return;
	}
	if (g_shrunkCoverageSetMap[occlusionContext] == frameId)
	{
		return;
	}
	g_shrunkCoverageSetMap[occlusionContext] = frameId;

	enum ShrinkSide
	{
		ShrinkSide_Left,
		ShrinkSide_Top,
		ShrinkSide_Right,
		ShrinkSide_Bottom
	};
	std::unordered_map<dwmcore::CZOrderedRect*, std::bitset<4>> targetOccluderSet{};
	std::unordered_set<const dwmcore::CZOrderedRect*> visibleGlassSet{};

	auto coverageSet = occlusionContext->GetArrayBasedCoverageSet();
	for (const auto& zorderRect : glassCoverageSet->GetViews())
	{
		if (
			!coverageSet->IsCovered(
				D2D1::RectF(
					zorderRect.m_transformedRect.left - extendedAmount,
					zorderRect.m_transformedRect.top - extendedAmount,
					zorderRect.m_transformedRect.right + extendedAmount,
					zorderRect.m_transformedRect.bottom + extendedAmount
				), 
				zorderRect.m_depth
			)
		)
		{
			visibleGlassSet.insert(&zorderRect);
		}
	}
	for (const auto& zorderRect : visibleGlassSet)
	{
		D2D1_RECT_F actualGlassRect
		{
			zorderRect->m_transformedRect.left - extendedAmount,
			zorderRect->m_transformedRect.top - extendedAmount,
			zorderRect->m_transformedRect.right + extendedAmount,
			zorderRect->m_transformedRect.bottom + extendedAmount
		};

		for (auto& occluder : coverageSet->GetOccluderArray()->views())
		{
			if (occluder.m_depth >= zorderRect->m_depth)
			{
				break;
			}

			if (
				!wil::rect_is_empty(occluder.m_transformedRect) &&
				std::fabs(wil::rect_height(occluder.m_transformedRect) * wil::rect_width(occluder.m_transformedRect)) > 1.f &&

				RectF::DoesIntersectUnsafe(occluder.m_transformedRect, actualGlassRect)
			)
			{
				std::bitset<4> sides{};
				if (occluder.m_transformedRect.left > actualGlassRect.left)
				{
					sides.set(ShrinkSide_Left, true);
				}
				if (occluder.m_transformedRect.top > actualGlassRect.top)
				{
					sides.set(ShrinkSide_Top, true);
				}
				if (occluder.m_transformedRect.right < actualGlassRect.right)
				{
					sides.set(ShrinkSide_Right, true);
				}
				if (occluder.m_transformedRect.bottom < actualGlassRect.bottom)
				{
					sides.set(ShrinkSide_Bottom, true);
				}
				targetOccluderSet.try_emplace(&occluder, sides).first->second |= sides;
			}
		}
	}

	const auto matrix = std::make_unique_for_overwrite<dwmcore::CMILMatrix>();
	memcpy_s(
		matrix.get(),
		sizeof(dwmcore::CMILMatrix),
		occlusionContext->GetDeviceTransform(),
		sizeof(D2D1_MATRIX_4X4_F)
	);
	for (auto& [occluder, sides] : targetOccluderSet)
	{
		if (sides.test(ShrinkSide_Left))
		{
			occluder->m_originalRect.left += extendedAmount;
		}
		if (sides.test(ShrinkSide_Top))
		{
			occluder->m_originalRect.top += extendedAmount;
		}
		if (sides.test(ShrinkSide_Right))
		{
			occluder->m_originalRect.right -= extendedAmount;
		}
		if (sides.test(ShrinkSide_Bottom))
		{
			occluder->m_originalRect.bottom -= extendedAmount;
		}

		if (wil::rect_is_empty(occluder->m_originalRect))
		{
			occluder->m_originalRect = {};
		}
		occluder->UpdateDeviceRect(matrix.get());
	}
}

HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCOcclusionContext_Compute(
	dwmcore::COcclusionContext* This,
	const dwmcore::CVisualTree* visualTree,
	const DWM::span<D2D1_RECT_F>& rectangles,
	float unknown,
	const DWM::span<dwmcore::COcclusionContext*>& overlays
)
{
	HRESULT hr{ S_OK };
	if (!g_COcclusionContext_DrawGeometry_Org)
	{
		g_COcclusionContext_DrawGeometry_Org_Address = reinterpret_cast<decltype(g_COcclusionContext_DrawGeometry_Org_Address)>(&HookHelper::vftbl_of(This)[4]);
		g_COcclusionContext_DrawGeometry_Org = HookHelper::WritePointer(g_COcclusionContext_DrawGeometry_Org_Address, MyCOcclusionContext_DrawGeometry);
	}
	if (auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(This); glassCoverageSet)
	{
		glassCoverageSet->Clear();
	}
	const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();
	if (
		extendedAmount && 
		rectangles.length
	)
	{
		auto extendedRectangles = std::make_unique_for_overwrite<D2D1_RECT_F[]>(rectangles.length);
		memcpy_s(
			extendedRectangles.get(),
			rectangles.length * sizeof(D2D1_RECT_F),
			rectangles.data,
			rectangles.length * sizeof(D2D1_RECT_F)
		);
		for (auto& rectangle : std::span{ extendedRectangles.get(), rectangles.length })
		{
			rectangle.left -= extendedAmount * 2.f;
			rectangle.top -= extendedAmount * 2.f;
			rectangle.right += extendedAmount * 2.f;
			rectangle.bottom += extendedAmount * 2.f;
		}

		hr = g_COcclusionContext_Compute_Org(
			This,
			visualTree,
			DWM::span{ rectangles.length, extendedRectangles.get() },
			unknown,
			overlays
		);
	}
	else
	{
		hr = g_COcclusionContext_Compute_Org(
			This,
			visualTree,
			rectangles,
			unknown,
			overlays
		);
	}

	return hr;
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCOcclusionContext_DrawGeometry(
	dwmcore::IDrawingContext* This,
	dwmcore::CLegacyMilBrush* brush,
	dwmcore::CGeometry* geometry
)
{
	auto hr = g_COcclusionContext_DrawGeometry_Org(
		This,
		brush,
		geometry
	);

	if (
		FAILED(hr) ||
		HookHelper::vftbl_of(brush) != dwmcore::CSolidColorLegacyMilBrush::vftable
	)
	{
		return hr;
	}

	dwmcore::CShapePtr geometryShape{};
	if (
		FAILED(geometry->GetShapeData(nullptr, &geometryShape)) ||
		!geometryShape ||
		geometryShape->IsEmpty()
	)
	{
		return hr;
	}

	const auto solidColorBrush = static_cast<dwmcore::CSolidColorLegacyMilBrush*>(brush);
	const auto color = solidColorBrush->GetRealizedColor();
	const auto active = (color.a == 0.5f);
	const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();
	if (
		float glassOpacity, blurBalance, afterglowBalance;
		solidColorBrush->IsConstantOpaque() ||
		(
			(color.a == 0.50f || color.a == 0.25f) &&
			(
				GlassKernel::CalculateRealizedAeroParams(
					active,
					extendedAmount,
					glassOpacity,
					blurBalance,
					afterglowBalance,
					nullptr
				),
				Shared::IsGlassFullyOpaque(
					glassOpacity,
					blurBalance,
					afterglowBalance
				)
			)
		)
	)
	{
		UINT count{};
		if (!geometryShape->IsRectangles(&count))
		{
			return hr;
		}
		auto rectangles = std::make_unique_for_overwrite<D2D1_RECT_F[]>(count);
		if (!geometryShape->GetRectangles(rectangles.get(), count))
		{
			return hr;
		}

		for (const auto& rect : std::span{ rectangles.get(), count })
		{
			RETURN_IF_FAILED(
				This->DrawSolidRectangle(
					rect,
					color
				)
			);
		}
	}
	// here are the glass regions
	else if (
		(color.a == 0.50f || color.a == 0.25f) &&
		extendedAmount
	)
	{
		const auto occlusionContext = This->GetOcclusionContext();
		if (auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(occlusionContext, true); glassCoverageSet)
		{
			D2D1_RECT_F bounds{};
			RETURN_IF_FAILED(geometryShape->GetTightBounds(&bounds, occlusionContext->GetWorldTransform()));
			if (
				!wil::rect_is_empty(bounds) &&
				std::fabs(wil::rect_height(bounds) * wil::rect_width(bounds)) > 1.f
			)
			{
				RETURN_IF_FAILED(
					glassCoverageSet->Add(
						bounds,
						occlusionContext->GetCurrentZ(),
						occlusionContext->GetDeviceTransform()
					)
				);
			}
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCOcclusionContext_SetDeviceTransform(
	dwmcore::COcclusionContext* This,
	const dwmcore::CMILMatrix* matrix
)
{
	if (
		auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(This);
		glassCoverageSet
	)
	{
		glassCoverageSet->SetDeviceTransform(matrix);
	}

	return g_COcclusionContext_SetDeviceTransform_Org(This, matrix);
}

void STDMETHODCALLTYPE GlassSafetyZone::MyCOcclusionContext_Destructor(dwmcore::COcclusionContext* This)
{
	GlassCoverageSetFactory::Remove(This);
	g_shrunkCoverageSetMap.erase(This);
	return g_COcclusionContext_Destructor_Org(This);
}


bool STDMETHODCALLTYPE GlassSafetyZone::MyCArrayBasedCoverageSet_IsCovered(
	dwmcore::CArrayBasedCoverageSet* This,
	const D2D1_RECT_F& coverage,
	int depth
)
{
	const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();
	auto covered = g_CArrayBasedCoverageSet_IsCovered_Org(This, coverage, depth);

	auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(This->GetOcclusionContext());
	if (
		g_calculationContext.IsActive() &&
		extendedAmount &&
		glassCoverageSet &&
		glassCoverageSet->IsPartiallyCovered(coverage, depth)
	)
	{
		// coverage is actually paged dirty rect
		const D2D1_RECT_F extendedCoverage
		{
			coverage.left - extendedAmount,
			coverage.top - extendedAmount,
			coverage.right + extendedAmount,
			coverage.bottom + extendedAmount
		};

		// unpaged dirty rect
		if (g_calculationContext.dirtyRect)
		{
			g_calculationContext.dirtyRect->left -= extendedAmount;
			g_calculationContext.dirtyRect->top -= extendedAmount;
			g_calculationContext.dirtyRect->right += extendedAmount;
			g_calculationContext.dirtyRect->bottom += extendedAmount;
		}

		covered = g_CArrayBasedCoverageSet_IsCovered_Org(This, extendedCoverage, depth);
	}
	g_calculationContext.dirtyRect = nullptr;

	return covered;
}
bool STDMETHODCALLTYPE GlassSafetyZone::MyCOcclusionContext_IsOccluded(
	dwmcore::COcclusionContext* This,
	const D2D1_RECT_F& coverage,
	int depth,
	bool ignoreDeviceTransform
)
{
	const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();
	auto occluded = g_COcclusionContext_IsOccluded_Org(
		This, 
		coverage, 
		depth, 
		ignoreDeviceTransform
	);

	auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(This);
	if (
		g_calculationContext.IsActive() &&
		extendedAmount &&
		glassCoverageSet &&
		glassCoverageSet->IsPartiallyCovered(coverage, depth)
	)
	{
		// coverage is actually unpaged dirty rect
		const_cast<D2D1_RECT_F&>(coverage).left -= extendedAmount;
		const_cast<D2D1_RECT_F&>(coverage).top -= extendedAmount;
		const_cast<D2D1_RECT_F&>(coverage).right += extendedAmount;
		const_cast<D2D1_RECT_F&>(coverage).bottom += extendedAmount;

		occluded = g_COcclusionContext_IsOccluded_Org(
			This,
			coverage,
			depth,
			ignoreDeviceTransform
		);
	}

	return occluded;
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCOcclusionContext_PageInPixelsRectToDeviceRect(
	dwmcore::COcclusionContext* This,
	const D2D1_RECT_F& src,
	D2D1_RECT_F* dst
)
{
	if (g_calculationContext.IsActive())
	{
		g_calculationContext.dirtyRect = const_cast<D2D1_RECT_F*>(&src);
	}

	return g_COcclusionContext_PageInPixelsRectToDeviceRect_Org(
		This,
		src,
		dst
	);
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCDirtyRegion_GetUnOccludedDirtyRect(
	dwmcore::CDirtyRegion* This,
	D2D1_RECT_F* dirtyRect,
	int i,
	const D2D1_RECT_F& bounds,
	bool useSuperSample,
	const DWM::span<dwmcore::CVisual>& visuals,
	dwmcore::COcclusionContext* occlusionContext
)
{
	auto context = occlusionContext ? occlusionContext : This->GetOcclusionContext();
	auto calculationScope = EnterUnoccludedDirtyRegionCalculationContext(
		&g_calculationContext,
		context->GetFrameId() == dwmcore::GetCurrentFrameId() ? context : nullptr
	);
	auto hr = g_CDirtyRegion_GetUnOccludedDirtyRect_Org(
		This,
		dirtyRect,
		i,
		bounds,
		useSuperSample,
		visuals,
		occlusionContext
	);
	/*const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();
	if (extendedAmount)
	{
		dirtyRect->left = std::max(dirtyRect->left - extendedAmount, bounds.left);
		dirtyRect->top = std::max(dirtyRect->top - extendedAmount, bounds.top);
		dirtyRect->right = std::min(dirtyRect->right + extendedAmount, bounds.right);
		dirtyRect->bottom = std::min(dirtyRect->bottom + extendedAmount, bounds.bottom);
	}*/

	return hr;
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCDirtyRegion_GetOptimizedRect(
	dwmcore::CDirtyRegion* This,
	D2D1_RECT_F* dirtyRect,
	int i,
	const D2D1_RECT_F& bounds,
	const dwmcore::CRegion* region,
	const dwmcore::CMILMatrix* matrix,
	bool useSuperSample,
	const DWM::span<dwmcore::CVisual>& visuals,
	dwmcore::COcclusionContext* occlusionContext
)
{
	auto context = occlusionContext ? occlusionContext : This->GetOcclusionContext();
	auto calculationScope = EnterUnoccludedDirtyRegionCalculationContext(
		&g_calculationContext,
		context->GetFrameId() == dwmcore::GetCurrentFrameId() ? context : nullptr
	);
	auto hr = g_CDirtyRegion_GetOptimizedRect_Org(
		This,
		dirtyRect,
		i,
		bounds,
		region,
		matrix,
		useSuperSample,
		visuals,
		occlusionContext
	);

	return hr;
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCTreeDirty_GetOptimizedRect(
	dwmcore::CTreeDirty* This,
	D2D1_RECT_F* dirtyRect,
	UINT i,
	const D2D1_RECT_F& bounds,
	dwmcore::COcclusionContext* occlusionContext,
	const dwmcore::CRegion* region,
	const dwmcore::CMILMatrix* matrix,
	bool useSuperSample,
	const DWM::span<dwmcore::CVisual>& visuals
)
{
	auto calculationScope = EnterUnoccludedDirtyRegionCalculationContext(
		&g_calculationContext,
		occlusionContext
	);
	auto hr = g_CTreeDirty_GetOptimizedRect_Org(
		This,
		dirtyRect,
		i,
		bounds,
		occlusionContext,
		region,
		matrix,
		useSuperSample,
		visuals
	);

	return hr;
}

template <typename T>
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCDrawingContext_DrawVisualTree(
	dwmcore::CDrawingContext* This,
	const D2D1_RECT_F& rectangle,
	dwmcore::COcclusionContext* occlusionContext,
	bool useSuperSample,
	T&& callback
)
{
	HRESULT hr{ S_OK };

	do
	{
		if (GlassKernel::IsInCVIHierarchy())
		{
			break;
		}

		if (
			!occlusionContext ||
			occlusionContext->GetFrameId() != dwmcore::GetCurrentFrameId()
		)
		{
			break;
		}

		{
			occlusionContext->SetDeviceTransform(This->GetDeviceTransform());
			const auto transformedRect = occlusionContext->PageInPixelsRectToDeviceRect(rectangle);
			const auto glassCoverageSet = GlassCoverageSetFactory::GetOrCreate(occlusionContext);

			if (
				(
					!glassCoverageSet ||
					!glassCoverageSet->IsOverlapped(transformedRect)
				) &&
				!Shared::g_overrideAccent
			)
			{
				break;
			}
		}

		const auto extendedAmount = GlassKernel::GetBlurExtendedAmount();
		if (!extendedAmount)
		{
			break;
		}

		const auto renderTargetBitmap = This->AcquireRenderTargetBitmap(extendedAmount > 0.f);
		if (!renderTargetBitmap)
		{
			break;
		}

		const auto context = This->GetD2DContext()->GetDeviceContext();
		winrt::com_ptr<ID2D1Device> device{ nullptr };
		context->GetDevice(device.put());
		// device lost
		if (g_deviceNoRef != device.get())
		{
			g_deviceNoRef = device.get();
			g_glassSafetyZoneLayer.Reset();
		}

		if (g_glassSafetyZoneLayer.GetOwner())
		{
			break;
		}

		D2D1_RECT_F extendedPixelRectangle
		{
			rectangle.left - extendedAmount,
			rectangle.top - extendedAmount,
			rectangle.right + extendedAmount,
			rectangle.bottom + extendedAmount,
		};
		if (
			FAILED(
				g_glassSafetyZoneLayer.Push(
					context,
					renderTargetBitmap.get(),
					This->GetDeviceTransform()->GetD2DMatrix(),
					rectangle,
					extendedAmount,
					useSuperSample
				)
			)
		)
		{
			break;
		}

		ShrinkOccluderGlassAboved(occlusionContext);
		const auto status = GlassRenderer::ControlBlurRendering(false);
		hr = callback(extendedPixelRectangle);
		GlassRenderer::ControlBlurRendering(status);

		This->FlushD2D();
		g_glassSafetyZoneLayer.Pop();

		return hr;
	}
	while (false);

	const auto status = GlassRenderer::ControlBlurRendering(true);
	hr = callback(rectangle);
	GlassRenderer::ControlBlurRendering(status);

	return hr;
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCDrawingContext_DrawVisualTree_Win10(
	dwmcore::CDrawingContext* This,
	dwmcore::CVisualTree* tree,
	const D2D1_RECT_F& rectangle,
	dwmcore::COcclusionContext* occlusionContext,
	int clearMode,
	bool useSuperSample
)
{
	return MyCDrawingContext_DrawVisualTree(
		This,
		rectangle,
		occlusionContext,
		useSuperSample,
		[=](const D2D1_RECT_F& replacedRectangle)
		{
			return reinterpret_cast<decltype(&MyCDrawingContext_DrawVisualTree_Win10)>(g_CDrawingContext_DrawVisualTree_Org)(
				This,
				tree,
				replacedRectangle,
				occlusionContext,
				clearMode,
				useSuperSample
			);
		}
	);
}
HRESULT STDMETHODCALLTYPE GlassSafetyZone::MyCDrawingContext_DrawVisualTree_Win11(
	dwmcore::CDrawingContext* This,
	dwmcore::CVisualTree* tree,
	const D2D1_RECT_F& rectangle,
	dwmcore::COcclusionContext* occlusionContext,
	int clearMode,
	bool useSuperSample,
	dwmcore::CVisual* visualOverride
)
{
	return MyCDrawingContext_DrawVisualTree(
		This,
		rectangle,
		occlusionContext,
		useSuperSample,
		[=](const D2D1_RECT_F& replacedRectangle)
		{
			return reinterpret_cast<decltype(&MyCDrawingContext_DrawVisualTree_Win11)>(g_CDrawingContext_DrawVisualTree_Org)(
				This,
				tree,
				replacedRectangle,
				occlusionContext,
				clearMode,
				useSuperSample,
				visualOverride
			);
		}
	);
}

void GlassSafetyZone::Update([[maybe_unused]] GlassEngine::UpdateType type)
{
	if (Shared::IsGlassFullyOpaque())
	{
		GlassCoverageSetFactory::Shutdown();
	}
}

void GlassSafetyZone::Startup()
{
	dwmcore::g_projectionArray.ApplyToVariable("COcclusionContext::Compute", g_COcclusionContext_Compute_Org);
	dwmcore::g_projectionArray.ApplyToVariable("COcclusionContext::SetDeviceTransform", g_COcclusionContext_SetDeviceTransform_Org);
	dwmcore::g_projectionArray.ApplyToVariable("COcclusionContext::~COcclusionContext", g_COcclusionContext_Destructor_Org);

	dwmcore::g_projectionArray.ApplyToVariable("CArrayBasedCoverageSet::IsCovered", g_CArrayBasedCoverageSet_IsCovered_Org);
	dwmcore::g_projectionArray.ApplyToVariable("COcclusionContext::IsOccluded", g_COcclusionContext_IsOccluded_Org);
	dwmcore::g_projectionArray.ApplyToVariable("COcclusionContext::PageInPixelsRectToDeviceRect", g_COcclusionContext_PageInPixelsRectToDeviceRect_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CDirtyRegion::GetUnOccludedDirtyRect", g_CDirtyRegion_GetUnOccludedDirtyRect_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CDirtyRegion::GetOptimizedRect", g_CDirtyRegion_GetOptimizedRect_Org);
	dwmcore::g_projectionArray.ApplyToVariable("CTreeDirty::GetOptimizedRect", g_CTreeDirty_GetOptimizedRect_Org);
	
	dwmcore::g_projectionArray.ApplyToVariable("CDrawingContext::DrawVisualTree", g_CDrawingContext_DrawVisualTree_Org);
	
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Attach(&g_COcclusionContext_Compute_Org, MyCOcclusionContext_Compute);
			HookHelper::Detours::Attach(&g_COcclusionContext_SetDeviceTransform_Org, MyCOcclusionContext_SetDeviceTransform);
			HookHelper::Detours::Attach(&g_COcclusionContext_Destructor_Org, MyCOcclusionContext_Destructor);

			if (dwmcore::g_buildNumber < os::build_w11_24h2)
			{
				HookHelper::Detours::Attach(&g_CArrayBasedCoverageSet_IsCovered_Org, MyCArrayBasedCoverageSet_IsCovered);
				HookHelper::Detours::Attach(&g_COcclusionContext_PageInPixelsRectToDeviceRect_Org, MyCOcclusionContext_PageInPixelsRectToDeviceRect);
				if (dwmcore::g_buildNumber < os::build_w11_21h2)
				{
					HookHelper::Detours::Attach(&g_CDirtyRegion_GetUnOccludedDirtyRect_Org, MyCDirtyRegion_GetUnOccludedDirtyRect);
				}
				else
				{
					HookHelper::Detours::Attach(&g_CDirtyRegion_GetOptimizedRect_Org, MyCDirtyRegion_GetOptimizedRect);
				}
			}
			else
			{
				HookHelper::Detours::Attach(&g_COcclusionContext_IsOccluded_Org, MyCOcclusionContext_IsOccluded);
				HookHelper::Detours::Attach(&g_CTreeDirty_GetOptimizedRect_Org, MyCTreeDirty_GetOptimizedRect);
			}

			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Attach(&g_CDrawingContext_DrawVisualTree_Org, MyCDrawingContext_DrawVisualTree_Win10);
			}
			else
			{
				HookHelper::Detours::Attach(&g_CDrawingContext_DrawVisualTree_Org, MyCDrawingContext_DrawVisualTree_Win11);
			}
		})
	);
}

void GlassSafetyZone::Shutdown()
{
	THROW_IF_FAILED(
		HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_COcclusionContext_Compute_Org, MyCOcclusionContext_Compute);
			HookHelper::Detours::Detach(&g_COcclusionContext_SetDeviceTransform_Org, MyCOcclusionContext_SetDeviceTransform);
			HookHelper::Detours::Detach(&g_COcclusionContext_Destructor_Org, MyCOcclusionContext_Destructor);

			if (dwmcore::g_buildNumber < os::build_w11_24h2)
			{
				HookHelper::Detours::Detach(&g_CArrayBasedCoverageSet_IsCovered_Org, MyCArrayBasedCoverageSet_IsCovered);
				HookHelper::Detours::Detach(&g_COcclusionContext_PageInPixelsRectToDeviceRect_Org, MyCOcclusionContext_PageInPixelsRectToDeviceRect);
				if (dwmcore::g_buildNumber < os::build_w11_21h2)
				{
					HookHelper::Detours::Detach(&g_CDirtyRegion_GetUnOccludedDirtyRect_Org, MyCDirtyRegion_GetUnOccludedDirtyRect);
				}
				else
				{
					HookHelper::Detours::Detach(&g_CDirtyRegion_GetOptimizedRect_Org, MyCDirtyRegion_GetOptimizedRect);
				}
			}
			else
			{
				HookHelper::Detours::Detach(&g_COcclusionContext_IsOccluded_Org, MyCOcclusionContext_IsOccluded);
				HookHelper::Detours::Detach(&g_CTreeDirty_GetOptimizedRect_Org, MyCTreeDirty_GetOptimizedRect);
			}

			if (dwmcore::g_buildNumber < os::build_w11_21h2)
			{
				HookHelper::Detours::Detach(&g_CDrawingContext_DrawVisualTree_Org, MyCDrawingContext_DrawVisualTree_Win10);
			}
			else
			{
				HookHelper::Detours::Detach(&g_CDrawingContext_DrawVisualTree_Org, MyCDrawingContext_DrawVisualTree_Win11);
			}
		})
	);

	Sleep(1);

	if (g_COcclusionContext_DrawGeometry_Org)
	{
		HookHelper::WritePointer(g_COcclusionContext_DrawGeometry_Org_Address, g_COcclusionContext_DrawGeometry_Org);
	}

	GlassCoverageSetFactory::Shutdown();
	g_shrunkCoverageSetMap.clear();
	g_glassSafetyZoneLayer.Reset();
}