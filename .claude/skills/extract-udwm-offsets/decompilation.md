# uDWM Decompilation Guide

## Pseudocode pattern reference

uDWM accessors are simpler than dwmcore — mostly inline member reads rather than complex function chains.

| Pseudocode pattern | Actual byte offset |
|---|---|
| `[rcx+Nh]` in a disassembled thunk | N |
| `*((_QWORD *)this + N)` | N × 8 |
| `*((_DWORD *)this + N)` | N × 4 |
| `*((float *)this + N)` | N × 4 |
| `*(BYTE *)(this + N)` | N |
| `*(QWORD *)(this + N)` (constructor) | N |

No `this` adjustment needed — uDWM has no virtual functions on subobject vtables.

## Output rules for `.hpp` entries

### Interval semantics

Each `OffsetInfo` defines the **right boundary** of the interval where its offset value applies. Entries must be sorted in ascending build order.

```
{ .offset = A, .build = B1 }   valid for [earliest, B1)
{ .offset = B, .build = B2 }   valid for [B1, B2)
{ .offset = C, .build = 0  }   valid for [B2, ...) terminal entry
```

`build == 0` is the TERMINAL sentinel: it matches the last supported system AND any future build. If the running system is newer than what the table knows about, this value is our best guess (assuming layout didn't change).

### uDWM-specific: codebase generations

Because uDWM is not recompiled for every Windows build, entries for the same codebase generation MUST share the same offset value:

| Generation | Build constant | Applies to |
|---|---|---|
| 17763 | `build_w10_1809` | 1809 (minimum supported) |
| 18362 | `build_w10_1903` | 1903, 1909 |
| 19041 | `build_w10_2004` | 20H1, 20H2, 21H1, 21H2 (post-RTM), Win10 22H2 |
| 20348 | `build_server_2022` | Server 2022 |
| 22000 | `build_w11_21h2` + `revision_21h2_rtm_0` | 21H2 RTM only |
| 26100 | `build_w11_24h2` + revisions | 24H2, 25H2 base, 25H2 CU |
| 28100 | (26H1) | MIL infrastructure removed |

When you see a single entry at `build_w10_2004`, it covers everything from 2004 through 22H2 — do NOT add redundant entries for 21H1, 21H2, or 22H2 with the same value.

### Build number disambiguation

Windows 10 22H2 (19045) ≠ Windows 11 22H2 (22621). The IDB path hint (e.g. `windows-10-22h2`) tells you which OS line. Win10 22H2 is 19041 codebase; Win11 22H2 is the 22621 recompile boundary.

### Adding a new last-supported system

When a new build/revision becomes the last-supported and offsets change:

1. Replace the old `build=0, revision=0` entry with the NEW system's build+revision (keeping the old offset value)
2. Add a new `build=0, revision=0` entry with the new offset

### Feature gates

Some offsets only exist in specific build ranges:

- If a member was **added in a specific build** (e.g. `GetDWriteTextVisual` in 22H2): the chain starts with that build+revision, no earlier entries
- If a member was **removed** (e.g. `GetCaptionBrush_OnThis` in 24H2): the chain ends with the last build that had it, without a terminal `build=0` entry
- If a member **doesn't exist yet** in a given build: report as "not present" — do NOT invent an offset for it

### Member existence

- **Member still exists**: the chain ends with `build=0`. This entry provides the offset for the last-supported system and acts as a catch-all fallback.
- **Member was REMOVED**: the chain does NOT end with `build=0`. The last entry names the last build that had the member. If `FindOffsetRecursive` reaches the end without a `build=0` match, it throws `E_UNEXPECTED` (the running system was never supposed to reference this offset). The caller must gate on build number to prevent reaching the removed array. Example: `GetCaptionBrush_Index_OnThis` (no `build=0`, removed in 24H2).
- **Feature gated**: the chain starts at the build where the feature was introduced. Caller must guard against earlier builds.

### Pointer-index vs DWORD-index disambiguation

When the decompiler shows `*((_DWORD *)this + N)`, the offset is `N × 4`. When it shows `*((_QWORD *)this + N)`, the offset is `N × 8`. When the constructor initializes with `*(_QWORD *)(v1 + N) = 0`, the byte offset N directly gives the pointer slot as `N / 8`. Always cross-check: does the `.hpp` express this offset in `sizeof(ULONG_PTR)`, `sizeof(DWORD)`, `sizeof(RECT)`, or plain bytes? The `.hpp` may have unit errors — e.g. a BYTE flag scaled by `sizeof(ULONG_PTR)` when it should be a plain byte offset.

- Verify vtable slot offsets by reading the actual vtable and looking up function addresses — never assume slot numbers from other builds.
- For member byte offsets confirmed via Set* functions, cross-validate with the constructor.
- For pointer-index slots in CTopLevelWindow, the constructor zero-initialization sequence provides strong confirmation.

## When a verification function can't be found

The function listed in a phase table or offset comment may be inlined, absent from the binary, or named differently. Try these strategies in order:

1. **Trace import cross-references**: `WICCreateImagingFactory_Proxy` → `CDesktopManager::Initialize` (WICFactory slot). Import xrefs bypass inlining completely.

2. **Search for the class constructor**: If `CWindowBorder` ctor doesn't exist, the class itself is absent. Don't keep searching for its member offsets.

3. **Trace callers of Create methods**: `CGraphicsDeviceManager::Create` is called from `CDesktopManager::Initialize` — find the slot where the returned pointer is stored.

4. **Look for semantically-clear setters**: `CLegacyNonClientBackground::SetBorderRegion` / `SetCaptionRegion` are more reliable than `ValidateVisual` for geometry/brush slots.

5. **Check parameter types on SetScale**: If `CVisual::SetScale` takes `double` params, only MilSizeD exists. If it takes `float`, D2D1 scale is in use.

6. **Feature existence check**: Search the binary for the class/function name before concluding it's absent. `SetSuppressBorderUpdates` absent → `GetIsBorderUpdatesSuppressed` doesn't exist in this build.
