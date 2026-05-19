# DWM Core Class Reference

Understanding the class relationships helps avoid misreading decompiled offsets.

## Codebase stability

dwmcore.dll is NOT recompiled for every Windows build. Generations:

| DLL codebase | Windows releases |
|---|---|
| 17763 | Windows 10 1809 (minimum supported) |
| 18362 | Windows 10 1903, 1909 |
| 19041 | Windows 10 2004, 20H2, 21H1, 21H2, 22H2 |
| 20348 | Windows Server 2022 |
| 22621 | Windows 11 22H2, 23H2 |
| 26100 | Windows 11 24H2, 25H2 |
| 28100 | Windows 11 26H1+ (MIL infrastructure partially removed) |

**Rule**: within a generation, all offsets are identical. Only recompile boundaries change values.

## Per-offset lookup

Each offset struct in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp) has inline comments with the accessor function, pseudocode pattern, and cross-references. Read the comment above each struct first when extracting offsets.

## Build-era awareness

**26H1+ (build 28100+)**: Microsoft removed MIL-based infrastructure.
- **Brush hierarchy is gone** — `CImageLegacyMilBrush`, `CSolidColorLegacyMilBrush`, `CLegacyMilBrush` no longer exist
- **IDrawingContext interface is gone** — absorbed into the base vtable or removed entirely. `CDrawingContext::Create` has only 2 vtable assignments instead of 3
- **COcclusionContext, CDrawingContext core, CD3DDevice** still exist but with revised layouts
- Gate each class before extracting — do not assume existence

## CDrawingContext family

```
CDrawingContext (base)
├── CGlobalDrawingContext (primary, has Create)
├── CShapeDrawingContext (variant, no separate Create)
└── CSubDrawingContext (variant, no separate Create)
```

- CDrawingContext implements **ID2DContextOwner**. Pre-26H1 it also implemented **IDrawingContext** (absorbed/removed in 26H1+).
- Focus offset extraction on **CDrawingContext**, not CGlobalDrawingContext.
- Vtable offsets differ across builds — always verify from the actual `Create` decompilation. Do not assume the pre-26H1 layout (+0=CMILCOMBaseT, +16=IDrawingContext, +24=ID2DContextOwner). Count the vtable assignments in Create and read the actual offsets.

## CDeviceTextureTarget

- Extremely complex inheritance — possible diamond hierarchy (CD2DBitmap → CDeviceResourceT → CMILCOMBaseT → …)
- Has multiple vtable pointers (9+): ID2DBitmapCacheSource, IPixelFormat, IBitmapUnlock, IDeviceResource (standalone + embedded in combined vtables for IDeviceTexture and IDeviceTarget)
- Do NOT attempt to fully reconstruct the inheritance graph — focus on extracting the specific offsets needed

## COcclusionContext

- Members can be **added or removed between revisions** (not just major builds)
- Closely coupled with **CArrayBasedCoverageSet** — when COcclusionContext grows, ArrayBasedCoverageSet offset tends to shift
- **Canary class**: CurrentZ and DeviceTransform offsets are the most volatile; check COcclusionContext FIRST before auditing any other class
- When 4 volatile members (CurrentZ, DeviceTransform, DeviceTransformFlag, ArrayBasedCoverageSet) all differ from existing entries by the same delta, trust the data

## Brush hierarchy

> [!WARNING]
> **Removed in 26H1+ (build 28100+).** The entire MIL brush hierarchy was deleted. Skip Phase 2 if `TryDrawCommandAsDrawList` no longer processes brush objects.

```
CLegacyMilBrush (base)
├── CImageLegacyMilBrush — image brush with viewport/viewbox/image source
└── CSolidColorLegacyMilBrush — solid color brush
```

- Both inherit from CLegacyMilBrush, sharing GetOpacityValue / GetFloatResource / Viewport / Viewbox layout
- **GetRealizedColor is unique to CSolidColorLegacyMilBrush** (CImageLegacyMilBrush does not have it)
- Pre-22H2: `CImageLegacyMilBrush::ProcessUpdate` handles ALL brush properties from a single MIL command struct
- 22H2+: properties split into individual setters on `CImageLegacyMilBrushGeneratedT<CImageLegacyMilBrush, CLegacyMilBrush>`

## CShape family

```
CShape (abstract base)
├── CRectanglesShape
├── CRoundedRectangleShape
└── CRegionShape
```

- **GetTightBounds / IsRectangles / GetRectangles vtable slots are consistent across all derived classes** — find them in any derived class's vtable, the slot index applies to all

## CDirtyRegion

- **GetOcclusionContext removed in 24H2+** — the member no longer exists. OcclusionContext is now passed as a parameter to `GetOptimizedRect` rather than stored in CDirtyRegion.
- In earlier builds (pre-24H2) the offset was 16. No `build=0` — this member is gone.

## CRenderData

- A type of visual content **IContent** — responsible for data storage and actual rendering pipeline
- `TryDrawCommandAsDrawList` is the key accessor for brush member offsets
- Resource array base pointer at `GetResources` offset — grows ~8 bytes per major version

## CD3DDevice / CD3DSurface / CD2DContext

- **CD3DDevice is historically stable** — offsets rarely change across any Win11 build, but verify in new major builds (26H1+)
- **CD3DSurface is not actively used** — its offsets are w10_2004 only; skip in current builds
- **CD2DContext::GetDevice** returns `this - N` → N is the CD2DContext offset within CD3DDevice

## Change priority — which offsets to check first

**Tier 1 — Always changes (check COcclusionContext FIRST):**
- `COcclusionContext`: all 6 offsets are the most volatile across CUs. Between 26100.1 and 26100.8457, 4 of 6 changed while everything else stayed stable. This is the **canary class**.

**Tier 2 — Occasionally changes (verify after COcclusionContext):**
- `CDrawingContext`: WorldTransform can shift between major builds. Other offsets mostly stable.
- `CRenderData_GetResources`: grows slowly (~8 bytes per major version).

**Tier 3 — Stable within a build series, but gate first:**
- Brush family (CImageLegacyMilBrush, CSolidColorLegacyMilBrush): stable across CUs within the same major build. **Removed entirely in 26H1+**.
- CShape vtable slots: consistent across derived classes and stable across builds.
- ID2DContextOwner vtable slots: stable within same major build; re-verify after any major build.

**Tier 4 — Historically frozen (verify once per major build):**
- CD3DDevice, CD3DSurface, CD2DContext: rarely change, but verify in new major builds (26H1+).
- CArrayBasedCoverageSet internals (OccluderArray at offset 0 within the struct).
- RenderTargetInfo_GetSDRBoost: historically 16, but verify.
- IDrawingContext_GetDrawingContext and IDeviceTarget_GetDeviceTexture: compute from constructor data.

**Known ICF — cannot resolve automatically:**
- `IDeviceTarget_GetRenderTargetView`: systematically merged with CPassthroughEffect/CKernelTransport symbols across multiple builds. Flag and skip every time.
