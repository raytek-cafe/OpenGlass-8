---
name: extract-dwmcore-offsets
description: Methodically extract all dwmcore.dll struct member and vtable offsets from the current IDA instance. Follow 7 phases in order — do NOT skip. Output ready-to-paste C++ for dwmcoreProjection.Offsets.hpp.
disable-model-invocation: true
allowed-tools: Bash(git *) mcp__ida-pro-mcp__*
---

# DWM Core Offset Extraction

Extract offsets from the dwmcore.dll currently loaded in IDA Pro. Follow every phase, in order. Do not skip. Do not summarize until the final phase.

## Build-agnostic principles

- **Never assume a class or member exists.** MIL infrastructure was removed in 26H1+. Classes may be renamed, restructured, or absent between any two builds.
- **Anchor-based identification** — find functions by what they DO, not what they're called. MSVC mangling, inlining, and class names can all change.
- **Gate before extract** — before extracting offsets for any class, first verify the class still exists in the binary. If absent, report and skip that phase.
- **The total offset count is fluid.** Count adapts to what actually exists in the binary. Some members are added or removed across builds — never assume a fixed number.

## Supporting files

- **[decompilation.md](decompilation.md)** — read BEFORE starting Phase 1. Covers stable anchors, pseudocode patterns, virtual-function `this` adjustment, `Create`-based layout analysis, cross-validation rules, and `.hpp` output rules.
- **[classes.md](classes.md)** — read when you need class hierarchy context. Covers inheritance relationships, member volatility tiers, known removals, and known ICF cases.

## Per-offset lookup guidance

Each offset struct in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp) now has inline comments documenting:
- The primary accessor function(s) and how to identify them
- Pseudocode patterns from the decompiler (both pre-22H2 ProcessUpdate and 22H2+ individual setters)
- Cross-reference functions (CChannel handlers, uDWM proxy counterparts)
- WARNING notes for easily confused members and ICF cases

**When re-extracting offsets for a new build**, read the comment above each struct first — it tells you exactly which function to decompile and what pattern to look for.

## Phase 0 — Identify the build

1. `survey_binary` → record `md5`, path hint
2. Check `OSHelper.hpp` for existing build constants and revision enums
3. **Feature-based classification** — use these reliable checks instead of image size:

| Feature check | Means at least |
|---|---|
| MIL brush infrastructure absent (no `TryDrawCommandAsDrawList`, no `CLegacyMilBrush`) | Windows 11 26H1 (28100+) |
| `IDrawingContext` vtable absent from `CDrawingContext::Create` | Windows 11 26H1+ |
| `CImageLegacyMilBrushGeneratedT` present (template-generated setters) | Windows 11 22H2 (22621) |
| `CImageLegacyMilBrush::ProcessUpdate` present (single MIL cmd handler) | pre-22H2 (19041 codebase) |

**Build number scheme**:
- 22H2 / 23H2 → both build **22621**, distinguished by revision
- 24H2 / 25H2 → both build **26100**, distinguished by revision
- 26H1 → build **28100** (26H2 may differ)

If the binary doesn't match any known row, use the closest match and note the discrepancy.

**MIL infrastructure gate**: starting in 26H1 (build 28100+), Microsoft removed MIL-based brush infrastructure. This affects Phase 2 (brush classes) and Phase 3's `GetInterface` (IDrawingContext). COcclusionContext, CDrawingContext core, and CD3DDevice still exist. Don't assume removal is total — check each class individually.

Output the build classification and expected MIL availability before continuing.

> [!IMPORTANT]
> **Do NOT blindly trust existing `.hpp` entries.** Every offset must be independently verified from the actual IDA data.

## Phase 1 — COcclusionContext (gate: any function comparing `*(QWORD*)(this+N) == globalFrameId`)

**Gate check**: find a function that compares `*(QWORD*)(COcclusionContext+N) == globalFrameId`. Primary accessor: `IsCurrent@COcclusionContext` (~30B). If absent, **do not fail the gate** — decompile `*GetOptimizedRect*` and look for the same comparison pattern:
```cpp
if ( !a10 )
    v13 = (COcclusionContext *)(a1 + CDirtyRegion_OcclusionContext_offset);
if ( g_pComposition )
    v16 = *((_QWORD *)g_pComposition + globalFrameId_slot);
if ( *((_QWORD *)v13 + frameId_qword_index) == v16 )  // → GetFrameId = index × 8
```
The `globalFrameId_slot` within `g_pComposition` varies by build. The code always follows the same structure: load `g_pComposition`, add a slot offset, compare against the COcclusionContext member.

For each offset, see the comment in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp) for the exact accessor function and pseudocode pattern. Cross-validate each with at least one other function. Also decompile the constructor (`??0COcclusionContext`) for the definitive layout.

