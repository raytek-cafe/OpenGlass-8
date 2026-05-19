# DWM Core Decompilation Guide

## Philosophy — semantic understanding over name matching

MSVC name mangling changes across versions, functions get inlined, and pseudocode formatting varies. **Do NOT hardcode function names or regex patterns.** Instead, understand what each offset **means** semantically, trace the code that touches it, and recognize the right constant in decompiler output through algorithmic behavior.

## Build-era awareness — gate before extracting

**26H1 (build 28100) removed MIL infrastructure.** Before old builds this was a non-issue, but going forward:
- Always verify a class EXISTS before trying to extract its offsets
- A missing accessor function is a valid signal, not a search failure
- If a class's gate check fails (e.g., no `TryDrawCommandAsDrawList` for brushes), report "REMOVED" and skip — do not keep searching under different names
- Some classes survive (COcclusionContext, CDrawingContext core, CD3DDevice); others don't (brushes, IDrawingContext interface)

## Stable anchors — find functions by xref chains, not names

Use these algorithmically-unique functions as navigation anchors:

| Anchor | Identifying algorithm |
|--------|----------------------|
| `CArrayBasedCoverageSet::Add` | Creates a `CZOrderedRect`, calls `CZOrderedRect::UpdateDeviceRect`, then `DynArray::AddMultipleAndSet` |
| `CMILMatrix::Multiply` | Performs 4×4 matrix multiplication (nested loops over float arrays) |
| `CMatrixStack::Push` | Pushes onto a stack data structure, increments count |
| `CResource::OnPropertyChanged` | Trivially small (~3 bytes), called after every property setter |

**How to use**: Trace xrefs FROM these anchors to find their callers. The callers are the accessor functions you need, regardless of their mangled names. For example: find `DynArray::AddMultipleAndSet` → xrefs-to → reveals `FlushOcclusionRects` and `CalcVisibleArea`.

## Per-offset lookup

Each offset struct in [dwmcoreProjection.Offsets.hpp](../../../OpenGlass/dwmcoreProjection.Offsets.hpp) has inline comments documenting the exact accessor function, pseudocode pattern, and cross-references. For brush offsets, comments cover both pre-22H2 (`ProcessUpdate`) and 22H2+ (individual setters) verification paths.

## Pseudocode Pattern Reference

| Pseudocode pattern | Actual byte offset |
|---|---|
| `*(D3DCOLORVALUE *)((char *)this + N)` | N |
| `*((_QWORD *)this + N)` | N × 8 |
| `*((_DWORD *)this + N)` | N × 4 |
| `*((float *)this + N)` | N × 4 |
| `*(_BYTE *)(this + N)` | N |
| `_mm_loadu_si128(a1 + N)` | N × 16 |
| `a1[N] = *a2` where `a1` is `__m128i *` | N × 16 |
| `*(this + N)` (no type info) | N × 8 (assume pointer) |

**When `this` is adjusted** (virtual function on a subobject vtable): the pseudocode offset is relative to the adjusted `this`, NOT the object base. Always add the subobject's byte offset within the containing class.

## Virtual function `this` pointer offset

When you decompile a virtual function, the `this` pointer in the pseudocode is **already adjusted** to the subobject that owns that vtable slot. Offsets you read are relative to the adjusted `this`, NOT the object base.

To get the base-object offset: **add the subobject offset** to what you read from the decompilation.

**Example — `GetWorldTransform3x2@CDrawingContext`**:
- This is slot 0 of the `ID2DContextOwner` vtable (at `CDrawingContext + 24`)
- Decompilation shows `*((_QWORD *)this + 33)` → looks like byte offset `33×8 = 264`
- But `this` is already `CDrawingContext + 24`, so actual offset = `24 + 264 = 288`
- **Without subobject adjustment you'd read 264 (wrong); correct is 288**

**How to detect**: If a function you decompile has `UEBA`/`UEAA` mangling (virtual), check which vtable slot it belongs to (`xrefs_to` → look for data refs in `.rdata`). Compare the data ref address against the vtable base address from the constructor to determine which interface owns it. Then look up the `Create` function to find that interface's subobject offset.

**Non-virtual functions** (direct calls, `QEBA`/`QEAA` mangling without vtable dispatch) receive the raw object pointer — no adjustment needed.

## Use `Create` to understand object layout

The `Create` factory function (e.g. `CDrawingContext::Create`) reveals:
- **Allocation size** → total object size
- **Vtable assignments** → which interfaces map to which byte offsets in the object
- **Subobject offsets** → use these to correct virtual-function `this` adjustments

Always decompile `Create` before analyzing virtual functions on that class. Match vtable addresses from the constructor against data references of the function you're analyzing.

## Cross-validate every non-trivial offset

For each offset, verify with at least two independent accessor functions or call sites. If values disagree, trace both paths and flag the discrepancy. A single accessor can be misleading if its `this` adjustment is non-obvious.

## Output rules for `.hpp` entries

### Interval semantics

Each `OffsetInfo` defines the **right boundary** of the interval where its offset value applies. Entries must be sorted in ascending build order.

```
{ .offset = A, .build = B1 }   valid for [earliest, B1)
{ .offset = B, .build = B2 }   valid for [B1, B2)
{ .offset = C, .build = 0  }   valid for [B2, ...) terminal entry
```

`build == 0` is the TERMINAL sentinel: it matches the last supported system AND any future build. If the running system is newer than what the table knows about, this value is our best guess (assuming layout didn't change).

### Adding a new last-supported system

1. Replace the old `build=0, revision=0` entry with the NEW system's build+revision (keeping the old offset value)
2. Add a new `build=0, revision=0` entry with the new offset

### Member existence

- **Member still exists**: the chain ends with `build=0`. This entry provides the offset for the last-supported system and acts as a catch-all fallback.
- **Member was REMOVED**: the chain does NOT end with `build=0`. The last entry names the last build that had the member. If `FindOffsetRecursive` reaches the end without a `build=0` match, it throws `E_UNEXPECTED` (the running system was never supposed to reference this offset). The caller must gate on build number to prevent reaching the removed array. Example: `GetOpacityValue_Double` — caller guards with `if (build < build_w10_1903)`.
- **Feature-gated** (member only exists from a specific build onward): the first entry names the gate build. Caller must guard against earlier builds.
