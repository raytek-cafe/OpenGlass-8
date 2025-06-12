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
	struct CDoubleResourceProxy : CBaseResourceProxy {};
	struct CRectangleGeometryProxy : CBaseGeometryProxy 
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetRectangle(
			float left,
			float top,
			float right,
			float bottom,
			float topLeftRadiusX,
			float topLeftRadiusY,
			float topRightRadiusX,
			float topRightRadiusY,
			float bottomLeftRadiusX,
			float bottomLeftRadiusY,
			float bottomRightRadiusX,
			float bottomRightRadiusY,
			bool animate
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CRectangleGeometryProxy::SetRectangle,
				left,
				top,
				right,
				bottom,
				topLeftRadiusX,
				topLeftRadiusY,
				topRightRadiusX,
				topRightRadiusY,
				bottomLeftRadiusX,
				bottomLeftRadiusY,
				bottomRightRadiusX,
				bottomRightRadiusY,
				animate
			);
		}
	};
	struct CCombinedGeometryProxy : CBaseGeometryProxy {};
	struct CRgnGeometryProxy : CBaseGeometryProxy {};
	struct CSolidColorLegacyMilBrushProxy : CBaseLegacyMilBrushProxy 
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE Update(double opacity, const D2D1_COLOR_F& color)
		{
			return HANDLE_PROJECTION_FUNCTION(CSolidColorLegacyMilBrushProxy::Update, opacity, color);
		}
	};
	struct CImageLegacyMilBrushProxy : CBaseLegacyMilBrushProxy
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE Update(
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
			return HANDLE_PROJECTION_FUNCTION(
				CImageLegacyMilBrushProxy::Update,
				opacity,
				viewport,
				viewbox,
				opacityAnimation,
				viewportUnits,
				viewboxUnits,
				viewportAnimations,
				viewboxAnimations,
				stretchMode,
				tileMode,
				alignmentX,
				alignmentY,
				imageSource
			);
		}
	};
	struct CVisualProxy;
	struct CCachedVisualImageProxy : CBaseImageProxy 
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE Update(
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
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetClip(CBaseGeometryProxy* geometry)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisualProxy::SetClip, geometry);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetSize(double cx, double cy)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisualProxy::SetSize, cx, cy);
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
		inline static PVOID* vftable{ nullptr };


		bool IsRTLMirrored() const
		{
			bool rtlMirrored{ false };

			if (g_buildNumber < os::build_w11_21h2)
			{
				rtlMirrored = (reinterpret_cast<BYTE const*>(this)[84] & 1) != 0;
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				rtlMirrored = (reinterpret_cast<BYTE const*>(this)[92] & 1) != 0;
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				rtlMirrored = (reinterpret_cast<BYTE const*>(this)[92] & 1) != 0;
			}
			else
			{
				rtlMirrored = (reinterpret_cast<BYTE const*>(this)[36] & 1) != 0;
			}

			return rtlMirrored;
		}
		MilSizeD GetScale() const
		{
			MilSizeD scale{};

			if (g_buildNumber < os::build_w11_21h2)
			{
				scale = *reinterpret_cast<MilSizeD const*>(reinterpret_cast<ULONG_PTR>(this) + 168);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				scale = *reinterpret_cast<MilSizeD const*>(reinterpret_cast<ULONG_PTR>(this) + 176);
			}
			else
			{
				const auto scaleF = reinterpret_cast<D2D1_SIZE_F const*>(reinterpret_cast<ULONG_PTR>(this) + 112);
				scale = { static_cast<double>(scaleF->width), static_cast<double>(scaleF->height) };
			}

			return scale;
		}

		const SIZE& GetSize() const
		{
			const SIZE* size{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				size = &reinterpret_cast<SIZE const*>(this)[15];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				size = &reinterpret_cast<SIZE const*>(this)[16];
			}
			else
			{
				size = &reinterpret_cast<SIZE const*>(this)[9];
			}

			return *size;
		}
		LONG GetWidth() const
		{
			return GetSize().cx;
		}
		LONG GetHeight() const
		{
			return GetSize().cy;
		}

		const POINT& GetOffset() const
		{
			const POINT* pt{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				pt = &reinterpret_cast<POINT const*>(this)[14];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				pt = &reinterpret_cast<POINT const*>(this)[15];
			}
			else
			{
				pt = &reinterpret_cast<POINT const*>(this)[8];
			}

			return *pt;
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
				const auto currentPt = current->GetOffset();
				pt.x += currentPt.x;
				pt.y += currentPt.y;
				current = current->GetTransformParent();
			}
			return pt;
		}
		void SetExcludeSubtree(bool exclude)
		{
			BYTE* properties{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				properties = &(reinterpret_cast<BYTE*>(this)[84]);
			}
			else
			{
				properties = &(reinterpret_cast<BYTE*>(this)[92]);
			}

			if (exclude)
			{
				*properties |= 8;
			}
			else
			{
				*properties &= ~8;
			}
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
			const auto vtfble = HookHelper::vftbl_of(this);

			if (g_buildNumber < os::build_w11_22h2)
			{
				return std::invoke(
					Util::force_cast_to<decltype(&CVisual::GetTransformParent)>(
						vtfble[10]
					),
					this
				);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				return std::invoke(
					Util::force_cast_to<decltype(&CVisual::GetTransformParent)>(
						vtfble[11]
					),
					this
				);
			}
			else
			{
				return std::invoke(
					Util::force_cast_to<decltype(&CVisual::GetTransformParent)>(
						vtfble[8]
					),
					this
				);
			}
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
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE SetInsetFromParentLeft(int left)
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::SetInsetFromParentLeft, left);
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
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE RenderRecursive()
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::RenderRecursive);
		}
		DWORD GetDirtyFlags() const
		{
			DWORD flags{};

			if (g_buildNumber < os::build_w11_21h2)
			{
				flags = reinterpret_cast<DWORD const*>(this)[20];
			}
			if (g_buildNumber < os::build_w11_24h2)
			{
				flags = reinterpret_cast<DWORD const*>(this)[22];
			}
			else
			{
				flags = reinterpret_cast<DWORD const*>(this)[8];
			}

			return flags;
		}
		HRESULT STDMETHODCALLTYPE _ValidateVisual()
		{
			decltype(&CVisual::_ValidateVisual) vfptr{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[6]);
			}
			else
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[4]);
			}

			return std::invoke(
				vfptr,
				this
			);
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
	struct CRectangleInstruction : CRenderDataInstruction
	{
		DECLSPEC_PROJECTION static HRESULT STDMETHODCALLTYPE Create(CRectangleInstruction** instruction)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CRectangleInstruction::Create,
				instruction
			);
		}

		void SetBrush1(CBaseLegacyMilBrushProxy* brush)
		{
			auto& m_brush = reinterpret_cast<CBaseLegacyMilBrushProxy**>(this)[2];

			if (brush)
			{
				m_brush = brush;
				m_brush->AddRef();
			}
			else if (m_brush)
			{
				m_brush->Release();
				m_brush = nullptr;
			}
		}
		void SetBrush2(CBaseLegacyMilBrushProxy* brush)
		{
			auto& m_brush = reinterpret_cast<CBaseLegacyMilBrushProxy**>(this)[3];

			if (brush)
			{
				m_brush = brush;
				m_brush->AddRef();
			}
			else if (m_brush)
			{
				m_brush->Release();
				m_brush = nullptr;
			}
		}
		void SetRectangle(const D2D1_RECT_F& rect)
		{
			reinterpret_cast<D2D1_RECT_F*>(this)[2] = rect;
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
		static HRESULT STDMETHODCALLTYPE Create(CVisual* visual, CDrawVisualTreeInstruction** instruction)
		{
			*instruction = new (std::nothrow) CDrawVisualTreeInstruction(visual);
			RETURN_HR_IF_NULL(E_OUTOFMEMORY, *instruction);
			return S_OK;
		}
		CDrawVisualTreeInstruction(CVisual* visual) : CRenderDataInstruction{} 
		{ 
			m_visual.copy_from(visual); 
		}
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
		bool IsRTLReading() const
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
		bool IsReverseAlignment() const
		{
			bool rtl{ false };

			if (g_buildNumber < os::build_w11_21h2)
			{
				rtl = (reinterpret_cast<BYTE const*>(this)[280] & 4) != 0;
			}
			else
			{
				rtl = (reinterpret_cast<BYTE const*>(this)[288] & 4) != 0;
			}

			return rtl;
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

		CDWriteText* GetDWriteText() const
		{
			CDWriteText* text{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				text = reinterpret_cast<CDWriteText*>(reinterpret_cast<ULONG_PTR>(this) - 272);
			}
			else
			{
				text = reinterpret_cast<CDWriteText*>(reinterpret_cast<ULONG_PTR>(this) - 168);
			}

			return text;
		}

		bool IsRTLReading() const
		{
			return reinterpret_cast<bool const*>(this)[256];
		}
		bool IsReverseAlignment() const
		{
			return reinterpret_cast<bool const*>(this)[257];
		}
	};
	struct CDWriteText : CVisual
	{
		IText* GetTextInterface() const
		{
			IText* text{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				text = reinterpret_cast<IText*>(reinterpret_cast<ULONG_PTR>(this) + 272);
			}
			else
			{
				text = reinterpret_cast<IText*>(reinterpret_cast<ULONG_PTR>(this) + 168);
			}

			return text;
		}
	};

	struct CImage : CVisual {};

	struct CBitmapSource : CBaseObject {};
	struct CBitmapSourceArray : DynArray<CBitmapSource*> {};

	struct CAtlasedRectsVisual : CVisual 
	{
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE InitializeVisualTreeClone(
			CAtlasedRectsVisual* clonedVisual, 
			UINT cloneOption
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CAtlasedRectsVisual::InitializeVisualTreeClone, clonedVisual, cloneOption);
		}
	};
	struct CTopLevelAtlasedRectsVisual : CAtlasedRectsVisual {};
	struct CAtlasedImage : CBaseObject
	{
		DWORD GetPartId() const
		{
			return reinterpret_cast<DWORD const*>(this)[30];
		}
	};

	struct CTimeline {};
	struct CButton : CAtlasedRectsVisual
	{
		inline static PVOID* vftable{ nullptr };

		DECLSPEC_PROJECTION static HRESULT STDMETHODCALLTYPE Create(CButton** visual)
		{
			return HANDLE_PROJECTION_FUNCTION(CButton::Create, visual);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetVisualStates_Win10(
			uDWM::CBitmapSourceArray* buttonArray, 
			uDWM::CBitmapSourceArray* glyphArray, 
			uDWM::CBitmapSource* glowBitmap, 
			float opacity
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CButton::SetVisualStates_Win10,
				buttonArray,
				glyphArray,
				glowBitmap,
				opacity
			);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetVisualStates_Win11(
			uDWM::CBitmapSourceArray* buttonArray,
			uDWM::CBitmapSourceArray* glyphArray,
			float opacity
		)
		{
			return HANDLE_PROJECTION_FUNCTION(
				CButton::SetVisualStates_Win11,
				buttonArray,
				glyphArray,
				opacity
			);
		}
		HRESULT STDMETHODCALLTYPE SetVisualStates(
			uDWM::CBitmapSourceArray* buttonArray,
			uDWM::CBitmapSourceArray* glyphArray,
			float opacity
		)
		{
			if (g_buildNumber < os::build_w11_21h2) [[likely]]
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

		float& GetGlyphOpacity()
		{
			float* glyphOpacity{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				glyphOpacity = &(reinterpret_cast<float*>(this)[101]);
			}

			else
			{
				glyphOpacity = &(reinterpret_cast<float*>(this)[89]);
			}

			return *glyphOpacity;
		}
		BYTE* GetButtonState()
		{
			BYTE* buttonState{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				buttonState = &(reinterpret_cast<BYTE*>(this)[280]);
			}
			else
			{
				buttonState = &(reinterpret_cast<BYTE*>(this)[288]);
			}

			return buttonState;
		}
		CTimeline* GetTimeline()
		{
			return reinterpret_cast<CTimeline**>(this)[49];
		}
		CBitmapSourceArray* GetGlyphBitmapArray()
		{
			CBitmapSourceArray* glyphBitmapArray{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				glyphBitmapArray = reinterpret_cast<CBitmapSourceArray*>(reinterpret_cast<ULONG_PTR>(this) + 304);
			}
			else
			{
				glyphBitmapArray = reinterpret_cast<CBitmapSourceArray*>(reinterpret_cast<ULONG_PTR>(this) + 312);
			}

			return glyphBitmapArray;
		}
		CBitmapSourceArray* GetButtonBitmapArray()
		{
			CBitmapSourceArray* buttonBitmapArray{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				buttonBitmapArray = reinterpret_cast<CBitmapSourceArray*>(reinterpret_cast<ULONG_PTR>(this) + 336);
			}
			else
			{
				buttonBitmapArray = reinterpret_cast<CBitmapSourceArray*>(reinterpret_cast<ULONG_PTR>(this) + 344);
			}

			return buttonBitmapArray;
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

		UINT GetWindowDPI() const
		{
			UINT dpi{ 0 };

			if (g_buildNumber < os::build_w11_21h2)
			{
				dpi = *reinterpret_cast<UINT const*>(reinterpret_cast<ULONG_PTR>(this) + 324);
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				dpi = *reinterpret_cast<UINT const*>(reinterpret_cast<ULONG_PTR>(this) + 348);
			}
			else
			{
				dpi = reinterpret_cast<UINT const*>(this)[87];
			}

			return dpi;
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
		inline static PVOID* vftable{ nullptr };
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
			else if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CText* const*>(this)[70];
			}
			else
			{
				visual = reinterpret_cast<CText* const*>(this)[65];
			}

			return visual;
		}
		CImage* GetIconVisual() const
		{
			CImage* visual{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				visual = reinterpret_cast<CImage* const*>(this)[66];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				visual = reinterpret_cast<CImage* const*>(this)[68];
			}
			else if (g_buildNumber < os::build_w11_23h2)
			{
				visual = reinterpret_cast<CImage* const*>(this)[72];
			}
			else
			{
				visual = reinterpret_cast<CImage* const*>(this)[67];
			}

			return visual;
		}
		CDWriteText* GetDWriteTextVisual() const
		{
			CDWriteText* visual{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				visual = reinterpret_cast<CDWriteText* const*>(this)[70];
			}
			else
			{
				visual = reinterpret_cast<CDWriteText* const*>(this)[65];
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
		bool IsWindowMaximized()
		{
			bool windowMaximized{ false };

			if (g_buildNumber < os::build_w10_2004)
			{
				windowMaximized = (reinterpret_cast<DWORD const*>(this)[146] & 0x20) != 0;
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				windowMaximized = (reinterpret_cast<DWORD const*>(this)[148] & 0x20) != 0;
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				windowMaximized = (reinterpret_cast<DWORD const*>(this)[152] & 0x20) != 0;
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				windowMaximized = (reinterpret_cast<DWORD const*>(this)[156] & 0x20) != 0;
			}
			else
			{
				windowMaximized = (reinterpret_cast<DWORD const*>(this)[146] & 0x20) != 0;
			}

			return windowMaximized;
		}
		bool IsToolWindow()
		{
			bool toolWindow{ false };

			if (g_buildNumber < os::build_w10_2004)
			{
				toolWindow = (reinterpret_cast<DWORD const*>(this)[146] & 2) != 0;
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				toolWindow = (reinterpret_cast<DWORD const*>(this)[148] & 2) != 0;
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				toolWindow = (reinterpret_cast<DWORD const*>(this)[152] & 2) != 0;
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				toolWindow = (reinterpret_cast<DWORD const*>(this)[156] & 2) != 0;
			}
			else
			{
				toolWindow = (reinterpret_cast<DWORD const*>(this)[146] & 2) != 0;
			}

			return toolWindow;
		}
		bool IsLoneButton()
		{
			bool loneButton{ false };

			if (g_buildNumber < os::build_w10_2004)
			{
				loneButton = (reinterpret_cast<DWORD const*>(this)[146] & 0xB00) == 0;
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				loneButton = (reinterpret_cast<DWORD const*>(this)[148] & 0xB00) == 0;
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				loneButton = (reinterpret_cast<DWORD const*>(this)[152] & 0xB00) == 0;
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				loneButton = (reinterpret_cast<DWORD const*>(this)[156] & 0xB00) == 0;
			}
			else
			{
				loneButton = (reinterpret_cast<DWORD const*>(this)[146] & 0xB00) == 0;
			}

			return loneButton;
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

		CButton* GetButton(int index)
		{
			CButton** button = nullptr;

			if (g_buildNumber < os::build_w11_21h2)
			{
				button = reinterpret_cast<CButton**>(reinterpret_cast<ULONG_PTR>(this) + 8 * (index + 61));
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				button = reinterpret_cast<CButton**>(reinterpret_cast<ULONG_PTR>(this) + 8 * (index + 63));
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				button = reinterpret_cast<CButton**>(reinterpret_cast<ULONG_PTR>(this) + 8 * (index + 66));
			}
			else
			{
				button = reinterpret_cast<CButton**>(reinterpret_cast<ULONG_PTR>(this) + 488 + 8 * index);
			}

			if (button && *button)
			{
				return *button;
			}

			return nullptr;
		}
		
		DWORD GetFrameThickness(CWindowData* data = nullptr)
		{
			if (!data)
			{
				data = GetData();
			}

			if (g_buildNumber < os::build_w11_21h2)
			{
				return *reinterpret_cast<DWORD const*>(reinterpret_cast<ULONG_PTR>(data) + 96);
			}
			else
			{
				return *reinterpret_cast<DWORD const*>(reinterpret_cast<ULONG_PTR>(data) + 112);
			}
		}
		MARGINS& GetMarginsVisibleOutside(bool zoomed)
		{
			MARGINS* margins{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + (zoomed ? 644 : 628));
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + (zoomed ? 660 : 644));
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + (zoomed ? 676 : 660));
			}
			else
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + (zoomed ? 636 : 620));
			}

			return *margins;
		}
		MARGINS& GetBorderMargins()
		{
			MARGINS* margins{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + 596);
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + 612);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + 628);
			}
			else
			{
				margins = reinterpret_cast<MARGINS*>(reinterpret_cast<ULONG_PTR>(this) + 588);
			}

			return *margins;
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
				array = reinterpret_cast<DynArray<LivePreviewResource>*>(reinterpret_cast<ULONG_PTR>(this) + 328);
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
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CreateSolidColorLegacyMilBrushProxy(CSolidColorLegacyMilBrushProxy** solidColorBrushProxy)
		{
			return HANDLE_PROJECTION_FUNCTION(CCompositor::CreateSolidColorLegacyMilBrushProxy, solidColorBrushProxy);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CreateImageLegacyMilBrushProxy(CImageLegacyMilBrushProxy** imageBrushProxy)
		{
			return HANDLE_PROJECTION_FUNCTION(CCompositor::CreateImageLegacyMilBrushProxy, imageBrushProxy);
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
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CreateRectangleGeometry(
			LPCRECT lprc,
			CRectangleGeometryProxy** geometry
		)
		{
			return HANDLE_PROJECTION_FUNCTION(ResourceHelper::CreateRectangleGeometry, lprc, geometry);
		}
	}

	inline uDWM::CTopLevelWindow* TryGetWindowFromVisual(uDWM::CVisual* visual)
	{
		auto current = visual->GetTransformParent();

		while (current && HookHelper::vftbl_of(current) != uDWM::CTopLevelWindow::vftable)
		{
			current = current->GetTransformParent();
		}

		return static_cast<uDWM::CTopLevelWindow*>(current);
	}

	inline auto g_projectionArray = make_projection_array(
		g_buildNumber,

		MAKE_FUNCTION_PROJECTION_TUPLE(CSolidColorLegacyMilBrushProxy::Update, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CImageLegacyMilBrushProxy::Update, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CVisual::UpdateOffset", os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetSize, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetInsetFromParent, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetInsetFromParentLeft, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::SetDirtyFlags, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::RenderRecursive, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisualProxy::SetClip, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisualProxy::SetSize, os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::Remove, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::RemoveAll, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(VisualCollection::InsertRelative, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CText::ValidateResources", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CText::InitializeVisualTreeClone", 0, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CDWriteText::ValidateVisual", os::build_w11_22h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDWriteText::InitializeVisualTreeClone", os::build_w11_22h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CSpriteVisual::SetSize", os::build_w11_22h2, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAtlasedRectsVisual::CloneVisualTree", 0, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE(CAtlasedRectsVisual::InitializeVisualTreeClone, 0, os::build_w11_22h2),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CButton::vftable, "CButton::`vftable'", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CButton::Create, 0, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CButton::SetVisualStates_Win10, "CButton::SetVisualStates", 0, os::build_w11_21h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CButton::SetVisualStates_Win11, "CButton::SetVisualStates", os::build_w11_21h2, os::build_w11_22h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CButton::UpdateCrossfade", 0, os::build_w11_21h2),

		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawGeometryInstruction::Create, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::Create, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::AddInstruction, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CRenderDataVisual::ClearInstructions, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::UpdateAccentPolicy", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAccent::_UpdateSolidFill", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CAccentBlurBehind::IsBlurBehindDirty", 0, os::build_w11_22h2),

		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowBorder::EnableBorder, os::build_w11_21h2, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelAtlasedRectsVisual::ShouldCloneAtlasImage", 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::vftable, "CTopLevelWindow::`vftable'", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::CloneVisualTreeForLivePreview_Win10, "CTopLevelWindow::CloneVisualTreeForLivePreview", 0, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CTopLevelWindow::CloneVisualTreeForLivePreview_Win11, "CTopLevelWindow::CloneVisualTreeForLivePreview", os::build_w11_22h2, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::GetActualWindowRect, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::TreatAsActiveWindow, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnClipUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnBlurBehindUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnAccentPolicyUpdated, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CTopLevelWindow::OnSystemBackdropUpdated, os::build_w11_21h2, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CTopLevelWindow::s_rgpwfWindowFrames, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateNCAreaBackground", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateNCAreaPositionsAndSizes", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateClientBlur", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::ValidateVisual", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::UpdateWindowVisuals", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::~CTopLevelWindow", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::IsShadowNCAreaPart", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::CreateBitmapFromAtlas", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CTopLevelWindow::CreateGlyphsFromAtlas", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("SetMargin", os::build_w11_21h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CLivePreview::_UpdateResources, os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_FadeOutToGlass", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CLivePreview::_UpdateInstructions", 0, os::build_w11_21h2),
		
		MAKE_EMPTY_PROJECTION_TUPLE("CAnimatedGlassSheet::OnRectUpdated", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CAnimatedGlassSheet::~CAnimatedGlassSheet", 0, os::build_w11_21h2),

		MAKE_EMPTY_PROJECTION_TUPLE("CGlassColorizationParameters::AdjustWindowColorization", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowList::GetWindowListForDesktop, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(CDesktopManager::s_pDesktopManagerInstance, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CDesktopManager::s_csDwmInstance, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDesktopManager::IsHighContrastMode", os::build_w11_21h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDesktopManager::ReleaseDXGIAdapter", 0, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CGraphicsDeviceManager::ReleaseGraphicsDevice", os::build_w11_21h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CCompositor::CreateSolidColorLegacyMilBrushProxy, "CCompositor::CreateProxy<CSolidColorLegacyMilBrushProxy>", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE_BY_ALIAS(CCompositor::CreateImageLegacyMilBrushProxy, "CCompositor::CreateProxy<CImageLegacyMilBrushProxy>", 0, 0),

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

		if (
			!strcmp(symbolName, "SetMargin") &&
			!strstr(info->Name, "HHHHPEBU1")
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
