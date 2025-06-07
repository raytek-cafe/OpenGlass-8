![header](banner.png)
# Experience native look of Aero Glass interface on Windows 10+
This utility returns the full glass effect to the window frame, just like [glass8](http://www.msfn.org/board/forum/180-aero-glass-for-windows-8/) did.

> [!NOTE]  
> Currently only the following official versions of Windows are supported.
> - Windows 10 20H2
> - Windows 10 21H2
> - Windows 10 22H2
> - Windows 11 21H2
> - Windows 11 22H2
> - Windows 11 23H2
> - Windows 11 24H2

> [!IMPORTANT]  
> This software is intended for advanced users only. If you are a beginner and you do not have deeper knowledge of Windows (such as registry editing etc.) you should not install this software.  
> For the average users, you should consider using [DWMBlurGlass](https://github.com/Maplespe/DWMBlurGlass).

## How to use this software
1. Extract the files from the Release page to `C:\`, better not put it in any other location. Important thing is that you must place all files in writable location (i.e. not in `Program Files`), because DWM process does not run under user's credential. If you don't do this, OpenGlass will not be able to download symbols or create debug logs! If you worry about the security, you can change the permission of OpenGlass.dll files to be writable by Administrators group only and leave it read-only to others.
2. Run `install.cmd` as administrator, this will create a scheduled task for you to run the OpenGlass host process which will inject DLL into DWM for you and also maintains that user settings are correctly loaded. 
3. Run `startup.cmd` as administrator, this will run the host process manually.
4. When you use it for the first time or just after updating your system, OpenGlass will try to download the symbol files and you will see its symbol downloading dialog, just be patient for about 15s. When the symbol files are ready, enjoy!
5. If OpenGlass is unable to download symbols, you can try running the `symchk-prepare-symbols.cmd`.
6. When you want to stop using OpenGlass or update the version of OpenGlass, running `shutdown.cmd` will remove the effects of OpenGlass for you and exit the host process. At this time, you can either replace the OpenGlass files or continue to run `uninstall.cmd` and manually delete the remaining files to complete the uninstallation.
7. When you experience a crash, OpenGlass is supposed to generate a large memory dump file in the `dumps` directory of the folder where it is located, please submit it to the developer if possible, this can help fix known or potential issues.

## What are the DWM symbols and where to get them? <br>I see "Your DWM is incompatible" message. What to do?

OpenGlass works by injecting re-implemented code into several DWM functions. This can be achieved only when the absolute location of each function is known. These locations are described in special files called "program database" and you can recognize them by their .PDB extension. They contain set of all public DWM variables and functions (which are called symbols) together with their memory offsets (relative locations) and other information. OpenGlass is able to load these files from "symbols" directory stored in your OpenGlass installation directory, find appropriate symbol and compute absolute memory location.

Since the DWM functions are in different locations for each version of Windows, OpenGlass has to load their location from the external program databases. The best practice to know more and to get symbol files is reading [Microsoft's documentation](http://msdn.microsoft.com/en-us/library/windows/desktop/ee416588(v=vs.85).aspx#getting_the_symbols_you_need) (you will need symbol files for dwmcore.dll and udwm.dll). The most important thing is that the version of the program database must correspond to the used DWM library version.

## How to change color of inactive windows border?
This can be easily done using the Windows Registry Editor in the branch `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\DWM`. You can find there already existing values with name `ColorizationXXX` (where XXX represents an individual settable component) intended for active windows. If you create same values with suffix Inactive (i.e. `ColorizationXXXInactive`), such settings will be applied to inactive windows.

But remember that colorization of the frame is also influenced by used theme. Majority of the themes have set opacity of inactive frames to 20%.

## How can I change OpenGlass settings?
You can change the settings of OpenGlass via editing the Windows registry.
> [!IMPORTANT]  
> Unless specified, the options below are stored in `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\DWM` (per-user) and `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\DWM` (global). OpenGlass prefers to read the settings in the HKCU.


### Colorization settings

| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| ColorizationColor<br>ColorizationColorInactive | DWORD | ARGB color used for the glass effect, alpha value is ignored.<br><br>ℹ️ `ColorizationColorInactive` is only used when `GlassType`=0x0 |
| ColorizationColorOverride | DWORD | Same as `ColorizationColor`, but OpenGlass prefers to use this item. |
| ColorizationAfterglow<br>ColorizationColorBalance<br>ColorizationAfterglowBalance<br>ColorizationBlurBalance | DWORD | Composition parameters for aero shader effect.<br><br>ℹ️ Only used when `GlassType`=0x1 |
| ColorizationAfterglowOverride<br>ColorizationColorBalanceOverride<br>ColorizationAfterglowBalanceOverride<br>ColorizationBlurBalanceOverride | DWORD | Same as above, but OpenGlass prefers to use these. |
| GlassOpacity<br>GlassOpacityInactive | DWORD | The intensity of the color (0-100%).<br><br>ℹ️ Only used when `GlassType`=0x0 |
| ColorizationColorCaption<br>ColorizationColorCaptionInactive | DWORD | Color used for drawing window titles. Format is 0xBBGGRR.<br><br>ℹ️ Only valid in Win10 |
| ColorizationOpaqueBlend | DWORD | Controls the transparency of glass effect. |
| ColorizationOpaqueBlendColor | DWORD | ARGB base color used for opaque blending, alpha value is ignored. |

### Glass settings
| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| GlassType | DWORD | The type of glass effect. <ul><li>0x0=Vista style blur.</li><li>0x1=Aero style blur.</li><ul> |
| GlassOverrideAccent | DWORD | Overrides surfaces with accent blur with OpenGlass effects, I.E: the taskbar. <br><br> **⚠️ This option is deprecated and may be removed in a future release. It may cause serious compatibility issues, you should NOT use this!** |
| CustomThemeReflection | String | path to PNG file which will be used as overlay image to simulate reflection (Aero stripes) effect |
| ColorizationGlassReflectionIntensity<br>ColorizationGlassReflectionIntensityInactive | DWORD | The intensity of reflection effect (0-100%). Default value is 0%. |
| ColorizationGlassReflectionParallaxIntensity | DWORD | The parallax intensity of the reflection effect (I.E when moving the windows side to side). Default value is 13%. | 
| ColorizationGlassReflectionPolicy | DWORD | Controls where reflections should be rendered. <ul><li>Titlebar=1<<0</li><li>Aero Peek=1<<2</li><li>Aero Snap=1<<3 (ℹ️ Only valid in Win10)</li></ul> |
| BlurDeviation | DWORD | Standard deviation for gaussian blur, default=30 (which means σ=3.0) <br>Value 0 results in non-blurred transparency. |
| BlurOptimization | DWORD | Quality of gaussian blur<ul><li>0x0=Speed first</li><li>0x1=Balance (default)</li><li>0x2=Quality first</li></ul>  |
| RoundRectRadius | DWORD | The radius of glass geometry, Win8=0, Win7=12 | 

### Theme settings
| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| CaptionButtons | DWORD | Changes caption buttons sizes and the opacity of the button glyphs.<ul><li>0x0=Windows 10 style (default)</li><li>0x1=Windows Vista style</li><li>0x2=Windows 7 style</li><li>0x3=Windows 8 style</li></ul> |
| CenterCaption | DWORD | Controls how title bar text is aligned.<ul><li>0x0=Keeps it on the left (default)</li><li>0x1=Centers it between the titlebar icon and the titlebar buttons</li><li>0x2=Centers the Win8 way</li></ul><br>ℹ️ Only valid in Win10 |
| TextGlowMode | DWORD | Specifies how window caption glow effect will be rendered <ul><li>0x0=No glow effect</li><li>0x1=Glow effect loaded from atlas (default)</li><li>0x2=Glow effect loaded from atlas and theme opacity is respected</li><li>0x3=Composited glow effect using your theme settings HIWORD of the value specifies glow size (0=theme default)</li></ul><br>ℹ️ Only valid in Win10 |
| CustomThemeAtlas | String | path to PNG file with theme resource (bitmap must have exactly the same layout as msstyle theme you are using!) |
| DisableModernBorders | DWORD | Disable modern rounded window borders. <ul><li>0x0=Enable modern borders</li><li>0x1=Disable modern borders</li></ul><br>ℹ️ Only valid in Win11 |

### Advanced settings

The following registry items are located only in `HKLM\Software\Microsoft\Windows\DWM`, you should NOT change the settings in this section unless you know what you are doing.
| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| DisableGlassOnBattery  | DWORD | <ul><li>0x1=When energy saver is on then the glass effect will be opaque to decrease energy consumption (default)</li><li>0x0=glass effect won't be opaque on energy saver</li></ul> |
| DisableMemoryDump  | DWORD | <ul><li>0x0=Generates a memory dump file when DWM crashes (default)</li><li>0x1=No memory dump files</li></ul> |
| DisabledHooks  | DWORD | Controls which module's hooks are disabled, which will also control the availability of features. <ul><li>0x0=No hooks are disabled (default)</li><li>0x1=Disables hooks for `CaptionTextHandler.cpp`</li></ul> |

## What are the custom theme atlas files?
When Desktop Windows Manager wants to draw frame controls (such as minimize/maximize/close buttons, frame shadow etc.), it uses images which are stored in your current theme. Normally, you would need to edit your theme and install UxTheme patch to be able to change the appearance of windows frames. OpenGlass comes with a feature that you can change these images directly without editing the theme. Just provide custom *.png image, point to it with CustomThemeAtlas registry settings and restart DWM.EXE process. The windows frames will be drawn using your custom image now. Just beware that the layout of theme resource depends on the current system theme. If you do not keep this layout, your frames will be rendered incorrectly.

Starting with the version 2.0, you can provide additional layout data by supplying a *.png.layout file in the folder where *.png atlas image is stored. OpenGlass will then ignore current theme layout and use data in this file. The description of the file format is:
- each line has format "x;y;z=a,b,c,d"
- x, y, z specify the part, its state and the property respectively
- a, b, c, d specify position rectangle in atlas image (for z = 8002) or the part sizing/content margins (for z = 3601/3602)
- see example layout file for description of individual part identifiers
- see MSDN documentation of GetThemeRect and GetThemeMargins function for further information
- line "CaptionHeight=n" allows you to change the size of caption buttons to "n" pixels
- everything after # is considered to be the comment and is ignored

## Why is there no blur effect in the region/window/etc.? <br>Is it possible to enable the blur effect on the taskbar (Alt-Tab window, any other window etc.) ?

Remember that DWM is the rendering engine only and OpenGlass is the extension for it. It will apply glass effect only on the regions which are marked to have blur behind. It does not alter any region behavior on its own. If some application wants to have glass effect somewhere, it must enable it explicitly. The taskbar belongs to Windows Explorer shell, thus you must enable blur behind on your own by calling DWM API function `DwmEnableBlurBehindWindow`. Blur effect will also apply tint of glass color.

Windows 8+ Desktop Window Manager contains new feature called accent. It is a kind of effect which makes whole window fully transparent with tint of glass color. This effect is enabled on the taskbar by default. If you enable blur behind the taskbar but don't disable accent effect (using `SetWindowCompositionAttribute` API function), you may notice that it will be much more colorized, because the color of both effects will be simply added together.

If you are not familiar with DWM programming API, you can use any of the existing tools that have option to call the `DwmEnableBlurBehindWindow` function, such as OpenShell, StartIsBack or StartAllBack. 

Also remember that taskbar appearance is influenced by used theme where texture used for the taskbar is different from the atlas texture used for windows frames rendering.

## How to refresh the configuration
OpenGlass does not provide any GUI nor commands to explicitly reload the configuration, you may consider using glass8's heritage `AeroGlassGUI.exe` to refresh some of the settings.   
However, it's actually quite easy to do so in C/C++.
```c++
PostMessage(FindWindow(TEXT("Dwm"), nullptr), WM_THEMECHANGED, 0, 0);            // refresh part of the settings related to theme
PostMessage(FindWindow(TEXT("Dwm"), nullptr), WM_DWMCOLORIZATIONCHANGED, 0, 0);  // refresh part of the settings related to color/backdrop
```

> [!TIP]  
> It is better to refresh only the corresponding part of the registry items, otherwise it will slow down the system. 


## Dependencies and References
### [Banner for OpenGlass](https://github.com/ALTaleX531/OpenGlass/discussions/11)
Provided by [@aubymori](https://github.com/aubymori)  
Wallpaper: [metalheart jawn #2](https://www.deviantart.com/kfh83/art/metalheart-jawn-2-1068250045) by [@kfh83](https://github.com/kfh83)
### [[MS-RDPCR2]: Remote Desktop Protocol: Composited Remoting V2](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpcr2)
Specifies the Remote Desktop Protocol: Composited Remoting V2, which displays the contents of the Windows-based desktop running on one machine on a second machine connected to the first via a network.

From there I can get a glimpse of how udwm and dwmcore interact in Windows 7 and how the relevant data structures are defined.
### [Microsoft Research Detours Package](https://github.com/microsoft/Detours)  
Detours is a software package for monitoring and instrumenting API calls on Windows.  
### [VC-LTL - An elegant way to compile lighter binaries.](https://github.com/Chuyu-Team/VC-LTL5)  
VC-LTL is an open source CRT library based on the MS VCRT that reduce program binary size and say goodbye to Microsoft runtime DLLs, such as msvcr120.dll, api-ms-win-crt-time-l1-1-0.dll and other dependencies.  
### [Windows Implementation Libraries (WIL)](https://github.com/Microsoft/wil)  
The Windows Implementation Libraries (WIL) is a header-only C++ library created to make life easier for developers on Windows through readable type-safe C++ interfaces for common Windows coding patterns.  
### [Interop Compositor](https://blog.adeltax.com/interopcompositor-and-coredispatcher/)
Saved me some decompiling and reverse engineering time thanks to ADeltaX's blog!