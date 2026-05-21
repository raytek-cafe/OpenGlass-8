# uDWM Class Reference

## Codebase stability

uDWM.dll is NOT recompiled for every Windows build. Generations:

| DLL codebase | Windows releases |
|---|---|
| 17763 | Windows 10 1809 (minimum supported) |
| 18362 | Windows 10 1903, 1909 |
| 19041 | Windows 10 2004, 20H2, 21H1, 21H2, 22H2 |
| 20348 | Windows Server 2022 |
| 22000 | Windows 11 21H2 RTM (Build 22000) |
| 22621 | Windows 11 22H2, 23H2 |
| 26100 | Windows 11 24H2, 25H2 |
| 28100 | Windows 11 26H1+ (MIL infrastructure removed) |

**Rule**: within a generation, all offsets are identical. Only recompile boundaries change values.

**Build number disambiguation**: Windows 10 22H2 = build 19045 (19041 codebase). Windows 11 22H2 = build 22621 (recompile boundary). The IDB path (e.g. `windows-10-22h2`) tells them apart. Use feature-based checks (Phase 0) to classify — do not rely on image size or function count.

## Per-offset lookup

Each offset struct in [uDwmProjection.Offsets.hpp](../../../OpenGlass/uDwmProjection.Offsets.hpp) has inline comments with the accessor function and pseudocode pattern. Read the comment above each struct first when extracting offsets.

## CVisual

Base visual class. Inherits from `CBaseObject`.

- **Pre-24H2**: the concrete base class for all visual types. Can be directly constructed via `CVisual::Create`.
- **24H2+**: becomes an abstract base — no longer instantiated directly. `CContainerVisual` takes over as the concrete general-purpose visual, inheriting from CVisual. `VisualCollection` moves from CVisual into CContainerVisual.
- **Major recompiles** (new codebase generations) can completely reshuffle all member positions. Insets, scales, DirtyFlags, vtable layout may all shift. Always verify every offset from the constructor and Set* functions.
- Constructor reveals full member layout: pointers zeroed, floats/doubles set to 1.0, sentinel DWORDs at 0x7FFFFFFF.
- **Scale storage format evolves across generations**: `double` (MilSizeD) pre-22H2 → `float`×2 (D2D) + `double` (MilSizeD) in 22H2-24H2 → D2D-only in 25H2+. When a member has no `build=0` entry, it was removed from tracking in the latest supported build.
- Vtable slots shift between generations (methods added/removed/reordered). Always read the actual vtable.
- `IsRTLMirrored` and `SetExcludeSubtree` are byte flags at independent offsets. Do NOT assume they share the same byte or are adjacent. They can share the same byte but use different bits.
- `GetScale_D2D1_SIZE_F` may reuse the same memory as `GetOffset` (both 8 bytes) in older codebases — a POINT and D2D1_SIZE_F at the same offset.

## CTopLevelWindow

Main window class.

- Pre-24H2: inherits from `CVisual`. Constructor takes no params.
- 24H2+: inherits from `CContainerVisual`. Constructor takes `CWindowData*` and stores it in a pointer slot (`GetData_Index`).
- CWindowData reverse-link: `*(QWORD*)(data + GetWindow_Index) = this`. Use `BlurBehindChange` to find the slot.
- The constructor zero-initializes all pointer slots and MARGINS in sequence — one decompilation reveals the entire layout.
- Allocation size varies by generation — read from `AllocClear` in `Create`.

### LegacyVisual migration

In 22H2+ (build 22621), caption/border geometry and brush moved from CTopLevelWindow to a nested `LegacyVisual` (CLegacyNonClientBackground). The `.hpp` tracks both `_OnThis` (pre-24H2 removal) and `_OnLegacy` (all post-22H2 builds) variants.

**Best verification**: `CLegacyNonClientBackground::SetBorderRegion` / `SetCaptionRegion` — these directly read the geometry/brush slots with clear semantic context. Also `CTopLevelWindow::UpdateNCAreaBackground` for the LegacyVisual slot index itself.

### CDWriteText / IText

- **GetDWriteTextVisual_Index**: exists from 22H2+ (build 22621). Look at cross-references to `CDWriteText::Create` and at `CTopLevelWindow::UpdateText`.

## CWindowData

Per-window data. Members accessed inline — no standalone getters exist.

- HWND and WindowRect are at historically stable offsets (unchanged across all Win11 builds).
- `GetNonClientAttribute` and `GetClientBlurAttribute` are byte flags verified in the CWindowData constructor and `BlurBehindChange`. They are always adjacent bytes (NonClient at N, ClientBlur at N+1). KB/security updates can shift these within the same codebase generation.
- `GetSystemBackdropType`: 21H2+ only. Verified via `CWindowList::SetSystemBackdropType`.
- DPI value accessed via `CalculateOutsideMargins` as `*(uint*)(this + N×4)`.
- Window-to-CTopLevelWindow back-pointer at `GetWindow_Index` slot — verified via `BlurBehindChange`.

## CLivePreview / LivePreviewResource

- `CLivePreview` maintains a DynArray of visual entries. The base pointer and count slots are revealed in `_CollectWindows`.
- `LivePreviewResource` is a simple struct (not a class). All members are at fixed byte offsets. Confirmed via `_UpdateResourcesForMonitor`.
- `IsWindowBoundingRectNotEmpty` and `IsGlassBoundingRectNotEmpty` are **byte** flags — use byte offset, not `×PTR` or `×sizeof(ULONG_PTR)`.
- **MonitorRect**: use `_UpdateResourcesForMonitorHelper` (NOT `_UpdateResourcesForMonitor`). Look for the first `IntersectRect` call — third argument is the MonitorRect.

## Change priority

**Tier 1 — Always changes at recompile boundaries:**
- CVisual vtable layout (all slot indices)
- CTopLevelWindow pointer slot indices (shift by 1-3 slots)
- CTopLevelWindow MARGINS byte offsets
- CWindowData size-based offsets (~8-16 byte shifts)

**Tier 2 — Occasionally changes:**
- CVisual member byte offsets (IsRTLMirrored, SetExcludeSubtree)
- CLivePreview pointer indices
- LivePreviewResource rect/geometry positions

**Tier 3 — Historically stable across all Win11 builds:**
- CWindowData::GetHwnd and GetWindowRect
- CDrawGeometryInstruction (GetBrush, GetGeometry)
- CVisual::GetVisualProxy
