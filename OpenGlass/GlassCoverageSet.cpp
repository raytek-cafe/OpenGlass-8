#include "pch.h"
#include "GlassCoverageSet.hpp"

using namespace OpenGlass;
namespace OpenGlass::GlassCoverageSetFactory
{
	std::unordered_map<dwmcore::COcclusionContext*, winrt::com_ptr<IGlassCoverageSet>> g_glassCoverageSetMap{};

	class CArrayBasedGlassCoverageSet : public winrt::implements<CArrayBasedGlassCoverageSet, IGlassCoverageSet, winrt::non_agile, winrt::no_weak_ref>
	{
		DWM::MyDynArrayImpl<dwmcore::CZOrderedRect> m_array{};
	public:
		void STDMETHODCALLTYPE SetDeviceTransform(const dwmcore::CMILMatrix* matrix) override;
		HRESULT STDMETHODCALLTYPE Add(
			const D2D1_RECT_F& coverage,
			int depth,
			const dwmcore::CMILMatrix* matrix
		) override;
		void STDMETHODCALLTYPE Clear() override { m_array.Clear(); }
		std::span<dwmcore::CZOrderedRect> STDMETHODCALLTYPE GetViews() const override { return m_array.views(); }
		bool STDMETHODCALLTYPE IsFullyCovered(
			const D2D1_RECT_F& coverage,
			int depth
		) const override;
		bool STDMETHODCALLTYPE IsPartiallyCovered(
			const D2D1_RECT_F& coverage,
			int depth
		) const override;
		bool STDMETHODCALLTYPE IsVisible(
			const D2D1_RECT_F& coverage,
			const dwmcore::CArrayBasedCoverageSet* occlusionCoverageSet
		) const override;
	};
}

void STDMETHODCALLTYPE GlassCoverageSetFactory::CArrayBasedGlassCoverageSet::SetDeviceTransform(const dwmcore::CMILMatrix* matrix)
{
	for (auto& zorderedRect : m_array.views())
	{
		zorderedRect.UpdateDeviceRect(matrix);
	}
}
HRESULT STDMETHODCALLTYPE GlassCoverageSetFactory::CArrayBasedGlassCoverageSet::Add(
	const D2D1_RECT_F& coverage,
	int depth,
	const dwmcore::CMILMatrix* matrix
)
{
	m_array.AddInPlace(coverage, depth, matrix);
	return S_OK;
}

bool STDMETHODCALLTYPE GlassCoverageSetFactory::CArrayBasedGlassCoverageSet::IsFullyCovered(
	const D2D1_RECT_F& coverage,
	int depth
) const
{
	for (const auto& zorderedRect : m_array.views())
	{
		if (zorderedRect.m_depth >= depth)
		{
			break;
		}

		if (
			!wil::rect_is_empty(zorderedRect.m_transformedRect) &&
			std::abs(wil::rect_height(zorderedRect.m_transformedRect) * wil::rect_width(zorderedRect.m_transformedRect)) > 1.f &&

			coverage.left >= zorderedRect.m_transformedRect.left &&
			coverage.top >= zorderedRect.m_transformedRect.top &&
			coverage.right <= zorderedRect.m_transformedRect.right &&
			coverage.bottom <= zorderedRect.m_transformedRect.bottom
		)
		{
			return true;
		}
	}

	return false;
}

bool STDMETHODCALLTYPE GlassCoverageSetFactory::CArrayBasedGlassCoverageSet::IsPartiallyCovered(
	const D2D1_RECT_F& coverage,
	int depth
) const
{
	for (const auto& zorderedRect : m_array.views())
	{
		if (zorderedRect.m_depth >= depth)
		{
			break;
		}

		if (
			!wil::rect_is_empty(zorderedRect.m_transformedRect) &&
			std::abs(wil::rect_height(zorderedRect.m_transformedRect) * wil::rect_width(zorderedRect.m_transformedRect)) > 1.f &&

			RectF::DoesIntersectUnsafe(zorderedRect.m_transformedRect, coverage)
		)
		{
			return true;
		}
	}

	return false;
}

bool STDMETHODCALLTYPE GlassCoverageSetFactory::CArrayBasedGlassCoverageSet::IsVisible(
	const D2D1_RECT_F& coverage,
	const dwmcore::CArrayBasedCoverageSet* occlusionCoverageSet
) const
{
	for (const auto& zorderedRect : m_array.views())
	{
		if (
			!wil::rect_is_empty(zorderedRect.m_transformedRect) &&
			std::abs(wil::rect_height(zorderedRect.m_transformedRect) * wil::rect_width(zorderedRect.m_transformedRect)) > 1.f &&

			RectF::DoesIntersectUnsafe(zorderedRect.m_transformedRect, coverage) &&
			!occlusionCoverageSet->IsCovered(coverage, zorderedRect.m_depth)
		)
		{
			return true;
		}
	}

	return false;
}

winrt::com_ptr<IGlassCoverageSet> GlassCoverageSetFactory::GetOrCreate(dwmcore::COcclusionContext* occlusionContext, bool createIfNecessary)
{
	auto it = g_glassCoverageSetMap.find(occlusionContext);

	if (createIfNecessary)
	{
		if (it == g_glassCoverageSetMap.end())
		{
			auto result = g_glassCoverageSetMap.emplace(occlusionContext, winrt::make<CArrayBasedGlassCoverageSet>());
			if (result.second == true)
			{
				it = result.first;
			}
		}
	}

	return it == g_glassCoverageSetMap.end() ? nullptr : it->second;
}
void GlassCoverageSetFactory::Remove(dwmcore::COcclusionContext* occlusionContext)
{
	auto it = g_glassCoverageSetMap.find(occlusionContext);

	if (it != g_glassCoverageSetMap.end())
	{
		g_glassCoverageSetMap.erase(it);
	}
}
void GlassCoverageSetFactory::Shutdown()
{
	g_glassCoverageSetMap.clear();
}
