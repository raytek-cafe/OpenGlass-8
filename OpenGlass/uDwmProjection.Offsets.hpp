#pragma once
#include "Util.hpp"

// ============================================================================
// uDWM Offset Stability Rules
// ============================================================================
// uDWM.dll is NOT recompiled for every Windows build. Offsets only change at
// recompile boundaries (codebase generations):
//
//   17763 — Windows 10 1809 (minimum supported)
//   18362 — Windows 10 1903, 1909
//   19041 — Windows 10 2004, 20H2, 21H1, 21H2, 22H2
//   20348 — Windows Server 2022
//   22000 — Windows 11 21H2 RTM (Build 22000)
//   22621 — Windows 11 22H2, 23H2
//   26100 — Windows 11 24H2, 25H2
//   28100 — Windows 11 26H1+ (MIL infrastructure removed)
//
// Feature gates:
//   GetSystemBackdropType → 21H2+
//   GetDWriteTextVisual_Index → 22H2+ (appears with CDWriteText)
//   _OnLegacy variants → 22H2+ (CLegacyNonClientBackground introduced)
//   CWindowBorder / GetWindowBorder_Index → 21H2+ (NOT 22H2+)
//   CAnimatedGlassSheet → pre-21H2; replaced by CAcrylicSheet in 21H2+
//
// Interval semantics: each OffsetInfo's .build is the RIGHT boundary of the
// interval where its offset applies. {build=0, revision=0} = last supported.
// ============================================================================

