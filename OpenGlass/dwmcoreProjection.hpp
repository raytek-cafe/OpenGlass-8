#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "ProjectionHelper.hpp"
#include "D2DPrivates.hpp"
#include "DWM.hpp"
#include "dwmcoreProjection.Offsets.hpp"

namespace OpenGlass::dwmcore
{
	using namespace DWM; 
	inline const auto g_moduleHandle{ GetModuleHandleW(L"dwmcore.dll") };
	inline const auto g_versionInfo{ Util::GetModuleVersionInfo(g_moduleHandle) };

	struct CResource : IUnknown {};
	struct IMILResource
	{
		virtual ULONG AddRef(void) = 0;
		virtual ULONG Release(void) = 0;
	};
	DECLSPEC_PROJECTION HRESULT CChannel::MatrixTransformUpdate(
		UINT handleId,
		MilMatrix3x2D* matrix
	)
	{
		return HANDLE_PROJECTION_FUNCTION(CChannel::MatrixTransformUpdate, this, handleId, matrix);
	}
	DECLSPEC_PROJECTION HRESULT CChannel::SolidColorLegacyMilBrushUpdate(
		UINT handleId,
		double opacity,
		const D2D1_COLOR_F& color,
		UINT opacityAnimationsHandleId,
		UINT transformHandleId,
		UINT relativeTransformHandleId
	)
	{
		return HANDLE_PROJECTION_FUNCTION(CChannel::SolidColorLegacyMilBrushUpdate, this, handleId, opacity, color, opacityAnimationsHandleId, transformHandleId, relativeTransformHandleId);
	}
	DECLSPEC_PROJECTION HRESULT CChannel::ImageLegacyMilBrushUpdate(
		UINT handleId,
		double opacity,
		const D2D1_RECT_F& viewport,
		const D2D1_RECT_F& viewbox,
		UINT opacityAnimationsHandleId,
		UINT transformHandleId,
		UINT relativeTransformHandleId,
		MilBrushMappingMode viewportUnits,
		MilBrushMappingMode viewboxUnits,
		UINT viewportAnimationsHandleId,
		UINT viewboxAnimationsHandleId,
		MilStretch stretchMode,
		MilTileMode tileMode,
		MilHorizontalAlignment alignmentX,
		MilVerticalAlignment alignmentY,
		UINT imageSourceHandleId
	)
	{
		return HANDLE_PROJECTION_FUNCTION(CChannel::ImageLegacyMilBrushUpdate, this, handleId, opacity, viewport, viewbox, opacityAnimationsHandleId, transformHandleId, relativeTransformHandleId, viewportUnits, viewboxUnits, viewportAnimationsHandleId, viewboxAnimationsHandleId, stretchMode, tileMode, alignmentX, alignmentY, imageSourceHandleId);
	}

