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
		virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
		virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
	};
	struct CChannel : IDwmChannel
	{
		IDwmChannelProvider* GetProvider() const
		{
			return reinterpret_cast<IDwmChannelProvider* const*>(this)[8];
		}

		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE MatrixTransformUpdate(
			UINT handleId,
			MilMatrix3x2D* matrix
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CChannel::MatrixTransformUpdate, handleId, matrix);
		}
	};

	struct CMILMatrix;
	struct COcclusionContext;
	struct CRegion;
	struct CVisual;
	struct CDirtyRegion;
	struct CVisualTree : CResource {};
	struct CDirtyRegion;
	struct CDesktopTree : CVisualTree 
	{
		inline static PVOID* vftable{ nullptr };
	};
	struct CVisual : CResource 
	{
		DECLSPEC_PROJECTION HWND STDMETHODCALLTYPE GetTopLevelWindow() const
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::GetTopLevelWindow);
		}
		DECLSPEC_PROJECTION CDesktopTree* STDMETHODCALLTYPE GetDesktopTree() const
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::GetDesktopTree);
		}
	};
	struct CFloatResource : CResource {};

	struct CMILMatrix : D2D1_MATRIX_4X4_F
	{
		int flag;
		inline static const CMILMatrix* Identity{ nullptr };

		DECLSPEC_PROJECTION void STDMETHODCALLTYPE Multiply(const CMILMatrix& matrix) const
		{
			return HANDLE_PROJECTION_FUNCTION(CMILMatrix::Multiply, matrix);
		}
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
		DECLSPEC_PROJECTION const CMILMatrix* STDMETHODCALLTYPE GetTopByReference() const
		{
			return HANDLE_PROJECTION_FUNCTION(CMatrixStack::GetTopByReference);
		}
	};
	struct CBaseClipStack
	{
		DECLSPEC_PROJECTION D2D1_RECT_F STDMETHODCALLTYPE Clip(
			const D2D1_RECT_F& inputRect
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(
				CBaseClipStack::Clip,
				inputRect
			);
		}
	};

	struct CZOrderedRect
	{
		D2D1_RECT_F m_transformedRect;
		int m_depth;
		D2D1_RECT_F m_originalRect;

		void STDMETHODCALLTYPE UpdateDeviceRect(const CMILMatrix* matrix)
		{
			if (matrix)
			{
				m_transformedRect = RectF::TransformRect(m_originalRect, matrix->GetD2DMatrix());
			}
			else
			{
				m_transformedRect = m_originalRect;
			}

			m_transformedRect.left = std::ceil(m_transformedRect.left);
			m_transformedRect.top = std::ceil(m_transformedRect.top);
			m_transformedRect.right = std::floor(m_transformedRect.right);
			m_transformedRect.bottom = std::floor(m_transformedRect.bottom);
		}
		CZOrderedRect() = default;
		CZOrderedRect(const D2D1_RECT_F& rect, int depth, const CMILMatrix* matrix) : m_depth{ depth }, m_originalRect{ rect }
		{
			UpdateDeviceRect(matrix);
		}
	};
	struct CZOrderedRect2
	{
		D2D1_RECT_F m_transformedRect;
		int m_depth;
		CVisual* m_visual;
		D2D1_RECT_F m_originalRect;

		void STDMETHODCALLTYPE UpdateDeviceRect(const CMILMatrix* matrix)
		{
			if (matrix)
			{
				m_transformedRect = RectF::TransformRect(m_originalRect, matrix->GetD2DMatrix());
			}
			else
			{
				m_transformedRect = m_originalRect;
			}

			m_transformedRect.left = std::ceil(m_transformedRect.left);
			m_transformedRect.top = std::ceil(m_transformedRect.top);
			m_transformedRect.right = std::floor(m_transformedRect.right);
			m_transformedRect.bottom = std::floor(m_transformedRect.bottom);
		}
		CZOrderedRect2() = default;
		CZOrderedRect2(const D2D1_RECT_F& rect, int depth, const CMILMatrix* matrix) : m_depth{ depth }, m_originalRect{ rect }
		{
			UpdateDeviceRect(matrix);
		}
	};
	struct CArrayBasedCoverageSet : CResource
	{
		DECLSPEC_PROJECTION DynArray<CZOrderedRect>* GetAntiOccluderArray() const
		{
			return Util::PointerExecuteUnsafe<CArrayBasedCoverageSet_GetAntiOccluderArray_Offsets, Util::OffsetBy<DynArray<CZOrderedRect>*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DynArray<CZOrderedRect>* GetOccluderArray() const
		{
			return reinterpret_cast<DynArray<CZOrderedRect>*>(const_cast<CArrayBasedCoverageSet*>(this));
		}
		DynArray<CZOrderedRect2>* GetOccluderArray2() const
		{
			return reinterpret_cast<DynArray<CZOrderedRect2>*>(const_cast<CArrayBasedCoverageSet*>(this));
		}

		DECLSPEC_PROJECTION COcclusionContext* GetOcclusionContext() const
		{
			return Util::PointerExecuteUnsafe<CArrayBasedCoverageSet_GetOcclusionContext_Offsets, Util::OffsetBy<COcclusionContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		bool STDMETHODCALLTYPE IsCovered(
			const D2D1_RECT_F& coverage,
			int depth
		) const
		{
			bool antiOccluderExisted{ false };
			int antiOccluderDepth{};
			if (g_versionInfo.build < os::build_w11_21h2)
			{
				for (const auto& zorderedRect : GetAntiOccluderArray()->views())
				{
					if (zorderedRect.m_depth >= depth)
					{
						break;
					}

					if (
						!wil::rect_is_empty(zorderedRect.m_transformedRect) &&
						std::fabs(wil::rect_height(zorderedRect.m_transformedRect) * wil::rect_width(zorderedRect.m_transformedRect)) > 1.f &&

						RectF::DoesIntersectUnsafe(zorderedRect.m_transformedRect, coverage)
					)
					{
						antiOccluderDepth = zorderedRect.m_depth;
						antiOccluderExisted = true;
						break;
					}
				}
			}

			auto actualCoverage = coverage;
			if (Util::VersionBefore<os::build_w11_24h2, os::revision_24h2_with_25h2_code_staged>(g_versionInfo.build, g_versionInfo.revision))
			{
				for (const auto& zorderedRect : GetOccluderArray()->views())
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
							zorderedRect.m_transformedRect.left <= actualCoverage.left &&
							zorderedRect.m_transformedRect.right >= actualCoverage.right
						)
						{
							if (actualCoverage.top >= zorderedRect.m_transformedRect.top)
							{
								if (zorderedRect.m_transformedRect.bottom >= actualCoverage.bottom)
								{
									return true;
								}
								if (zorderedRect.m_transformedRect.bottom > actualCoverage.top)
								{
									actualCoverage.top = zorderedRect.m_transformedRect.bottom;
								}
							}
							else if (zorderedRect.m_transformedRect.bottom >= coverage.bottom && coverage.bottom > zorderedRect.m_transformedRect.top)
							{
								actualCoverage.bottom = zorderedRect.m_transformedRect.top;
							}
						}
					}
				}
			}
			else
			{
				for (const auto& zorderedRect : GetOccluderArray2()->views())
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
							zorderedRect.m_transformedRect.left <= actualCoverage.left &&
							zorderedRect.m_transformedRect.right >= actualCoverage.right
						)
						{
							if (actualCoverage.top >= zorderedRect.m_transformedRect.top)
							{
								if (zorderedRect.m_transformedRect.bottom >= actualCoverage.bottom)
								{
									return true;
								}
								if (zorderedRect.m_transformedRect.bottom > actualCoverage.top)
								{
									actualCoverage.top = zorderedRect.m_transformedRect.bottom;
								}
							}
							else if (zorderedRect.m_transformedRect.bottom >= coverage.bottom && coverage.bottom > zorderedRect.m_transformedRect.top)
							{
								actualCoverage.bottom = zorderedRect.m_transformedRect.top;
							}
						}
					}
				}
			}

			return false;
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
	struct CLegacyMilBrush : CResource 
	{
		DECLSPEC_PROJECTION static float STDMETHODCALLTYPE GetOpacity(float opacity, const CFloatResource* resource)
		{
			return HANDLE_PROJECTION_FUNCTION(CLegacyMilBrush::GetOpacity, opacity, resource);
		}
	};
	struct CSolidColorLegacyMilBrush : CLegacyMilBrush
	{
		DECLSPEC_PROJECTION float GetOpacityValue() const
		{
			return Util::PointerExecuteUnsafe<CSolidColorLegacyMilBrush_GetOpacityValue_Offsets, Util::DereferenceAt<float>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CFloatResource* GetFloatResource() const
		{
			return Util::PointerExecuteUnsafe<CSolidColorLegacyMilBrush_GetFloatResource_Offsets, Util::DereferenceAt<CFloatResource*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION bool STDMETHODCALLTYPE IsConstantOpaque() const
		{
			return HANDLE_PROJECTION_FUNCTION(CSolidColorLegacyMilBrush::IsConstantOpaque);
		}
		float GetRealizedOpacity() const
		{
			return GetOpacity(GetOpacityValue(), GetFloatResource());
		}
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
		virtual UINT STDMETHODCALLTYPE GetType() const = 0;
		virtual bool STDMETHODCALLTYPE IsEmpty() const = 0;
		virtual HRESULT STDMETHODCALLTYPE GetD2DGeometry(const CMILMatrix* matrix, ID2D1Geometry** geometry) const = 0;

		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE CopyShape(
			const CMILMatrix* matrix,
			CShape** shape
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(CShape::CopyShape, matrix, shape);
		}

		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE GetTightBounds(D2D1_RECT_F* lprc, const CMILMatrix* matrix) const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CShape_GetTightBounds_Offsets, Util::DereferenceAt<decltype(&CShape::GetTightBounds)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this,
				lprc,
				matrix
			);
		}
		DECLSPEC_PROJECTION bool STDMETHODCALLTYPE IsRectangles(UINT* count) const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CShape_IsRectangles_Offsets, Util::DereferenceAt<decltype(&CShape::IsRectangles)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this,
				count
			);
		}
		DECLSPEC_PROJECTION bool STDMETHODCALLTYPE GetRectangles(D2D1_RECT_F* buffer, UINT count) const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<CShape_GetRectangles_Offsets, Util::DereferenceAt<decltype(&CShape::GetRectangles)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this,
				buffer,
				count
			);
		}
	};
	struct CRegionShape : CShape {};
	struct CRectanglesShape : CShape {};
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
		DECLSPEC_PROJECTION	HRESULT STDMETHODCALLTYPE GetShapeData(
			const D2D1_SIZE_F* size,
			CShapePtr* shape
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CGeometry::GetShapeData, size, shape);
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
		virtual HRESULT STDMETHODCALLTYPE Clear(const D2D1_COLOR_F& color) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawRectangle(const D2D1_RECT_F& lprc, CLegacyMilBrush* brush, CResource* resource) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawSolidRectangle(const D2D1_RECT_F& lprc, const D2D1_COLOR_F& color) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawImage(CResource* image, const D2D1_RECT_F* lprc, CResource* resource) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawGeometry(CLegacyMilBrush* brush, CGeometry* geometry) = 0;
		virtual HRESULT STDMETHODCALLTYPE TileImage(CResource* image, D2D1_RECT_F& lprc, D2D1_POINT_2F& point, float) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawBitmap(CResource* bitmap) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawInk(ID2D1Ink* ink, const D2D1_COLOR_F& color, ID2D1InkStyle* inkStyle) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawGenericInk(struct IDCompositionDirectInkWetStrokePartner*, bool) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawYCbCrBitmap(CResource*, CResource*, D2D1_YCBCR_CHROMA_SUBSAMPLING) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawMesh2D(CGeometry2D* geometry, CImageSource* imageSource) = 0;
		virtual HRESULT STDMETHODCALLTYPE DrawVisual(CVisual* visual) = 0;
		virtual HRESULT STDMETHODCALLTYPE Pop() = 0;
		virtual HRESULT STDMETHODCALLTYPE PushTransform(CTransform* transfrom) = 0;
		virtual HRESULT STDMETHODCALLTYPE ApplyRenderState() = 0;

		DECLSPEC_PROJECTION CDrawingContext* GetDrawingContext() const
		{
			return Util::PointerExecuteUnsafe<IDrawingContext_GetDrawingContext_Offsets, Util::OffsetBy<CDrawingContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		COcclusionContext* GetOcclusionContext() const
		{
			return reinterpret_cast<COcclusionContext*>(reinterpret_cast<ULONG_PTR>(this));
		}
	};

	struct ID2DContextOwner
	{
		DECLSPEC_PROJECTION UINT STDMETHODCALLTYPE GetCurrentZ() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<ID2DContextOwner_GetCurrentZ_Offsets, Util::DereferenceAt<decltype(&ID2DContextOwner::GetCurrentZ)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
	};
	

	struct CD2DContext : CResource
	{
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE EnsureBeginDraw()
		{
			return HANDLE_PROJECTION_FUNCTION(CD2DContext::EnsureBeginDraw);
		}
		DECLSPEC_PROJECTION ID2D1DeviceContext* GetDeviceContext() const
		{
			return Util::PointerExecuteUnsafe<CD2DContext_GetDeviceContext_Offsets, Util::DereferenceAt<ID2D1DeviceContext*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
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
			return reinterpret_cast<CD2DContext*>(reinterpret_cast<ULONG_PTR>(this) + 16);
		}
	};
	struct IBitmapResource;
	struct IDeviceTexture
	{
		DECLSPEC_PROJECTION ID3D11Texture2D* GetTexture2D() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IDeviceTexture_GetTexture2D_Offsets, Util::DereferenceAt<decltype(&IDeviceTexture::GetTexture2D)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
		DECLSPEC_PROJECTION ID3D11ShaderResourceView* GetShaderResourceView() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IDeviceTexture_GetShaderResourceView_Offsets, Util::DereferenceAt<decltype(&IDeviceTexture::GetShaderResourceView)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
	};
	struct IDeviceTarget
	{
		DECLSPEC_PROJECTION ID3D11RenderTargetView* GetRenderTargetView() const
		{
			return std::invoke(
				Util::PointerExecuteUnsafe<IDeviceTarget_GetRenderTargetView_Offsets, Util::DereferenceAt<decltype(&IDeviceTarget::GetRenderTargetView)>>(HookHelper::vftbl_of(this), g_versionInfo.build, g_versionInfo.revision),
				this
			);
		}
		DECLSPEC_PROJECTION IDeviceTexture* GetDeviceTexture() const
		{
			return Util::PointerExecuteUnsafe<IDeviceTarget_GetDeviceTexture_Offsets, Util::OffsetBy<IDeviceTexture*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CDrawingContext
	{
		IDeviceTarget* GetDeviceTarget() const
		{
			return reinterpret_cast<IDeviceTarget* const*>(this)[4];
		}
		CD3DDevice* GetD3DDevice() const
		{
			return reinterpret_cast<CD3DDevice* const*>(this)[5];
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
		DECLSPEC_PROJECTION CVisualTree* GetCurrentVisualTree() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetCurrentVisualTree_Offsets, Util::DereferenceAt<CVisualTree*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		
		DECLSPEC_PROJECTION const CMILMatrix* GetWorldTransform() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetWorldTransform_Offsets, Util::OffsetBy<CMatrixStack*>>(this, g_versionInfo.build, g_versionInfo.revision)->GetTopByReference();
		}
		DECLSPEC_PROJECTION CMILMatrix* GetDeviceTransform() const
		{
			return Util::PointerExecuteUnsafe<CDrawingContext_GetDeviceTransform_Offsets, Util::OffsetBy<CMILMatrix*>>(this, g_versionInfo.build, g_versionInfo.revision);
		}

		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE PushTransformInternal(CVisual* visual, const CMILMatrix* matrix, bool unknown1, bool unknown2)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::PushTransformInternal, visual, matrix, unknown1, unknown2);
		}
		DECLSPEC_PROJECTION void STDMETHODCALLTYPE PopTransformInternal(bool popStack)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::PopTransformInternal, popStack);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE GetUnOccludedWorldShape(const CShape* shape, int depth, CShape** worldShape) const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::GetUnOccludedWorldShape, shape, depth, worldShape);
		}
		DECLSPEC_PROJECTION ULONG_PTR STDMETHODCALLTYPE GetClipBoundsWorld(D2D1_RECT_F* lprc) const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::GetClipBoundsWorld, lprc);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE FillShapeWithBrush(const CShape* shape, const ID2D1Brush* brush)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::FillShapeWithBrush, shape, brush);
		}
		// this function cannot render the shape that is not consisted of rectangles
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE FillShapeWithSolidColor(const CShape* shape, const D2D1_COLOR_F& color)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::FillShapeWithSolidColor, shape, color);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE FlushD2D()
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::FlushD2D);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE ApplyRenderStateInternal(bool unknown)
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::ApplyRenderStateInternal, unknown);
		}
		DECLSPEC_PROJECTION CVisual* STDMETHODCALLTYPE GetCurrentVisual() const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::GetCurrentVisual);
		}

		CVisual* GetCurrentVisualHelper() const
		{
			return (g_versionInfo.build >= os::build_w11_24h2 ? this : reinterpret_cast<dwmcore::CDrawingContext*>(GetD2DContextOwner()))->GetCurrentVisual();
		}
	};
	struct COcclusionContext : IDrawingContext
	{
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
		D2D1_RECT_F PageInPixelsRectToDeviceRect(const D2D1_RECT_F& pixelsRect)
		{
			auto result = pixelsRect;

			if (*GetDeviceTransformFlag() & 0x1)
			{
				result = RectF::TransformRect(pixelsRect, GetDeviceTransform()->GetD2DMatrix());
			}

			return result;
		}
		DECLSPEC_PROJECTION CBaseClipStack* GetClipStack() const
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetClipStack_Offsets, Util::OffsetBy<CBaseClipStack* const>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION CArrayBasedCoverageSet* GetArrayBasedCoverageSet() const
		{
			return Util::PointerExecuteUnsafe<COcclusionContext_GetArrayBasedCoverageSet_Offsets, Util::OffsetBy<CArrayBasedCoverageSet* const>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE SetDeviceTransform(
			const dwmcore::CMILMatrix* matrix
		)
		{
			return HANDLE_PROJECTION_FUNCTION(COcclusionContext::SetDeviceTransform, matrix);
		}
	};
	struct CDirtyRegion
	{
		DECLSPEC_PROJECTION COcclusionContext* GetOcclusionContext() const
		{
			return Util::PointerExecuteUnsafe<CDirtyRegion_GetOcclusionContext_Offsets, Util::OffsetBy<COcclusionContext* const>>(this, g_versionInfo.build, g_versionInfo.revision);
		}
	};
	struct CTreeDirty {};
	struct COverlayContext : CResource {};
	struct CWindowOcclusionInfo : CResource {};
	struct CWindowNode : CResource
	{
		DECLSPEC_PROJECTION HWND STDMETHODCALLTYPE GetHwnd() const
		{
			return HANDLE_PROJECTION_FUNCTION(CWindowNode::GetHwnd);
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

	DECLSPEC_PROJECTION ULONGLONG STDMETHODCALLTYPE GetCurrentFrameId()
	{
		return HANDLE_PROJECTION_FUNCTION(GetCurrentFrameId);
	}

	inline ID2D1Factory8** g_DeviceManager{ nullptr };

	namespace CCommonRegistryData
	{
		inline PULONG m_dwOverlayTestMode{ nullptr };
		inline PULONGLONG m_backdropBlurCachingThrottleQPCTimeDelta{ nullptr };
	}

	inline auto g_projectionArray = make_projection_array(
		g_versionInfo.build,

		MAKE_FUNCTION_PROJECTION_TUPLE(CChannel::MatrixTransformUpdate, 0, os::build_w11_22h2),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CDesktopTree::vftable, "CDesktopTree::`vftable'", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::GetTopLevelWindow, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::GetDesktopTree, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(CMILMatrix::Identity, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CMILMatrix::Multiply, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CMatrixStack::GetTopByReference, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CBaseClipStack::Clip, 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CShape::CopyShape, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CRenderDataBuilder::DrawGeometry", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CRenderData::TryDrawCommandAsDrawList", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CRenderData::DrawImageResource_FillMode", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CGeometry::GetShapeData, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CRectangleGeometry::vftable, "CRectangleGeometry::`vftable'", 0, 0),
		
		MAKE_FUNCTION_PROJECTION_TUPLE(CLegacyMilBrush::GetOpacity, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CSolidColorLegacyMilBrush::IsConstantOpaque, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CSolidColorLegacyMilBrush::vftable, "CSolidColorLegacyMilBrush::`vftable'", 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CImageLegacyMilBrush::vftable, "CImageLegacyMilBrush::`vftable'", 0, 0),

		MAKE_OPTIONAL_EMPTY_PROJECTION_TUPLE("CArrayBasedCoverageSet::IsCovered", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CD2DContext::DestroyDeviceResources", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CD2DContext::EnsureBeginDraw, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::PushTransformInternal, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::PopTransformInternal, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetUnOccludedWorldShape, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetClipBoundsWorld, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FillShapeWithBrush, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FillShapeWithSolidColor, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FlushD2D, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::ApplyRenderStateInternal, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetCurrentVisual, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDrawingContext::DrawVisualTree", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDrawingContext::PreSubgraph", 0, 0),
		
		MAKE_FUNCTION_PROJECTION_TUPLE(COcclusionContext::SetDeviceTransform, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::IsOccluded", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::Compute", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::PageInPixelsRectToDeviceRect", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::~COcclusionContext", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetUnOccludedDirtyRect", os::build_w10_2004, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetOptimizedRect", os::build_w11_21h2, os::build_w11_24h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CTreeDirty::GetOptimizedRect", os::build_w11_24h2, 0),
		
#ifdef BUILD_BETA
		MAKE_FUNCTION_PROJECTION_TUPLE(CWindowNode::GetHwnd, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CWindowNode::RenderImage", 0, 0),
#endif

		MAKE_EMPTY_PROJECTION_TUPLE("CCachedVisualImage::CCachedTarget::Update", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(GetCurrentFrameId, 0, 0),
		
		MAKE_VARIABLE_PROJECTION_TUPLE(CCommonRegistryData::m_dwOverlayTestMode, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CCommonRegistryData::m_backdropBlurCachingThrottleQPCTimeDelta, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(g_DeviceManager, 0, 0)
	);
	
	inline bool ParserCallback(PSYMBOL_INFO info, [[maybe_unused]] ULONG size)
	{
		CHAR symbolName[128]{};
		UnDecorateSymbolName(info->Name, symbolName, std::size(symbolName), UNDNAME_NAME_ONLY);

		if (
			!strcmp(symbolName, "CCombinedGeometry::vftable") &&
			!strstr(info->Name, "CGeometry")
		)
		{
			return true;
		}
		if (
			!strcmp(symbolName, "CRenderData::DrawImageResource_FillMode") &&
			!strstr(info->Name, "PEBUD2D_RECT_F")
		)
		{
			return true;
		}
		if (
			!strcmp(symbolName, "CVisual::GetWorldTransform") &&
			!strstr(info->Name, "W4WalkReason@@PEAVCMILMatrix@@PEA_N2@Z")
		)
		{
			return true;
		}
		if (
			!strcmp(symbolName, "CMILMatrix::Multiply") &&
			strcmp(info->Name, "?Multiply@CMILMatrix@@QEAAXAEBV1@@Z") != 0
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
