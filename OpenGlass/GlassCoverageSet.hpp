#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"

namespace OpenGlass
{
	// [Guid("5258B8F9-28D5-4F90-A2D9-5E716239DD57")]
	DECLARE_INTERFACE_IID_(IGlassCoverageSet, IUnknown, "5258B8F9-28D5-4F90-A2D9-5E716239DD57")
	{
		virtual void STDMETHODCALLTYPE SetDeviceTransform(const dwmcore::CMILMatrix * matrix) = 0;
		virtual HRESULT STDMETHODCALLTYPE Add(
			const D2D1_RECT_F& coverage,
			int depth,
			const dwmcore::CMILMatrix* matrix
		) = 0;
		virtual void STDMETHODCALLTYPE Clear() = 0;
		virtual std::span<dwmcore::CZOrderedRect> STDMETHODCALLTYPE GetViews() const = 0;
		virtual bool STDMETHODCALLTYPE IsFullyCovered(
			const D2D1_RECT_F & coverage,
			int depth
		) const = 0;
		virtual bool STDMETHODCALLTYPE IsPartiallyCovered(
			const D2D1_RECT_F& coverage,
			int depth
		) const = 0;
		virtual bool STDMETHODCALLTYPE IsVisible(
			const D2D1_RECT_F& coverage,
			const dwmcore::CArrayBasedCoverageSet* occlusionCoverageSet
		) const = 0;
	};

	namespace GlassCoverageSetFactory
	{
		winrt::com_ptr<IGlassCoverageSet> GetOrCreate(
			dwmcore::COcclusionContext* occlusionContext,
			bool createIfNecessary = false
		);
		void Remove(dwmcore::COcclusionContext* occlusionContext);
		void Shutdown();
	}
}
