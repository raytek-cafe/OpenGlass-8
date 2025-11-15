![header](banner.png)
# Experience native look of Aero Glass interface on Windows 10+
This utility returns the full glass effect to the window frame, just like [glass8](http://www.msfn.org/board/forum/180-aero-glass-for-windows-8/) did.

> [!WARNING]
> OpenGlass is in maintenance mode.   
> It is still being updated to support the latest Windows. However, no new functionality or feature updates are planned.

> [!NOTE]  
> Currently only the following **64-bit** Windows (not including server edition) from general availability channel are fully supported.
> - Windows 10 20H1
> - Windows 10 20H2
> - Windows 10 21H2
> - Windows 10 22H2
> - Windows 11 21H2
> - Windows 11 22H2
> - Windows 11 23H2
> - Windows 11 24H2
> - Windows 11 25H2
> 
> Tested on:
> - [x] Windows 10 22H2 Build 19045.5131 
> - [x] Windows 11 21H2 Build 22000.2416 
> - [x] Windows 11 22H2 Build 22621.3296 
> - [x] Windows 11 24H2 Build 26100.6584 
> - [x] Windows 11 25H2 Build 26200.6584 

> [!IMPORTANT]  
> This software is intended for advanced users only. If you are a beginner and you do not have deeper knowledge of Windows (such as registry editing etc.) you should not install this software.  
> For the average users, you should consider using [DWMBlurGlass](https://github.com/Maplespe/DWMBlurGlass).

## How to use this software
1. Extract the files from the Release page to `C:\`, better not put it in any other location. Important thing is that you must place all files in writable location (i.e. not in `Program Files`, `Users`), because DWM process does not run under user's credential. If you don't do this, OpenGlass will not be able to download symbols or create debug logs! If you worry about the security, you can change the permission of OpenGlass.dll files to be writable by Administrators group only and leave it read-only to others.
2. Run `install.cmd` as administrator, this will create a scheduled task for you to run the OpenGlass host process which will inject DLL into DWM for you and also maintains that user settings are correctly loaded. 
3. Run `startup.cmd` as administrator, this will run the host process manually.
4. When you use it for the first time or just after updating your system, OpenGlass will try to download the symbol files and you will see its symbol downloading dialog, just be patient for about 15s. When the symbol files are ready, enjoy!
5. If OpenGlass is unable to download symbols, you can try running the `symchk-prepare-symbols.cmd`. Unable to download symbols is most commonly due to DWM being prevented from using the Internet connection, issues with certain system components, or Microsoft symbol server is blocked in your country or region. Of course, another possibility is that you update your system too frequently, in which case Microsoft typically releases the symbol files for your Windows to the server after about two weeks.
6. When you want to stop using OpenGlass or update the version of OpenGlass, running `shutdown.cmd` will remove the effects of OpenGlass for you and exit the host process. At this time, you can either replace the OpenGlass files or continue to run `uninstall.cmd` and manually delete the remaining files to complete the uninstallation.
7. When your desktop freezes, you can try pressing <kbd>Ctrl</kbd>+<kbd>Win</kbd>+<kbd>Shift</kbd>+<kbd>Q</kbd> to force the DWM process to terminate and generate a crash dump.
8. When you experience a crash, OpenGlass is supposed to generate a large memory dump file in the `dumps` directory of the folder where it is located, please submit it to the developer if possible, this can help fix known or potential issues.

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
> 
> When certain keys do not exist, OpenGlass uses the pre-defined default values. OpenGlass will NOT create or delete any registry items for you, therefore you must manually create and modify any items you need, and delete those you no longer need.
> 
> However, you may still use glass8's heritage `AeroGlassGUI.exe` to modify a small part of the settings.

### Colorization settings

| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| ColorizationColor<br>ColorizationColorInactive | DWORD | ARGB color used for the glass effect, alpha channel is ignored.<br><br>ℹ️ `ColorizationColorInactive` is only used when `GlassType` = 0x0 |
| ColorizationColorOverride | DWORD | Same as `ColorizationColor`, but OpenGlass prefers to use this item. It is optional.<br><br>ℹ️ The reason this value exists is that some of the functions in uxtheme.dll since Windows 10 continually reset the value of this key, and you want to have it fixed. |
| ColorizationAfterglow<br>ColorizationColorBalance<br>ColorizationAfterglowBalance<br>ColorizationBlurBalance | DWORD | Composition parameters for aero shader effect.<br><br>ℹ️ Only used when `GlassType` = 0x1 |
| ColorizationAfterglowOverride<br>ColorizationColorBalanceOverride<br>ColorizationAfterglowBalanceOverride<br>ColorizationBlurBalanceOverride | DWORD | Same as above, but OpenGlass prefers to use these. They are optional.<br><br>ℹ️ The reason these values exist is that some of the functions in `uxtheme.dll` since Windows 10 continually reset the values of some keys, and you want to have them fixed. |
| GlassOpacity<br>GlassOpacityInactive | DWORD | The intensity of the color (0-100%). Default value is 63%.<br><br>ℹ️ Only used when `GlassType` = 0x0 |
| ColorizationColorCaption<br>ColorizationColorCaptionInactive<br>ColorizationColorCaptionMaximized<br>ColorizationColorCaptionInactiveMaximized | DWORD | Color used for drawing window titles (default = 0xFFFFFFFF, determined by the system). Format is 0xBBGGRR.<br><br>ℹ️ 0xFFFFFFFE = Read the `TEXTCOLOR` property from the current theme to obtain them. |
| ColorizationOpaqueBlend | DWORD | Controls the transparency of glass effect (default = 0). |
| ColorizationOpaqueBlendColor<br>ColorizationOpaqueBlendColorMaximized | DWORD | ARGB base color used for opaque blending, alpha channel is ignored (default = 0xFFDFDFDF for non-maximized, default = 0 for maximized).<br><br> ℹ️ Setting `ColorisationOpaqueBlendColorMaximized` to a value that alpha channel is non-zero will make maximized windows or windows with `fTransitionOnMaximized` opaque to match Windows Vista behavior. |
| ColorizationOpaqueBlendPriority | DWORD | Behavior of choosing opaque blend base color. <br><br><ul><li>0x0 = Windows Vista (default).</li><li>0x1 = Windows 7.</li></ul>ℹ️ For Windows Vista, `ColorizationOpaqueBlendColorMaximized` is preferred, whereas for Windows 7 it is `ColorizationOpaqueBlendColor`. |
| ColorizationBlendingOpacity<br>ColorizationBlendingOpacityInactive<br>ColorizationBlendingOpacityMaximized<br>ColorizationBlendingOpacityInactiveMaximized | DWORD | (Additional) factors applied to glass color blending. (0%-100%). <br><br><ul><li>0xFFFFFFFD = Automatically select the appropriate factors to maintain compatibility with previous settings. (default).</li><li>0xFFFFFFFE = Automatically select the appropriate factors to match the msstyle for Windows Vista and Windows 7.</li><li>0xFFFFFFFF = Read the `COLORIZATIONOPACITY` property from the current theme to obtain them.</li></ul>💡 No need to modify `GlassOpacityInactive` and `ColorizationColorInactive` anymore if you only wish to achieve the Windows Vista appearance! |

### Glass settings
| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| GlassType | DWORD | The type of glass effect. <br><br><ul><li>0x0 = Windows Vista style blur (default).</li><li>0x1 = Windows 7 style blur.</li></ul> |
| GlassOverrideAccent | DWORD | Overrides accent blur surfaces with OpenGlass glass effects, I.E: the win10 taskbar (default = 0).<br><br>⚠️ Deprecated, which is not recommended for use, as it is harmful to performance. This key may be removed in future. |
| CustomThemeReflection | String | path to file with texture that is stretched over whole desktop and rendered above glass regions (default is Aero Glass Win7 reflection texture) |
| ColorizationGlassReflectionIntensity<br>ColorizationGlassReflectionIntensityInactive<br>ColorizationGlassReflectionIntensityMaximized<br>ColorizationGlassReflectionIntensityInactiveMaximized | DWORD | The intensity of reflection effect (0-100%). Default value is 0%. |
| ColorizationGlassReflectionParallaxIntensity | DWORD | The parallax intensity of the reflection effect (I.E when moving the windows side to side). Default value is 13%. | 
| ColorizationGlassReflectionPolicy | DWORD | Controls where reflections should be rendered (default = 0xFFFFFFFF). <br><br><ul><li>Titlebar = 1<<0</li><li>Aero Peek = 1<<2</li><li>Aero Snap = 1<<3 (ℹ️ Only valid in Win10)</li><li>Render everywhere if possible = 0xFFFFFFFF</li></ul> |
| BlurDeviation | DWORD | Standard deviation for gaussian blur, default = 30 (which means σ = 3.0) <br>Value 0 results in non-blurred transparency.<br><br>ℹ️ Only valid when `UseDirect3DRendering` = 0x0 |
| BlurOptimization | DWORD | Quality of gaussian blur<br><br><ul><li>0x0 = Speed first (default)</li><li>0x1 = Balance</li><li>0x2 = Quality first</li></ul>  |
| RoundRectRadius | DWORD | The radius of glass geometry (default = 0), Win8=0, Win7=6 | 
| CustomThemeMaterial | String | path to file with texture that is rendered (tiled) above glass regions (default is Acrylic noise texture) |
| MaterialOpacity | DWORD | opacity of material texture (default = 0) |
| UseDirect3DRendering | DWORD | Set 1 to use d3d11 as glass renderer backend, and the blur radius is hardcoded to 3. (default = 0)<br><br>ℹ️ Only valid when `GlassType` = 0x1<br>⚠️ The glass area will ignore the modern borders which appears visually overflow. |
| GlassSafetyZoneMode | DWORD | Set 0 to disable glass safety zone. (default = 1)<br><br>⚠️ Do not touch this unless you know what will happen! |

### Theme settings
| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| CaptionButtons | DWORD | Changes caption buttons sizes and the opacity of the button glyphs.<br><br><ul><li>0x0 = Windows 10 style (default)</li><li>0x1 = Windows Vista style</li><li>0x2 = Windows 7 style</li><li>0x3 = Windows 8 style</li><li>0x4 = Windows 8 DP/CP style</li><li>0x5 = Custom</li></ul> |
| CustomHeight, CustomLoneWidth, CustomMinWidth, CustomMaxWidth, CustomCloseWidth, CustomTitlebarHeight, CustomToolWidth, CustomToolHeight | DWORD |⚠️ Only works on Custom mode! Uses the user set value to each corresponding key name. |
| ButtonGroupOffsetX, ButtonGroupOffsetY, ButtonSpacing, CloseOffsetX, CloseOffsetY, MaxOffsetX, MaxOffsetY, MinOffsetX, MinOffsetY, ToolOffsetX, ToolOffsetY | DWORD | Ditto to the above. |
| TopInsert | DWORD | Makes the white line appear on maximized mode. <br><br><ul><li>0x0 = Disabled (default)</li><li>0x1 = Enabled</li></ul> |
| CenterCaption | DWORD | Controls how title bar text is aligned.<ul><li>0x0=Keeps it on the left (default)</li><li>0x1=Centers it between the titlebar icon and the titlebar buttons</li><li>0x2=Centers the Win8 way</li></ul><br>ℹ️ Untested on Win11 |
| TextGlowMode | DWORD | Specifies how window caption glow effect will be rendered <br><br><ul><li>0x0 = No glow effect</li><li>0x1 = Glow effect loaded from atlas (default)</li><li>0x2 = Glow effect loaded from atlas and theme opacity is respected</li><li>0x3 = Composited glow effect using your theme settings HIWORD of the value specifies glow size (0 = theme default)</li> |
| CustomThemeAtlas | String | path to PNG file with theme resource (bitmap must have exactly the same layout as msstyle theme you are using!) |
| DisableModernBorders | DWORD | Disable modern rounded window borders. <br><br><ul><li>0x0 = Enable modern borders (default)</li><li>0x1 = Disable modern borders</li></ul><br>ℹ️ Only valid in Win11 |

### Advanced settings

The following registry items are located only in `HKLM\Software\Microsoft\Windows\DWM`, you should NOT change the settings in this section unless you know what you are doing.

| Key Name | Type | Description | 
| -------- | ---- | ----------- |
| DisableGlassOnBattery | DWORD | <ul><li>0x1 = When energy saver is on then the glass effect will be opaque to decrease energy consumption (default)</li><li>0x0 = glass effect won't be opaque on energy saver</li></ul> |
| DisableMemoryDump | DWORD | <ul><li>0x0 = Generates a memory dump file when DWM crashes (default)</li><li>0x1 = No memory dump files</li></ul> |
| DisabledHooks | DWORD | Controls which module's hooks are disabled, which will also control the availability of features. <br><br><ul><li>0x0 = No hooks are disabled (default)</li><li>0x1 = Disables hooks for [CaptionTextHandler.cpp](OpenGlass/CaptionTextHandler.cpp)</li><li>0x2 = Disables hooks for [AccentOverrider.cpp](OpenGlass/AccentOverrider.cpp)</li><li>0x4 = Disables hooks for [GlassFrameHandler.cpp](OpenGlass/GlassFrameHandler.cpp)</li><li>0x8 = Disables hooks for [GlassReflectionHandler.cpp](OpenGlass/GlassReflectionHandler.cpp)</li></ul><br>⚠️ Unless you want to ensure compatibility with third-party applications, you should not modify this key. |

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

## Support
The software was developed in free time and is now open source under the GPLv3 license.

Since Desktop Window Manager does not provide any official support for its extensibility, OpenGlass internally uses many undocumented techniques to achieve the desired effect. Although these techniques have been developed precisely with an emphasis on stability, performance and compatibility, it may happen that some future Windows update will break it. The developer cannot ensure that OpenGlass will be fully functional in such case but he will do its best to make it work as soon as possible.

If you are satisfied with my work, please consider donating via Ko-fi to support OpenGlass. By donation you agree with the following statements:
* **You donate voluntarily without a claim for consideration**.
* You donate as a natural person.


[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/altalex531)
