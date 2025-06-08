#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "HookHelper.hpp"

namespace OpenGlass::Util
{
	inline const auto g_processHeap = GetProcessHeap();
	inline const auto g_thisModulePath = wil::GetModuleFileNameW<std::wstring, MAX_PATH>(wil::GetModuleInstanceHandle());

	using unique_rouninitialize_call = wil::unique_call<decltype(&::RoUninitialize), ::RoUninitialize>;

	consteval size_t compile_time_hash(const char* str, size_t seed)
	{
		return 0 == *str ? seed : compile_time_hash(str + 1, seed ^ (*str + 0x9e3779b9 + (seed << 6) + (seed >> 2)));
	}

	FORCEINLINE auto to_error_string(HRESULT hr)
	{
		return winrt::hresult_error{ hr }.message();
	}

	template <typename T>
	FORCEINLINE constexpr auto force_cast_to(PVOID ptr)
	{
		T target_ptr{ nullptr };
		*reinterpret_cast<PVOID*>(&target_ptr) = ptr;
		return target_ptr;
	}
	template <typename T>
	FORCEINLINE constexpr PVOID force_cast_from(T ptr)
	{
		return *reinterpret_cast<PVOID*>(&ptr);
	}

	inline auto make_current_folder_file(std::wstring_view baseFileName)
	{
		const auto bufferSize = g_thisModulePath.size() + baseFileName.size() + 1;
		auto filePath = std::make_unique<WCHAR[]>(bufferSize);
		wcscpy_s(
			filePath.get(),
			bufferSize,
			g_thisModulePath.c_str()
		);
		PathCchRemoveFileSpec(filePath.get(), bufferSize);
		if (!baseFileName.empty())
		{
			PathCchAppend(filePath.get(), bufferSize, baseFileName.data());
		}

		return filePath;
	}
	
	template <UINT id>
	FORCEINLINE const auto GetResourceStringView()
	{
		LPCWSTR buffer{ nullptr };
		auto length = LoadStringW(wil::GetModuleInstanceHandle(), id, reinterpret_cast<LPWSTR>(&buffer), 0);
		return std::wstring_view{ buffer, static_cast<size_t>(length) };
	}
	template <UINT id>
	FORCEINLINE std::wstring GetResourceString()
	{
		return std::wstring{ GetResourceStringView<id>() };
	}

	inline HRESULT MB2WC(std::unique_ptr<WCHAR[]>& buffer, LPCSTR str, int size = -1, int* outLength = nullptr)
	{
		auto length = MultiByteToWideChar(CP_ACP, 0, str, size, nullptr, 0);
		RETURN_LAST_ERROR_IF(length == 0);
		buffer.reset(new WCHAR[length]);
		RETURN_LAST_ERROR_IF_NULL(buffer);
		length = MultiByteToWideChar(CP_ACP, 0, str, size, buffer.get(), length);
		RETURN_LAST_ERROR_IF(length == 0);
		if (outLength) { *outLength = length; }
		return S_OK;
	}
	inline HRESULT WC2MB(std::unique_ptr<CHAR[]>& buffer, LPCWSTR str, int size = -1, int* outLength = nullptr)
	{
		auto length = WideCharToMultiByte(CP_UTF8, 0, str, size, nullptr, 0, nullptr, nullptr);
		RETURN_LAST_ERROR_IF(length == 0);
		buffer.reset(new CHAR[length]);
		RETURN_LAST_ERROR_IF_NULL(buffer);
		length = WideCharToMultiByte(CP_UTF8, 0, str, size, buffer.get(), length, nullptr, nullptr);
		RETURN_LAST_ERROR_IF(length == 0);
		if (outLength) { *outLength = length; }
		return S_OK;
	}

