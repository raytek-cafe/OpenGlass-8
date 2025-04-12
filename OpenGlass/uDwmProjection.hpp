#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "ProjectionHelper.hpp"
#include "DWM.hpp"

namespace OpenGlass::uDWM
{
	using namespace DWM;
	inline const auto g_moduleHandle{ GetModuleHandleW(L"uDWM.dll")};
	inline const auto g_buildNumber{ Util::GetModuleBuildNumber(g_moduleHandle) };

	struct CBaseObject
	{
		size_t AddRef()
		{
			return InterlockedIncrement(reinterpret_cast<DWORD*>(this) + 2);
		}
		size_t Release()
		{
			auto result = InterlockedDecrement(reinterpret_cast<DWORD*>(this) + 2);
			if (!result)
			{
				delete this;
			}
			return result;
		}
		HRESULT QueryInterface(
			[[maybe_unused]] REFIID riid, 
			[[maybe_unused]] PVOID* ppvObject
		)
		{
			return E_NOTIMPL;
		}
		protected: virtual ~CBaseObject() {}
	};

	struct CResourceProxy
	{
		IDwmChannel* GetChannel() const
		{
			return *reinterpret_cast<IDwmChannel* const*>(reinterpret_cast<ULONG_PTR>(this) + 16);
		}
		UINT GetHandleId() const
		{
			return *reinterpret_cast<UINT const*>(reinterpret_cast<ULONG_PTR>(this) + 24);
		}
	};
	struct CResource : CBaseObject 
	{
		CResourceProxy* GetProxy()
		{
			return reinterpret_cast<CResourceProxy**>(this)[2];
		}
	};

	struct CBaseLegacyMilBrushProxy : CResource {};
	struct CBaseGeometryProxy : CResource {};
	struct CBaseTransformProxy : CResource {};
	struct CBaseResourceProxy: CResource {};
	struct CBaseImageProxy : CResource {};

