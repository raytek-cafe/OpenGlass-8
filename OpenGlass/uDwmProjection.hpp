#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "ProjectionHelper.hpp"
#include "DWM.hpp"
#include "uDwmProjection.Offsets.hpp"
#include "DCompPrivates.hpp"

namespace OpenGlass::uDWM
{
	using namespace DWM;
	inline const auto g_moduleHandle{ GetModuleHandleW(L"uDWM.dll") };
	inline const auto g_versionInfo{ Util::GetModuleVersionInfo(g_moduleHandle) };

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
		CResourceProxy* GetProxy() const
		{
			if (g_versionInfo.build < os::build_w10_1903)
			{
				return reinterpret_cast<CResourceProxy*>(const_cast<CResource*>(this));
			}
			return reinterpret_cast<CResourceProxy* const*>(this)[2];
		}
		DECLSPEC_PROJECTION static HRESULT Create(
			DwmResourceType type,
			IDwmChannel* channel,
			CResource** resource
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CResource::Create,
				type,
				channel,
				resource
			);
		}
	};

	struct CBaseLegacyMilBrushProxy : CResource {};
	struct CBaseGeometryProxy : CResource {};
	struct CBaseTransformProxy : CResource {};
	struct CBaseResourceProxy: CResource {};
	struct CBaseImageProxy : CResource {};

	struct CRectResourceProxy : CBaseResourceProxy {};
	struct CSizeResourceProxy : CBaseResourceProxy {};
	struct CDoubleResourceProxy : CBaseResourceProxy {};
	struct CRectangleGeometryProxy : CBaseGeometryProxy {};
	struct CCombinedGeometryProxy : CBaseGeometryProxy {};
	struct CRgnGeometryProxy : CBaseGeometryProxy {};
	struct CSolidColorLegacyMilBrushProxy : CBaseLegacyMilBrushProxy 
	{
		DECLSPEC_PROJECTION HRESULT Update(double opacity, const D2D1_COLOR_F& color)
		{
			return static_cast<dwmcore::CChannel*>(GetProxy()->GetChannel())->SolidColorLegacyMilBrushUpdate(
				GetProxy()->GetHandleId(),
				opacity,
				color,
				0,
				0,
				0
			);
		}
	};
	struct CImageLegacyMilBrushProxy : CBaseLegacyMilBrushProxy
	{
		DECLSPEC_PROJECTION HRESULT Update(
			double opacity,
			[[maybe_unused]] const D2D1_RECT_F& viewport, // this parameter is will always be ignored, the bounding box of the geometry will be used instead
			const D2D1_RECT_F& viewbox,
			const CDoubleResourceProxy* opacityAnimation,
			MilBrushMappingMode viewportUnits,
			MilBrushMappingMode viewboxUnits,
			const CRectResourceProxy* viewportAnimations,
			const CRectResourceProxy* viewboxAnimations,
			MilStretch stretchMode,
			MilTileMode tileMode,
			MilHorizontalAlignment alignmentX,
			MilVerticalAlignment alignmentY,
			const CBaseImageProxy* imageSource 
		)
		{
			return static_cast<dwmcore::CChannel*>(GetProxy()->GetChannel())->ImageLegacyMilBrushUpdate(
				GetProxy()->GetHandleId(),
				opacity,
				viewport,
				viewbox,
				opacityAnimation ? opacityAnimation->GetProxy()->GetHandleId() : 0,
				0,
				0,
				viewportUnits,
				viewboxUnits,
				viewportAnimations ? viewportAnimations->GetProxy()->GetHandleId() : 0,
				viewboxAnimations ? viewboxAnimations->GetProxy()->GetHandleId() : 0,
				stretchMode,
				tileMode,
				alignmentX,
				alignmentY,
				imageSource ? imageSource->GetProxy()->GetHandleId() : 0
			);
		}
	};
	struct CVisualProxy;
	struct CCachedVisualImageProxy : CBaseImageProxy 
	{
		DECLSPEC_PROJECTION HRESULT Update(
			const D2D1_RECT_F& viewbox,
			const DWM::MilSizeD& realizationSize,
			const uDWM::CRectResourceProxy* rectProxy,
			const uDWM::CSizeResourceProxy* sizeProxy,
			const uDWM::CVisualProxy* visualProxy,
			DWM::MilBrushMappingMode viewboxUnits
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CCachedVisualImageProxy::Update,
				this,
				viewbox,
				realizationSize,
				rectProxy,
				sizeProxy,
				visualProxy,
				viewboxUnits
			);
		}
	};

	struct VisualCollection;
	struct CVisualProxy : CResource 
	{
		DECLSPEC_PROJECTION HRESULT SetSize(double cx, double cy)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisualProxy::SetSize, this, cx, cy);
		}
	};

	struct IVisual : CBaseObject
	{
		STDMETHOD(Initialize)() PURE;
		STDMETHOD(InitializeFromSharedHandle)(HANDLE sharedHandle) PURE;
		STDMETHOD_(void, SetDirtyFlags)(ULONG flags) PURE;
	};
	struct CVisual : CBaseObject
	{
		DECLSPEC_PROJECTION bool IsRTLMirrored() const
		{
			const auto propByte = Util::PointerExecuteUnsafe<CVisual_IsRTLMirrored_ByteOffset_Offsets, Util::OffsetBy<BYTE const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			return (*propByte & 1) != 0;
		}
		DECLSPEC_PROJECTION MilSizeD GetScale() const
		{
			if (g_versionInfo.build < os::build_w11_24h2)
			{
				return Util::PointerExecuteUnsafe<CVisual_GetScale_MilSizeD_Offsets, Util::DereferenceAt<MilSizeD>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			const auto scaleF = Util::PointerExecuteUnsafe<CVisual_GetScale_D2D1_SIZE_F_Offsets, Util::OffsetBy<D2D1_SIZE_F const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			return { static_cast<double>(scaleF->width), static_cast<double>(scaleF->height) };
		}

		DECLSPEC_PROJECTION const SIZE& GetSize() const
		{
			return *Util::PointerExecuteUnsafe<CVisual_GetSize_Offsets, Util::OffsetBy<SIZE const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		LONG GetWidth() const
		{
			return GetSize().cx;
		}
		LONG GetHeight() const
		{
			return GetSize().cy;
		}

		DECLSPEC_PROJECTION const POINT& GetOffset() const
		{
			return *Util::PointerExecuteUnsafe<CVisual_GetOffset_Offsets, Util::OffsetBy<POINT const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		LONG GetX() const
		{
			return GetOffset().x;
		}
		LONG GetY() const
		{
			return GetOffset().y;
		}
		POINT GetLocalToParentVisualOffset(CVisual* parent = nullptr) const
		{
			auto pt = GetOffset();
			auto current = GetTransformParent();
			while (current && current != parent)
			{
				const auto& currentPt = current->GetOffset();
				pt.x += currentPt.x;
				pt.y += currentPt.y;
				current = current->GetTransformParent();
			}
			return pt;
		}
		DECLSPEC_PROJECTION void SetExcludeSubtree(bool exclude)
		{
			auto properties = Util::PointerExecuteUnsafe<CVisual_SetExcludeSubtree_ByteOffset_Offsets, Util::OffsetBy<BYTE*>>(this, g_versionInfo.build, g_versionInfo.revision);
			if (exclude)
			{
				*properties |= 8;
			}
			else
			{
				*properties &= ~8;
			}
		}
		DECLSPEC_PROJECTION VisualCollection* GetVisualCollection()
		{
			return Util::PointerExecuteUnsafe<CVisual_GetVisualCollection_Offsets, Util::OffsetBy<VisualCollection*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CVisualProxy* GetVisualProxy() const
		{
			return *Util::PointerExecuteUnsafe<CVisual_GetVisualProxy_Offsets, Util::OffsetBy<CVisualProxy**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CVisual* GetTransformParent() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CVisual_GetTransformParent_Offsets, Util::DereferenceAt<decltype(&CVisual::GetTransformParent)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}

		DECLSPEC_PROJECTION void SetSize(const SIZE* size)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetSize, this, size);
		}
		DECLSPEC_PROJECTION void SetInsetFromParent(const MARGINS& margins)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetInsetFromParent, this, margins);
		}
		DECLSPEC_PROJECTION void SetInsetFromParentLeft(int left)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetInsetFromParentLeft, this, left);
		}
		DECLSPEC_PROJECTION void SetDirtyFlags(int flags)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetDirtyFlags, this, flags);
		}
		DECLSPEC_PROJECTION HRESULT RenderRecursive()
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::RenderRecursive, this);
		}
		DECLSPEC_PROJECTION DWORD GetDirtyFlags() const
		{
			return *Util::PointerExecuteUnsafe<CVisual_GetDirtyFlags_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION HRESULT _ValidateVisual()
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CVisual__ValidateVisual_Offsets, Util::DereferenceAt<decltype(&CVisual::_ValidateVisual)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
	};

	struct VisualCollection : CResource
	{
		DECLSPEC_PROJECTION HRESULT Remove(CVisual* visual)
		{
			return HANDLE_PROJECTION_FUNCTION(VisualCollection::Remove, this, visual);
		}
		DECLSPEC_PROJECTION HRESULT InsertRelative(
			CVisual* visual,
			CVisual* referenceVisual,
			bool insertAfter,
			bool connectNow
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				VisualCollection::InsertRelative,
				this,
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
		DECLSPEC_PROJECTION static HRESULT Create(CBaseLegacyMilBrushProxy* brush, CBaseGeometryProxy* geometry, CDrawGeometryInstruction** instruction)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CDrawGeometryInstruction::Create, 
				brush,
				geometry,
				instruction
			);
		}

		DECLSPEC_PROJECTION CBaseLegacyMilBrushProxy*& GetBrush()
		{
			return *Util::PointerExecuteUnsafe<CDrawGeometryInstruction_GetBrush_Offsets, Util::OffsetBy<CBaseLegacyMilBrushProxy**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CBaseGeometryProxy*& GetGeometry()
		{
			return *Util::PointerExecuteUnsafe<CDrawGeometryInstruction_GetGeometry_Offsets, Util::OffsetBy<CBaseGeometryProxy**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CRectangleInstruction : CRenderDataInstruction {};
	struct CSolidRectangleInstruction : CRenderDataInstruction
	{
		DWORD m_refCount{ 1 };
		DWORD m_padding{ 0 };
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
	class CDrawVisualTreeInstruction : public CRenderDataInstruction {};

	struct CRenderDataVisual : CVisual
	{
		DECLSPEC_PROJECTION DynArray<CRenderDataInstruction*>& GetInstructions() const
		{
			return *Util::PointerExecuteUnsafe<CRenderDataVisual_GetInstructions_Offsets, Util::OffsetBy<DynArray<CRenderDataInstruction*>*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DWORD GetCount() const
		{
			return GetInstructions().count;
		}
		FORCEINLINE bool IsEmpty() const { return GetCount() == 0; }

		DECLSPEC_PROJECTION HRESULT AddInstruction(CRenderDataInstruction* instruction)
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::AddInstruction, this, instruction);
		}
		DECLSPEC_PROJECTION HRESULT ClearInstructions()
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::ClearInstructions, this);
		}
		DECLSPEC_PROJECTION HRESULT UpdateRenderData()
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::UpdateRenderData, this);
		}
		DECLSPEC_PROJECTION static HRESULT Create_At_Least_W10_1903(CRenderDataVisual** visual)
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::Create_At_Least_W10_1903, visual);
		}
		DECLSPEC_PROJECTION static HRESULT Create_Pre_W10_1903(IDwmChannel* channel, CRenderDataVisual** visual)
		{
			return HANDLE_PROJECTION_FUNCTION(CRenderDataVisual::Create_Pre_W10_1903, channel, visual);
		}
		static HRESULT Create(CRenderDataVisual** visual);
	};
	struct CCanvasVisual : CRenderDataVisual {};
	struct CText : CRenderDataVisual 
	{
		DECLSPEC_PROJECTION bool IsRTLReading() const
		{
			const auto p = Util::PointerExecuteUnsafe<CText_RTL_FlagByteOffset_Offsets, Util::OffsetBy<BYTE const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			return (*p & 2) != 0;
		}
		DECLSPEC_PROJECTION bool IsReverseAlignment() const
		{
			const auto p = Util::PointerExecuteUnsafe<CText_RTL_FlagByteOffset_Offsets, Util::OffsetBy<BYTE const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			return (*p & 4) != 0;
		}
	};
	struct CDWriteText;
	struct IText
	{
		STDMETHODV_(void, SetColor)(COLORREF) PURE;
		STDMETHODV_(void, SetFont)(const LOGFONTW*) PURE;
		STDMETHODV_(void, SetScalingFactor)(double) PURE;
		STDMETHODV_(void, SetRTLReading)(bool) PURE;
		STDMETHODV_(void, SetBackgroundColor)(ULONG) PURE;
		STDMETHODV_(void, SetReverseAlignment)(bool) PURE;
		STDMETHODV_(ULONG_PTR, SetText)(LPCWSTR) PURE;

		DECLSPEC_PROJECTION CDWriteText* GetDWriteText() const
		{
			return Util::PointerExecuteUnsafe<IText_GetDWriteText_NegativeOffset_Offsets, Util::OffsetBy<CDWriteText*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION bool IsRTLReading() const
		{
			return *Util::PointerExecuteUnsafe<IText_RTL_Index_Offsets, Util::OffsetBy<bool const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION bool IsReverseAlignment() const
		{
			return *Util::PointerExecuteUnsafe<IText_Reverse_Index_Offsets, Util::OffsetBy<bool const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CDWriteText : CVisual
	{
		DECLSPEC_PROJECTION IText* GetTextInterface() const
		{
			return Util::PointerExecuteUnsafe<CDWriteText_GetTextInterface_Offsets, Util::OffsetBy<IText*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};

	struct CImage : CVisual {};
	struct CBitmapSource : CBaseObject {};
	struct CBitmapSourceArray : DynArray<CBitmapSource*> {};

	struct CAtlasedRectsVisual : CVisual 
	{
		DECLSPEC_PROJECTION HRESULT InitializeVisualTreeClone(
			CAtlasedRectsVisual* clonedVisual, 
			UINT cloneOption
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CAtlasedRectsVisual::InitializeVisualTreeClone, this, clonedVisual, cloneOption);
		}
	};
	struct CTopLevelAtlasedRectsVisual : CAtlasedRectsVisual {};
	struct CAtlasedImage : CBaseObject
	{
		DECLSPEC_PROJECTION DWORD GetPartId() const
		{
			return *Util::PointerExecuteUnsafe<CAtlasedImage_GetPartId_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};

	struct CTimeline {};
	struct CButton : CAtlasedRectsVisual
	{
		inline static PVOID* vftable{ nullptr };

		DECLSPEC_PROJECTION static HRESULT Create(CButton** visual)
		{
			return HANDLE_PROJECTION_FUNCTION(CButton::Create, visual);
		}
		DECLSPEC_PROJECTION HRESULT SetVisualStates_Win10(
			uDWM::CBitmapSourceArray* buttonArray, 
			uDWM::CBitmapSourceArray* glyphArray, 
			uDWM::CBitmapSource* glowBitmap, 
			float opacity
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CButton::SetVisualStates_Win10,
				this,
				buttonArray,
				glyphArray,
				glowBitmap,
				opacity
			);
		}
		DECLSPEC_PROJECTION HRESULT SetVisualStates_Win11(
			uDWM::CBitmapSourceArray* buttonArray,
			uDWM::CBitmapSourceArray* glyphArray,
			float opacity
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CButton::SetVisualStates_Win11,
				this,
				buttonArray,
				glyphArray,
				opacity
			);
		}
		HRESULT SetVisualStates(
			uDWM::CBitmapSourceArray* buttonArray,
			uDWM::CBitmapSourceArray* glyphArray,
			float opacity
		)
		{
			if (g_versionInfo.build < os::build_w11_21h2) [[likely]]
			{
				return SetVisualStates_Win10(
					buttonArray,
					glyphArray,
					nullptr,
					opacity
				);
			}
			else
			{
				return SetVisualStates_Win11(
					buttonArray,
					glyphArray,
					opacity
				);
			}
		}

		DECLSPEC_PROJECTION float& GetGlyphOpacity()
		{
			return *Util::PointerExecuteUnsafe<CButton_GetGlyphOpacity_Offsets, Util::OffsetBy<float*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION BYTE* GetButtonState()
		{
			return Util::PointerExecuteUnsafe<CButton_GetButtonState_Offsets, Util::OffsetBy<BYTE*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION DWORD* GetVisualState()
		{
			return Util::PointerExecuteUnsafe<CButton_GetVisualState_Offsets, Util::OffsetBy<DWORD*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION void** GetFirstAtlasImage()
		{
			return Util::PointerExecuteUnsafe<CButton_GetFirstAtlasImage_Offsets, Util::OffsetBy<void**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION void** GetSecondAtlasImage()
		{
			return Util::PointerExecuteUnsafe<CButton_GetSecondAtlasImage_Offsets, Util::OffsetBy<void**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CTimeline* GetTimeline()
		{
			return *Util::PointerExecuteUnsafe<CButton_GetTimeline_Offsets, Util::OffsetBy<CTimeline**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CBitmapSourceArray* GetGlyphBitmapArray()
		{
			return Util::PointerExecuteUnsafe<CButton_GetGlyphBitmapArray_Offsets, Util::OffsetBy<CBitmapSourceArray*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CBitmapSourceArray* GetButtonBitmapArray()
		{
			return Util::PointerExecuteUnsafe<CButton_GetButtonBitmapArray_Offsets, Util::OffsetBy<CBitmapSourceArray*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
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
		bool IsClipRegionEffective() const
		{
			return (AccentFlags & ((1 << 4) | (1 << 9))) != 0;
		}
	};
	struct CAccent : CRenderDataVisual
	{
		ACCENT_POLICY& GetAccentPolicy()
		{
			return *Util::PointerExecuteUnsafe<CAccent_GetAccentPolicy_Offsets, Util::OffsetBy<ACCENT_POLICY*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CAccentBlurBehind : CRenderDataVisual {};
	struct CAccentAcrylicBlurBehind : CRenderDataVisual {};
	struct CWindowBorder : CVisual
	{
		DECLSPEC_PROJECTION HRESULT EnableBorder(bool enable)
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowBorder::EnableBorder, this, enable);
		}
	};

	struct IDwmWindow;
	struct CTopLevelWindow;
	struct CWindowData : CBaseObject
	{
		DECLSPEC_PROJECTION HWND GetHwnd() const
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetHwnd_Offsets, Util::OffsetBy<const HWND*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CTopLevelWindow* GetWindow() const
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetWindow_Index_Offsets, Util::OffsetBy<CTopLevelWindow* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION ACCENT_POLICY* GetAccentPolicy() const
		{
			return Util::PointerExecuteUnsafe<CWindowData_GetAccentPolicy_Offsets, Util::OffsetBy<ACCENT_POLICY*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION DWM_SYSTEMBACKDROP_TYPE& GetSystemBackdropType()
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetSystemBackdropType_Index_Offsets, Util::OffsetBy<DWM_SYSTEMBACKDROP_TYPE*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION UINT GetWindowDPI() const
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetWindowDPI_Offsets, Util::OffsetBy<UINT const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION DWORD GetFrameThickness()
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetFrameThickness_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION BYTE& GetNonClientAttributeReference()
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetNonClientAttribute_Offsets, Util::OffsetBy<BYTE*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION BYTE GetNonClientAttribute() const
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetNonClientAttribute_Offsets, Util::OffsetBy<BYTE const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION BYTE GetClientBlurAttribute() const
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetClientBlurAttribute_Offsets, Util::OffsetBy<BYTE const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION BYTE& GetClientBlurAttributeReference()
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetClientBlurAttribute_Offsets, Util::OffsetBy<BYTE*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		bool ShouldTransitionOnMaximized() const
		{
			return GetClientBlurAttribute() & 0x8;
		}
		DECLSPEC_PROJECTION RECT& GetWindowRect() const
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetWindowRect_Offsets, Util::OffsetBy<RECT*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION MARGINS& GetClientMargins()
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetClientMargins_Offsets, Util::OffsetBy<MARGINS*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION MARGINS& GetExtendedFrameMargins()
		{
			return *Util::PointerExecuteUnsafe<CWindowData_GetExtendedFrameMargins_Offsets, Util::OffsetBy<MARGINS*>>(this, g_versionInfo.build, g_versionInfo.revision);
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

			return IsSheetOfGlass(margins);
		}
		static bool IsSheetOfGlass(const MARGINS& margins)
		{
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
		DECLSPEC_PROJECTION const float& GetBalance() const
		{
			return reinterpret_cast<float const*>(this)[8];
		}
		DECLSPEC_PROJECTION const D2D1_COLOR_F& GetColor() const
		{
			return reinterpret_cast<D2D1_COLOR_F const*>(this)[1];
		}
		DECLSPEC_PROJECTION D2D1_COLOR_F getArgbcolor() const
		{
			const auto& balance = GetBalance();
			const auto& color = GetColor();
			return D2D1::ColorF(
				color.r * balance,
				color.g * balance,
				color.b * balance,
				color.a
			);
		}
	};
	struct CTopLevelWindow : CVisual
	{
		struct WindowFrame
		{
			DECLSPEC_PROJECTION UINT32& GetCornerRadius()
			{
				return *Util::PointerExecuteUnsafe<CTopLevelWindow_WindowFrame_GetCornerRadius_Offsets, Util::OffsetBy<UINT32*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
		};
		inline static PVOID* vftable{ nullptr };
		inline static WindowFrame*** s_rgpwfWindowFrames{ nullptr };
		static auto GetWindowFrames()
		{
			return *s_rgpwfWindowFrames;
		}

		bool IsOffscreen()
		{
			const auto& offset = GetOffset();

			return offset.x == -32000 || offset.y == -32000;
		}

		DECLSPEC_PROJECTION CWindowData* GetData() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetData_Index_Offsets, Util::OffsetBy<CWindowData* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CText* GetTextVisual() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetTextVisual_Index_Offsets, Util::OffsetBy<CText* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CImage* GetIconVisual() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetIconVisual_Index_Offsets, Util::OffsetBy<CImage* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CDWriteText* GetDWriteTextVisual() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetDWriteTextVisual_Index_Offsets, Util::OffsetBy<CDWriteText* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CAccent* GetAccent() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetAccent_Index_Offsets, Util::OffsetBy<CAccent* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CCanvasVisual* GetLegacyVisual() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetLegacyVisual_Index_Offsets, Util::OffsetBy<CCanvasVisual* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CCanvasVisual* GetClientBlurVisual() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetClientBlurVisual_Index_Offsets, Util::OffsetBy<CCanvasVisual* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CWindowBorder* GetWindowBorder() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetWindowBorder_Index_Offsets, Util::OffsetBy<CWindowBorder* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}


		DECLSPEC_PROJECTION CRgnGeometryProxy* GetBorderGeometry() const
		{
			if (g_versionInfo.build < os::build_w11_22h2)
			{
				return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetBorderGeometry_Index_OnThis_Offsets, Util::OffsetBy<CRgnGeometryProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			const auto legacyVisual = GetLegacyVisual();
			return legacyVisual ? *Util::PointerExecuteUnsafe<CTopLevelWindow_GetBorderGeometry_Index_OnLegacy_Offsets, Util::OffsetBy<CRgnGeometryProxy* const*>>(legacyVisual, g_versionInfo.build, g_versionInfo.revision) : nullptr;
		}
		DECLSPEC_PROJECTION CRgnGeometryProxy* GetCaptionGeometry() const
		{
			if (g_versionInfo.build < os::build_w11_22h2)
			{
				return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetCaptionGeometry_Index_OnThis_Offsets, Util::OffsetBy<CRgnGeometryProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			const auto legacyVisual = GetLegacyVisual();
			return legacyVisual ? *Util::PointerExecuteUnsafe<CTopLevelWindow_GetCaptionGeometry_Index_OnLegacy_Offsets, Util::OffsetBy<CRgnGeometryProxy* const*>>(legacyVisual, g_versionInfo.build, g_versionInfo.revision) : nullptr;
		}
		DECLSPEC_PROJECTION CSolidColorLegacyMilBrushProxy* GetCaptionBrush() const
		{
			if (g_versionInfo.build < os::build_w11_22h2)
			{
				return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetCaptionBrush_Index_OnThis_Offsets, Util::OffsetBy<CSolidColorLegacyMilBrushProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			const auto legacyVisual = GetLegacyVisual();
			return legacyVisual ? *Util::PointerExecuteUnsafe<CTopLevelWindow_GetCaptionBrush_Index_OnLegacy_Offsets, Util::OffsetBy<CSolidColorLegacyMilBrushProxy* const*>>(legacyVisual, g_versionInfo.build, g_versionInfo.revision) : nullptr;
		}
		DECLSPEC_PROJECTION CSolidColorLegacyMilBrushProxy* GetBorderBrush() const
		{
			if (g_versionInfo.build < os::build_w11_22h2)
			{
				return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetBorderBrush_Index_OnThis_Offsets, Util::OffsetBy<CSolidColorLegacyMilBrushProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			const auto legacyVisual = GetLegacyVisual();
			return legacyVisual ? *Util::PointerExecuteUnsafe<CTopLevelWindow_GetBorderBrush_Index_OnLegacy_Offsets, Util::OffsetBy<CSolidColorLegacyMilBrushProxy* const*>>(legacyVisual, g_versionInfo.build, g_versionInfo.revision) : nullptr;
		}
		DECLSPEC_PROJECTION CSolidColorLegacyMilBrushProxy* GetClientBlurBrush() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetClientBlurBrush_Index_OnThis_Offsets, Util::OffsetBy<CSolidColorLegacyMilBrushProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION bool& GetIsBorderUpdatesSuppressed()
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetIsBorderUpdatesSuppressed_ByteIndex_Offsets, Util::OffsetBy<bool*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION bool IsRTLMirrored() const
		{
			return (*Util::PointerExecuteUnsafe<CTopLevelWindow_StateDwordIndex_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision) & 0x20000) != 0;
		}
		DECLSPEC_PROJECTION bool IsWindowMaximized() const
		{
			return (*Util::PointerExecuteUnsafe<CTopLevelWindow_StateDwordIndex_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision) & 0x20) != 0;
		}
		DECLSPEC_PROJECTION bool IsToolWindow() const
		{
			return (*Util::PointerExecuteUnsafe<CTopLevelWindow_StateDwordIndex_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision) & 2) != 0;
		}
		DECLSPEC_PROJECTION bool IsLoneButton() const
		{
			return (*Util::PointerExecuteUnsafe<CTopLevelWindow_StateDwordIndex_Offsets, Util::OffsetBy<DWORD const*>>(this, g_versionInfo.build, g_versionInfo.revision) & 0xB00) == 0;
		}

		DECLSPEC_PROJECTION bool HasNonClientArea() const
		{
			const auto margins = GetFrameInsideMargins();
			return margins.cxLeftWidth || margins.cxRightWidth || margins.cyBottomHeight || margins.cyTopHeight;
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

		bool TreatAsMaximized(const CWindowData* data) const;

		DECLSPEC_PROJECTION CGlassColorizationResources* GetCaptionColorizationParameters() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetCaptionColorizationParameters_Index_Offsets, Util::OffsetBy<CGlassColorizationResources* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION CButton* GetButton(int index)
		{
			const auto base = Util::PointerExecuteUnsafe<CTopLevelWindow_GetButton_BasePointerIndex_Offsets, Util::OffsetBy<BYTE*>>(this, g_versionInfo.build, g_versionInfo.revision);
			CButton** button = reinterpret_cast<CButton**>(base + index * sizeof(void*));
			return (button && *button) ? *button : nullptr;
		}
		
		DECLSPEC_PROJECTION MARGINS& GetFrameOutsideMargins(bool zoomed)
		{
			if (zoomed)
			{
				return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetFrameOutsideMargins_Zoomed_Offsets, Util::OffsetBy<MARGINS*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetFrameOutsideMargins_Normal_Offsets, Util::OffsetBy<MARGINS*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION MARGINS& GetFrameInsideMargins() const
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetFrameInsideMargins_Offsets, Util::OffsetBy<MARGINS*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION MARGINS& GetBorderMargins()
		{
			return *Util::PointerExecuteUnsafe<CTopLevelWindow_GetBorderMargins_Offsets, Util::OffsetBy<MARGINS*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION HRESULT CloneVisualTreeForLivePreview_Win10(
			bool windowFramesOnly,
			bool unused1,
			bool unused2,
			CTopLevelWindow** clonedWindow
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CTopLevelWindow::CloneVisualTreeForLivePreview_Win10,
				this,
				windowFramesOnly,
				unused1,
				unused2,
				clonedWindow
			);
		}
		DECLSPEC_PROJECTION HRESULT CloneVisualTreeForLivePreview_Win11(
			bool windowFramesOnly,
			CTopLevelWindow** clonedWindow
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CTopLevelWindow::CloneVisualTreeForLivePreview_Win11,
				this,
				windowFramesOnly,
				clonedWindow
			);
		}
		HRESULT CloneVisualTreeForLivePreview(
			bool windowFramesOnly,
			CTopLevelWindow** clonedWindow
		)
		{
			if (g_versionInfo.build < os::build_w11_22h2) [[likely]]
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
		DECLSPEC_PROJECTION bool TreatAsActiveWindow()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::TreatAsActiveWindow, this);
		}
		DECLSPEC_PROJECTION HRESULT OnBlurBehindUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnBlurBehindUpdated, this);
		}
		DECLSPEC_PROJECTION HRESULT OnAccentPolicyUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnAccentPolicyUpdated, this);
		}
		DECLSPEC_PROJECTION HRESULT OnSystemBackdropUpdated()
		{
			return HANDLE_PROJECTION_FUNCTION(CTopLevelWindow::OnSystemBackdropUpdated, this);
		}
		DECLSPEC_PROJECTION RECT* GetActualWindowRect(
			RECT* rect,
			char relative,
			char respectMaximizedClip,
			bool excludeShadowMargins
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(
				CTopLevelWindow::GetActualWindowRect,
				this,
				rect,
				relative,
				respectMaximizedClip,
				excludeShadowMargins
			);
		}
	};
	struct CTopLevelWindow3D : CRenderDataVisual {};

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

		DECLSPEC_PROJECTION const RECT* GetWindowBoundingRect() const
		{
			return reinterpret_cast<const RECT*>(this);
		}
		DECLSPEC_PROJECTION CRectangleGeometryProxy* GetWindowBoundingGeometry() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_GetWindowBoundingGeometry_Offsets, Util::OffsetBy<CRectangleGeometryProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CBaseLegacyMilBrushProxy* GetWindowVisualBrush() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_GetWindowVisualBrush_Offsets, Util::OffsetBy<CBaseLegacyMilBrushProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION const RECT* GetGlassBoundingRect() const
		{
			return Util::PointerExecuteUnsafe<LivePreviewResource_GetGlassBoundingRect_Offsets, Util::OffsetBy<const RECT*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CRectangleGeometryProxy* GetGlassBoundingGeometry() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_GetGlassBoundingGeometry_Offsets, Util::OffsetBy<CRectangleGeometryProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CBaseLegacyMilBrushProxy* GetGlassVisualBrush() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_GetGlassVisualBrush_Offsets, Util::OffsetBy<CBaseLegacyMilBrushProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION HRGN GetReflectionRegion() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_GetReflectionRegion_Offsets, Util::OffsetBy<HRGN const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CRgnGeometryProxy* GetReflectionGeometry() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_GetReflectionGeometry_Offsets, Util::OffsetBy<CRgnGeometryProxy* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION const RECT* GetMonitorRect() const
		{
			return reinterpret_cast<const RECT*>(reinterpret_cast<ULONG_PTR>(this) + Util::PointerExecuteUnsafe<LivePreviewResource_GetMonitorRect_Offsets, Util::OffsetBy<ULONG_PTR>>(this, g_versionInfo.build, g_versionInfo.revision));
		}

		DECLSPEC_PROJECTION bool IsWindowBoundingRectNotEmpty() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_IsWindowBoundingRectNotEmpty_Index_Offsets, Util::OffsetBy<bool* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION bool IsGlassBoundingRectNotEmpty() const
		{
			return *Util::PointerExecuteUnsafe<LivePreviewResource_IsGlassBoundingRectNotEmpty_Index_Offsets, Util::OffsetBy<bool* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CLivePreview : CRenderDataVisual
	{
		DECLSPEC_PROJECTION CRenderDataVisual* GetThumbnailVisual() const
		{
			return *Util::PointerExecuteUnsafe<CLivePreview_GetThumbnailVisual_Offsets, Util::OffsetBy<CRenderDataVisual* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CRenderDataVisual* GetGlassVisual() const
		{
			return *Util::PointerExecuteUnsafe<CLivePreview_GetGlassVisual_Offsets, Util::OffsetBy<CRenderDataVisual* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION DynArray<LivePreviewResource>* GetLivePreviewResourceArray() const
		{
			return Util::PointerExecuteUnsafe<CLivePreview_GetLivePreviewResourceArray_Offsets, Util::OffsetBy<DynArray<LivePreviewResource>*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION DynArray<LivePreviewVisual>* GetLivePreviewVisualArray() const
		{
			return Util::PointerExecuteUnsafe<CLivePreview_GetLivePreviewVisualArray_Offsets, Util::OffsetBy<DynArray<LivePreviewVisual>*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION static bool IsWindowIncluded(const CWindowData* data)
		{
			const auto p = Util::PointerExecuteUnsafe<CLivePreview_IsWindowIncluded_ByteOffset_Offsets, Util::OffsetBy<BYTE const*>>(data, g_versionInfo.build, g_versionInfo.revision);
			return (*p & 1) == 0;
		}
		DECLSPEC_PROJECTION static bool IsWindowCloneAsWindowFrames(const CWindowData* data)
		{
			return *Util::PointerExecuteUnsafe<CLivePreview_IsWindowCloneAsWindowFrames_DwordIndex_Offsets, Util::OffsetBy<DWORD const*>>(data, g_versionInfo.build, g_versionInfo.revision) == 1;
		}
		DECLSPEC_PROJECTION HRESULT _UpdateResources()
		{
			return HANDLE_PROJECTION_FUNCTION(CLivePreview::_UpdateResources, this);
		}
		DECLSPEC_PROJECTION bool _IsTrulyMaximized(CWindowData* data)
		{
			return HANDLE_PROJECTION_FUNCTION(CLivePreview::_IsTrulyMaximized, this, data);
		}
	};
	struct CAnimatedGlassSheet : CVisual 
	{
		DECLSPEC_PROJECTION const RECT& GetSourceRect() const
		{
			return *Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetSourceRect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION const RECT& GetDestinationRect() const
		{
			return *Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetDestinationRect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION const RECT& GetAdjustedDestinationRect()
		{
			return *Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetAdjustedDestinationRect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION LONG GetAtlasPaddingTop() const
		{
			return Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetAtlasPaddingTop_Rect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision)->left;
		}
		DECLSPEC_PROJECTION LONG GetAtlasPaddingLeft() const
		{
			return Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetAtlasPaddingLeft_Rect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision)->right;
		}
		DECLSPEC_PROJECTION LONG GetAtlasPaddingRight() const
		{
			return -Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetAtlasPaddingRight_Rect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision)->bottom;
		}
		DECLSPEC_PROJECTION LONG GetAtlasPaddingBottom() const
		{
			return -Util::PointerExecuteUnsafe<CAnimatedGlassSheet_GetAtlasPaddingBottom_Rect_Offsets, Util::OffsetBy<RECT const*>>(this, g_versionInfo.build, g_versionInfo.revision)->top;
		}
	};
	struct CWindowList : CBaseObject
	{
		DECLSPEC_PROJECTION PRLIST_ENTRY GetWindowListForDesktop(ULONG_PTR desktopID)
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowList::GetWindowListForDesktop, this, desktopID);
		}
	};

	struct CCompositor
	{
		DECLSPEC_PROJECTION HRESULT CreateSolidColorLegacyMilBrushProxy(CSolidColorLegacyMilBrushProxy** solidColorBrushProxy)
		{
			return CResource::Create(
				DwmResourceType::SolidColorLegacyMilBrushProxy,
				GetChannel(),
				reinterpret_cast<CResource**>(solidColorBrushProxy)
			);
		}
		DECLSPEC_PROJECTION HRESULT CreateImageLegacyMilBrushProxy(CImageLegacyMilBrushProxy** imageBrushProxy)
		{
			return CResource::Create(
				DwmResourceType::ImageLegacyMilBrushProxy,
				GetChannel(),
				reinterpret_cast<CResource**>(imageBrushProxy)
			);
		}
		IDwmChannel* GetChannel() const
		{
			if (g_versionInfo.build < os::build_w10_1903)
			{
				return reinterpret_cast<IDwmChannel*>(const_cast<CCompositor*>(this));
			}
			return *Util::PointerExecuteUnsafe<CCompositor_GetChannel_Index_Offsets, Util::OffsetBy<IDwmChannel* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		IDCompositionDesktopDevicePartner* GetInteropCompositorDCompDevicePartner() const
		{
			return *Util::PointerExecuteUnsafe<CCompositor_GetInteropCompositorDCompDevicePartner_Offsets, Util::OffsetBy<IDCompositionDesktopDevicePartner* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
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
		DECLSPEC_PROJECTION CLivePreview* GetLivePreview() const
		{
			return *Util::PointerExecuteUnsafe<CDesktopManager_GetLivePreview_Index_Offsets, Util::OffsetBy<CLivePreview* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CCompositor* GetCompositor() const
		{
			return *Util::PointerExecuteUnsafe<CDesktopManager_GetCompositor_Index_Offsets, Util::OffsetBy<CCompositor* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CWindowList* GetWindowList() const
		{
			return *Util::PointerExecuteUnsafe<CDesktopManager_GetWindowList_Index_Offsets, Util::OffsetBy<CWindowList* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION IWICImagingFactory* GetWICFactory() const
		{
			return *Util::PointerExecuteUnsafe<CDesktopManager_GetWICFactory_Index_Offsets, Util::OffsetBy<IWICImagingFactory* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION ID2D1Device* GetD2DDevice() const
		{
			if (g_versionInfo.build < os::build_server_2022)
			{
				return *Util::PointerExecuteUnsafe<CDesktopManager_GetD2DDevice_OnThis_Index_Offsets, Util::OffsetBy<ID2D1Device* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			auto graphicsManager = *Util::PointerExecuteUnsafe<CDesktopManager_GetGraphicsManager_Index_Offsets, Util::OffsetBy<void* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			return graphicsManager ? *Util::PointerExecuteUnsafe<CGraphicsManager_GetD2DDevice_Index_Offsets, Util::OffsetBy<ID2D1Device**>>(graphicsManager, g_versionInfo.build, g_versionInfo.revision) : nullptr;
		}
		DECLSPEC_PROJECTION IDCompositionDesktopDevicePartner* GetInteropCompositorDCompDevicePartner() const
		{
			if (g_versionInfo.build < os::build_server_2022)
			{
				return *Util::PointerExecuteUnsafe<CDesktopManager_GetDCompDevice_OnThis_Index_Offsets, Util::OffsetBy<IDCompositionDesktopDevicePartner* const*>>(this, g_versionInfo.build, g_versionInfo.revision);
			}
			return GetCompositor()->GetInteropCompositorDCompDevicePartner();
		}
		DECLSPEC_PROJECTION bool& GetIsHighContrastMode()
		{
			return *Util::PointerExecuteUnsafe<CDesktopManager_GetIsHighContrastMode_BoolIndex_Offsets, Util::OffsetBy<bool*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION bool HasMaximizedWindows() const
		{
			return Util::PointerExecuteUnsafe<CDesktopManager_HasMaximizedWindows_BoolIndex_Offsets, Util::DereferenceAt<bool>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION double GetDPIValue() const
		{
			return *Util::PointerExecuteUnsafe<CDesktopManager_GetDPIValue_Index_Offsets, Util::OffsetBy<double const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	inline bool CTopLevelWindow::TreatAsMaximized(const CWindowData* data = nullptr) const
	{
		if (!data)
		{
			data = GetData();
		}

		return 
		IsWindowMaximized() || 
		(
			data &&
			data->ShouldTransitionOnMaximized() &&
			CDesktopManager::GetInstance()->HasMaximizedWindows()
		);
	}
	inline HRESULT CRenderDataVisual::Create(CRenderDataVisual** visual)
	{
		HRESULT hr{ S_OK };

		if (g_versionInfo.build < os::build_w10_1903)
		{
			struct CDesktopManager;
			hr = Create_Pre_W10_1903(
				uDWM::CDesktopManager::GetInstance()->GetCompositor()->GetChannel(),
				visual
			);
		}
		else
		{
			hr = Create_At_Least_W10_1903(visual);
		}

		return hr;
	}

	namespace ResourceHelper
	{
		DECLSPEC_PROJECTION HRESULT CreateGeometryFromHRGN(
			HRGN hrgn,
			CRgnGeometryProxy** geometry
		)
		{
			return HANDLE_PROJECTION_FUNCTION(ResourceHelper::CreateGeometryFromHRGN, hrgn, geometry);
		}
		DECLSPEC_PROJECTION HRESULT CreateCombinedGeometry(
			CBaseGeometryProxy* geometry1,
			CBaseGeometryProxy* geometry2,
			D2D1_COMBINE_MODE combineMode, // this param is always ignored in udwm implementation
			CCombinedGeometryProxy** geometry
		)
		{
			return HANDLE_PROJECTION_FUNCTION(ResourceHelper::CreateCombinedGeometry, geometry1, geometry2, combineMode, geometry);
		}
	}

	inline uDWM::CTopLevelWindow* TryGetWindowFromVisual(uDWM::CVisual* visual)
	{
		auto current = visual->GetTransformParent();

		while (current && HookHelper::get_vftable_from(current) != uDWM::CTopLevelWindow::vftable)
		{
			current = current->GetTransformParent();
		}

		return static_cast<uDWM::CTopLevelWindow*>(current);
	}

	inline auto g_projectionArray = make_projection_array(
		g_versionInfo.build,

		MAKE_FUNCTION_PROJECTION_TUPLE(CResource::Create, 0, os::build_w10_1903),

		MAKE_FUNCTION_PROJECTION_TUPLE(CSolidColorLegacyMilBrushProxy::Update, os::build_w10_1903, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CImageLegacyMilBrushProxy::Update, os::build_w10_1903, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CVisual::UpdateOffset", os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetSize, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetInsetFromParent, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetInsetFromParentLeft, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetDirtyFlags, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::RenderRecursive, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisualProxy::SetSize, os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::Remove, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::InsertRelative, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CText::ValidateResources", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CText::CloneVisualTree", 0, os::build_w10_2004),
		MAKE_EMPTY_PROJECTION_TUPLE("CText::InitializeVisualTreeClone", os::build_w10_2004, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CDWriteText::ValidateVisual", os::build_w11_22h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDWriteText::InitializeVisualTreeClone", os::build_w11_22h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CSpriteVisual::SetSize", os::build_w11_22h2, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedRectsVisual::CloneVisualTree", os::build_w10_2004, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE(CAtlasedRectsVisual::InitializeVisualTreeClone, os::build_w10_2004, os::build_w11_22h2),

		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelAtlasedRectsVisual::ShouldCloneAtlasImage", 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CButton::vftable, "CButton::`vftable'", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CButton::Create, os::build_w10_2004 , os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CButton::SetVisualStates_Win10, "CButton::SetVisualStates", 0, os::build_w11_21h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CButton::SetVisualStates_Win11, "CButton::SetVisualStates", os::build_w11_21h2, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CButton::UpdateCrossfade", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CButton::DrawStateW", os::build_w11_21h2, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasButton::AppendAtlas", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasButton::AddApproximateAtlasSize", os::build_w11_21h2, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedImage::AppendAtlasNineGrid", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedImage::AddNineGridAtlasSize", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedImage::SetDirtyFlags", os::build_w11_21h2, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateButtonVisuals", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CVisual::MoveToFront", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CButton::RedrawVisual", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedRectsVisual::RemoveAtlasImage", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedRectsVisual::AddAtlasImage", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedRectsVisual::InsertAtlasImageAtIndex", os::build_w11_21h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawGeometryInstruction::Create, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CRenderDataVisual::Create_Pre_W10_1903, "CRenderDataVisual::Create", 0, os::build_w10_1903),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CRenderDataVisual::Create_At_Least_W10_1903, "CRenderDataVisual::Create", os::build_w10_1903, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::AddInstruction, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::ClearInstructions, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::UpdateRenderData, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::UpdateAccentPolicy", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::_UpdateSolidFill", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::SetClipRegion", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::~CAccent", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowBorder::EnableBorder, os::build_w11_21h2, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::vftable, "CTopLevelWindow::`vftable'", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::CloneVisualTreeForLivePreview_Win10, "CTopLevelWindow::CloneVisualTreeForLivePreview", 0, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::CloneVisualTreeForLivePreview_Win11, "CTopLevelWindow::CloneVisualTreeForLivePreview", os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::GetActualWindowRect, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::TreatAsActiveWindow, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnBlurBehindUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnAccentPolicyUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnSystemBackdropUpdated, os::build_w11_21h2, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CTopLevelWindow::s_rgpwfWindowFrames, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateNCAreaBackground", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateNCAreaPositionsAndSizes", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateClientBlur", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::ValidateVisual", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateWindowVisuals", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::~CTopLevelWindow", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::IsShadowNCAreaPart", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::CreateBitmapFromAtlas", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::CreateGlyphsFromAtlas", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::EnsureImages", 0, os::build_w11_24h2),
		MAKE_EMPTY_PROJECTION_TUPLE("SetMargin", os::build_w11_21h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CLivePreview::_UpdateResources, os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_FadeOutToGlass", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_UpdateInstructions", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_UpdateResourcesForMonitorHelper", os::build_w11_24h2, 0),
		
		MAKE_EMPTY_PROJECTION_TUPLE("CAnimatedGlassSheet::OnRectUpdated", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CAnimatedGlassSheet::~CAnimatedGlassSheet", 0, os::build_w11_21h2),

		MAKE_EMPTY_PROJECTION_TUPLE("CGlassColorizationParameters::AdjustWindowColorization", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowList::GetWindowListForDesktop, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(CDesktopManager::s_pDesktopManagerInstance, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CDesktopManager::s_csDwmInstance, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDesktopManager::IsHighContrastMode", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDesktopManager::ReleaseDXGIAdapter", 0, os::build_server_2022),
		MAKE_EMPTY_PROJECTION_TUPLE("CGraphicsDeviceManager::ReleaseGraphicsDevice", os::build_server_2022, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CCompositor::CreateSolidColorLegacyMilBrushProxy, "CCompositor::CreateProxy<CSolidColorLegacyMilBrushProxy>", os::build_w10_1903, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CCompositor::CreateImageLegacyMilBrushProxy, "CCompositor::CreateProxy<CImageLegacyMilBrushProxy>", os::build_w10_1903, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(ResourceHelper::CreateGeometryFromHRGN, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(ResourceHelper::CreateCombinedGeometry, 0, 0)
	);
	
	inline bool SymbolParserCallback(PSYMBOL_INFO info, [[maybe_unused]] ULONG size)
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

		if (
			!strcmp(symbolName, "SetMargin") &&
			!strstr(info->Name, "HHHHPEBU1")
		)
		{
			return true;
		}

		g_projectionArray.Apply(
			symbolName,
			reinterpret_cast<PVOID>(info->Address)
		);

		return !g_projectionArray.IsAllReady();
	}
}
