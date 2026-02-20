#pragma once
#include "framework.hpp"
#include "OSHelper.hpp"

namespace OpenGlass
{
	interface DX_DECLARE_INTERFACE("8C803E30-9E41-4DDF-B206-46F28E90E405") IDXGISwapChainTest : public IUnknown
	{
		virtual bool HasProxyFrontBufferSurface() = 0;
		virtual HRESULT GetFrameStatisticsTest(void*) = 0;
		virtual void EmulateXBOXBehavior(BOOL) = 0;
		virtual DXGI_COLOR_SPACE_TYPE GetColorSpace1() = 0;
		// ...
	};

	interface DX_DECLARE_INTERFACE("f69f223b-45d3-4aa0-98c8-c40c2b231029") IDXGISwapChainDWM1 : public IDXGIDeviceSubObject
	{
		virtual HRESULT Present(
			/* [in] */ UINT SyncInterval,
			/* [in] */ UINT Flags
		) = 0;

		virtual HRESULT GetBuffer(
			/* [in] */ UINT Buffer,
			/* [annotation][in] */
			_In_  REFIID riid,
			/* [annotation][out][in] */
			_COM_Outptr_  void** ppSurface
		) = 0;

		virtual HRESULT GetDesc(
			/* [annotation][out] */
			_Out_  DXGI_SWAP_CHAIN_DESC* pDesc
		) = 0;

		virtual HRESULT ResizeBuffers(
			/* [in] */ UINT BufferCount,
			/* [in] */ UINT Width,
			/* [in] */ UINT Height,
			/* [in] */ DXGI_FORMAT NewFormat,
			/* [in] */ UINT SwapChainFlags) = 0;

		virtual HRESULT ResizeTarget(
			/* [annotation][in] */
			_In_  const DXGI_MODE_DESC* pNewTargetParameters
		) = 0;

		virtual HRESULT GetContainingOutput(
			/* [annotation][out] */
			_COM_Outptr_  IDXGIOutput** ppOutput
		) = 0;

		virtual HRESULT GetFrameStatistics(
			/* [annotation][out] */
			_Out_  DXGI_FRAME_STATISTICS* pStats
		) = 0;

		virtual HRESULT GetLastPresentCount(
			/* [annotation][out] */
			_Out_  UINT* pLastPresentCount
		) = 0;

		virtual HRESULT PresentDWM(
			UINT,
			UINT,
			UINT,
			RECT const*,
			UINT,
			struct DXGI_SCROLL_RECT const*,
			IDXGIResource*,
			UINT
		) = 0;
		virtual HRESULT GetLogicalSurfaceHandle(unsigned __int64*) = 0;
		virtual HRESULT CheckDirectFlipSupport(UINT, IDXGIResource*, BOOL*) = 0;
		virtual HRESULT GetCompositionSurface(void**) = 0;
		virtual HRESULT GetFrameStatisticsDWM(struct DXGI_FRAME_STATISTICS_DWM*) = 0;
		virtual HRESULT GetMultiplaneOverlayCaps(struct DXGI_MULTIPLANE_OVERLAY_CAPS*) = 0;
		virtual HRESULT CheckMultiplaneOverlaySupport(
			UINT,
			struct DXGI_CHECK_MULTIPLANEOVERLAYSUPPORT_PLANE_INFO const*,
			int*,
			int*
		) = 0;
		virtual HRESULT PresentMultiplaneOverlay(
			UINT,
			UINT,
			DXGI_HDR_METADATA_TYPE,
			void const*,
			UINT,
			struct DXGI_PRESENT_MULTIPLANE_OVERLAY const*
		) = 0;
		virtual HRESULT CheckPresentDurationSupport(UINT, UINT*, UINT*) = 0;
		virtual HRESULT SetPrivateFrameDuration(UINT, UINT) = 0;
		virtual HRESULT SetHardwareProtection(BOOL) = 0;
		virtual HRESULT GetHardwareProtection(BOOL*) = 0;
		// ...
	};

	struct DXGI_OUTPUT_DWM_DESC
	{
		LUID AdapterLuid;
		UINT32 SourceId;
		CHAR Reserved[186];

		float GetSdrWhiteLevelInNits() const
		{
			if (os::buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<float const*>(this)[39];
			}
			else
			{
				return reinterpret_cast<float const*>(this)[44];
			}
		}
	};
	interface DX_DECLARE_INTERFACE("6f66a9a0-bece-4ee8-b11b-990eb38ed976") IDXGIOutputDWM : public IUnknown
	{
		virtual bool HasDDAClient(void) = 0;
		virtual HRESULT GetDesc(DXGI_OUTPUT_DWM_DESC*);
		// ...
	};
}