	// type = 1, input desktop (grpdeskRitInput)
	// type = 2, default desktop? (grpdeskIODefault)
	// type = 3, unknown
	// type = 4, winlogon desktop (grpdeskLogon)
	// type = ..., desktop created by CreateDesktop?
	FORCEINLINE bool WINAPI GetDesktopID(ULONG_PTR type, ULONG_PTR* desktopID)
	{
		static const auto pfnGetDesktopID = reinterpret_cast<decltype(&GetDesktopID)>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDesktopID"));
		if (pfnGetDesktopID) [[likely]]
		{
			return pfnGetDesktopID(type, desktopID);
		}

		return false;
	}

	FORCEINLINE bool WINAPI IsTransparencyEnabled()
	{
		DWORD value{ 0 };
		wil::reg::get_value_dword_nothrow(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency", &value);
		return static_cast<bool>(value);
	}
	FORCEINLINE bool WINAPI IsTransparencyEnabled(HKEY key)
	{
		DWORD value{ 0 };
		wil::reg::get_value_dword_nothrow(key, L"EnableTransparency", &value);
		return static_cast<bool>(value);
	}

	FORCEINLINE bool WINAPI IsRunAsLocalSystem()
	{
		BOOL isLocalSystem{ FALSE };
		wil::unique_sid sid{ nullptr };
		SID_IDENTIFIER_AUTHORITY authority{ SECURITY_NT_AUTHORITY };
		if (AllocateAndInitializeSid(&authority, 1ul, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &sid))
		{
			CheckTokenMembership(0, sid.get(), &isLocalSystem);
		}
		return static_cast<bool>(isLocalSystem);
	}

	inline HRESULT WINAPI TaskDialog(
		_In_opt_ HWND hwndOwner,
		_In_opt_ HINSTANCE hInstance,
		_In_opt_ PCWSTR pszWindowTitle,
		_In_opt_ PCWSTR pszMainInstruction,
		_In_opt_ PCWSTR pszContent,
		TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons, 
		_In_opt_ PCWSTR pszIcon,
		_Out_opt_ int* pnButton
	)
	{
		TASKDIALOGCONFIG config
		{ 
			sizeof(TASKDIALOGCONFIG), 
			hwndOwner, 
			hInstance,
			TDF_SIZE_TO_CONTENT | TDF_ALLOW_DIALOG_CANCELLATION,
			dwCommonButtons,
			pszWindowTitle,
			{ .pszMainIcon{ pszIcon } },
			pszMainInstruction,
			pszContent,
			0, 
			nullptr, 
			0, 
			0, 
			nullptr, 
			0, 
			nullptr, 
			nullptr, 
			nullptr, 
			nullptr, 
			{}, 
			nullptr, 
			nullptr, 
			0, 
			0 
		};
		return TaskDialogIndirect(
			&config,
			pnButton,
			nullptr,
			nullptr
		);
	}

	inline ULONG GetModuleBuildNumber(HMODULE moduleHandle)
	{
		const auto moduleName = wil::GetModuleFileNameW<std::wstring, MAX_PATH>(moduleHandle);

		DWORD size{ GetFileVersionInfoSizeW(moduleName.c_str(), nullptr)};
		THROW_LAST_ERROR_IF(!size);

		auto data{ std::make_unique_for_overwrite<BYTE[]>(size) };
		#pragma warning(suppress : 6388)
		THROW_LAST_ERROR_IF(!GetFileVersionInfoW(moduleName.c_str(), 0, size, data.get()));

		UINT len{ 0 };
		VS_FIXEDFILEINFO* fileInfo{ nullptr };
		THROW_IF_WIN32_BOOL_FALSE(VerQueryValueW(data.get(), L"\\", reinterpret_cast<PVOID*>(&fileInfo), &len));
		THROW_LAST_ERROR_IF(!len);

		THROW_WIN32_IF(ERROR_INVALID_IMAGE_HASH, fileInfo->dwSignature != VS_FFI_SIGNATURE);

		return HIWORD(fileInfo->dwFileVersionLS);
	}

	inline winrt::com_ptr<ID2D1Bitmap1> GetTargetBitmapFromDeviceContext(
		ID2D1DeviceContext* context
	)
	{
		winrt::com_ptr<ID2D1Bitmap1> targetBitmap{ nullptr };

		winrt::com_ptr<ID2D1Image> targetImage{ nullptr };
		context->GetTarget(targetImage.put());
		if (!targetImage)
		{
			return targetBitmap;
		}

		LOG_IF_FAILED(targetImage->QueryInterface(targetBitmap.put()));

		return targetBitmap;
	}

	inline HRESULT DrawNineGridBitmap(
		ID2D1DeviceContext* context,
		ID2D1Bitmap1* bitmap,
		const D2D1_RECT_F& destinationRect,
		const MARGINS& margins,
		float opacity
	)
	{
		const auto primitiveBlend = context->GetPrimitiveBlend();
		context->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
		context->PushAxisAlignedClip(destinationRect, D2D1_ANTIALIAS_MODE_ALIASED);
		const auto restoreBlendMode = wil::scope_exit([&] 
		{
			context->PopAxisAlignedClip();
			context->SetPrimitiveBlend(primitiveBlend);
		});

		const auto destinationWidth = wil::rect_width(destinationRect);
		const auto destinationHeight = wil::rect_height(destinationRect);

		if (destinationWidth <= 0.f || destinationHeight <= 0.f)
		{
			return S_OK; // Nothing to draw for non-positive destination size
		}

		const auto bitmapSize = bitmap->GetSize();

		// Source margin sizes from input MARGINS, converted to float
		// Assuming margins members (cxLeftWidth etc.) are non-negative as is typical.
		D2D1_RECT_F srcMargins
		{
			static_cast<float>(margins.cxLeftWidth),
			static_cast<float>(margins.cyTopHeight),
			static_cast<float>(margins.cxRightWidth),
			static_cast<float>(margins.cyBottomHeight)
		};

		// Source center part dimensions from bitmap
		const auto totalSrcHorizontalMargins = srcMargins.left + srcMargins.right;
		const auto totalSrcVerticalMargins = srcMargins.top + srcMargins.bottom;
		const auto srcCenterPartWidth = std::max(0.f, bitmapSize.width - totalSrcHorizontalMargins);
		const auto srcCenterPartHeight = std::max(0.f, bitmapSize.height - totalSrcVerticalMargins);

		// Initialize effective destination dimensions for margins and center parts
		D2D1_RECT_F destMargins = srcMargins;

		auto destCenterPartWidth = destinationWidth - totalSrcHorizontalMargins;
		auto destCenterPartHeight = destinationHeight - totalSrcVerticalMargins;

		// Adjust horizontal destination parts if calculated center width is negative
		if (destCenterPartWidth < 0.f)
		{
			destCenterPartWidth = 0.f; // Center part cannot be negative
			
			const auto ratio = destinationWidth / totalSrcHorizontalMargins;
			destMargins.left *= ratio;
			destMargins.left = std::round(destMargins.left);
			destMargins.right = destinationWidth - destMargins.left;
		}

		// Adjust vertical destination parts if calculated center height is negative
		if (destCenterPartHeight < 0.f)
		{
			destCenterPartHeight = 0.f; // Center part cannot be negative

			const auto ratio = destinationHeight / totalSrcVerticalMargins;
			destMargins.top *= ratio;
			destMargins.top = std::round(destMargins.top);
			destMargins.bottom = destinationHeight - destMargins.top;
		}

		// Top-Left
		context->DrawBitmap(
			bitmap,
			D2D1::RectF(destinationRect.left, destinationRect.top, destinationRect.left + destMargins.left, destinationRect.top + destMargins.top),
			opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			D2D1::RectF(0.f, 0.f, srcMargins.left, srcMargins.top)
		);

		if (destCenterPartWidth)
		{
			// Top-Center
			context->DrawBitmap(
				bitmap,
				D2D1::RectF(destinationRect.left + destMargins.left, destinationRect.top, destinationRect.left + destMargins.left + destCenterPartWidth, destinationRect.top + destMargins.top),
				opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
				D2D1::RectF(srcMargins.left, 0.f, srcMargins.left + srcCenterPartWidth, srcMargins.top)
			);
		}

		// Top-Right
		context->DrawBitmap(
			bitmap,
			D2D1::RectF(destinationRect.right - destMargins.right, destinationRect.top, destinationRect.right, destinationRect.top + destMargins.top),
			opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			D2D1::RectF(bitmapSize.width - srcMargins.right, 0.f, bitmapSize.width, srcMargins.top)
		);

		if (destCenterPartHeight)
		{
			// Middle-Left
			context->DrawBitmap(
				bitmap,
				D2D1::RectF(destinationRect.left, destinationRect.top + destMargins.top, destinationRect.left + destMargins.left, destinationRect.top + destMargins.top + destCenterPartHeight),
				opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
				D2D1::RectF(0.f, srcMargins.top, srcMargins.left, srcMargins.top + srcCenterPartHeight)
			);
		}

		if (destCenterPartWidth && destCenterPartHeight)
		{
			// Middle-Center
			context->DrawBitmap(
				bitmap,
				D2D1::RectF(destinationRect.left + destMargins.left, destinationRect.top + destMargins.top, destinationRect.left + destMargins.left + destCenterPartWidth, destinationRect.top + destMargins.top + destCenterPartHeight),
				opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
				D2D1::RectF(srcMargins.left, srcMargins.top, srcMargins.left + srcCenterPartWidth, srcMargins.top + srcCenterPartHeight)
			);
		}

		if (destCenterPartHeight)
		{
			// Middle-Right
			context->DrawBitmap(
				bitmap,
				D2D1::RectF(destinationRect.right - destMargins.right, destinationRect.top + destMargins.top, destinationRect.right, destinationRect.top + destMargins.top + destCenterPartHeight),
				opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
				D2D1::RectF(bitmapSize.width - srcMargins.right, srcMargins.top, bitmapSize.width, srcMargins.top + srcCenterPartHeight)
			);
		}

		// Bottom-Left
		context->DrawBitmap(
			bitmap,
			D2D1::RectF(destinationRect.left, destinationRect.bottom - destMargins.bottom, destinationRect.left + destMargins.left, destinationRect.bottom),
			opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			D2D1::RectF(0.f, bitmapSize.height - srcMargins.bottom, srcMargins.left, bitmapSize.height)
		);

		if (destCenterPartWidth)
		{
			// Bottom-Center
			context->DrawBitmap(
				bitmap,
				D2D1::RectF(destinationRect.left + destMargins.left, destinationRect.bottom - destMargins.bottom, destinationRect.left + destMargins.left + destCenterPartWidth, destinationRect.bottom),
				opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
				D2D1::RectF(srcMargins.left, bitmapSize.height - srcMargins.bottom, srcMargins.left + srcCenterPartWidth, bitmapSize.height)
			);
		}

		// Bottom-Right
		context->DrawBitmap(
			bitmap,
			D2D1::RectF(destinationRect.right - destMargins.right, destinationRect.bottom - destMargins.bottom, destinationRect.right, destinationRect.bottom),
			opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			D2D1::RectF(bitmapSize.width - srcMargins.right, bitmapSize.height - srcMargins.bottom, bitmapSize.width, bitmapSize.height)
		);

		return S_OK;
	}
	inline HRESULT DrawNineGridBitmap(
		HDC hdc,
		HBITMAP bitmap,
		const RECT& destinationRect,
		const MARGINS& margins,
		int opacity
	)
	{
		wil::unique_hdc compatibleDC{ CreateCompatibleDC(hdc) };
		RETURN_LAST_ERROR_IF_NULL(compatibleDC);

		BITMAP bmp{};
		RETURN_LAST_ERROR_IF(GetObjectW(bitmap, sizeof(bmp), &bmp) == 0);

		SelectObject(compatibleDC.get(), bitmap);

		const auto destinationWidth = wil::rect_width(destinationRect);
		const auto destinationHeight = wil::rect_height(destinationRect);

		if (destinationWidth <= 0 || destinationHeight <= 0)
		{
			return S_OK; // Nothing to draw for non-positive destination size
		}

		// Source center part dimensions from bitmap
		const auto totalSrcHorizontalMargins = margins.cxLeftWidth + margins.cxRightWidth;
		const auto totalSrcVerticalMargins = margins.cyTopHeight + margins.cyBottomHeight;
		const auto srcCenterPartWidth = std::max(0l, bmp.bmWidth - totalSrcHorizontalMargins);
		const auto srcCenterPartHeight = std::max(0l, bmp.bmHeight - totalSrcVerticalMargins);

		// Calculate effective destination dimensions for margins and center parts
		MARGINS destMargins = margins;

		int destCenterPartWidth = destinationWidth - totalSrcHorizontalMargins;
		int destCenterPartHeight = destinationHeight - totalSrcVerticalMargins;

		// Adjust horizontal destination parts if calculated center width is negative
		if (destCenterPartWidth < 0)
		{
			destCenterPartWidth = 0; // Center part cannot be negative

			const auto ratio = static_cast<float>(destinationWidth) / static_cast<float>(totalSrcHorizontalMargins);
			destMargins.cxLeftWidth = static_cast<int>(destMargins.cxLeftWidth * ratio);
			destMargins.cxRightWidth = destinationWidth - destMargins.cxLeftWidth;
		}

		// Adjust vertical destination parts if calculated center height is negative
		if (destCenterPartHeight < 0)
		{
			destCenterPartHeight = 0; // Center part cannot be negative

			const auto ratio = static_cast<float>(destinationHeight) / static_cast<float>(totalSrcVerticalMargins);
			destMargins.cyTopHeight = static_cast<int>(destMargins.cyTopHeight * ratio);
			destMargins.cyBottomHeight = destinationHeight - destMargins.cyTopHeight;
		}

		BLENDFUNCTION bf
		{
			AC_SRC_OVER,
			0,
			static_cast<BYTE>(opacity),
			AC_SRC_ALPHA
		};
		// left, top
		GdiAlphaBlend(
			hdc,
			destinationRect.left,
			destinationRect.top,
			destMargins.cxLeftWidth, 
			destMargins.cyTopHeight,
			compatibleDC.get(),
			0, 
			0, 
			margins.cxLeftWidth, 
			margins.cyTopHeight,
			bf
		);
		// right, top
		GdiAlphaBlend(
			hdc,
			destinationRect.right - destMargins.cxRightWidth,
			destinationRect.top,
			destMargins.cxRightWidth, 
			destMargins.cyTopHeight,
			compatibleDC.get(),
			bmp.bmWidth - margins.cxRightWidth, 
			0, 
			margins.cxRightWidth, 
			margins.cyTopHeight,
			bf
		);
		// left, bottom
		GdiAlphaBlend(
			hdc,
			destinationRect.left,
			destinationRect.bottom - destMargins.cyBottomHeight,
			destMargins.cxLeftWidth, 
			destMargins.cyBottomHeight,
			compatibleDC.get(),
			0, 
			bmp.bmHeight - margins.cyBottomHeight, 
			margins.cxLeftWidth, 
			margins.cyBottomHeight,
			bf
		);
		// right, bottom
		GdiAlphaBlend(
			hdc,
			destinationRect.right - destMargins.cxRightWidth,
			destinationRect.bottom - destMargins.cyBottomHeight,
			destMargins.cxRightWidth, 
			destMargins.cyBottomHeight,
			compatibleDC.get(),
			bmp.bmWidth - margins.cxRightWidth, 
			bmp.bmHeight - margins.cyBottomHeight, 
			margins.cxRightWidth, 
			margins.cyBottomHeight,
			bf
		);
		if (destCenterPartWidth)
		{
			// center, top
			GdiAlphaBlend(
				hdc,
				destinationRect.left + destMargins.cxLeftWidth,
				destinationRect.top,
				destCenterPartWidth, 
				destMargins.cyTopHeight,
				compatibleDC.get(),
				margins.cxLeftWidth, 
				0, 
				srcCenterPartWidth, 
				margins.cyTopHeight,
				bf
			);
		}
		if (destCenterPartHeight)
		{
			// left, center
			GdiAlphaBlend(
				hdc,
				destinationRect.left,
				destinationRect.top + destMargins.cyTopHeight,
				destMargins.cxLeftWidth, 
				destCenterPartHeight,
				compatibleDC.get(),
				0, 
				margins.cyTopHeight, 
				margins.cxLeftWidth, 
				srcCenterPartHeight,
				bf
			);
		}
		if (destCenterPartHeight)
		{
			// right, center
			GdiAlphaBlend(
				hdc,
				destinationRect.right - destMargins.cxRightWidth,
				destinationRect.top + destMargins.cyTopHeight,
				destMargins.cxRightWidth, 
				destCenterPartHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth, 
				margins.cyTopHeight, 
				margins.cxRightWidth, 
				srcCenterPartHeight,
				bf
			);
		}
		if (destCenterPartWidth)
		{
			// center, bottom
			GdiAlphaBlend(
				hdc,
				destinationRect.left + destMargins.cxLeftWidth,
				destinationRect.bottom - destMargins.cyBottomHeight,
				destCenterPartWidth, 
				destMargins.cyBottomHeight,
				compatibleDC.get(),
				margins.cxLeftWidth, 
				bmp.bmHeight - margins.cyBottomHeight, 
				srcCenterPartWidth, 
				margins.cyBottomHeight,
				bf
			);
		}
		if (destCenterPartWidth && destCenterPartHeight)
		{
			// center, center
			GdiAlphaBlend(
				hdc,
				destinationRect.left + destMargins.cxLeftWidth,
				destinationRect.top + destMargins.cyTopHeight,
				destCenterPartWidth, 
				destCenterPartHeight,
				compatibleDC.get(),
				margins.cxLeftWidth, 
				margins.cyTopHeight, 
				srcCenterPartWidth, 
				srcCenterPartHeight,
				bf
			);
		}

		return S_OK;
	}
}
namespace OpenGlass
{
	template<class T>
	struct CClassFactoryT : winrt::implements<CClassFactoryT<T>, IClassFactory, winrt::non_agile, winrt::no_weak_ref>
	{
		HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) noexcept override
		{
			if (!pUnkOuter)
			{
				*ppvObject = nullptr;
				return winrt::make<T>().as(riid, ppvObject);
			}

			return CLASS_E_NOAGGREGATION;
		}
		HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) noexcept override
		{
			if (fLock)
			{
				++winrt::get_module_lock();
			}
			else
			{
				--winrt::get_module_lock();
			}

			return S_OK;
		}
	};
}
namespace OpenGlass
{
	namespace Color
	{
		FORCEINLINE D2D1_COLOR_F sRGBToscRGB(const D2D1_COLOR_F& color)
		{
			return D2D1ConvertColorSpace(
				D2D1_COLOR_SPACE_SRGB,
				D2D1_COLOR_SPACE_SCRGB,
				&color
			);
		}
		FORCEINLINE D2D1_COLOR_F scRGBTosRGB(const D2D1_COLOR_F& color)
		{
			return D2D1ConvertColorSpace(
				D2D1_COLOR_SPACE_SCRGB,
				D2D1_COLOR_SPACE_SRGB,
				&color
			);
		}
		FORCEINLINE constexpr D2D1_COLOR_F FromAbgr(DWORD color, bool ignoreAlpha = true)
		{
			return
			{
				static_cast<float>((color & 0x000000FF) >> 0) / 255.f,
				static_cast<float>((color & 0x0000FF00) >> 8) / 255.f,
				static_cast<float>((color & 0x00FF0000) >> 16) / 255.f,
				ignoreAlpha ? 1.f : static_cast<float>((color & 0xFF000000) >> 24) / 255.f
			};
		}
		FORCEINLINE constexpr D2D1_COLOR_F FromArgb(DWORD color, bool ignoreAlpha = true)
		{
			return
			{
				static_cast<float>((color & 0x00FF0000) >> 16) / 255.f,
				static_cast<float>((color & 0x0000FF00) >> 8) / 255.f,
				static_cast<float>((color & 0x000000FF) >> 0) / 255.f,
				ignoreAlpha ? 1.f : static_cast<float>((color & 0xFF000000) >> 24) / 255.f
			};
		}
		FORCEINLINE constexpr DWORD ToArgb(const D2D1_COLOR_F& color, bool ignoreAlpha = true)
		{
			return
				((ignoreAlpha ? 0xFFul : std::min(static_cast<DWORD>(color.a * 255.f + 0.5f), 255ul)) << 24) |
				(std::min(static_cast<DWORD>(color.r * 255.f + 0.5f), 255ul) << 16) |
				(std::min(static_cast<DWORD>(color.g * 255.f + 0.5f), 255ul) << 8) |
				(std::min(static_cast<DWORD>(color.b * 255.f + 0.5f), 255ul) << 0);
		}
		FORCEINLINE constexpr DWORD ToAbgr(const D2D1_COLOR_F& color, bool ignoreAlpha = true)
		{
			return
				((ignoreAlpha ? 0xFFul : std::min(static_cast<DWORD>(color.a * 255.f + 0.5f), 255ul)) << 24) |
				(std::min(static_cast<DWORD>(color.b * 255.f + 0.5f), 255ul) << 16) |
				(std::min(static_cast<DWORD>(color.g * 255.f + 0.5f), 255ul) << 8) |
				(std::min(static_cast<DWORD>(color.r * 255.f + 0.5f), 255ul) << 0);
		}
	}