	struct CMILMatrix;
	struct COcclusionContext;
	struct CRegion;
	struct CVisual;
	struct CDirtyRegion;
	struct CVisualTree : CResource {};
	struct CDirtyRegion;
	struct CDesktopTree : CVisualTree {};
	struct CVisual : CResource 
	{
		DECLSPEC_PROJECTION HWND GetTopLevelWindow() const
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::GetTopLevelWindow, this);
		}
	};
	struct CFloatResource : CResource {};

	struct CMILMatrix : D2D1_MATRIX_4X4_F
	{
		int flag;
		inline static const CMILMatrix* Identity{ nullptr };

		D2D1_MATRIX_3X2_F GetD2DMatrix() const
		{
			return D2D1::Matrix3x2F(
				_11, _12,
				_21, _22,
				_41, _42
			);
		}
		D2D1_MATRIX_4X4_F GetD3DMatrix() const
		{
			return D2D1::Matrix4x4F(
				_11, _12, _13, _14,
				_21, _22, _23, _24,
				_31, _32, _33, _34,
				_41, _42, _43, _44
			);
		}
	};
	struct CMatrixStack
	{
		DECLSPEC_PROJECTION const CMILMatrix* GetTopByReference() const
		{
			return HANDLE_PROJECTION_FUNCTION(CMatrixStack::GetTopByReference, this);
		}
	};

	struct CZOrderedRectBase
	{
		D2D1_RECT_F m_transformedRect;
		int m_depth;

		D2D1_RECT_F& GetOriginalRect();
		void UpdateDeviceRectInternal(
			const CMILMatrix* matrix,
			const D2D1_RECT_F& originalRect
		)
		{
			auto& transformedRect = m_transformedRect;
			if (matrix)
			{
				transformedRect = RectF::TransformRect(originalRect, matrix->GetD2DMatrix());
			}
			else
			{
				transformedRect = originalRect;
			}
			transformedRect.left = std::ceil(transformedRect.left);
			transformedRect.top = std::ceil(transformedRect.top);
			transformedRect.right = std::floor(transformedRect.right);
			transformedRect.bottom = std::floor(transformedRect.bottom);
		}
		void UpdateDeviceRect(const CMILMatrix* matrix)
		{
			UpdateDeviceRectInternal(
				matrix,
				GetOriginalRect()
			);
		}
		CZOrderedRectBase() = default;
		CZOrderedRectBase(int depth) : m_depth{ depth } {}
	};
	struct CZOrderedRect : CZOrderedRectBase
	{
		D2D1_RECT_F m_originalRect;

		void UpdateDeviceRect(const CMILMatrix* matrix)
		{
			UpdateDeviceRectInternal(
				matrix,
				m_originalRect
			);
		}
		CZOrderedRect() = default;
		CZOrderedRect(const D2D1_RECT_F& rect, int depth, const CMILMatrix* matrix) : CZOrderedRectBase{ depth }, m_originalRect{ rect }
		{
			UpdateDeviceRect(matrix);
		}
	};
	struct CZOrderedRect2 : CZOrderedRectBase
	{
		CVisual* m_visual;
		D2D1_RECT_F m_originalRect;

		void UpdateDeviceRect(const CMILMatrix* matrix)
		{
			UpdateDeviceRectInternal(
				matrix,
				m_originalRect
			);
		}
		CZOrderedRect2() = default;
		CZOrderedRect2(const D2D1_RECT_F& rect, int depth, const CMILMatrix* matrix) : CZOrderedRectBase{ depth }, m_originalRect{ rect }
		{
			UpdateDeviceRect(matrix);
		}
	};
	inline D2D1_RECT_F& CZOrderedRectBase::GetOriginalRect()
	{
		return *reinterpret_cast<D2D1_RECT_F*>(
			reinterpret_cast<ULONG_PTR>(this) +
			(
				Util::VersionBefore<os::build_w11_24h2, os::revision_24h2_with_25h2_code_staged>(g_versionInfo.build, g_versionInfo.revision) ?
				(offsetof(CZOrderedRect, m_originalRect)) :
				(offsetof(CZOrderedRect2, m_originalRect))
			)
		);
	}

	struct CArrayBasedCoverageSet : CResource
	{
		DECLSPEC_PROJECTION DynArray<CZOrderedRect>* GetAntiOccluderArray() const
		{
			return Util::PointerExecuteUnsafe<CArrayBasedCoverageSet_GetAntiOccluderArray_Offsets, Util::OffsetBy<DynArray<CZOrderedRect>*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DynArray<CZOrderedRect>* GetOccluderArray() const
		{
			return Util::PointerExecuteUnsafe<CArrayBasedCoverageSet_GetOccluderArray_Offsets, Util::OffsetBy<DynArray<CZOrderedRect>*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DynArray<CZOrderedRect2>* GetOccluderArray2() const
		{
			return reinterpret_cast<DynArray<CZOrderedRect2>*>(const_cast<CArrayBasedCoverageSet*>(this));
		}

		bool IsCovered(
			const D2D1_RECT_F& coverage,
			int depth
		) const
		{
			bool antiOccluderExisted{ false };
			int antiOccluderDepth{};
			if (g_versionInfo.build < os::build_server_2022)
			{
				for (const auto& zorderedRect : GetAntiOccluderArray()->views())
				{
					if (zorderedRect.m_depth >= depth)
					{
						break;
					}

					if (
						!wil::rect_is_empty(zorderedRect.m_transformedRect) &&
						std::abs(wil::rect_height(zorderedRect.m_transformedRect) * wil::rect_width(zorderedRect.m_transformedRect)) > 1.f &&

						RectF::DoesIntersectUnsafe(zorderedRect.m_transformedRect, coverage)
					)
					{
						antiOccluderDepth = zorderedRect.m_depth;
						antiOccluderExisted = true;
						break;
					}
				}
			}

			const auto checkArrayBasedCoverageSet = [&coverage, depth, antiOccluderExisted, antiOccluderDepth](auto&& views)
			{
				auto visibleRect = coverage;

				for (const auto& zorderedRect : views)
				{
					if (zorderedRect.m_depth >= depth)
					{
						break;
					}

					if (
						!wil::rect_is_empty(zorderedRect.m_transformedRect) &&
						(!antiOccluderExisted || zorderedRect.m_depth > antiOccluderDepth)
					)
					{
						if (
							zorderedRect.m_transformedRect.left <= visibleRect.left &&
							zorderedRect.m_transformedRect.right >= visibleRect.right
						)
						{
							if (visibleRect.top >= zorderedRect.m_transformedRect.top)
							{
								if (zorderedRect.m_transformedRect.bottom >= visibleRect.bottom)
								{
									return true;
								}
								if (zorderedRect.m_transformedRect.bottom > visibleRect.top)
								{
									visibleRect.top = zorderedRect.m_transformedRect.bottom;
								}
							}
							else if (zorderedRect.m_transformedRect.bottom >= coverage.bottom && coverage.bottom > zorderedRect.m_transformedRect.top)
							{
								visibleRect.bottom = zorderedRect.m_transformedRect.top;
							}
						}
					}
				}

				return false;
			};

			if (Util::VersionBefore<os::build_w11_24h2, os::revision_24h2_with_25h2_code_staged>(g_versionInfo.build, g_versionInfo.revision))
			{
				return checkArrayBasedCoverageSet(GetOccluderArray()->views());
			}
			else
			{
				return checkArrayBasedCoverageSet(GetOccluderArray2()->views());
			}
		}
	};

	struct CRenderCommand
	{
		UINT type;
	};
	struct CDrawGeometryCommand : CRenderCommand
	{
		UINT brushIndex;
		UINT geometryIndex;
	};
	struct CRenderDataBuilder : CResource {};
	struct CRenderData : CResource
	{
		using CRenderDataResourceArray = DynArray<CResource*>;
		DECLSPEC_PROJECTION CRenderDataResourceArray* GetResources() const
		{
			return Util::PointerExecuteUnsafe<CRenderData_GetResources_Offsets, Util::OffsetBy<CRenderDataResourceArray*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CLegacyMilBrush : CResource {};
	struct CSolidColorLegacyMilBrush : CLegacyMilBrush
	{
		DECLSPEC_PROJECTION const D2D1_COLOR_F& GetRealizedColor() const
		{
			return *Util::PointerExecuteUnsafe<CSolidColorLegacyMilBrush_GetRealizedColor_Offsets, Util::OffsetBy<D2D1_COLOR_F*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		inline static PVOID* vftable{ nullptr };
	};
	struct CImageSource : CResource {};
	struct CImageLegacyMilBrush : CLegacyMilBrush
	{
		inline static PVOID* vftable{ nullptr };

		DECLSPEC_PROJECTION CImageSource* GetImageSource() const
		{
			return Util::PointerExecuteUnsafe<CImageLegacyMilBrush_GetImageSource_Offsets, Util::DereferenceAt<CImageSource*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION float GetOpacityValue() const
		{
			if (g_versionInfo.build < os::build_w10_1903)
			{
				return static_cast<float>(Util::PointerExecuteUnsafe<CImageLegacyMilBrush_GetOpacityValue_Double_Offsets, Util::DereferenceAt<double>>(this, g_versionInfo.build, g_versionInfo.revision));
			}
			return Util::PointerExecuteUnsafe<CImageLegacyMilBrush_GetOpacityValue_Offsets, Util::DereferenceAt<float>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CFloatResource* GetFloatResource() const
		{
			return Util::PointerExecuteUnsafe<CImageLegacyMilBrush_GetFloatResource_Offsets, Util::DereferenceAt<CFloatResource*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION const D2D1_RECT_F& GetViewport() const
		{
			return *Util::PointerExecuteUnsafe<CImageLegacyMilBrush_GetViewport_Offsets, Util::OffsetBy<D2D1_RECT_F*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION const D2D1_RECT_F& GetViewbox() const
		{
			return *Util::PointerExecuteUnsafe<CImageLegacyMilBrush_GetViewbox_Offsets, Util::OffsetBy<D2D1_RECT_F*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};

	struct CTransform : CResource {};
	struct CShape
	{
		virtual ~CShape() = default;
		virtual UINT GetType() const = 0;
		virtual bool IsEmpty() const = 0;
		virtual HRESULT GetD2DGeometry(const CMILMatrix* matrix, ID2D1Geometry** geometry) const = 0;

		DECLSPEC_PROJECTION HRESULT CopyShape(
			const CMILMatrix* matrix,
			CShape** shape
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(CShape::CopyShape, this, matrix, shape);
		}
		DECLSPEC_PROJECTION static HRESULT Combine(
			const CShape* shape1,
			const CMILMatrix* matrix1,
			const CShape* shape2,
			const CMILMatrix* matrix2,
			D2D1_COMBINE_MODE mode,
			CShape** shape
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CShape::Combine, shape1, matrix1, shape2, matrix2, mode, shape);
		}

		DECLSPEC_PROJECTION HRESULT GetTightBounds(D2D1_RECT_F* lprc, const CMILMatrix* matrix) const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CShape_GetTightBounds_Offsets, Util::DereferenceAt<decltype(&CShape::GetTightBounds)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this,
				lprc,
				matrix
			);
		}
		DECLSPEC_PROJECTION bool IsRectangles(UINT* count) const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CShape_IsRectangles_Offsets, Util::DereferenceAt<decltype(&CShape::IsRectangles)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this,
				count
			);
		}
		DECLSPEC_PROJECTION bool GetRectangles(D2D1_RECT_F* buffer, UINT count) const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CShape_GetRectangles_Offsets, Util::DereferenceAt<decltype(&CShape::GetRectangles)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this,
				buffer,
				count
			);
		}
	};
	struct CRectanglesShape : CShape {};
	struct CRegionShape : CShape
	{
		static inline PVOID* vftable{ nullptr };
		static inline PVOID dtor{ nullptr };

		DECLSPEC_PROJECTION HRESULT BuildFromRects(const D2D1_RECT_L* buffer, UINT count)
		{
			return HANDLE_PROJECTION_FUNCTION(CRegionShape::BuildFromRects, this, buffer, count);
		}
	};
	// valid until (build, revision)
	template <ULONG build = 0, ULONG revision = 0>
	struct CRegionShapeImpl
	{
		static_assert(false, "Not implemented.");
	};
	template <>
	struct CRegionShapeImpl<os::build_w10_2004, 0>
	{
		void* vftable;
		void* unknown;
		char regionData[72]; // FastRegion::CRegion
		ID2D1Geometry* geometry;

		CRegionShapeImpl()
		{
			memset(this, 0, sizeof(*this));
			vftable = CRegionShape::vftable;
			*(ULONG_PTR*)(reinterpret_cast<ULONG_PTR>(this) + 16) = reinterpret_cast<ULONG_PTR>(this) + 24;
			*(DWORD*)(reinterpret_cast<ULONG_PTR>(this) + 24) = 0;
			geometry = 0;
		}
		~CRegionShapeImpl()
		{
			std::invoke(
				Util::force_cast_to<void(CRegionShape::*)()>(CRegionShape::dtor),
				reinterpret_cast<CRegionShape*>(this)
			);
		}
		CRegionShape* As()
		{
			return reinterpret_cast<CRegionShape*>(this);
		}
	};

	class CShapePtr
	{
		CShape* m_ptr{ nullptr };
		bool m_release{ true };
	public:
		CShape* operator->() const
		{
			return m_ptr;
		}
		CShape* get() const
		{
			return m_ptr;
		}
		CShape** put()
		{
			Release();
			return &m_ptr;
		}
		explicit operator bool() const
		{
			return m_ptr != nullptr;
		}
		void Release()
		{
			if (m_release && m_ptr)
			{
				std::invoke(
					**reinterpret_cast<void(CShape::***)(BYTE)>(m_ptr),
					m_ptr,
					true
				);
			}

			m_ptr = nullptr;
		}

		CShapePtr() noexcept = default;
		CShapePtr(CShape* src, bool release = true) noexcept : m_ptr{ src }, m_release{ !release } {}
		CShapePtr(const CShapePtr&) noexcept = delete;
		CShapePtr(CShapePtr&& src) noexcept
		{
			Release();
			m_ptr = src.m_ptr;
			m_release = src.m_release;
			src.m_ptr = nullptr;
		}
		~CShapePtr() noexcept
		{
			Release();
		}
	};
	struct CGeometry : CResource 
	{
		DECLSPEC_PROJECTION	HRESULT GetShapeData(
			const D2D1_SIZE_F* size,
			CShapePtr* shape
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CGeometry::GetShapeData, this, size, shape);
		}
	};
	struct CRectangleGeometry : CGeometry
	{
		inline static PVOID* vftable{ nullptr };
	};
	struct CGeometry2D : CGeometry {};
	struct CDrawingContext;
	struct COcclusionContext;
	struct IDrawingContext
	{
		virtual HRESULT Clear(const D2D1_COLOR_F& color) = 0;
		virtual HRESULT DrawRectangle(const D2D1_RECT_F& lprc, CLegacyMilBrush* brush, CResource* resource) = 0;
		virtual HRESULT DrawSolidRectangle(const D2D1_RECT_F& lprc, const D2D1_COLOR_F& color) = 0;
		virtual HRESULT DrawImage(CResource* image, const D2D1_RECT_F* lprc, CResource* resource) = 0;
		virtual HRESULT DrawGeometry(CLegacyMilBrush* brush, CGeometry* geometry) = 0;
		virtual HRESULT TileImage(CResource* image, D2D1_RECT_F& lprc, D2D1_POINT_2F& point, float) = 0;
		virtual HRESULT DrawBitmap(CResource* bitmap) = 0;
		virtual HRESULT DrawInk(ID2D1Ink* ink, const D2D1_COLOR_F& color, ID2D1InkStyle* inkStyle) = 0;
		virtual HRESULT DrawGenericInk(struct IDCompositionDirectInkWetStrokePartner*, bool) = 0;
		virtual HRESULT DrawYCbCrBitmap(CResource*, CResource*, D2D1_YCBCR_CHROMA_SUBSAMPLING) = 0;
		virtual HRESULT DrawMesh2D(CGeometry2D* geometry, CImageSource* imageSource) = 0;
		virtual HRESULT DrawVisual(CVisual* visual) = 0;
		virtual HRESULT Pop() = 0;
		virtual HRESULT PushTransform(CTransform* transfrom) = 0;
		virtual HRESULT ApplyRenderState() = 0;

		DECLSPEC_PROJECTION CDrawingContext* GetDrawingContext() const
		{
			return Util::PointerExecuteUnsafe<IDrawingContext_GetDrawingContext_Offsets, Util::OffsetBy<CDrawingContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		COcclusionContext* GetOcclusionContext() const
		{
			return reinterpret_cast<COcclusionContext*>(reinterpret_cast<ULONG_PTR>(this));
		}
	};

	struct RenderTargetInfo
	{
		// since windows 10 22h2
		DECLSPEC_PROJECTION float GetSDRBoost() const
		{
			return *Util::PointerExecuteUnsafe<RenderTargetInfo_GetSDRBoost_Offsets, Util::OffsetBy<float*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct ID2DContextOwner
	{
		DECLSPEC_PROJECTION UINT GetCurrentZ() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<ID2DContextOwner_GetCurrentZ_Offsets, Util::DereferenceAt<decltype(&ID2DContextOwner::GetCurrentZ)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
		DECLSPEC_PROJECTION const RenderTargetInfo& GetCurrentRenderTargetInfo() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<ID2DContextOwner_GetCurrentRenderTargetInfo_Offsets, Util::DereferenceAt<decltype(&ID2DContextOwner::GetCurrentRenderTargetInfo)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
	};
	
	enum class DisplayId : DWORD;
	struct CD2DContext : CResource
	{
		DECLSPEC_PROJECTION void EnsureBeginDraw()
		{
			return HANDLE_PROJECTION_FUNCTION(CD2DContext::EnsureBeginDraw, this);
		}
		DECLSPEC_PROJECTION ID2D1DeviceContext* GetDeviceContext() const
		{
			return Util::PointerExecuteUnsafe<CD2DContext_GetDeviceContext_Offsets, Util::DereferenceAt<ID2D1DeviceContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	// CD3DDeviceLevel1 before windows 10 2004
	struct CD3DDevice
	{
		DECLSPEC_PROJECTION ID3D11Device* GetDevice() const
		{
			return Util::PointerExecuteUnsafe<CD3DDevice_GetDevice_Offsets, Util::DereferenceAt<ID3D11Device*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION ID3D11DeviceContext* GetImmediateContext() const
		{
			return Util::PointerExecuteUnsafe<CD3DDevice_GetImmediateContext_Offsets, Util::DereferenceAt<ID3D11DeviceContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		CD2DContext* GetD2DContext() const
		{
			return Util::PointerExecuteUnsafe<CD3DDevice_GetD2DContext_Offsets, Util::OffsetBy<CD2DContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct IBitmapResource;
	struct IDeviceTexture
	{
		DECLSPEC_PROJECTION ID3D11Texture2D* GetTexture2D() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IDeviceTexture_GetTexture2D_Offsets, Util::DereferenceAt<decltype(&IDeviceTexture::GetTexture2D)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
		DECLSPEC_PROJECTION ID3D11ShaderResourceView* GetShaderResourceView() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IDeviceTexture_GetShaderResourceView_Offsets, Util::DereferenceAt<decltype(&IDeviceTexture::GetShaderResourceView)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
	};
	struct CD3DSurface
	{
		DECLSPEC_PROJECTION ID3D11Texture2D* GetTexture2D() const
		{
			return Util::PointerExecuteUnsafe<CD3DSurface_GetTexture2D_Offsets, Util::DereferenceAt<ID3D11Texture2D*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION ID3D11ShaderResourceView* GetShaderResourceView() const
		{
			return Util::PointerExecuteUnsafe<CD3DSurface_GetShaderResourceView_Offsets, Util::DereferenceAt<ID3D11ShaderResourceView*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION ID3D11RenderTargetView* GetRenderTargetView() const
		{
			return Util::PointerExecuteUnsafe<CD3DSurface_GetRenderTargetView_Offsets, Util::DereferenceAt<ID3D11RenderTargetView*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct IRenderTarget
	{
		DECLSPEC_PROJECTION CD3DSurface* GetTargetSurfaceNoRef() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IRenderTarget_GetTargetSurfaceNoRef_Offsets, Util::DereferenceAt<decltype(&IRenderTarget::GetTargetSurfaceNoRef)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
		DECLSPEC_PROJECTION float GetSDRBoost() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IRenderTarget_GetSDRBoost_Offsets, Util::DereferenceAt<decltype(&IRenderTarget::GetSDRBoost)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
	};
	struct IDeviceTarget
	{
		DECLSPEC_PROJECTION ID3D11RenderTargetView* GetRenderTargetView() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IDeviceTarget_GetRenderTargetView_Offsets, Util::DereferenceAt<decltype(&IDeviceTarget::GetRenderTargetView)>>(HookHelper::get_vftable_from(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
		DECLSPEC_PROJECTION IDeviceTexture* GetDeviceTexture() const
		{
			return Util::PointerExecuteUnsafe<IDeviceTarget_GetDeviceTexture_Offsets, Util::OffsetBy<IDeviceTexture*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CComposeTop;
	struct CHwndRenderTarget : CResource {};
	struct COverlayContext : CResource
	{
		struct OverlayPlaneInfo;
	};
	struct CDrawingContext
	{
		// since windows 10 2004
		IDeviceTarget* GetDeviceTarget() const
		{
			return reinterpret_cast<IDeviceTarget* const*>(this)[4];
		}
		// before windows 10 2004
		IRenderTarget* GetRenderTarget() const
		{
			return reinterpret_cast<IRenderTarget* const*>(this)[44];
		}
		CD3DDevice* GetD3DDevice() const
		{
			return *Util::PointerExecuteUnsafe<CDrawingContext_GetD3DDevice_Offsets, Util::OffsetBy<CD3DDevice**>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION IDrawingContext* GetInterface() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetInterface_Offsets, Util::OffsetBy<IDrawingContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION ID2DContextOwner* GetD2DContextOwner() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetD2DContextOwner_Offsets, Util::OffsetBy<ID2DContextOwner*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION COcclusionContext* GetOcclusionContext() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetOcclusionContext_Offsets, Util::DereferenceAt<COcclusionContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		
		DECLSPEC_PROJECTION const CMILMatrix* GetWorldTransform() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetWorldTransform_Offsets, Util::OffsetBy<CMatrixStack*>>(this, g_versionInfo.build, g_versionInfo.revision)->GetTopByReference();
		}
		DECLSPEC_PROJECTION CMILMatrix* GetDeviceTransform() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetDeviceTransform_Offsets, Util::OffsetBy<CMILMatrix*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION bool IsBounding() const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::IsBounding, this);
		}
		DECLSPEC_PROJECTION bool CalcPartiallyVisibleRectangleSet(
			const D2D1_RECT_F& bounds,
			int depth,
			D2D1_RECT_F* rectangles[4],
			UINT* count
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::CalcPartiallyVisibleRectangleSet, this, bounds, depth, rectangles, count);
		}
		DECLSPEC_PROJECTION HRESULT GetUnOccludedWorldShape(const CShape* shape, int depth, CShape** worldShape) const
		{
			const auto worldTransform = GetWorldTransform();
			D2D1_RECT_F bounds{};
			RETURN_IF_FAILED(shape->GetTightBounds(&bounds, nullptr));

			D2D1_RECT_F rectanglesF[4]{};
			UINT count{};
			if (
				D2D1_RECT_F* buffer[4]{ &rectanglesF[0], &rectanglesF[1], &rectanglesF[2], &rectanglesF[3] };
				!CalcPartiallyVisibleRectangleSet(bounds, depth, buffer, &count)
			)
			{
				return MILERR_GENERIC_IGNORE;
			}

			CRegionShapeImpl<os::build_w10_2004> regionShape{};
			D2D1_RECT_L rectanglesL[4]{};
			for (uint32_t i = 0; i < count; i++)
			{
				rectanglesL[i] = RectF::ToRectL(rectanglesF[i]);
			}
			RETURN_IF_FAILED(regionShape.As()->BuildFromRects(rectanglesL, count));

			return CShape::Combine(
				regionShape.As(),
				nullptr,
				shape,
				worldTransform,
				D2D1_COMBINE_MODE_INTERSECT,
				worldShape
			);
		}
		DECLSPEC_PROJECTION void CalcWorldSpaceClippedBounds(const D2D1_RECT_F& rect, D2D_RECT_F* clippedBounds) const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::CalcWorldSpaceClippedBounds, this, rect, clippedBounds);
		}
		DECLSPEC_PROJECTION void GetClipBoundsWorld(D2D1_RECT_F& rect) const
		{
			CalcWorldSpaceClippedBounds({ -INFINITY, -INFINITY, INFINITY, INFINITY }, &rect);
		}
		DECLSPEC_PROJECTION HRESULT FillShapeWithBrush(const CShape* shape, const ID2D1Brush* brush)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::FillShapeWithBrush, this, shape, brush);
		}
		DECLSPEC_PROJECTION HRESULT FlushD2D()
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::FlushD2D, this);
		}
		DECLSPEC_PROJECTION HRESULT ApplyRenderStateInternal(bool unknown)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::ApplyRenderStateInternal, this, unknown);
		}
		DECLSPEC_PROJECTION CVisual* GetCurrentVisual() const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::GetCurrentVisual, this);
		}

		CVisual* GetCurrentVisualHelper() const
		{
			return (g_versionInfo.build >= os::build_w11_24h2 ? this : reinterpret_cast<dwmcore::CDrawingContext*>(GetD2DContextOwner()))->GetCurrentVisual();
		}
		float GetSDRBoost() const
		{
			if (g_versionInfo.build < os::build_w10_2004)
			{
				return GetRenderTarget()->GetSDRBoost();
			}
			else
			{
				return GetD2DContextOwner()->GetCurrentRenderTargetInfo().GetSDRBoost();
			}
		}
	};
	enum OverlaySize : int {};
	struct COcclusionContext : IDrawingContext
	{
		// not needed before windows 10 2004
		DECLSPEC_PROJECTION ULONGLONG GetFrameId() const
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetFrameId_Offsets, Util::DereferenceAt<ULONGLONG>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION UINT GetCurrentZ() const
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetCurrentZ_Offsets, Util::DereferenceAt<UINT>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION const CMILMatrix* GetWorldTransform() const
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetWorldTransform_Offsets, Util::OffsetBy<CMatrixStack*>>(this, g_versionInfo.build, g_versionInfo.revision)->GetTopByReference();
		}
		DECLSPEC_PROJECTION const CMILMatrix* GetDeviceTransform() const
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetDeviceTransform_Offsets, Util::OffsetBy<CMILMatrix const*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION UINT* GetDeviceTransformFlag()
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetDeviceTransformFlag_Offsets, Util::OffsetBy<UINT*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION UINT GetDeviceTransformFlagValue() const
		{
			return *Util::PointerExecuteUnsafe<COcclusionContext_GetDeviceTransformFlag_Offsets, Util::OffsetBy<UINT*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		D2D1_RECT_F PageInPixelsRectToDeviceRect(const D2D1_RECT_F& pixelsRect) const
		{
			auto result = pixelsRect;

			if (GetDeviceTransformFlagValue() & 0x1)
			{
				result = RectF::TransformRect(pixelsRect, GetDeviceTransform()->GetD2DMatrix());
			}

			return result;
		}
		DECLSPEC_PROJECTION CArrayBasedCoverageSet* GetArrayBasedCoverageSet() const
		{
			const auto result = Util::PointerExecuteUnsafe<COcclusionContext_GetArrayBasedCoverageSet_Offsets, Util::OffsetBy<CArrayBasedCoverageSet* const>>(this, g_versionInfo.build, g_versionInfo.revision);

			// stored as a pointer before
			if (g_versionInfo.build < os::build_w10_2004)
			{
				return *reinterpret_cast<CArrayBasedCoverageSet* const*>(result);
			}

			return result;
		}
		DECLSPEC_PROJECTION HRESULT SetDeviceTransform(
			const dwmcore::CMILMatrix* matrix
		)
		{
			return HANDLE_PROJECTION_FUNCTION(COcclusionContext::SetDeviceTransform, this, matrix);
		}
	};
	struct CDirtyRegion
	{
		// not exist before windows 10 2004
		DECLSPEC_PROJECTION COcclusionContext* GetOcclusionContext() const
		{
			return Util::PointerExecuteUnsafe<CDirtyRegion_GetOcclusionContext_Offsets, Util::OffsetBy<COcclusionContext* const>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CTreeDirty {};
	struct CWindowOcclusionInfo : CResource {};
	struct CWindowNode : CResource
	{
		DECLSPEC_PROJECTION HWND GetHwnd() const
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowNode::GetHwnd, this);
		}
	};
	struct CCachedVisualImage : CResource
	{
		struct CCachedTarget : CResource
		{
		};
	};
	struct CDrawListCache : CResource {};
	struct CDrawListEntryBuilder : CResource {};

	DECLSPEC_PROJECTION ULONGLONG GetCurrentFrameId()
	{
		return HANDLE_PROJECTION_FUNCTION(GetCurrentFrameId);
	}

	inline auto g_projectionArray = make_projection_array(
		g_versionInfo.build,

		MAKE_FUNCTION_PROJECTION_TUPLE(CChannel::MatrixTransformUpdate, 0, os::build_w11_22h2),
		MAKE_FUNCTION_PROJECTION_TUPLE(CChannel::SolidColorLegacyMilBrushUpdate, 0, os::build_w10_1903),
		MAKE_FUNCTION_PROJECTION_TUPLE(CChannel::ImageLegacyMilBrushUpdate, 0, os::build_w10_1903),

		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::GetTopLevelWindow, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(CMILMatrix::Identity, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CMatrixStack::GetTopByReference, 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CShape::CopyShape, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CShape::Combine, 0, os::build_w10_2004),

		MAKE_FUNCTION_PROJECTION_TUPLE(CRegionShape::BuildFromRects, 0, os::build_w10_2004),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CRegionShape::vftable, "CRegionShape::`vftable'", 0, os::build_w10_2004),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CRegionShape::dtor, "CRegionShape::~CRegionShape", 0, os::build_w10_2004),

		MAKE_EMPTY_PROJECTION_TUPLE("CRenderDataBuilder::DrawGeometry", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CRenderData::TryDrawCommandAsDrawList", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CRenderData::DrawImageResource_FillMode", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CGeometry::GetShapeData, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CRectangleGeometry::vftable, "CRectangleGeometry::`vftable'", 0, 0),
		
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CSolidColorLegacyMilBrush::vftable, "CSolidColorLegacyMilBrush::`vftable'", 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CImageLegacyMilBrush::vftable, "CImageLegacyMilBrush::`vftable'", 0, 0),

		MAKE_OPTIONAL_FUNCTION_PROJECTION_TUPLE(CArrayBasedCoverageSet::IsCovered, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CD2DContext::DestroyDeviceResources", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CD2DContext::EnsureBeginDraw, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::IsBounding, 0, os::build_w10_2004),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::CalcPartiallyVisibleRectangleSet, 0, os::build_w10_2004),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetUnOccludedWorldShape, os::build_w10_2004, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::CalcWorldSpaceClippedBounds, 0, os::build_w10_2004),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetClipBoundsWorld, os::build_w10_2004, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FillShapeWithBrush, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FlushD2D, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::ApplyRenderStateInternal, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetCurrentVisual, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDrawingContext::DrawVisualTree", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDrawingContext::PreSubgraph", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("COverlayContext::EndOverlayCandidateCollection", 0, os::build_w11_24h2),
		MAKE_EMPTY_PROJECTION_TUPLE("COverlayContext::IsCandidateOverlayCompatible", os::build_w11_24h2, 0),
		
		MAKE_FUNCTION_PROJECTION_TUPLE(COcclusionContext::SetDeviceTransform, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::IsOccluded", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::Compute", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::PageInPixelsRectToDeviceRect", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::~COcclusionContext", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CHwndRenderTarget::RenderDirtyRegion", os::build_w10_1903, os::build_w10_2004),

		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetUnOccludedDirtyRegion", 0, os::build_w10_2004),
		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetUnOccludedDirtyRect", os::build_w10_2004, os::build_server_2022),
		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetOptimizedRect", os::build_server_2022, os::build_w11_24h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CTreeDirty::GetOptimizedRect", os::build_w11_24h2, 0),
		
#ifdef _DEBUG
		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowNode::GetHwnd, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CWindowNode::RenderImage", 0, 0),
#endif

		MAKE_EMPTY_PROJECTION_TUPLE("CCachedVisualImage::RenderTargetBitmapInfo::Update", 0, os::build_w10_2004),
		MAKE_EMPTY_PROJECTION_TUPLE("CCachedVisualImage::CCachedTarget::Update", os::build_w10_2004, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(GetCurrentFrameId, 0, 0)
	);
	
	inline bool SymbolParserCallback(PSYMBOL_INFO info, [[maybe_unused]] ULONG size)
	{
		CHAR symbolName[128]{};
		UnDecorateSymbolName(info->Name, symbolName, std::size(symbolName), UNDNAME_NAME_ONLY);

		if (
			!strcmp(symbolName, "CRenderData::DrawImageResource_FillMode") &&
			!strstr(info->Name, "PEBUD2D_RECT_F")
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
