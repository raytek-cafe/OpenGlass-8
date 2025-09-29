#pragma once
#include "Util.hpp"

namespace OpenGlass::uDWM
{
	// CVisual flags and fields
	struct CVisual_IsRTLMirrored_ByteOffset_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 84, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 92, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 36, .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual_GetScale_MilSizeD_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 168, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 176, .build = os::build_w11_24h2, .revision = 0 }
			};
		}
	};
	struct CVisual_GetScale_D2D1_SIZE_F_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 112, .build = 0, .revision = 0 } };
		}
	};
	struct CVisual_GetSize_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 15 * (LONGLONG)sizeof(SIZE), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 16 * (LONGLONG)sizeof(SIZE), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 9 * (LONGLONG)sizeof(SIZE), .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual_GetOffset_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 14 * (LONGLONG)sizeof(POINT), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 15 * (LONGLONG)sizeof(POINT), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8 * (LONGLONG)sizeof(POINT), .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual_SetExcludeSubtree_ByteOffset_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 84, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 92, .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual_GetVisualCollection_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 32, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 144, .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual_GetVisualProxy_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } };
		}
	};
	struct CVisual_GetTransformParent_Offsets {
		consteval static auto operator()() {
			return std::array{
				Util::OffsetInfo{ .offset = 10 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0},
				Util::OffsetInfo{ .offset = 11 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8 * sizeof(ULONG_PTR),  .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual_GetDirtyFlags_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 20 * 4LL, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 22 * 4LL, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8 * 4LL,  .build = 0, .revision = 0 }
			};
		}
	};
	struct CVisual__ValidateVisual_Offsets {
		consteval static auto operator()() {
			return std::array{
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0}
			};
		}
	};

	// CRenderDataVisual
	struct CRenderDataVisual_GetInstructions_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 248, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 256, .build = os::build_w11_23h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 256, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 208, .build = 0, .revision = 0 }
			};
		}
	};

	// CDrawGeometryInstruction
	struct CDrawGeometryInstruction_GetBrush_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } };
		}
	};
	struct CDrawGeometryInstruction_GetGeometry_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } };
		}
	};

	// CText flags
	struct CText_RTL_FlagByteOffset_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 280, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 288, .build = 0, .revision = 0 }
			};
		}
	};

	// IText and CDWriteText cross-offsets
	struct IText_GetDWriteText_NegativeOffset_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = -272, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = -168, .build = 0, .revision = 0 }
			};
		}
	};
	struct IText_RTL_Index_Offsets { // byte index bits at absolute offsets
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 256, .build = 0, .revision = 0 } };
		}
	};
	struct IText_Reverse_Index_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 257, .build = 0, .revision = 0 } };
		}
	};
	struct CDWriteText_GetTextInterface_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 272, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 168, .build = 0, .revision = 0 }
			};
		}
	};

	// CAtlasedImage
	struct CAtlasedImage_GetPartId_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 30 * 4LL, .build = 0, .revision = 0 } };
		}
	};

	// CButton assorted
	struct CButton_GetGlyphOpacity_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 101 * 4LL, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 89 * 4LL,  .build = 0, .revision = 0 }
			};
		}
	};
	struct CButton_GetButtonState_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 280, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 288, .build = 0, .revision = 0 }
			};
		}
	};
	struct CButton_GetTimeline_Offsets {
		consteval static auto operator()(){
			return std::array{ Util::OffsetInfo{ .offset = 49 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } };
		}
	};
	struct CButton_GetGlyphBitmapArray_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 304, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 312, .build = 0, .revision = 0 }
			};
		}
	};
	struct CButton_GetButtonBitmapArray_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 336, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 344, .build = 0, .revision = 0 }
			};
		}
	};

	// CWindowData
	struct CWindowData_GetHwnd_Offsets {
		consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 5 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; }
	};
	struct CWindowData_GetWindowContext_Offsets {
		consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; }
	};
	struct CWindowData_GetWindow_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 49 * sizeof(ULONG_PTR), .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 50 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 48 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 55 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CWindowData_GetAccentPolicy_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 152, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 168, .build = 0, .revision = 0 }
			};
		}
	};
	struct CWindowData_GetSystemBackdropType_Index_Offsets {
		consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 51 * 4LL, .build = 0, .revision = 0 } }; }
	};
	struct CWindowData_ColorOverride_Index_Offsets {
		consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 47 * 4LL, .build = 0, .revision = 0 } }; }
	};
	struct CWindowData_GetWindowDPI_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 324, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 348, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 87 * 4LL, .build = 0, .revision = 0 }
			};
		}
	};
	struct CWindowData_GetNonClientAttribute_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 596, .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 604, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 608, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 664, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 672, .build = 0, .revision = 0 }
			};
		}
	};
	struct CWindowData_GetClientBlurAttribute_Offsets {
		consteval static auto operator()() {
			return std::array{
				Util::OffsetInfo{ .offset = 609, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 665, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 673, .build = 0, .revision = 0 }
			};
		}
	};
	struct CWindowData_GetExtendedFrameMargins_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 80, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 96, .build = 0, .revision = 0 }
			};
		}
	};

	// CLivePreview
	struct CLivePreview_GetThumbnailVisual_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 62 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 63 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 59 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 53 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CLivePreview_GetGlassVisual_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 55 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CLivePreview_GetLivePreviewResourceArray_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 368, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 376, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 328, .build = 0, .revision = 0 }
			};
		}
	};
	struct CLivePreview_GetLivePreviewVisualArray_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 304, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 312, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 264, .build = 0, .revision = 0 }
			};
		}
	};
	struct CLivePreview_IsWindowIncluded_ByteOffset_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 611, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 675, .build = 0, .revision = 0 }
			};
		}
	};
	struct CLivePreview_IsWindowCloneAsWindowFrames_DwordIndex_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 28 * 4LL, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 33 * 4LL, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 32 * 4LL, .build = 0, .revision = 0 }
			};
		}
	};

	// CTopLevelWindow indices
	struct CTopLevelWindow_GetData_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 90 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 91 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 94 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 89 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetTextVisual_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 67 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetIconVisual_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 66 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 68 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 72 * sizeof(ULONG_PTR), .build = os::build_w11_23h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 67 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetDWriteTextVisual_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
					Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
					Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetNonClientVisual_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 32 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 36 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetAccent_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 35 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 32 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetLegacyVisual_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 35 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 36 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetClientBlurVisual_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 36 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 42 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetWindowBorder_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 28 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetIsBorderUpdatesSuppressed_ByteIndex_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 888, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 864, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 832, .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_StateDwordIndex_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 146 * 4LL, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 148 * 4LL, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 152 * 4LL, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 156 * 4LL, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 146 * 4LL, .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_HasNonClientArea_BaseDwordIndex_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 151 * 4LL, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 153 * 4LL, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 157 * 4LL, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 161 * 4LL, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 151 * 4LL, .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetCaptionColorizationParameters_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 72 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 73 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 75 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 77 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 71 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetButton_BasePointerIndex_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 63 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 66 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetFrameThickness_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 96, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 112, .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetMarginsVisibleOutside_Zoomed_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 644, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 660, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 676, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 636, .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetMarginsVisibleOutside_Normal_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 628, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 644, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 660, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 620, .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetBorderMargins_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 596, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 612, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 628, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 588, .build = 0, .revision = 0 }
			};
		}
	};

	// CTopLevelWindow: indices that moved to LegacyVisual on >= 22H2
	struct CTopLevelWindow_GetBorderGeometry_Index_OnThis_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 68 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 71 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 68 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetBorderGeometry_Index_OnLegacy_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 40 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetCaptionGeometry_Index_OnThis_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 72 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetCaptionGeometry_Index_OnLegacy_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetCaptionBrush_Index_OnThis_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 95 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 99 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetCaptionBrush_Index_OnLegacy_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetBorderBrush_Index_OnThis_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 94 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 98 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }
			};
		}
	};
	struct CTopLevelWindow_GetBorderBrush_Index_OnLegacy_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 38 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = 0, .revision = 0 },
			};
		}
	};
	struct CTopLevelWindow_GetClientBlurBrush_Index_OnThis_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 96 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 100 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 98 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 93 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CWindowData extra color overrides
	struct CWindowData_BorderColorOverride_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 48 * 4LL, .build = 0, .revision = 0 } }; } };
	struct CWindowData_TextColorOverride_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 49 * 4LL, .build = 0, .revision = 0 } }; } };

	// LivePreviewResource offsets
	struct LivePreviewResource_GetGlassBoundingRect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 40, .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetWindowBoundingGeometry_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetWindowVisualBrush_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetGlassBoundingGeometry_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 7 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetGlassVisualBrush_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 9 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetReflectionRegion_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 12 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetReflectionGeometry_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 13 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_GetMonitorRect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 7 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_IsWindowBoundingRectNotEmpty_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 128 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct LivePreviewResource_IsGlassBoundingRectNotEmpty_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 129 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };

	// CDesktopManager
	struct CDesktopManager_GetLivePreview_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 }, Util::OffsetInfo{ .offset = 55 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 }, Util::OffsetInfo{ .offset = 57 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetCompositor_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 5 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }, Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetConnection_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetWindowList_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 }, Util::OffsetInfo{ .offset = 52 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }, Util::OffsetInfo{ .offset = 54 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 }, Util::OffsetInfo{ .offset = 53 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetWICFactory_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 }, Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }, Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 }, Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetIsHighContrastMode_BoolIndex_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 26, .build = os::build_w11_24h2, .revision = 0 }, Util::OffsetInfo{ .offset = 27, .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_HasMaximizedWindows_BoolIndex_Offsets { consteval static auto operator()() { return std::array{ Util::OffsetInfo{ .offset = 21, .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetDPIValue_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 60 * (LONGLONG)sizeof(double), .build = os::build_w11_21h2, .revision = 0 }, Util::OffsetInfo{ .offset = 51 * (LONGLONG)sizeof(double), .build = os::build_w11_22h2, .revision = 0 }, Util::OffsetInfo{ .offset = 53 * (LONGLONG)sizeof(double), .build = os::build_w11_24h2, .revision = 0 }, Util::OffsetInfo{ .offset = 52 * (LONGLONG)sizeof(double), .build = 0, .revision = 0 } }; } };

	// Pre-21H2 direct fields on DesktopManager
	struct CDesktopManager_GetD2DDevice_OnThis_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 29 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
	struct CDesktopManager_GetDCompDevice_OnThis_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 27 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };

	// CCompositor
	struct CCompositor_GetChannel_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				// >= 24H2 uses index 3; earlier uses index 2
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CAnimatedGlassSheet rects
	struct CAnimatedGlassSheet_GetSourceRect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 24 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct CAnimatedGlassSheet_GetDestinationRect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 25 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct CAnimatedGlassSheet_GetAdjustedDestinationRect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 26 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct CAnimatedGlassSheet_GetAtlasPaddingTop_Rect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 30 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct CAnimatedGlassSheet_GetAtlasPaddingLeft_Rect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 29 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct CAnimatedGlassSheet_GetAtlasPaddingRight_Rect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 29 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };
	struct CAnimatedGlassSheet_GetAtlasPaddingBottom_Rect_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 30 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 } }; } };

	// Graphics manager nested indices
	struct CDesktopManager_GetGraphicsManager_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				// For builds < 22H2, index = 6; for 22H2 and later, index = 7
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 7 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CGraphicsManager_GetD2DDevice_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CDesktopManager_GetInteropManager_Index_Offsets {
		consteval static auto operator()(){
			return std::array{
				Util::OffsetInfo{ .offset = 5 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CInteropManager_GetDCompDevice_Index_Offsets { consteval static auto operator()(){ return std::array{ Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 } }; } };
}
