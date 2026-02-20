#pragma once
#include "Util.hpp"

namespace OpenGlass::dwmcore
{
	struct CArrayBasedCoverageSet_GetAntiOccluderArray_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 52 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0},
				Util::OffsetInfo{ .offset = 49 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 }
			};
		}
	};
	struct CArrayBasedCoverageSet_GetOccluderArray_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 24, .build = os::build_w10_2004, .revision = 0},
				Util::OffsetInfo{ .offset = 0, .build = 0, .revision = 0 }
			};
		}
	};
	struct CRenderData_GetResources_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 104, .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 120, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 128, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 136, .build = 0, .revision = 0 }
			};
		}
	};
	struct CSolidColorLegacyMilBrush_GetRealizedColor_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 88, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 96, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 104, .build = 0, .revision = 0 }
			};
		}
	};
	struct CImageLegacyMilBrush_GetImageSource_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 23 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 24 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CImageLegacyMilBrush_GetOpacityValue_Double_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 15 * sizeof(double), .build = 0, .revision = 0 }
			};
		}
	};
	struct CImageLegacyMilBrush_GetOpacityValue_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 31 * sizeof(float), .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(float), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 32 * sizeof(float), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 16 * sizeof(float), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 18 * sizeof(float), .build = 0, .revision = 0 }
			};
		}
	};
	struct CImageLegacyMilBrush_GetFloatResource_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 16 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 17 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 9 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 10 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CImageLegacyMilBrush_GetViewport_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 160, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 168, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 104, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 112, .build = 0, .revision = 0 }
			};
		}
	};
	struct CImageLegacyMilBrush_GetViewbox_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 184, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 192, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 120, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 128, .build = 0, .revision = 0 }
			};
		}
	};
	struct CShape_GetTightBounds_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 32, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 48, .build = 0, .revision = 0 }
			};
		}
	};
	struct CShape_IsRectangles_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{.offset = 40, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{.offset = 64, .build = 0, .revision = 0 }
			};
		}
	};
	struct CShape_GetRectangles_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{.offset = 48, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{.offset = 80, .build = 0, .revision = 0 }
			};
		}
	};
	struct IDrawingContext_GetDrawingContext_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 0, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = -16, .build = 0, .revision = 0 }
			};
		}
	};
	struct RenderTargetInfo_GetSDRBoost_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 16, .build = 0, .revision = 0 }
			};
		}
	};
	struct ID2DContextOwner_GetCurrentZ_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 32, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 24, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8, .build = 0, .revision = 0 }
			};
		}
	};
	struct ID2DContextOwner_GetCurrentRenderTargetInfo_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 80, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 40, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 16, .build = 0, .revision = 0 }
			};
		}
	};
	struct CD2DContext_GetDeviceContext_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 29 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 25 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CD3DDevice_GetDevice_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 79 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 74 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CD3DSurface_GetTexture2D_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 16 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 }
			};
		}
	};
	struct CD3DSurface_GetShaderResourceView_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 25 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 }
			};
		}
	};
	struct CD3DSurface_GetRenderTargetView_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 24 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 }
			};
		}
	};
	struct IRenderTarget_GetTargetSurfaceNoRef_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 13 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 }
			};
		}
	};
	struct IRenderTarget_IsHardwareProtected_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 18 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 }
			};
		}
	};
	struct IRenderTarget_GetSDRBoost_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 19 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 }
			};
		}
	};
	struct IDeviceResource_IsHardwareProtected_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct IDeviceTarget_GetRenderTargetView_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 7 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 22 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct IDeviceTarget_GetDeviceTexture_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = -24, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = -16, .build = 0, .revision = 0 }
			};
		}
	};
	struct IDeviceTarget_GetDeviceResource_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = -312, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = -168, .build = 0, .revision = 0 }
			};
		}
	};
	struct IDeviceTexture_GetTexture2D_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 0 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 15 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct IDeviceTexture_GetShaderResourceView_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 1 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 16 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CD3DDevice_GetImmediateContext_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 80 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 75 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CD3DDevice_GetD2DContext_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 0, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 16, .build = 0, .revision = 0 }
			};
		}
	};
	struct CDrawingContext_GetD3DDevice_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 48 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 5 * sizeof(ULONG_PTR), .build = 0, .revision = 0}
			};
		}
	};
	struct CDrawingContext_GetInterface_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 0, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 16, .build = 0, .revision = 0 }
			};
		}
	};
	struct CDrawingContext_GetD2DContextOwner_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo{ .offset = 8, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 24, .build = 0, .revision = 0 }
			};
		}
	};
	struct CDrawingContext_GetOcclusionContext_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 6272,
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 5936,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 7944,
					.build = os::build_w11_22h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 8072,
					.build = os::build_w11_24h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 7960,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct CDrawingContext_GetWorldTransform_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 480,
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 408,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 368,
					.build = os::build_w11_22h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 400,
					.build = os::build_w11_24h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 288,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct CDrawingContext_GetDeviceTransform_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 3648,
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 96,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct CDirtyRegion_GetOcclusionContext_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 16,
					.build = os::build_w11_24h2,
					.revision = 0
				}
			};
		}
	};
	struct COcclusionContext_GetFrameId_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 16,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 24,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct COcclusionContext_GetCurrentZ_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 1032,
					.build = os::build_w10_1903,
					.revision = 752
				},
				Util::OffsetInfo
				{
					.offset = 1040,
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1456,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1428,
					.build = os::build_w11_24h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1316,
					.build = os::build_w11_24h2,
					.revision = os::revision_24h2_with_25h2_code_staged
				},
				Util::OffsetInfo
				{
					.offset = 1708,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct COcclusionContext_GetWorldTransform_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 8,
					.build = os::build_w10_1903,
					.revision = 752
				},
				Util::OffsetInfo
				{
					.offset = 16,
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 24,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 32,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct COcclusionContext_GetDeviceTransform_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 820,
					.build = os::build_w10_1903,
					.revision = 752
				},
				Util::OffsetInfo
				{
					.offset = 828,
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1248,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1208,
					.build = os::build_w11_24h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1180,
					.build = os::build_w11_24h2,
					.revision = os::revision_24h2_with_25h2_code_staged
				},
				Util::OffsetInfo
				{
					.offset = 1572,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct COcclusionContext_GetDeviceTransformFlag_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 816, 
					.build = os::build_w10_1903,
					.revision = 752
				},
				Util::OffsetInfo
				{
					.offset = 824, 
					.build = os::build_w10_2004,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1244,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1204,
					.build = os::build_w11_24h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 1172,
					.build = os::build_w11_24h2,
					.revision = os::revision_24h2_with_25h2_code_staged
				},
				Util::OffsetInfo
				{
					.offset = 1564,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
	struct COcclusionContext_GetArrayBasedCoverageSet_Offsets
	{
		consteval static auto operator()()
		{
			return std::array
			{
				Util::OffsetInfo
				{
					.offset = 392,
					.build = os::build_w10_1903,
					.revision = 752
				},
				Util::OffsetInfo
				{
					.offset = 400,
					.build = os::build_w10_2004,
					.revision = 0
				},

				Util::OffsetInfo
				{
					.offset = 408,
					.build = os::build_w11_21h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 448,
					.build = os::build_w11_24h2,
					.revision = 0
				},
				Util::OffsetInfo
				{
					.offset = 568,
					.build = os::build_w11_24h2,
					.revision = os::revision_24h2_with_25h2_code_staged
				},
				Util::OffsetInfo
				{
					.offset = 616,
					.build = 0,
					.revision = 0
				}
			};
		}
	};
}