namespace OpenGlass::uDWM
{
	// CVisual::SetRTLMirror
	struct CVisual_IsRTLMirrored_ByteOffset_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 84, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 92, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 36, .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual::SetScale
	struct CVisual_GetScale_MilSizeD_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 168, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 176, .build = os::build_w11_24h2, .revision = 0 }
			};
		}
	};
	// CVisual::SetScale
	struct CVisual_GetScale_D2D1_SIZE_F_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 112, .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual::SetSize
	struct CVisual_GetSize_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 15 * (LONGLONG)sizeof(SIZE), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 16 * (LONGLONG)sizeof(SIZE), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 9 * (LONGLONG)sizeof(SIZE), .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual::SetOffset
	struct CVisual_GetOffset_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 14 * (LONGLONG)sizeof(POINT), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 15 * (LONGLONG)sizeof(POINT), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8 * (LONGLONG)sizeof(POINT), .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual::InitializeVisualTreeClone
	// if ( (this[x] & 8) == 0 )
	struct CVisual_SetExcludeSubtree_ByteOffset_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 84, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 92, .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual_GetVisualCollection
	// Pre-24H2: constructor assigns &VisualCollection::vftable → xrefs to find offset
	// 24H2+: xref to VisualCollection::VisualCollection, find CContainerVisual::CContainerVisual:
	//   VisualCollection::VisualCollection((VisualCollection *)(this + 18));
	struct CVisual_GetVisualCollection_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 32, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 144, .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual::ConnectToParent
	// xref to CVisualProxy::InsertChildAt
	struct CVisual_GetVisualProxy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CVisual::`vftable'
	struct CVisual_GetTransformParent_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 10 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 11 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8 * sizeof(ULONG_PTR),  .build = 0, .revision = 0 }
			};
		}
	};
	// CVisual::SetDirtyFlags
	struct CVisual_GetDirtyFlags_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 20 * sizeof(DWORD), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 22 * sizeof(DWORD), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 8 * sizeof(DWORD),  .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CVisual::`vftable'
	struct CVisual__ValidateVisual_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CRenderDataVisual::AddInstruction
	// CRenderDataVisual::ClearInstructions
	// xrefs to DynArrayImpl<0>::Grow
	// WARNING: Look for DynArray base in AddInstruction/ClearInstructions, NOT WriteInstruction (iterates the array)
	struct CRenderDataVisual_GetInstructions_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 248, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 256, .build = os::build_w11_23h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 256, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 208, .build = 0, .revision = 0 }
			};
		}
	};

	// CDrawGeometryInstruction::Create
	// CDrawGeometryInstruction::WriteInstruction
	struct CDrawGeometryInstruction_GetBrush_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	struct CDrawGeometryInstruction_GetGeometry_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CText::SetRTLReading
	struct CText_RTL_FlagByteOffset_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 280, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 288, .build = 0, .revision = 0 }
			};
		}
	};

	// xrefs to CDWriteText::`vftable'{for `IText'} → find CDWriteText constructor
	// Constructor reveals the IText vtable subobject offset within CDWriteText.
	// Negative offset = -(subobject offset) to go from IText* back to CDWriteText*.
	struct IText_GetDWriteText_NegativeOffset_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = -272, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = -168, .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CDWriteText::`vftable'{for `IText'} → decompile SetRTLReading on IText vtable
	// Look for byte bit-test and conditional branch on the memory operand
	struct IText_RTL_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 256, .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CDWriteText::`vftable'{for `IText'} → decompile SetReverseAlignment on IText vtable
	// Same pattern: byte bit-test at an adjacent absolute offset
	struct IText_Reverse_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 257, .build = 0, .revision = 0 }
			};
		}
	};
	// CDWriteText::CDWriteText
	// xrefs to CDWriteText::`vftable'{for `IText'}
	struct CDWriteText_GetTextInterface_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 272, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 168, .build = 0, .revision = 0 }
			};
		}
	};

	// CTopLevelAtlasedRectsVisual::ShouldCloneAtlasImage
	// CTopLevelAtlasedRectsVisual::ShouldDrawAtlasImage
	// WARNING: The PartId range check is (value - 9) > 8. Use ShouldDrawAtlasImage
	// or ShouldCloneAtlasImage, NOT CAtlasedImage::SetDirtyFlags (different DWORD).
	struct CAtlasedImage_GetPartId_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 33 * sizeof(DWORD), .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(DWORD), .build = 0, .revision = 0 }
			};
		}
	};

	// CButton::SetVisualStates
	// WARNING: Use SetVisualStates (base/default glyph opacity), NOT
	// UpdateCurrentGlyphOpacity (runtime/current opacity, different DWORD offset).
	struct CButton_GetGlyphOpacity_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 101 * sizeof(DWORD), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 89 * sizeof(DWORD),  .build = 0, .revision = 0 }
			};
		}
	};
	// CButton::RedrawVisual
	// CButton::DrawStateW
	// this[x] & y
	struct CButton_GetButtonState_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 280, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 288, .build = 0, .revision = 0 }
			};
		}
	};
	// CButton::RedrawVisual
	// xrefs to CButton::ActivateTimeline
	struct CButton_GetTimeline_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 49 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CButton::SetVisualStates
	struct CButton_GetGlyphBitmapArray_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 304, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 312, .build = 0, .revision = 0 }
			};
		}
	};
	// CButton::SetVisualStates
	struct CButton_GetButtonBitmapArray_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 336, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 344, .build = 0, .revision = 0 }
			};
		}
	};
	// CAccent::UpdateAccentPolicy
	struct CAccent_GetAccentPolicy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 280, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 288, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 240, .build = 0, .revision = 0 }
			};
		}
	};

	// CWindowData::IsGhostWindow
	// xrefs to GetPropW — first parameter is the HWND, trace back to the CWindowData offset
	struct CWindowData_GetHwnd_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 5 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CWindowList::EnsureTopLevelWindow
	struct CWindowData_GetWindow_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 49 * sizeof(ULONG_PTR), .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 50 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 48 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 55 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateAccent
	struct CWindowData_GetAccentPolicy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 152, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 168, .build = 0, .revision = 0 }
			};
		}
	};
	// CWindowList::SetSystemBackdropType
	struct CWindowData_GetSystemBackdropType_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 51 * sizeof(DWORD), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::CalculateOutsideMargins
	// xrefs to GetSystemMetricsForDpi
	struct CWindowData_GetWindowDPI_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 324, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 348, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 87 * sizeof(DWORD), .build = 0, .revision = 0 }
			};
		}
	};
	// CWindowList::BlurBehindChange
	// data[x] & y
	struct CWindowData_GetNonClientAttribute_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 596, .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 604, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 608, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 664, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 672, .build = os::build_w11_24h2, .revision = os::revision_24h2_kb5067036 },
				Util::OffsetInfo{ .offset = 736, .build = 0, .revision = 0 }
			};
		}
	};
	// CWindowList::ForceIconicRepresentationChange
	// CWindowList::AlphaChange
	// data[x] & y
	struct CWindowData_GetClientBlurAttribute_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 597, .build = os::build_w10_1903, .revision = 0 },
				Util::OffsetInfo{ .offset = 605, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 609, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 665, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 673, .build = os::build_w11_24h2, .revision = os::revision_24h2_kb5067036 },
				Util::OffsetInfo{ .offset = 737, .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::GetActualWindowRect
	// xrefs to OffsetRect
	struct CWindowData_GetWindowRect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 48, .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::CalculateOutsideMargins
	// if (data[x1] >= y1 && datap[x2] >= y2 && data[x3] >= y3 && data[x4] >= y4)
	struct CWindowData_GetClientMargins_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 64, .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::IsSheetOfGlass
	struct CWindowData_GetExtendedFrameMargins_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 80, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 96, .build = 0, .revision = 0 }
			};
		}
	};

	// CLivePreview
	// CLivePreview::_FadeOutToGlass
	// xrefs to VisualCollection::InsertRelative
	// if ( *(DWORD *)this[x] + y) )
	//     goto z;
	struct CLivePreview_GetThumbnailVisual_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 62 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 63 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 59 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 53 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateInstructions
	// xrefs to CRenderDataVisual::ClearInstructions
	struct CLivePreview_GetGlassVisual_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 66 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 55 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::s_UpdateResourcesForMonitor
	// xrefs to CLivePreview::s_UpdateResourcesForMonitor
	struct CLivePreview_GetLivePreviewResourceArray_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 368, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 376, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 328, .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::OnWindowShowHide
	// xrefs to DynArray<LivePreviewVisual,0>::InsertAt
	struct CLivePreview_GetLivePreviewVisualArray_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 304, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 312, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 264, .build = 0, .revision = 0 }
			};
		}
	};
	// currently deprecated
	// find it in CLivePreview::_FadeOutToGlass, CLivePreview::OnWindowShowHide
	// 26100.3775:674, 26100.7309:738
	struct CLivePreview_IsWindowIncluded_ByteOffset_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 611, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 675, .build = 0, .revision = 0 }
			};
		}
	};
	// currently deprecated
	// find it in CLivePreview::_FadeOutToGlass, CLivePreview::OnWindowShowHide
	struct CLivePreview_IsWindowCloneAsWindowFrames_DwordIndex_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 28 * sizeof(DWORD), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 33 * sizeof(DWORD), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 32 * sizeof(DWORD), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::EnsureImages
	// i = 6
	// while ((--i) > 0)
	// {
	//     CTopLevelWindow::s_rgpwfWindowFrames[y][x] = 0
	// }
	struct CTopLevelWindow_WindowFrame_GetCornerRadius_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 471 * sizeof(UINT32), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 466 * sizeof(UINT32), .build = 0, .revision = 0 }
			};
		}
	};
	// CWindowList::EnsureTopLevelWindow
	struct CTopLevelWindow_GetData_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 90 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 91 * sizeof(ULONG_PTR), .build = os::build_server_2022, .revision = 0 },
				Util::OffsetInfo{ .offset = 92 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 94 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 89 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CText::Create
	struct CTopLevelWindow_GetTextVisual_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 67 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateIcon
	// (CImage *)this[x];
	struct CTopLevelWindow_GetIconVisual_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 66 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 68 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 72 * sizeof(ULONG_PTR), .build = os::build_w11_23h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 67 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CDWriteText::Create
	struct CTopLevelWindow_GetDWriteTextVisual_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 65 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateAccent
	// xrefs to CAccent::Create
	struct CTopLevelWindow_GetAccent_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 35 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 32 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaBackground
	// xrefs to CLegacyNonClientBackground::Create (windows 11 22h2+)
	struct CTopLevelWindow_GetLegacyVisual_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 35 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 36 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateClientBlur
	// xrefs to CRenderDataVisual::AddInstruction
	struct CTopLevelWindow_GetClientBlurVisual_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 36 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 42 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CWindowBorder::Create
	struct CTopLevelWindow_GetWindowBorder_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 28 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::SetSuppressBorderUpdates (could be inlined)
	// CTopLevelWindow::UpdateWindowVisuals
	// if ( this[x] )
	//     goto y;
	struct CTopLevelWindow_GetIsBorderUpdatesSuppressed_ByteIndex_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 888, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 864, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 832, .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CText::SetRTLReading/CVisual::SetRTLMirror
	struct CTopLevelWindow_StateDwordIndex_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 146 * sizeof(DWORD), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 148 * sizeof(DWORD), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 152 * sizeof(DWORD), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 156 * sizeof(DWORD), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 146 * sizeof(DWORD), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateColorizationColor
	// CTopLevelWindow::UpdateNCAreaBackground
	// if ( y1 == 0x7FFFFFFF && y2 == 0x7FFFFFFF && y3 == 0x7FFFFFFF && y4 == 0x7FFFFFFF )
	//  z = this[x]
	// else
	//  z = this[w]
	struct CTopLevelWindow_GetCaptionColorizationParameters_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 72 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 73 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 75 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 77 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 71 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// xrefs to CButton::Create
	struct CTopLevelWindow_GetButton_BasePointerIndex_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 60 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 63 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 66 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::GetBorderMargins/CTopLevelWindow::_GetRightFrameThickness
	// if ( y <= 0 )
	//     y = data[x]
	struct CWindowData_GetFrameThickness_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 96, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 112, .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaPositionsAndSizes
	// x = x1;
	// if ( !zoomed )
	//     x = x2;
	// y = this[x]
	struct CTopLevelWindow_GetFrameOutsideMargins_Zoomed_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 636, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 644, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 660, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 676, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 636, .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaPositionsAndSizes
	// x = x1;
	// if ( !zoomed )
	//     x = x2;
	// y = this[x]  (Normal path, same function as Zoomed)
	struct CTopLevelWindow_GetFrameOutsideMargins_Normal_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 620, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 628, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 644, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 660, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 620, .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaBackground
	// if ( this[x1] == 0x7FFFFFFF && this[x2] == 0x7FFFFFFF && this[x3] == 0x7FFFFFFF && this[x4] == 0x7FFFFFFF )
	//  z = this[y]
	// else
	//  z = this[w]
	//
	// (x = min(x1, x2, x3, x4))
	struct CTopLevelWindow_GetFrameInsideMargins_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 151 * sizeof(DWORD), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 153 * sizeof(DWORD), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 157 * sizeof(DWORD), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 161 * sizeof(DWORD), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 151 * sizeof(DWORD), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::GetBorderMargins
	// margins = (MARGINS)this[x]
	struct CTopLevelWindow_GetBorderMargins_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 588, .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 596, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 612, .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 628, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 588, .build = 0, .revision = 0 }
			};
		}
	};

	// CTopLevelWindow: indices that moved to LegacyVisual on >= 22H2
	// CTopLevelWindow::UpdateNCAreaBackground
	// ...
	// CDrawGeometryInstruction::Create({border brush}, {border geometry}, {border instruction})
	// ...
	// CDrawGeometryInstruction::Create({caption brush}, {caption geometry}, {caption instruction})
	// ...
	// CRenderDataVisual::AddInstruction({legacy visual}, {border instruction})
	// ...
	// CRenderDataVisual::AddInstruction({legacy visual}, {caption instruction})
	struct CTopLevelWindow_GetBorderGeometry_Index_OnThis_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 68 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 71 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 68 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLegacyNonClientBackground::SetBorderRegion
	struct CTopLevelWindow_GetBorderGeometry_Index_OnLegacy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 40 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 34 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaBackground
	// ...
	// CDrawGeometryInstruction::Create({border brush}, {border geometry}, {border instruction})
	// ...
	// CDrawGeometryInstruction::Create({caption brush}, {caption geometry}, {caption instruction})
	// ...
	// CRenderDataVisual::AddInstruction({legacy visual}, {border instruction})
	// ...
	// CRenderDataVisual::AddInstruction({legacy visual}, {caption instruction})
	struct CTopLevelWindow_GetCaptionGeometry_Index_OnThis_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 70 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 72 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 69 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLegacyNonClientBackground::SetCaptionRegion
	struct CTopLevelWindow_GetCaptionGeometry_Index_OnLegacy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 33 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaBackground
	// {caption color}.a = 1.f
	// ...
	// CSolidColorLegacyMilBrushProxy::Update({caption brush}, 1.0, {caption color})
	struct CTopLevelWindow_GetCaptionBrush_Index_OnThis_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 94 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 95 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 99 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }
			};
		}
	};
	// CLegacyNonClientBackground::SetCaptionColor
	struct CTopLevelWindow_GetCaptionBrush_Index_OnLegacy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 37 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateNCAreaBackground
	// ...
	// CSolidColorLegacyMilBrushProxy::Update({border brush}, {opacity}, {border color})
	struct CTopLevelWindow_GetBorderBrush_Index_OnThis_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 93 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 94 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 98 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 }
			};
		}
	};
	// CLegacyNonClientBackground::SetBorderColor
	struct CTopLevelWindow_GetBorderBrush_Index_OnLegacy_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 38 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CTopLevelWindow::UpdateClientBlur
	// CSolidColorLegacyMilBrushProxy::Update({client blur brush}, 1.0, {client blur color});
	struct CTopLevelWindow_GetClientBlurBrush_Index_OnThis_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 95 * sizeof(ULONG_PTR), .build = os::build_w10_2004, .revision = 0 },
				Util::OffsetInfo{ .offset = 96 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 100 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 98 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 93 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CLivePreview::_UpdateResourcesForMonitor
	// SetRectEmpty((LPRECT)((char *)resource + 40))
	struct LivePreviewResource_GetGlassBoundingRect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 40, .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// ResourceHelper::CreateRectangleGeometry(..., (CRectangleGeometryProxy **)resource + 2)
	struct LivePreviewResource_GetWindowBoundingGeometry_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// CImageLegacyMilBrushProxy::Update(resource[4], resource + 32, ...)
	struct LivePreviewResource_GetWindowVisualBrush_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// ResourceHelper::CreateRectangleGeometry(..., (CRectangleGeometryProxy **)resource + 7)
	struct LivePreviewResource_GetGlassBoundingGeometry_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 7 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// CImageLegacyMilBrushProxy::Update(resource[9], resource + 72, ...)
	struct LivePreviewResource_GetGlassVisualBrush_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 9 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// *((QWORD *)resource + 12) = CreateRectRgn(0, 0, 0, 0)
	struct LivePreviewResource_GetReflectionRegion_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 12 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// ResourceHelper::CreateGeometryFromHRGN(resource[12], (CRgnGeometryProxy **)resource + 13)
	struct LivePreviewResource_GetReflectionGeometry_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 13 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitorHelper
	// IntersectRect(&rcDst, &rcDst, (const RECT *)resource + 7)
	struct LivePreviewResource_GetMonitorRect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 7 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// *((BYTE *)resource + 128) = !IsRectEmpty(resource)
	struct LivePreviewResource_IsWindowBoundingRectNotEmpty_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 128, .build = 0, .revision = 0 }
			};
		}
	};
	// CLivePreview::_UpdateResourcesForMonitor
	// *((BYTE *)resource + 129) = !IsRectEmpty((const RECT *)((char *)resource + 40))
	struct LivePreviewResource_IsGlassBoundingRectNotEmpty_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 129, .build = 0, .revision = 0 }
			};
		}
	};

	// CDesktopManager::Initialize
	// CLivePreview::Create(&this->livePreview)
	struct CDesktopManager_GetLivePreview_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 64 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 55 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 57 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CDesktopManager::Initialize
	// CCompositor::Create(connection, &this->compositor)
	struct CDesktopManager_GetCompositor_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 5 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CDesktopManager::Initialize / CLivePreview::_FadeOutToGlass
	// *((QWORD *)CDesktopManager::s_pDesktopManagerInstance + N) → CWindowList*
	struct CDesktopManager_GetWindowList_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 61 * sizeof(ULONG_PTR), .build = os::build_server_2022, .revision = 0 },
				Util::OffsetInfo{ .offset = 53 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 51 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = os::revision_21h2_post_rtm_0 },
				Util::OffsetInfo{ .offset = 52 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 54 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 53 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CDesktopManager::Initialize
	// WICCreateImagingFactory_Proxy(567, &this->wicFactory)
	struct CDesktopManager_GetWICFactory_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 39 * sizeof(ULONG_PTR), .build = os::build_server_2022, .revision = 0 },
				Util::OffsetInfo{ .offset = 248, .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 29 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = os::revision_21h2_post_rtm_0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 31 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 30 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CDesktopManager::InitializeHighContrast
	// this[26] = IsHighContrastMode
	struct CDesktopManager_GetIsHighContrastMode_BoolIndex_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 26, .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 27, .build = 0, .revision = 0 }
			};
		}
	};
	// CWindowList::CheckForMaximizedChange
	// s_pDesktopManagerInstance[N] = hasMaximized
	struct CDesktopManager_HasMaximizedWindows_BoolIndex_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 21, .build = 0, .revision = 0 }
			};
		}
	};
	// CDesktopManager::SetupDPIValues
	// *((double *)this + N) = GetDpiForSystem() / 96.0
	struct CDesktopManager_GetDPIValue_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 60 * (LONGLONG)sizeof(double), .build = os::build_server_2022, .revision = 0 },
				Util::OffsetInfo{ .offset = 52 * (LONGLONG)sizeof(double), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 51 * (LONGLONG)sizeof(double), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 53 * (LONGLONG)sizeof(double), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 52 * (LONGLONG)sizeof(double), .build = 0, .revision = 0 }
			};
		}
	};

	// Pre-21H2: D2D device stored directly on CDesktopManager
	// CDesktopManager::EnsureDCompositionInteropDevice → D2D1CreateFactory → stores at this + N
	struct CDesktopManager_GetD2DDevice_OnThis_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 29 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// Pre-21H2: DComp device stored directly on CDesktopManager
	// xrefs to RoGetActivationFactory("Windows.UI.Composition.Compositor")
	// QueryInterface GUID_d14b6158_c3fa_4bce_9c1f_b61d8665eab0
	// CDesktopManager::EnsureDCompositionInteropDevice → DCompositionCreateDevice3 → stores at this + N
	struct CDesktopManager_GetDCompDevice_OnThis_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 27 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CCompositor::Initialize
	// *((QWORD *)this + N) = MilConnection_CreateChannel(connection, &channel)
	// >= 24H2 uses index 3; earlier uses index 2
	struct CCompositor_GetChannel_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CDesktopManager::EnsureDCompositionInteropDevice/CCompositor::InitializeInteropCompositor
	// xrefs to RoGetActivationFactory("Windows.UI.Composition.Compositor")
	// QueryInterface GUID_d14b6158_c3fa_4bce_9c1f_b61d8665eab0
	// Retrieves IDCompositionDesktopDevicePartner from CCompositor's DComp device at index 4
	struct CCompositor_GetInteropCompositorDCompDevicePartner_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};

	// CAnimatedGlassSheet::Initialize / AdjustTargetRect / StartRectAnimation / UpdateRectAnimation
	// this->sourceRect
	struct CAnimatedGlassSheet_GetSourceRect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 24 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	// CAnimatedGlassSheet::Initialize / AdjustTargetRect / StartRectAnimation / UpdateRectAnimation
	// this->destinationRect
	struct CAnimatedGlassSheet_GetDestinationRect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 25 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	// CAnimatedGlassSheet::AdjustTargetRect / UpdateRectAnimation
	// this->adjustedDestinationRect
	struct CAnimatedGlassSheet_GetAdjustedDestinationRect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 26 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	// CAnimatedGlassSheet::Initialize
	// Atlas padding insets for the glass sheet region
	struct CAnimatedGlassSheet_GetAtlasPaddingTop_Rect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 30 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	struct CAnimatedGlassSheet_GetAtlasPaddingLeft_Rect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 29 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	struct CAnimatedGlassSheet_GetAtlasPaddingRight_Rect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 29 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};
	struct CAnimatedGlassSheet_GetAtlasPaddingBottom_Rect_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 30 * (LONGLONG)sizeof(RECT), .build = 0, .revision = 0 }
			};
		}
	};

	// CDesktopManager::Initialize
	// CGraphicsDeviceManager::Create → stored at this + N
	// For builds < 22H2, index = 6; for 22H2 and later, index = 7
	struct CDesktopManager_GetGraphicsManager_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 6 * sizeof(ULONG_PTR), .build = os::build_w11_22h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 7 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
	// CGraphicsDeviceManager::InitializeGraphicsDevice
	// D2D1CreateFactory → stores D2D device at this + N
	struct CGraphicsManager_GetD2DDevice_Index_Offsets
	{
		consteval static auto operator()()
		{
			return std::array{
				Util::OffsetInfo{ .offset = 2 * sizeof(ULONG_PTR), .build = os::build_w11_21h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 3 * sizeof(ULONG_PTR), .build = os::build_w11_24h2, .revision = 0 },
				Util::OffsetInfo{ .offset = 4 * sizeof(ULONG_PTR), .build = 0, .revision = 0 }
			};
		}
	};
}
