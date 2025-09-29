#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass::Shared
{
	inline bool g_xxSaver{ false };
	inline bool g_transparencyEnabled{ true };

	inline enum class GlassType : UINT
	{
		Blur,
		Aero,

		Invalid = 0xFF
	} g_type{ 0 };
	inline enum ReflectionPolicy : UINT
	{
		NonClient = 1 << 0,
		LivePreview = 1 << 2,
		AnimatedGlassSheet = 1 << 3,
	} g_reflectionPolicy{ 0 };

	inline bool g_disableOnBattery{ true };
	inline bool g_overrideAccent{ false };
	inline bool g_disableModernBorders{ false };
	inline int g_roundRectRadius{};
	inline float g_blurAmount{};
	inline D2D1_GAUSSIANBLUR_OPTIMIZATION g_blurOptimization{};
	inline float g_glassOpacity{};
	inline float g_glassOpacityInactive{};
	inline bool g_useD3DRendering{};

	inline D2D1_COLOR_F g_color{};
	inline D2D1_COLOR_F g_colorInactive{};

	// vista: 1.f
	// w7: 1.f
	inline float g_colorizationBlendingOpacity{};
	// vista: 0.55f
	// w7: 0.4f
	inline float g_colorizationBlendingOpacityInactive{};
	// vista: 0.75f
	// w7: 1.f
	inline float g_colorizationBlendingOpacityMaximized{};
	// vista: 0.75f
	// w7: 0.4f
	inline float g_colorizationBlendingOpacityInactiveMaximized{};

	inline enum OpaqueBlendPriority : UINT
	{
		Vista,
		Win7
	} g_opaqueBlendPriority{ 0 };
	inline bool g_opaqueBlend{};
	inline D2D1_COLOR_F g_opaqueBlendColor{};
	inline D2D1_COLOR_F g_opaqueBlendColorMaximized{};

	inline float g_colorBalance{};
	inline float g_afterglowBalance{};
	inline float g_blurBalance{};
	inline D2D1_COLOR_F g_afterglow{};

	inline BITMAPINFO g_textGlowBitmapInfo{};
	inline PVOID g_textGlowBitmapPixels{};
	inline wil::unique_hbitmap g_textGlowBitmap{};
	inline int g_textGlowMode{};
	inline bool g_dontDeflateInactiveFrameGeometry{ true };
	inline std::optional<LONG> g_captionHeight{};

	inline float g_reflectionIntensity{};
	inline float g_reflectionIntensityInactive{};
	inline float g_reflectionParallaxIntensity{};
	inline std::wstring g_reflectionTexturePath{};

	inline float g_materialIntensity{};
	inline std::wstring g_materialTexturePath{};

	FORCEINLINE bool IsTransparencyDisabled()
	{
		if (g_opaqueBlend)
		{
			return true;
		}
		if (g_xxSaver && g_disableOnBattery)
		{
			return true;
		}
		if (!g_transparencyEnabled)
		{
			return true;
		}

		return false;
	}
	FORCEINLINE bool IsOpaqueOnMaximized(bool maximized)
	{
		return maximized && g_opaqueBlendColorMaximized.a != 0.f;
	}
	FORCEINLINE bool IsGlassFullyOpaque(
		float alpha,
		float blurBalance,
		float afterglowBalance
	)
	{
		if (
			g_type == GlassType::Blur &&
			alpha == 1.f
		)
		{
			return true;
		}
		// uDWM!CCapturedGlassColorizationParameters::IsGlassFullyOpaque (Windows 7)
		if (
			#pragma warning(suppress:26813)
			g_type == GlassType::Aero &&
			blurBalance == 0.f &&
			afterglowBalance == 0.f
		)
		{
			return true;
		}

		return false;
	}
}
