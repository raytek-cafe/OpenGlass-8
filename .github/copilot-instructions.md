# OpenGlass AI Development Guide

Practical conventions and architecture quick-start for OpenGlass (injected DLL + service + wxWidgets GUI).

## Architecture and Key Interactions
- **Three components**: `OpenGlass.dll` (core logic/payload), `OpenGlassHost.exe` (service wrapper), `OpenGlassGUI.exe` (wxWidgets frontend).
- **Service-DLL Relationship**: `OpenGlassHost.exe` loads `OpenGlass.dll` and calls its `ServiceMain` (in `ServiceHandler.cpp`) to run service logic.
- **Injection and IPC**: The service logic in `OpenGlass.dll` (specifically `GlassService.cpp`) injects another instance of itself into `dwm.exe`. It communicates via the named pipe `\\.\pipe\Global\OpenGlassHostPipe`.
- **DWM Internals Hooks**: Relies on `knsoft-slimdetours` for hooking, build-specific offsets (`dwmcoreProjection.Offsets.hpp`, `uDWMProjection.Offsets.hpp`), and symbol loading (`SymbolParser.hpp`) to hook into `dwmcore.dll` and `uDWM.dll`.
- **Configuration source**: Registry `HKCU\SOFTWARE\Microsoft\Windows\DWM` (user) and `HKLM\SOFTWARE\Microsoft\Windows\DWM` (system). UI access is in `OpenGlassGUI/RegistryConfig.cpp`.

## Code Style and Conventions
- **Indentation**: Tabs.
- **Braces**: Allman style (brace on new line).
- **Naming**: `PascalCase` for classes/namespaces/functions; `m_` prefix for members; `g_` prefix for globals.
- **Error handling**: Heavy use of Windows Implementation Library (WIL) (`wil/resource.h`, `wil/result.h`). Prefer `THROW_IF_FAILED` / `RETURN_IF_FAILED`.
- **Memory Management**: Injected DLL uses `HeapAlloc` from `OpenGlass::Util::g_processHeap` (overloaded `new`/`delete` in `dllmain.cpp`).

## Graphics and Rendering
- **Shaders**: Gaussian blur and Windows 7 style effects are implemented in HLSL (`.hlsl` files) and used via D3D11 in `BlurEffect.cpp` and `AeroColorizationEffect.cpp`.
- **Theme Assets**: Supports custom atlas files (`CustomThemeAtlas`) and reflection textures.

## Development and Debugging
- **Build System**: MSBuild with `vcpkg` for dependencies (`vcpkg.json`: `wxwidgets`, `wil`, `knsoft-slimdetours`).
- **Emergency Exit**: Press `Ctrl`+`Win`+`Shift`+`Q` to kill `dwm.exe` if it hangs or crashes (registered in `OpenGlass.cpp`).
- **Logging**: Uses `LOG_IF_FAILED` and similar WIL macros; results can be viewed via debug output (DbgView).

## GUI Interaction Patterns
- **GUI Structure**: Split into `MainFrame.Core.cpp`, `MainFrame.State.cpp`, and `MainFrame.Tabs.cpp`.
- **Settings change notifications**: `MainFrame::NotifySettingsChange` sends `WM_DWMCOLORIZATIONCOLORCHANGED` / `WM_THEMECHANGED` to trigger DWM updates.
- **Path validation**: Use `ensureFilePath` in `RegistryConfig.cpp` for validating custom asset paths.

## Reference
- Registry keys and defaults: [README.md](README.md)
- Internal offsets: [OpenGlass/dwmcoreProjection.Offsets.hpp](OpenGlass/dwmcoreProjection.Offsets.hpp), [OpenGlass/uDWMProjection.Offsets.hpp](OpenGlass/uDWMProjection.Offsets.hpp)