**Volatile member correlation**: GetCurrentZ, GetDeviceTransform, GetDeviceTransformFlag, and GetArrayBasedCoverageSet tend to shift by the same byte delta across CUs. If all four differ by the same amount, trust the data.

## Phase 2 — Brush classes (gate: `TryDrawCommandAsDrawList`)

**Gate check**: search for a ~2500-byte function with `*DrawCommandAsDrawList*` in the name. This function is never inlined and processes brush data. If absent → MIL brush infrastructure removed, skip entire phase.

For each offset (GetImageSource, GetOpacityValue, GetFloatResource, GetViewport, GetViewbox, GetRealizedColor), see the comment in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp).

Key cross-reference chain:
- Pre-22H2: `CImageLegacyMilBrush::ProcessUpdate` (single function processes all MILCMD_IMAGELEGACYMILBRUSH properties)
- 22H2+: `CImageLegacyMilBrushGeneratedT::SetViewport` / `SetViewbox` / etc. (individual setters)
- All builds: `CChannel::ImageLegacyMilBrushUpdate` (channel dispatcher)
- uDWM 24H2+: `CImageLegacyMilBrushProxy::Update` (batch proxy-side)

## Phase 3 — CDrawingContext (gate: `Create@CDrawingContext`)

**Gate check**: search for `*Create@CDrawingContext*`. Decompile it. Record:
- **Allocation size** (the `AllocClear` argument)
- **Vtable assignments** — count them and note their offsets

Expected vtables in pre-26H1: CMILCOMBaseT at +0, IDrawingContext at +16, ID2DContextOwner at +24.
In 26H1+: CMILCOMBaseT at +0, ID2DContextOwner at +16 (IDrawingContext absorbed into base).

For each offset, see the comment in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp).

**Critical**: `GetWorldTransform3x2` is virtual (slot 0 of ID2DContextOwner vtable). The `this` pointer in the decompilation is **already adjusted** to the ID2DContextOwner subobject (CDrawingContext + 24). Actual base offset = subobject_offset + decompiler_offset.

## Phase 4 — Remaining struct member offsets

Gate each class individually. If a class's accessor doesn't exist, skip that offset.

For each offset, see the comment in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp).

Key notes:
- **CDirtyRegion_GetOcclusionContext**: Already removed in 24H2+ (OcclusionContext passed as parameter instead of stored in CDirtyRegion). No `build=0` — do not search for it.
- **CArrayBasedCoverageSet_GetOccluderArray**: Always at offset 0 within the struct.
- **CArrayBasedCoverageSet_GetAntiOccluderArray**: Win10 only — skip in Win11+.
- **CD3DDevice offsets**: Historically frozen — if thunks unchanged, trust existing values.
- **CD3DSurface**: w10_2004 only — skip in current builds.

## Phase 5 — Vtable offsets

Gate each interface individually. If the implementing class constructor doesn't exist, skip that vtable.

Method for each: find the constructor, locate vtable assignments, read vtable bytes, verify via known anchor functions, find target slot. See [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp) for exact slot indices and their build variations.

**Known ICF — cannot resolve automatically:**
- `IDeviceTarget_GetRenderTargetView`: systematically merged with CPassthroughEffect/CKernelTransport symbols across multiple builds. Flag and skip every time.

**CDeviceTextureTarget**: Complex inheritance (possible diamond) — 9+ vtable pointers. Focus on extracting the specific offset, not full inheritance graph.

## Phase 6 — Negative/Relative offsets

Gate each. These derive from vtable layout in constructors.

For each offset, see the comment in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp).

## Phase 7 — Final report

```
================================================================
COMPLETE OFFSET REPORT — <build description>
image_size: 0x...  functions: N  MIL status: <full|partial|none>
================================================================

CHANGED FROM PREVIOUS BUILD:
  OffsetName: old → new

UNCHANGED:
  [✓] OffsetName  value  (source)

SKIPPED — CLASS/INTERFACE REMOVED:
  [✗] OffsetName — reason (e.g. "MIL brush infrastructure removed in 26H1")

ICF FLAGGED:
  [⚠] OffsetName — reason

REMOVED IN THIS BUILD (member gone, class still exists):
  [✗] OffsetName — no code accesses this member

NOT VERIFIED (accessor not found, may be renamed):
  [?] OffsetName — class may exist but accessor pattern not located

TOTAL: N verified (M changed, X unchanged), Y skipped/removed, Z ICF
================================================================
```

**Before emitting C++**: Apply output rules from decompilation.md. Member exists → add `build=0` entry as the last interval boundary. Member/class removed → last entry names the last build that had it, no `build=0`. The absence of `build=0` signals non-existence.

Only after all phases complete and report is shown, ask whether the user wants C++ emitted.
