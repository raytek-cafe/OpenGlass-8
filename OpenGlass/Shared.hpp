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

	inline D2D1_COLOR_F g_color{};
	inline D2D1_COLOR_F g_colorInactive{};

	inline bool g_opaqueBlend{};
	inline D2D1_COLOR_F g_opaqueBlendColor{};
	// exclusively used by aero backdrop - begin
	inline float g_colorBalance{};
	inline float g_afterglowBalance{};
	inline float g_blurBalance{};
	inline D2D1_COLOR_F g_afterglow{};
	// exclusively used by aero backdrop - end

	inline wil::unique_hbitmap g_textGlowBitmap{};
	inline float g_textOpacity{};
	inline float g_textOpacityInactive{};
	inline int g_textGlowMode{};
	inline bool g_dontDeflateInactiveFrameGeometry{ true };
	inline std::optional<LONG> g_captionHeight{};

	inline float g_reflectionIntensity{};
	inline float g_reflectionIntensityInactive{};
	inline float g_reflectionParallaxIntensity{};
	inline std::wstring g_reflectionTexturePath{};

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
	FORCEINLINE bool IsGlassFullyOpaque()
	{
		if (IsTransparencyDisabled())
		{
			return true;
		}
		if (
			g_type == GlassType::Blur &&
			g_glassOpacity == 1.f &&
			g_glassOpacityInactive == 1.f
		)
		{
			return true;
		}

		return false;
	}
	FORCEINLINE bool IsGlassFullyOpaque(
		float glassOpacity,
		float blurBalance,
		float afterglowBalance
	)
	{
		if (IsTransparencyDisabled())
		{
			return true;
		}
		if (
			g_type == GlassType::Blur &&
			glassOpacity == 1.f
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