	namespace RectF
	{
		FORCEINLINE D2D1_RECT_F TransformRect(const D2D1_RECT_F& rectangle, const D2D1_MATRIX_3X2_F& matrix)
		{
			const auto matrixHelper = D2D1::Matrix3x2F::ReinterpretBaseType(&matrix);
			const auto topLeft = matrixHelper->TransformPoint(D2D1::Point2F(rectangle.left, rectangle.top));
			const auto topRight = matrixHelper->TransformPoint(D2D1::Point2F(rectangle.right, rectangle.top));
			const auto bottomLeft = matrixHelper->TransformPoint(D2D1::Point2F(rectangle.left, rectangle.bottom));
			const auto bottomRight = matrixHelper->TransformPoint(D2D1::Point2F(rectangle.right, rectangle.bottom));

			D2D1_RECT_F transformedRect{};
			transformedRect.left = std::min({ topLeft.x, topRight.x, bottomLeft.x, bottomRight.x });
			transformedRect.top = std::min({ topLeft.y, topRight.y, bottomLeft.y, bottomRight.y });
			transformedRect.right = std::max({ topLeft.x, topRight.x, bottomLeft.x, bottomRight.x });
			transformedRect.bottom = std::max({ topLeft.y, topRight.y, bottomLeft.y, bottomRight.y });

			return transformedRect;
		}
		FORCEINLINE constexpr bool DoesIntersectUnsafe(
			const D2D1_RECT_F& rc1,
			const D2D1_RECT_F& rc2
		)
		{
			if (
				!wil::rect_is_empty(rc1) &&
				!wil::rect_is_empty(rc2) &&

				rc1.right > rc2.left &&
				rc2.right > rc1.left &&
				rc1.bottom > rc2.top &&
				rc2.bottom > rc1.top
			)
			{
				return true;
			}
			return false;
		}
		inline constexpr bool IntersectUnsafe(
			D2D1_RECT_F& dst,
			const D2D1_RECT_F& src
		)
		{
			dst.left = std::max(dst.left, src.left);
			dst.top = std::max(dst.top, src.top);
			dst.right = std::min(dst.right, src.right);
			dst.bottom = std::min(dst.bottom, src.bottom);

			if (!wil::rect_is_empty(dst))
			{
				return true;
			}

			dst.left = dst.top = dst.right = dst.bottom = 0.f;
			return false;
		}
	}
}
