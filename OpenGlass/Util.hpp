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
		
		POINT pt{};
		RETURN_IF_WIN32_BOOL_FALSE(OffsetViewportOrgEx(hdc, destinationRect.left, destinationRect.top, &pt));
		const auto cleanUp = wil::scope_exit([hdc, &pt]
		{
			SetViewportOrgEx(hdc, pt.x, pt.y, nullptr);
		});

		const auto destinationWidth = wil::rect_width(destinationRect);
		const auto destinationHeight = wil::rect_height(destinationRect);

		if (opacity == 255)
		{
			// left, top
			StretchBlt(
				hdc,
				0,
				0,
				margins.cxLeftWidth,
				margins.cyTopHeight,
				compatibleDC.get(),
				0,
				0,
				margins.cxLeftWidth,
				margins.cyTopHeight,
				SRCCOPY
			);
			// right, top
			StretchBlt(
				hdc,
				destinationWidth - margins.cxRightWidth,
				0,
				margins.cxRightWidth,
				margins.cyTopHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth,
				0,
				margins.cxRightWidth,
				margins.cyTopHeight,
				SRCCOPY
			);
			// left, bottom
			StretchBlt(
				hdc,
				0,
				destinationHeight - margins.cyBottomHeight,
				margins.cxLeftWidth,
				margins.cyBottomHeight,
				compatibleDC.get(),
				0,
				bmp.bmHeight - margins.cyBottomHeight,
				margins.cxLeftWidth,
				margins.cyBottomHeight,
				SRCCOPY
			);
			// right, bottom
			StretchBlt(
				hdc,
				destinationWidth - margins.cxRightWidth,
				destinationHeight - margins.cyBottomHeight,
				margins.cxRightWidth,
				margins.cyBottomHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth,
				bmp.bmHeight - margins.cyBottomHeight,
				margins.cxRightWidth,
				margins.cyBottomHeight,
				SRCCOPY
			);
			// center, top
			StretchBlt(
				hdc,
				margins.cxLeftWidth,
				0,
				destinationWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				compatibleDC.get(),
				margins.cxLeftWidth,
				0,
				bmp.bmWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				SRCCOPY
			);
			// left, center
			StretchBlt(
				hdc,
				0,
				margins.cyTopHeight,
				margins.cxLeftWidth,
				destinationHeight - margins.cyTopHeight - margins.cyBottomHeight,
				compatibleDC.get(),
				0,
				margins.cyTopHeight,
				margins.cxLeftWidth,
				bmp.bmHeight - margins.cyTopHeight - margins.cyBottomHeight,
				SRCCOPY
			);
			// right, center
			StretchBlt(
				hdc,
				destinationWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				margins.cxRightWidth,
				destinationHeight - margins.cyTopHeight - margins.cyBottomHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				margins.cxRightWidth,
				bmp.bmHeight - margins.cyTopHeight - margins.cyBottomHeight,
				SRCCOPY
			);
			// center, bottom
			StretchBlt(
				hdc,
				margins.cxLeftWidth,
				destinationHeight - margins.cyBottomHeight,
				destinationWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyBottomHeight,
				compatibleDC.get(),
				margins.cxLeftWidth,
				bmp.bmHeight - margins.cyBottomHeight,
				bmp.bmWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyBottomHeight,
				SRCCOPY
			);
			// center, center
			StretchBlt(
				hdc,
				margins.cxLeftWidth,
				margins.cyTopHeight,
				destinationWidth - margins.cxLeftWidth - margins.cxRightWidth,
				destinationHeight - margins.cyTopHeight - margins.cyBottomHeight,
				compatibleDC.get(),
				margins.cxLeftWidth,
				margins.cyTopHeight,
				bmp.bmWidth - margins.cxLeftWidth - margins.cxRightWidth,
				bmp.bmHeight - margins.cyTopHeight - margins.cyBottomHeight,
				SRCCOPY
			);
		}
		else
		{
			BLENDFUNCTION bf
			{
				AC_SRC_OVER,
				0,
				static_cast<BYTE>(opacity),
				AC_SRC_ALPHA
			};
			BP_PAINTPARAMS paintParams
			{
				sizeof(BP_PAINTPARAMS),
				0,
				nullptr,
				&bf
			};
			HDC bufferedDC;
			RECT targetRect
			{
				0,
				0,
				destinationWidth,
				destinationHeight
			};
			const auto paintBuffer = BeginBufferedPaint(
				hdc, 
				&targetRect,
				BPBF_TOPDOWNDIB, 
				&paintParams,
				&bufferedDC
			);
			RETURN_LAST_ERROR_IF_NULL(paintBuffer);

			// left, top
			StretchBlt(
				bufferedDC,
				0,
				0,
				margins.cxLeftWidth,
				margins.cyTopHeight,
				compatibleDC.get(),
				0,
				0,
				margins.cxLeftWidth,
				margins.cyTopHeight,
				SRCCOPY
			);
			// right, top
			StretchBlt(
				bufferedDC,
				destinationWidth - margins.cxRightWidth,
				0,
				margins.cxRightWidth,
				margins.cyTopHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth,
				0,
				margins.cxRightWidth,
				margins.cyTopHeight,
				SRCCOPY
			);
			// left, bottom
			StretchBlt(
				bufferedDC,
				0,
				destinationHeight - margins.cyBottomHeight,
				margins.cxLeftWidth,
				margins.cyBottomHeight,
				compatibleDC.get(),
				0,
				bmp.bmHeight - margins.cyBottomHeight,
				margins.cxLeftWidth,
				margins.cyBottomHeight,
				SRCCOPY
			);
			// right, bottom
			StretchBlt(
				bufferedDC,
				destinationWidth - margins.cxRightWidth,
				destinationHeight - margins.cyBottomHeight,
				margins.cxRightWidth,
				margins.cyBottomHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth,
				bmp.bmHeight - margins.cyBottomHeight,
				margins.cxRightWidth,
				margins.cyBottomHeight,
				SRCCOPY
			);
			// center, top
			StretchBlt(
				bufferedDC,
				margins.cxLeftWidth,
				0,
				destinationWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				compatibleDC.get(),
				margins.cxLeftWidth,
				0,
				bmp.bmWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				SRCCOPY
			);
			// left, center
			StretchBlt(
				bufferedDC,
				0,
				margins.cyTopHeight,
				margins.cxLeftWidth,
				destinationHeight - margins.cyTopHeight - margins.cyBottomHeight,
				compatibleDC.get(),
				0,
				margins.cyTopHeight,
				margins.cxLeftWidth,
				bmp.bmHeight - margins.cyTopHeight - margins.cyBottomHeight,
				SRCCOPY
			);
			// right, center
			StretchBlt(
				bufferedDC,
				destinationWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				margins.cxRightWidth,
				destinationHeight - margins.cyTopHeight - margins.cyBottomHeight,
				compatibleDC.get(),
				bmp.bmWidth - margins.cxRightWidth,
				margins.cyTopHeight,
				margins.cxRightWidth,
				bmp.bmHeight - margins.cyTopHeight - margins.cyBottomHeight,
				SRCCOPY
			);
			// center, bottom
			StretchBlt(
				bufferedDC,
				margins.cxLeftWidth,
				destinationHeight - margins.cyBottomHeight,
				destinationWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyBottomHeight,
				compatibleDC.get(),
				margins.cxLeftWidth,
				bmp.bmHeight - margins.cyBottomHeight,
				bmp.bmWidth - margins.cxLeftWidth - margins.cxRightWidth,
				margins.cyBottomHeight,
				SRCCOPY
			);
			// center, center
			StretchBlt(
				bufferedDC,
				margins.cxLeftWidth,
				margins.cyTopHeight,
				destinationWidth - margins.cxLeftWidth - margins.cxRightWidth,
				destinationHeight - margins.cyTopHeight - margins.cyBottomHeight,
				compatibleDC.get(),
				margins.cxLeftWidth,
				margins.cyTopHeight,
				bmp.bmWidth - margins.cxLeftWidth - margins.cxRightWidth,
				bmp.bmHeight - margins.cyTopHeight - margins.cyBottomHeight,
				SRCCOPY
			);

			RETURN_IF_FAILED(EndBufferedPaint(paintBuffer, TRUE));
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