	struct CRectResourceProxy : CBaseResourceProxy {};
	struct CSizeResourceProxy : CBaseResourceProxy {};
	struct CRectangleGeometryProxy : CBaseGeometryProxy {};
	struct CCombinedGeometryProxy : CBaseGeometryProxy {};
	struct CRgnGeometryProxy : CBaseGeometryProxy {};
	struct CSolidColorLegacyMilBrushProxy : CBaseLegacyMilBrushProxy 
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE Update(double opacity, const D2D1_COLOR_F& color)
		{
			return HANDLE_PROJECTION_FUNCTION(CSolidColorLegacyMilBrushProxy::Update, opacity, color);
		}
	};
	struct CCachedVisualImageProxy : CBaseImageProxy {};

	struct VisualCollection;
	struct CVisualProxy : CResource 
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetClip(CBaseGeometryProxy* geometry)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisualProxy::SetClip, geometry);
		}
	};

	struct CVisual : CBaseObject
	{
		char reserved[280];
		inline static PVOID* vftable{ nullptr };

		inline static CVisual* (STDMETHODCALLTYPE CVisual::* s_ctor)();
		inline static void(STDMETHODCALLTYPE CVisual::* s_dtor)();
		FORCEINLINE CVisual() : reserved{}
		{
			std::invoke(s_ctor, this);
		}
		FORCEINLINE ~CVisual()
		{
			std::invoke(s_dtor, this);
		}

		LONG GetWidth() const
		{
			LONG width{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				width = reinterpret_cast<LONG const*>(this)[30];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				width = reinterpret_cast<LONG const*>(this)[32];
			}
			else
			{
				width = reinterpret_cast<LONG const*>(this)[18];
			}

			return width;
		}
		LONG GetHeight() const
		{
			LONG height{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				height = reinterpret_cast<LONG const*>(this)[31];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				height = reinterpret_cast<LONG const*>(this)[33];
			}
			else
			{
				height = reinterpret_cast<LONG const*>(this)[19];
			}

			return height;
		}
		LONG GetX()
		{
			LONG x{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				x = reinterpret_cast<LONG const*>(this)[28];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				x = reinterpret_cast<LONG const*>(this)[30];
			}
			else
			{
				x = reinterpret_cast<LONG const*>(this)[16];
			}

			return x;
		}
		LONG GetY()
		{
			LONG y{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				y = reinterpret_cast<LONG const*>(this)[29];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				y = reinterpret_cast<LONG const*>(this)[31];
			}
			else
			{
				y = reinterpret_cast<LONG const*>(this)[17];
			}

			return y;
		}
		POINT GetOffsetRelativeToRoot()
		{
			POINT pt{ GetX(), GetY() };
			auto parent = GetTransformParent();
			while (parent)
			{
				pt.x += parent->GetX();
				pt.y += parent->GetY();
				parent = parent->GetTransformParent();
			}
			return pt;
		}
		VisualCollection* GetVisualCollection()
		{
			if (g_buildNumber < os::build_w11_24h2)
			{
				return reinterpret_cast<VisualCollection*>(reinterpret_cast<ULONG_PTR>(this) + 32);
			}

			return reinterpret_cast<VisualCollection*>(reinterpret_cast<ULONG_PTR>(this) + 144);
		}
		CVisualProxy* GetVisualProxy() const
		{
			return reinterpret_cast<CVisualProxy* const*>(this)[2];
		}
		CVisual* GetTransformParent() const
		{
			return reinterpret_cast<CVisual* const*>(this)[3];
		}

		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE InitializeFromSharedHandle(HANDLE visualHandle)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::InitializeFromSharedHandle, visualHandle);
		}
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE SetParent(CVisual* parent)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetParent, parent);
		}
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE SetSize(const SIZE* size)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetSize, size);
		}
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE SetOpacity(double opacity)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetOpacity, opacity);
		}
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE SetInsetFromParent(const MARGINS& margins)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetInsetFromParent, margins);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE InitializeVisualTreeClone(
			CVisual* clonedVisual,
			UINT cloneOption
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::InitializeVisualTreeClone, clonedVisual, cloneOption);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE ValidateVisual()
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::ValidateVisual);
		}
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE SetDirtyFlags(int flags)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetDirtyFlags, flags);
		}
	};

	struct VisualCollection : CResource
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE Remove(CVisual* visual)
		{
			return HANDLE_PROJECTION_FUNCTION(VisualCollection::Remove, visual);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE RemoveAll()
		{
			return HANDLE_PROJECTION_FUNCTION(VisualCollection::RemoveAll);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE InsertRelative(
			CVisual* visual,
			CVisual* referenceVisual,
			bool insertAfter,
			bool connectNow
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				VisualCollection::InsertRelative,
				visual,
				referenceVisual,
				insertAfter,
				connectNow
			);
		}
	};

	struct IRenderDataBuilder : IUnknown
	{
		STDMETHOD(DrawBitmap)(UINT bitmapHandleId) PURE;
		STDMETHOD(DrawGeometry)(UINT geometryHandleId, UINT brushHandleId) PURE;
		STDMETHOD(DrawImage)(const D2D1_RECT_F& rect, UINT imageHandleId) PURE;
		STDMETHOD(DrawMesh2D)(UINT meshHandleId, UINT brushHandleId) PURE;
		STDMETHOD(DrawRectangle)(const D2D1_RECT_F* rect, UINT brushHandleId) PURE;
		STDMETHOD(DrawTileImage)(UINT imageHandleId, const D2D1_RECT_F& rect, float opacity, const D2D1_POINT_2F& point) PURE;
		STDMETHOD(DrawVisual)(UINT visualHandleId) PURE;
		STDMETHOD(Pop)() PURE;
		STDMETHOD(PushTransform)(UINT transformHandleId) PURE;
		STDMETHOD(DrawSolidRectangle)(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color) PURE;
	};
	struct CRenderDataInstruction : CResource
	{
		STDMETHOD(WriteInstruction)(
			IRenderDataBuilder* builder,
			const struct CVisual* visual
		) PURE;
	};
	struct CDrawGeometryInstruction : CRenderDataInstruction
	{
		DECLSPEC_PROJECTION static HRESULT STDMETHODCALLTYPE Create(CBaseLegacyMilBrushProxy* brush, CBaseGeometryProxy* geometry, CDrawGeometryInstruction** instruction)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CDrawGeometryInstruction::Create, 
				brush,
				geometry,
				instruction
			);
		}

		CBaseLegacyMilBrushProxy*& GetBrush()
		{
			return reinterpret_cast<CBaseLegacyMilBrushProxy**>(this)[2];
		}
		CBaseGeometryProxy*& GetGeometry()
		{
			return reinterpret_cast<CBaseGeometryProxy**>(this)[3];
		}
	};
	struct CSolidRectangleInstruction : CRenderDataInstruction
	{
		DWORD m_refCount{ 1 };
		DWORD m_unknown{ 0 };
		D2D1_COLOR_F m_color{};
		D2D1_RECT_F m_drawRect{};
	public:
		STDMETHOD(WriteInstruction)(
			IRenderDataBuilder* builder,
			[[maybe_unused]] const CVisual* visual
		) override
		{
			return builder->DrawSolidRectangle(m_drawRect, m_color);
		}
		D2D1_COLOR_F& GetColor() { return m_color; }
		D2D1_RECT_F& GetRectangle() { return m_drawRect; }
	};

	class CDrawVisualTreeInstruction : public CRenderDataInstruction
	{
		DWORD m_refCount{ 1 };
		winrt::com_ptr<CVisual> m_visual{ nullptr };
	public:
		CDrawVisualTreeInstruction(CVisual* visual) : CRenderDataInstruction{} { m_visual.copy_from(visual); }
		STDMETHOD(WriteInstruction)(
			IRenderDataBuilder* builder,
			[[maybe_unused]] const CVisual* visual
		) override
		{
			UINT visualHandleId{ 0 };
			if (visual)
			{
				visualHandleId = visual->GetVisualProxy()->GetProxy()->GetHandleId();
			}

			return builder->DrawVisual(visualHandleId);
		}
	};
	struct CRenderDataVisual : CVisual
	{
		DynArray<CRenderDataInstruction*>& GetInstructions() const
		{
			DynArray<CRenderDataInstruction*>* instructionArray{};

			if (g_buildNumber < os::build_w11_21h2)
			{
				instructionArray = reinterpret_cast<DynArray<CRenderDataInstruction*>*>(reinterpret_cast<ULONG_PTR>(this) + 248);
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				instructionArray = reinterpret_cast<DynArray<CRenderDataInstruction*>*>(reinterpret_cast<ULONG_PTR>(this) + 256);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				instructionArray = reinterpret_cast<DynArray<CRenderDataInstruction*>*>(reinterpret_cast<ULONG_PTR>(this) + 256);
			}
			else
			{
				instructionArray = reinterpret_cast<DynArray<CRenderDataInstruction*>*>(reinterpret_cast<ULONG_PTR>(this) + 208);
			}

			return *instructionArray;
		}
		DWORD GetCount() const
		{
			return GetInstructions().size;
		}
		FORCEINLINE bool IsEmpty() const { return GetCount() == 0; }
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE AddInstruction(CRenderDataInstruction* instruction)
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::AddInstruction, instruction);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE ClearInstructions()
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::ClearInstructions);
		}
		DECLSPEC_PROJECTION static HRESULT STDMETHODCALLTYPE Create(CRenderDataVisual** visual)
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::Create, visual);
		}
	};
	struct CCanvasVisual : CRenderDataVisual {};
	struct CText : CRenderDataVisual 
	{
		bool IsRTL() const
		{
			bool rtl{ false };

			if (g_buildNumber < os::build_w11_21h2)
			{
				rtl = (reinterpret_cast<BYTE const*>(this)[280] & 2) != 0;
			}
			else
			{
				rtl = (reinterpret_cast<BYTE const*>(this)[288] & 2) != 0;
			}

			return rtl;
		}
	};

	struct CButton : CVisual
	{
		inline static PVOID* vftable{ nullptr };
	};

	struct ACCENT_POLICY
	{
		DWORD AccentState;
		DWORD AccentFlags;
		DWORD dwGradientColor;
		DWORD dwAnimationId;

		bool IsActive() const
		{
			return AccentState >= 1 && AccentState <= 4;
		}
		bool IsAccentBlurRectEnabled() const
		{
			return (AccentFlags & (1 << 9)) != 0;
		}
		bool IsGdiRegionRespected() const
		{
			return (AccentFlags & (1 << 4)) != 0;
		}
	};
	struct CAccent : CRenderDataVisual
	{
		bool IsVisibleAndUncloaked() const
		{
			if (g_buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<bool const*>(this)[397];
			}

			return reinterpret_cast<bool const*>(this)[405];
		}
		ACCENT_POLICY* GetPolicy()
		{
			return reinterpret_cast<ACCENT_POLICY*>(this) + 35;
		}
		HWND GetHwnd() const
		{
			if (g_buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<HWND const*>(this)[50];
			}

			return reinterpret_cast<HWND const*>(this)[51];
		}
	};
	struct CAccentBlurBehind : CRenderDataVisual {};
	struct CAccentAcrylicBlurBehind : CRenderDataVisual {};
	struct CWindowBorder : CVisual
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE EnableBorder(bool enable)
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowBorder::EnableBorder, enable);
		}
		CVisual* GetContainerVisual() const
		{
			CVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CVisual* const*>(this)[31];
			}
			else
			{
				visual = reinterpret_cast<CVisual* const*>(this)[25];
			}

			return visual;
		}
		CVisual* GetContentVisual() const
		{
			CVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CVisual* const*>(this)[32];
			}
			else
			{
				visual = reinterpret_cast<CVisual* const*>(this)[26];
			}

			return visual;
		}
	};

	struct IDwmWindow;
	struct CTopLevelWindow;
	struct CWindowData : CBaseObject
	{
		HWND GetHwnd() const
		{
			return reinterpret_cast<const HWND*>(this)[5];
		}
		IDwmWindow* GetWindowContext() const
		{
			return reinterpret_cast<IDwmWindow* const*>(this)[3];
		}
		CTopLevelWindow* GetWindow() const
		{
			CTopLevelWindow* window{ nullptr };

			if (g_buildNumber < os::build_w10_1903)
			{
				window = reinterpret_cast<CTopLevelWindow* const*>(this)[49];
			}
			else if (g_buildNumber < os::build_w10_2004)
			{
				window = reinterpret_cast<CTopLevelWindow* const*>(this)[50];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				window = reinterpret_cast<CTopLevelWindow* const*>(this)[48];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				window = reinterpret_cast<CTopLevelWindow* const*>(this)[55];
			}
			else
			{
				window = reinterpret_cast<CTopLevelWindow* const*>(this)[55];
			}

			return window;
		}

		ACCENT_POLICY* GetAccentPolicy() const
		{
			ACCENT_POLICY* accentPolicy{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				accentPolicy = reinterpret_cast<ACCENT_POLICY*>(reinterpret_cast<ULONG_PTR>(this) + 152);
			}
			else
			{
				accentPolicy = reinterpret_cast<ACCENT_POLICY*>(reinterpret_cast<ULONG_PTR>(this) + 168);
			}

			return accentPolicy;
		}

		DWM_SYSTEMBACKDROP_TYPE& GetSystemBackdropType()
		{
			DWM_SYSTEMBACKDROP_TYPE* systemBackdrop{ nullptr };

			systemBackdrop = &reinterpret_cast<DWM_SYSTEMBACKDROP_TYPE*>(this)[51];

			return *systemBackdrop;
		}

		DWORD& GetCaptionColorOverride()
		{
			DWORD* captionColor{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				captionColor = &reinterpret_cast<DWORD*>(this)[47];
			}
			else
			{
				captionColor = &reinterpret_cast<DWORD*>(this)[47];
			}

			return *captionColor;
		}
		DWORD& GetBorderColorOverride()
		{
			DWORD* captionColor{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				captionColor = &reinterpret_cast<DWORD*>(this)[48];
			}
			else
			{
				captionColor = &reinterpret_cast<DWORD*>(this)[48];
			}

			return *captionColor;
		}
		DWORD& GetTextColorOverride()
		{
			DWORD* captionColor{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				captionColor = &reinterpret_cast<DWORD*>(this)[49];
			}
			else
			{
				captionColor = &reinterpret_cast<DWORD*>(this)[49];
			}

			return *captionColor;
		}

		BYTE GetNonClientAttribute() const
		{
			BYTE attribute{ 0 };

			if (g_buildNumber < os::build_w10_1903)
			{
				attribute = reinterpret_cast<BYTE const*>(this)[596];
			}
			else if (g_buildNumber < os::build_w10_2004)
			{
				attribute = reinterpret_cast<BYTE const*>(this)[604];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				attribute = reinterpret_cast<BYTE const*>(this)[608];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				attribute = reinterpret_cast<BYTE const*>(this)[664];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				attribute = reinterpret_cast<BYTE const*>(this)[672];
			}
			else
			{
				attribute = reinterpret_cast<BYTE const*>(this)[672];
			}

			return attribute;
		}
		MARGINS& GetExtendedFrameMargins()
		{
			MARGINS* margins{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + 80);
			}
			else
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + 96);
			}

			return *margins;
		}
		bool IsFrameExtendedIntoClientAreaLRB()
		{
			MARGINS& margins = GetExtendedFrameMargins();

			return
			margins.cxLeftWidth ||
			margins.cxRightWidth ||
			margins.cyBottomHeight;
		}
		bool IsSheetOfGlass()
		{
			MARGINS& margins = GetExtendedFrameMargins();

			return 
			margins.cxLeftWidth == 0x7FFFFFFF ||
			margins.cxRightWidth == 0x7FFFFFFF ||
			margins.cyTopHeight == 0x7FFFFFFF ||
			margins.cyBottomHeight == 0x7FFFFFFF;
		}
	};

	union GpCC
	{
		struct
		#pragma warning(suppress : 4201)
		{
			BYTE b;
			BYTE g;
			BYTE r;
			BYTE a;
		};
		UINT32 argb;
	};
	struct CGlassColorizationParameters
	{
		UINT32 color;
		UINT32 afterglow;
		UINT32 colorBalance;
		UINT32 afterglowBalance;
		UINT32 blurBalance;
		BOOL windowColorization;
		UINT32 glassAttribute;
		UINT32 reserved;
	};
	struct CGlassColorizationResources
	{
		float* GetBalance()
		{
			return &(reinterpret_cast<float*>(this)[8]);
		}
		D2D1_COLOR_F* GetColor()
		{
			return &(reinterpret_cast<D2D1_COLOR_F*>(this)[1]);
		}
		D2D1_COLOR_F getArgbcolor() const
		{
			const auto balance = reinterpret_cast<float const*>(this)[8];
			return D2D1::ColorF(
				reinterpret_cast<float const*>(this)[4] * balance,
				reinterpret_cast<float const*>(this)[5] * balance,
				reinterpret_cast<float const*>(this)[6] * balance,
				reinterpret_cast<float const*>(this)[7]
			);
		}
	};
	struct CTopLevelWindow : CVisual
	{
		inline static PVOID** s_rgpwfWindowFrames{ nullptr };
		static auto GetWindowFrames()
		{
			return *s_rgpwfWindowFrames;
		}

		bool IsTrullyMinimized()
		{
			RECT borderRect{};
			THROW_HR_IF_NULL(E_INVALIDARG, GetActualWindowRect(&borderRect, false, true, false));

			return borderRect.left <= -32000 || borderRect.top <= -32000;
		}

		CWindowData* GetData() const
		{
			CWindowData* data{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				data = reinterpret_cast<CWindowData* const*>(this)[90];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				data = reinterpret_cast<CWindowData* const*>(this)[91];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				data = reinterpret_cast<CWindowData* const*>(this)[94];
			}
			else
			{
				data = reinterpret_cast<CWindowData* const*>(this)[89];
			}

			return data;
		}
		CText* GetTextVisual() const
		{
			CText* visual{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				visual = reinterpret_cast<CText* const*>(this)[64];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CText* const*>(this)[65];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CText* const*>(this)[67];
			}

			return visual;
		}
		CCanvasVisual* GetNonClientVisual() const
		{
			CCanvasVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[32];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[33];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[34];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[36];
			}
			else
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[31];
			}

			return visual;
		}
		CAccent* GetAccent() const
		{
			CAccent* accent{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				accent = reinterpret_cast<CAccent* const*>(this)[33];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				accent = reinterpret_cast<CAccent* const*>(this)[34];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				accent = reinterpret_cast<CAccent* const*>(this)[35];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				accent = reinterpret_cast<CAccent* const*>(this)[37];
			}
			else
			{
				accent = reinterpret_cast<CAccent* const*>(this)[32];
			}

			return accent;
		}
		CCanvasVisual* GetLegacyVisual() const
		{
			CCanvasVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[35];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[36];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[37];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[39];
			}
			else
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[34];
			}

			return visual;
		}
		CCanvasVisual* GetClientBlurVisual() const
		{
			CCanvasVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[36];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[37];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[39];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[42];
			}
			else
			{
				visual = reinterpret_cast<CCanvasVisual* const*>(this)[37];
			}

			return visual;
		}
		CWindowBorder* GetWindowBorder() const
		{
			CWindowBorder* windowBorder{ nullptr };

			if (g_buildNumber < os::build_w11_22h2)
			{
				windowBorder = reinterpret_cast<CWindowBorder* const*>(this)[33];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				windowBorder = reinterpret_cast<CWindowBorder* const*>(this)[34];
			}
			else
			{
				windowBorder = reinterpret_cast<CWindowBorder* const*>(this)[28];
			}

			return windowBorder;
		}


		CRgnGeometryProxy* GetBorderGeometry() const
		{
			CRgnGeometryProxy* geometry{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				geometry = reinterpret_cast<CRgnGeometryProxy* const*>(this)[68];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				geometry = reinterpret_cast<CRgnGeometryProxy* const*>(this)[69];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				geometry = reinterpret_cast<CRgnGeometryProxy* const*>(this)[71];
			}
			else
			{
				auto legacyVisual = GetLegacyVisual();
				if (legacyVisual)
				{
					if (g_buildNumber < os::build_w11_24h2)
					{
						geometry = reinterpret_cast<CRgnGeometryProxy* const*>(legacyVisual)[40];
					}
					else
					{
						geometry = reinterpret_cast<CRgnGeometryProxy* const*>(legacyVisual)[34];
					}
				}
			}

			return geometry;
		}
		CRgnGeometryProxy* GetCaptionGeometry() const
		{
			CRgnGeometryProxy* geometry{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				geometry = reinterpret_cast<CRgnGeometryProxy* const*>(this)[69];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				geometry = reinterpret_cast<CRgnGeometryProxy* const*>(this)[70];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				geometry = reinterpret_cast<CRgnGeometryProxy* const*>(this)[72];
			}
			else
			{
				auto legacyVisual = GetLegacyVisual();
				if (legacyVisual)
				{
					if (g_buildNumber < os::build_w11_24h2)
					{
						geometry = reinterpret_cast<CRgnGeometryProxy* const*>(legacyVisual)[39];
					}
					else
					{
						geometry = reinterpret_cast<CRgnGeometryProxy* const*>(legacyVisual)[33];
					}
				}
			}

			return geometry;
		}
		CSolidColorLegacyMilBrushProxy* GetCaptionBrush() const
		{
			CSolidColorLegacyMilBrushProxy* brush{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[95];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[99];
			}
			else
			{
				auto legacyVisual = GetLegacyVisual();
				if (legacyVisual)
				{
					if (g_buildNumber < os::build_w11_24h2)
					{
						brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(legacyVisual)[37];
					}
					else
					{
						brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(legacyVisual)[31];
					}
				}
			}

			return brush;
		}
		CSolidColorLegacyMilBrushProxy* GetBorderBrush() const
		{
			CSolidColorLegacyMilBrushProxy* brush{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[94];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[98];
			}
			else
			{
				auto legacyVisual = GetLegacyVisual();
				if (legacyVisual)
				{
					if (g_buildNumber < os::build_w11_24h2)
					{
						brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(legacyVisual)[38];
					}
					else
					{
						brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(legacyVisual)[30];
					}
				}
			}

			return brush;
		}
		CSolidColorLegacyMilBrushProxy* GetClientBlurBrush() const
		{
			CSolidColorLegacyMilBrushProxy* brush{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[96];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[100];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[98];
			}
			else
			{
				brush = reinterpret_cast<CSolidColorLegacyMilBrushProxy* const*>(this)[93];
			}

			return brush;
		}

		bool& GetIsBorderUpdatesSuppressed()
		{
			bool* suppressed{ nullptr };

			if (g_buildNumber < os::build_w11_22h2)
			{
				suppressed = &reinterpret_cast<bool*>(this)[888];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				suppressed = &reinterpret_cast<bool*>(this)[864];
			}
			else
			{
				suppressed = &reinterpret_cast<bool*>(this)[832];
			}

			return *suppressed;
		}

		bool IsRTLMirrored() const
		{
			bool rtlMirrored{ false };
			
			if (g_buildNumber < os::build_w10_2004)
			{
				rtlMirrored = (reinterpret_cast<DWORD const*>(this)[146] & 0x20000) != 0;
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				rtlMirrored = (reinterpret_cast<DWORD const*>(this)[148] & 0x20000) != 0;
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				rtlMirrored = (reinterpret_cast<DWORD const*>(this)[152] & 0x20000) != 0;
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				rtlMirrored = (reinterpret_cast<DWORD const*>(this)[156] & 0x20000) != 0;
			}
			else
			{
				rtlMirrored = (reinterpret_cast<DWORD const*>(this)[146] & 0x20000) != 0;
			}

			return rtlMirrored;
		}
		bool HasNonClientArea() const
		{
			bool hasNonClientArea{ false };

			if (g_buildNumber < os::build_w10_2004)
			{
				hasNonClientArea = reinterpret_cast<DWORD const*>(this)[151] ||
					reinterpret_cast<DWORD const*>(this)[152] ||
					reinterpret_cast<DWORD const*>(this)[153] ||
					reinterpret_cast<DWORD const*>(this)[154];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				hasNonClientArea = reinterpret_cast<DWORD const*>(this)[153] ||
					reinterpret_cast<DWORD const*>(this)[154] ||
					reinterpret_cast<DWORD const*>(this)[155] ||
					reinterpret_cast<DWORD const*>(this)[156];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				hasNonClientArea = reinterpret_cast<DWORD const*>(this)[157] ||
					reinterpret_cast<DWORD const*>(this)[158] ||
					reinterpret_cast<DWORD const*>(this)[159] ||
					reinterpret_cast<DWORD const*>(this)[160];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				hasNonClientArea = reinterpret_cast<DWORD const*>(this)[161] ||
					reinterpret_cast<DWORD const*>(this)[162] ||
					reinterpret_cast<DWORD const*>(this)[163] ||
					reinterpret_cast<DWORD const*>(this)[164];
			}
			else
			{
				hasNonClientArea = reinterpret_cast<DWORD const*>(this)[151] ||
					reinterpret_cast<DWORD const*>(this)[152] ||
					reinterpret_cast<DWORD const*>(this)[153] ||
					reinterpret_cast<DWORD const*>(this)[154];
			}

			return hasNonClientArea;
		}
		bool HasNonClientBackground(CWindowData* data = nullptr) const
		{
			if (!data)
			{
				data = GetData();
			}
			if ((data->GetNonClientAttribute() & 8) == 0)
			{
				return false;
			}

			if (!HasNonClientArea())
			{
				return false;
			}

			return true;
		}

		CGlassColorizationResources* GetCaptionColorizationParameters() const
		{
			CGlassColorizationResources* parameters{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				parameters = reinterpret_cast<CGlassColorizationResources* const*>(this)[72];
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				parameters = reinterpret_cast<CGlassColorizationResources* const*>(this)[73];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				parameters = reinterpret_cast<CGlassColorizationResources* const*>(this)[75];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				parameters = reinterpret_cast<CGlassColorizationResources* const*>(this)[77];
			}
			else
			{
				parameters = reinterpret_cast<CGlassColorizationResources* const*>(this)[71];
			}

			return parameters;
		}

		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CloneVisualTreeForLivePreview_Win10(
			bool windowFramesOnly,
			bool unused1,
			bool unused2,
			CTopLevelWindow** clonedWindow
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CTopLevelWindow::CloneVisualTreeForLivePreview_Win10,
				windowFramesOnly,
				unused1,
				unused2,
				clonedWindow
			);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CloneVisualTreeForLivePreview_Win11(
			bool windowFramesOnly,
			CTopLevelWindow** clonedWindow
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CTopLevelWindow::CloneVisualTreeForLivePreview_Win11,
				windowFramesOnly,
				clonedWindow
			);
		}
		HRESULT STDMETHODCALLTYPE CloneVisualTreeForLivePreview(
			bool windowFramesOnly,
			CTopLevelWindow** clonedWindow
		)
		{
			if (g_buildNumber < os::build_w11_22h2) [[likely]]
			{
				return CloneVisualTreeForLivePreview_Win10(
					windowFramesOnly,
					false,
					false,
					clonedWindow
				);
			}
			else
			{
				return CloneVisualTreeForLivePreview_Win11(
					windowFramesOnly,
					clonedWindow
				);
			}
		}
		DECLSPEC_PROJECTION bool STDMETHODCALLTYPE TreatAsActiveWindow()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::TreatAsActiveWindow);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE OnClipUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnClipUpdated);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE OnBlurBehindUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnBlurBehindUpdated);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE OnAccentPolicyUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnAccentPolicyUpdated);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE OnSystemBackdropUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnSystemBackdropUpdated);
		}
		DECLSPEC_PROJECTION RECT* STDMETHODCALLTYPE GetActualWindowRect(
			RECT* rect,
			char eraseOffset,
			char includeNonClient,
			bool excludeBorderMargins
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(
				CTopLevelWindow::GetActualWindowRect,
				rect,
				eraseOffset,
				includeNonClient,
				excludeBorderMargins
			);
		}
	};

	struct LivePreviewVisual
	{
		CWindowData* data;
		CTopLevelWindow* livePreview;
		CTopLevelWindow* windowFrames;
		bool unknown0;
		bool unknown1;
		bool unknown2;
		bool unknown3;
		DWORD unkown4;
		ULONG_PTR unknown5;
	};
	struct LivePreviewResource
	{
		CHAR reserved[136];

		const RECT* GetWindowBoundingRect() const
		{
			return reinterpret_cast<const RECT*>(this);
		}
		CRectangleGeometryProxy* GetWindowBoundingGeometry() const
		{
			return reinterpret_cast<CRectangleGeometryProxy* const*>(this)[2];
		}
		CBaseLegacyMilBrushProxy* GetWindowVisualBrush() const
		{
			return reinterpret_cast<CBaseLegacyMilBrushProxy* const*>(this)[4];
		}

		const RECT* GetGlassBoundingRect() const
		{
			return reinterpret_cast<const RECT*>(reinterpret_cast<ULONG_PTR>(this) + 40);
		}
		CRectangleGeometryProxy* GetGlassBoundingGeometry() const
		{
			return reinterpret_cast<CRectangleGeometryProxy* const*>(this)[7];
		}
		CBaseLegacyMilBrushProxy* GetGlassVisualBrush() const
		{
			return reinterpret_cast<CBaseLegacyMilBrushProxy* const*>(this)[9];
		}

		HRGN GetReflectionRegion() const
		{
			return reinterpret_cast<HRGN const*>(this)[12];
		}
		CRgnGeometryProxy* GetReflectionGeometry() const
		{
			return reinterpret_cast<CRgnGeometryProxy* const*>(this)[13];
		}

		const RECT* GetMonitorRect() const
		{
			return reinterpret_cast<const RECT*>(this) + 7;
		}

		bool IsWindowBoundingRectNotEmpty() const
		{
			return reinterpret_cast<bool* const*>(this)[128];
		}
		bool IsGlassBoundingRectNotEmpty() const
		{
			return reinterpret_cast<bool* const*>(this)[129];
		}
	};
	struct CLivePreview : CRenderDataVisual
	{
		CRenderDataVisual* GetThumbnailVisual() const
		{
			CRenderDataVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[62];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[63];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[59];
			}
			else
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[53];
			}
			
			return visual;
		}
		CRenderDataVisual* GetGlassVisual() const
		{
			CRenderDataVisual* visual{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[64];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[65];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[61];
			}
			else
			{
				visual = reinterpret_cast<CRenderDataVisual* const*>(this)[55];
			}

			return visual;
		}
		DynArray<LivePreviewResource>* GetLivePreviewResourceArray() const
		{
			DynArray<LivePreviewResource>* array{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				array = reinterpret_cast<DynArray<LivePreviewResource>*>(reinterpret_cast<ULONG_PTR>(this) + 368);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				array = reinterpret_cast<DynArray<LivePreviewResource>*>(reinterpret_cast<ULONG_PTR>(this) + 376);
			}
			else
			{
				array = reinterpret_cast<DynArray<LivePreviewResource>*>(reinterpret_cast<ULONG_PTR>(this) + 352);
			}

			return array;
		}
		DynArray<LivePreviewVisual>* GetLivePreviewVisualArray() const
		{
			DynArray<LivePreviewVisual>* array{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				array = reinterpret_cast<DynArray<LivePreviewVisual>*>(reinterpret_cast<ULONG_PTR>(this) + 304);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				array = reinterpret_cast<DynArray<LivePreviewVisual>*>(reinterpret_cast<ULONG_PTR>(this) + 312);
			}
			else
			{
				array = reinterpret_cast<DynArray<LivePreviewVisual>*>(reinterpret_cast<ULONG_PTR>(this) + 264);
			}

			return array;
		}

		static bool IsWindowIncluded(const CWindowData* data)
		{
			BYTE attribute{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				attribute = reinterpret_cast<BYTE const*>(data)[611];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				attribute = reinterpret_cast<BYTE const*>(data)[675];
			}
			else
			{
				attribute = reinterpret_cast<BYTE const*>(data)[675];
			}

			return (attribute & 1) == 0;
		}
		static bool IsWindowCloneAsWindowFrames(const CWindowData* data)
		{
			DWORD attribute{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				attribute = reinterpret_cast<DWORD const*>(data)[28];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				attribute = reinterpret_cast<DWORD const*>(data)[33];
			}
			else
			{
				attribute = reinterpret_cast<DWORD const*>(data)[32];
			}

			return attribute == 1;
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE _UpdateResources()
		{
			return HANDLE_PROJECTION_FUNCTION(CLivePreview::_UpdateResources);
		}
		DECLSPEC_PROJECTION bool STDMETHODCALLTYPE _IsTrulyMaximized(CWindowData* data)
		{
			return HANDLE_PROJECTION_FUNCTION(CLivePreview::_IsTrulyMaximized, data);
		}
	};
	struct CAnimatedGlassSheet : CVisual 
	{
		const RECT& GetSourceRect() const
		{
			return reinterpret_cast<RECT const*>(this)[24];
		}
		const RECT& GetDestinationRect() const
		{
			return reinterpret_cast<RECT const*>(this)[25];
		}
		const RECT& GetAdjustedDestinationRect()
		{
			return reinterpret_cast<RECT const*>(this)[26];
		}

		LONG GetAtlasPaddingTop() const
		{
			return reinterpret_cast<RECT const*>(this)[30].left;
		}
		LONG GetAtlasPaddingLeft() const
		{
			return reinterpret_cast<RECT const*>(this)[29].right;
		}
		LONG GetAtlasPaddingRight() const
		{
			return -reinterpret_cast<RECT const*>(this)[29].bottom;
		}
		LONG GetAtlasPaddingBottom() const
		{
			return -reinterpret_cast<RECT const*>(this)[30].top;
		}
	};
	struct CWindowList : CBaseObject
	{
		DECLSPEC_PROJECTION PRLIST_ENTRY STDMETHODCALLTYPE GetWindowListForDesktop(ULONG_PTR desktopID)
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowList::GetWindowListForDesktop, desktopID);
		}
		DECLSPEC_PROJECTION CWindowData* STDMETHODCALLTYPE FindWindowDataByHwnd(HWND hwnd)
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowList::FindWindowDataByHwnd, hwnd);
		}
	};

	struct CCompositor
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CreateSolidColorLegacyMilBrushProxy(CSolidColorLegacyMilBrushProxy** milBrushProxy)
		{
			return HANDLE_PROJECTION_FUNCTION(CCompositor::CreateSolidColorLegacyMilBrushProxy, milBrushProxy);
		}
		IDwmChannel* GetChannel() const
		{
			IDwmChannel* channel{ nullptr };

			if (g_buildNumber < os::build_w10_1903)
			{
				channel = reinterpret_cast<IDwmChannel*>(const_cast<CCompositor*>(this));
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				channel = reinterpret_cast<IDwmChannel* const*>(this)[2];
			}
			else
			{
				channel = reinterpret_cast<IDwmChannel* const*>(this)[3];
			}

			return channel;
		}
	};
	struct CDesktopManager
	{
		inline static CDesktopManager** s_pDesktopManagerInstance{ nullptr };
		inline static LPCRITICAL_SECTION s_csDwmInstance{ nullptr };

		static CDesktopManager* GetInstance()
		{
			return *s_pDesktopManagerInstance;
		}
		CLivePreview* GetLivePreview() const
		{
			if (g_buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<CLivePreview* const*>(this)[64];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				return reinterpret_cast<CLivePreview* const*>(this)[55];
			}

			return reinterpret_cast<CLivePreview* const*>(this)[57];
		}
		CCompositor* GetCompositor() const
		{
			CCompositor* compositor{ nullptr };

			if (g_buildNumber < os::build_w11_22h2)
			{
				compositor = reinterpret_cast<CCompositor* const*>(this)[5];
			}
			else
			{
				compositor = reinterpret_cast<CCompositor* const*>(this)[6];
			}

			return compositor;
		}
		HMIL_CONNECTION GetConnection() const
		{
			HMIL_CONNECTION connection{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				connection = reinterpret_cast<HMIL_CONNECTION const*>(this)[6];
			}
			else
			{
				connection = nullptr;
			}

			return connection;
		}
		CWindowList* GetWindowList() const
		{
			CWindowList* windowList{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				windowList = reinterpret_cast<CWindowList* const*>(this)[61];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				windowList = reinterpret_cast<CWindowList* const*>(this)[52];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				windowList = reinterpret_cast<CWindowList* const*>(this)[54];
			}
			else
			{
				windowList = reinterpret_cast<CWindowList* const*>(this)[53];
			}

			return windowList;
		}
		IWICImagingFactory2* GetWICFactory() const
		{
			IWICImagingFactory2* factory{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				factory = reinterpret_cast<IWICImagingFactory2* const*>(this)[39];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				factory = reinterpret_cast<IWICImagingFactory2* const*>(this)[30];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				factory = reinterpret_cast<IWICImagingFactory2* const*>(this)[31];
			}
			else
			{
				factory = reinterpret_cast<IWICImagingFactory2* const*>(this)[30];
			}

			return factory;
		}
		ID2D1Device* GetD2DDevice() const
		{
			ID2D1Device* d2dDevice{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				d2dDevice = reinterpret_cast<ID2D1Device* const*>(this)[29];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				d2dDevice = reinterpret_cast<ID2D1Device**>(reinterpret_cast<void* const*>(this)[6])[3];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				d2dDevice = reinterpret_cast<ID2D1Device**>(reinterpret_cast<void* const*>(this)[7])[3];
			}
			else
			{
				d2dDevice = reinterpret_cast<ID2D1Device**>(reinterpret_cast<void* const*>(this)[7])[4];
			}

			return d2dDevice;
		}
		IDCompositionDesktopDevice* GetDCompDevice() const
		{
			IDCompositionDesktopDevice* interopDevice{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				interopDevice = reinterpret_cast<IDCompositionDesktopDevice* const*>(this)[27];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				interopDevice = reinterpret_cast<IDCompositionDesktopDevice**>(reinterpret_cast<void* const*>(this)[5])[4];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				interopDevice = reinterpret_cast<IDCompositionDesktopDevice**>(reinterpret_cast<void* const*>(this)[6])[4];
			}
			else
			{
				interopDevice = reinterpret_cast<IDCompositionDesktopDevice**>(reinterpret_cast<void* const*>(this)[6])[4];
			}

			return interopDevice;
		}
		bool& GetIsHighContrastMode()
		{
			bool* highContrast{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				highContrast = &reinterpret_cast<bool*>(this)[26];
			}
			else
			{
				highContrast = &reinterpret_cast<bool*>(this)[27];
			}

			return *highContrast;
		}
		double GetDPIValue() const
		{
			double value{ 1.0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				value = reinterpret_cast<double const*>(this)[60];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				value = reinterpret_cast<double const*>(this)[51];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				value = reinterpret_cast<double const*>(this)[53];
			}
			else
			{
				value = reinterpret_cast<double const*>(this)[52];
			}

			return value;
		}
	};

	namespace ResourceHelper
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CreateGeometryFromHRGN(
			HRGN hrgn,
			CRgnGeometryProxy** geometry
		)
		{
			return HANDLE_PROJECTION_FUNCTION(ResourceHelper::CreateGeometryFromHRGN, hrgn, geometry);
		}
	}

	inline auto g_projectionArray = make_projection_array(
		g_buildNumber,

		MAKE_FUNCTION_PROJECTION_TUPLE(CSolidColorLegacyMilBrushProxy::Update, 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CVisualProxy::SetClip, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CVisual::vftable, "CVisual::`vftable'", 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CVisual::s_ctor, "CVisual::CVisual", 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CVisual::s_dtor, "CVisual::~CVisual", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CVisual::Initialize", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CVisual::CloneVisualTree", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::InitializeFromSharedHandle, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetParent, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetSize, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetOpacity, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetInsetFromParent, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::InitializeVisualTreeClone, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::ValidateVisual, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetDirtyFlags, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::Remove, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::RemoveAll, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::InsertRelative, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CText::ValidateResources", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CText::SetSize", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CText::InitializeVisualTreeClone", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CText::`scalar deleting destructor'", 0, os::build_w11_22h2),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CButton::vftable, "CButton::`vftable'", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawGeometryInstruction::Create, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::Create, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::AddInstruction, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::ClearInstructions, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::_UpdateAccentBlurBehind", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CAccentBlurBehind::IsBlurBehindDirty", 0, os::build_w11_22h2),

		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowBorder::EnableBorder, os::build_w11_21h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::CloneVisualTreeForLivePreview_Win10, "CTopLevelWindow::CloneVisualTreeForLivePreview", os::build_w11_21h2, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::CloneVisualTreeForLivePreview_Win11, "CTopLevelWindow::CloneVisualTreeForLivePreview", os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::GetActualWindowRect, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::TreatAsActiveWindow, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnClipUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnBlurBehindUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnAccentPolicyUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnSystemBackdropUpdated, os::build_w11_21h2, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CTopLevelWindow::s_rgpwfWindowFrames, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateNCAreaBackground", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateClientBlur", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::ValidateVisual", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateWindowVisuals", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::IsShadowNCAreaPart", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::CreateBitmapFromAtlas", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::CreateGlyphsFromAtlas", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("SetMargin", os::build_w11_21h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CLivePreview::_IsTrulyMaximized, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CLivePreview::_UpdateResources, os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_FadeOutToGlass", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_UpdateResourcesForMonitor", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_UpdateInstructions", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CCachedVisualImageProxy::Update", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAnimatedGlassSheet::OnRectUpdated", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CAnimatedGlassSheet::~CAnimatedGlassSheet", 0, os::build_w11_21h2),

		MAKE_EMPTY_PROJECTION_TUPLE("CGlassColorizationParameters::AdjustWindowColorization", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowList::GetWindowListForDesktop, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowList::FindWindowDataByHwnd, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CDesktopManager::IsHighContrastMode", os::build_w11_21h2, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CDesktopManager::s_pDesktopManagerInstance, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CDesktopManager::s_csDwmInstance, 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CCompositor::CreateSolidColorLegacyMilBrushProxy, "CCompositor::CreateProxy<CSolidColorLegacyMilBrushProxy>", 0, 0),
		
		MAKE_FUNCTION_PROJECTION_TUPLE(ResourceHelper::CreateGeometryFromHRGN, 0, 0)
	);
	
	inline bool ParserCallback(PSYMBOL_INFO info, [[maybe_unused]] ULONG size)
	{
		CHAR symbolName[128]{};
		UnDecorateSymbolName(info->Name, symbolName, std::size(symbolName), UNDNAME_NAME_ONLY);

		if (
			!strcmp(symbolName, "CVisual::SetSize") &&
			!strstr(info->Name, "tagSIZE@@@Z")
		)
		{
			return true;
		}

		g_projectionArray.Apply(
			symbolName,
			HookHelper::OffsetStorage::From(
				info->ModBase,
				info->Address
			).To(g_moduleHandle)
		);

		return !g_projectionArray.IsAllReady();
	}
}