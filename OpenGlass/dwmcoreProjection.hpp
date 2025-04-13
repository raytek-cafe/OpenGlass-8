#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "ProjectionHelper.hpp"
#include "D2DPrivates.hpp"
#include "DWM.hpp"

namespace OpenGlass::dwmcore
{
	using namespace DWM; 
	inline const auto g_moduleHandle{ GetModuleHandleW(L"dwmcore.dll") };
	inline const auto g_buildNumber{ Util::GetModuleBuildNumber(g_moduleHandle) };

	struct CResource : IUnknown {};
	struct IMILResource
	{
		virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
		virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
	};
	struct CResourceTable : IMILResource {};
	struct CChannelContext : IMILResource
	{
		CResourceTable* GetResourceTable() const
		{
			if (g_buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<CResourceTable* const*>(this)[3];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				return reinterpret_cast<CResourceTable* const*>(this)[4];
			}

			return reinterpret_cast<CResourceTable* const*>(this)[6];
		}
	};
	struct CComposition : IMILResource {};
	struct CGlobalComposition : CComposition {};
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
		DECLSPEC_PROJECTION CDesktopTree* STDMETHODCALLTYPE GetDesktopTree() const
		{
			return HANDLE_PROJECTION_FUNCTION(CVisual::GetDesktopTree);
		}
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE GetWorldTransform(
			const CVisualTree* tree,
			UINT walkReason,
			CMILMatrix* matrix,
			bool* unknown1,
			CMILMatrix* unknown2
		) const
		{
			return HANDLE_PROJECTION_FUNCTION(
				CVisual::GetWorldTransform,
				tree,
				walkReason,
				matrix,
				unknown1,
				unknown2
			);
		}
	};
	struct CFloatResource : CResource {};

	struct CMILMatrix : D2D1_MATRIX_4X4_F
	{
		int reserved;
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
		DECLSPEC_PROJECTION HRESULT STDMETHODCALLTYPE Top(CMILMatrix* matrix) const
		{
			return HANDLE_PROJECTION_FUNCTION(CMatrixStack::Top, matrix);
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
			if (std::fabs(m_transformedRect.top) < 8388608.f)
			{
				m_transformedRect.top = std::ceil(m_transformedRect.top);
			}
			if (std::fabs(m_transformedRect.right) < 8388608.f)
			{
				m_transformedRect.right = std::floor(m_transformedRect.right);
			}
			if (std::fabs(m_transformedRect.bottom) < 8388608.f)
			{
				m_transformedRect.bottom = std::floor(m_transformedRect.bottom);
			}
		}
		CZOrderedRect() = default;
		CZOrderedRect(const D2D1_RECT_F& rect, int depth, const CMILMatrix* matrix) : m_depth{ depth }, m_originalRect{ rect }
		{
			UpdateDeviceRect(matrix);
		}
	};
	struct CArrayBasedCoverageSet : CResource
	{
		DynArray<CZOrderedRect>* GetAntiOccluderArray() const
		{
			DynArray<CZOrderedRect>* array{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				array = reinterpret_cast<DynArray<CZOrderedRect>*>(const_cast<CArrayBasedCoverageSet*>(this + 52));
			}
			else if (g_buildNumber < os::build_w11_21h2)
			{
				array = reinterpret_cast<DynArray<CZOrderedRect>*>(const_cast<CArrayBasedCoverageSet*>(this + 49));
			}
			else
			{
				array = nullptr;
			}

			return array;
		}
		DynArray<CZOrderedRect>* GetOccluderArray() const
		{
			return reinterpret_cast<DynArray<CZOrderedRect>*>(const_cast<CArrayBasedCoverageSet*>(this));
		}

		COcclusionContext* GetOcclusionContext() const
		{
			COcclusionContext* context{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				context = reinterpret_cast<COcclusionContext* const>(reinterpret_cast<ULONG_PTR>(this) - 408);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				context = reinterpret_cast<COcclusionContext* const>(reinterpret_cast<ULONG_PTR>(this) - 448);
			}
			else
			{
				context = reinterpret_cast<COcclusionContext* const>(reinterpret_cast<ULONG_PTR>(this) - 568);
			}

			return context;
		}

		bool STDMETHODCALLTYPE IsCovered(
			const D2D1_RECT_F& coverage,
			int depth
		) const
		{
			bool antiOccluderExisted{ false };
			int antiOccluderDepth{};
			if (g_buildNumber < os::build_w11_21h2)
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
		CRenderDataResourceArray* GetResources() const
		{
			CRenderDataResourceArray* array{ nullptr };
			if (g_buildNumber < os::build_w11_21h2)
			{
				array = reinterpret_cast<CRenderDataResourceArray*>(reinterpret_cast<ULONG_PTR>(this) + 128);
			}
			else
			{
				array = reinterpret_cast<CRenderDataResourceArray*>(reinterpret_cast<ULONG_PTR>(this) + 136);
			}

			return array;
		}
	};
	struct CLegacyMilBrush : CResource 
	{
		CFloatResource* GetFloatResource() const
		{
			CFloatResource* resource{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				resource = reinterpret_cast<CFloatResource* const*>(this)[8];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				resource = reinterpret_cast<CFloatResource* const*>(this)[9];
			}
			else
			{
				resource = reinterpret_cast<CFloatResource* const*>(this)[10];
			}

			return resource;
		}
		float GetOpacityValue() const
		{
			float opacity{};

			if (g_buildNumber < os::build_w11_21h2)
			{
				opacity = reinterpret_cast<float const*>(this)[14];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				opacity = reinterpret_cast<float const*>(this)[16];
			}
			else
			{
				opacity = reinterpret_cast<float const*>(this)[18];
			}

			return opacity;
		}
		DECLSPEC_PROJECTION static float STDMETHODCALLTYPE GetOpacity(float opacity, const CFloatResource* resource)
		{
			return HANDLE_PROJECTION_FUNCTION(CLegacyMilBrush::GetOpacity, opacity, resource);
		}
	};
	struct CSolidColorLegacyMilBrush : CLegacyMilBrush
	{
		DECLSPEC_PROJECTION bool STDMETHODCALLTYPE IsConstantOpaque() const
		{
			return HANDLE_PROJECTION_FUNCTION(CSolidColorLegacyMilBrush::IsConstantOpaque);
		}
		float GetRealizedOpacity() const
		{
			return GetOpacity(GetOpacityValue(), GetFloatResource());
		}
		const D2D1_COLOR_F& GetRealizedColor() const
		{
			D2D1_COLOR_F* color{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				color = reinterpret_cast<D2D1_COLOR_F*>(reinterpret_cast<ULONG_PTR>(this) + 88);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				color = reinterpret_cast<D2D1_COLOR_F*>(reinterpret_cast<ULONG_PTR>(this) + 96);
			}
			else
			{
				color = reinterpret_cast<D2D1_COLOR_F*>(reinterpret_cast<ULONG_PTR>(this) + 104);
			}

			return *color;
		}
		inline static PVOID* vftable{ nullptr };
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

		HRESULT STDMETHODCALLTYPE GetTightBounds(D2D1_RECT_F* lprc, const CMILMatrix* matrix) const
		{
			decltype(&CShape::GetTightBounds) vfptr{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[4]);
			}
			else
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[6]);
			}

			return std::invoke(
				vfptr,
				this, 
				lprc, 
				matrix
			);
		}
		bool STDMETHODCALLTYPE IsRectangles(UINT* count) const
		{
			decltype(&CShape::IsRectangles) vfptr{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[5]);
			}
			else
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[8]);
			}

			return std::invoke(
				vfptr,
				this,
				count
			);
		}
		bool STDMETHODCALLTYPE GetRectangles(D2D1_RECT_F* buffer, UINT count) const
		{
			decltype(&CShape::GetRectangles) vfptr{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[6]);
			}
			else
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[10]);
			}

			return std::invoke(
				vfptr,
				this,
				buffer,
				count
			);
		}

		DECLSPEC_PROJECTION static HRESULT STDMETHODCALLTYPE Combine(
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
		DECLSPEC_PROJECTION static HRESULT STDMETHODCALLTYPE BuildFromRectFs(
			const D2D1_RECT_F& rect,
			UINT unused,
			CShape** shape
		)
		{
			return HANDLE_PROJECTION_FUNCTION(CShape::BuildFromRectFs, rect, unused, shape);
		}
	};
	struct CRegionShape : CShape {};
	struct CRectanglesShape : CShape {};
	struct CShapePtr
	{
		CShape* ptr{ nullptr };
		bool notRef{ true };

		CShapePtr() = default;
		CShapePtr(const CShape* src) noexcept
		{
			Release();
			ptr = const_cast<CShape*>(src);
			notRef = false;
		}
		CShapePtr(CShapePtr&& src) noexcept
		{
			Release();
			ptr = src.ptr;
			notRef = src.notRef;
			src.ptr = nullptr;
		}

		CShape* operator=(const CShape* src)
		{
			Release();
			ptr = const_cast<CShape*>(src);
			notRef = false;

			return ptr;
		}
		CShape* operator->()
		{
			return ptr;
		}
		CShape* get() const
		{
			return ptr;
		}
		CShape** put()
		{
			Release();
			return &ptr;
		}
		operator bool()
		{
			return ptr != nullptr;
		}
		void Release()
		{
			if (notRef && ptr)
			{
				std::invoke(
					**reinterpret_cast<void(CShape::***)(BYTE)>(ptr),
					ptr,
					true
				);
			}

			ptr = nullptr;
		}
		~CShapePtr()
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
	struct CGeometry2D : CGeometry {};
	struct CImageSource : CResource {};
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

		CDrawingContext* GetDrawingContext() const
		{
			CDrawingContext* value{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				value = reinterpret_cast<CDrawingContext*>(reinterpret_cast<ULONG_PTR>(this));
			}
			else
			{
				value = reinterpret_cast<CDrawingContext*>(reinterpret_cast<ULONG_PTR>(this) - 16);
			}

			return value;
		}
		COcclusionContext* GetOcclusionContext() const
		{
			return reinterpret_cast<COcclusionContext*>(reinterpret_cast<ULONG_PTR>(this));
		}
	};

	struct RenderTargetInfo;
	struct ID2DContextOwner
	{
		UINT STDMETHODCALLTYPE GetCurrentZ() const
		{
			decltype(&ID2DContextOwner::GetCurrentZ) vfptr{ nullptr };

			if (g_buildNumber < os::build_w11_24h2)
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[3]);
			}
			else
			{
				vfptr = Util::force_cast_to<decltype(vfptr)>(HookHelper::vftbl_of(this)[1]);
			}

			return std::invoke(
				vfptr,
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
		ID2D1DeviceContext* GetDeviceContext() const
		{
			ID2D1DeviceContext* context{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				context = reinterpret_cast<ID2D1DeviceContext* const*>(this)[30];
			}
			else
			{
				context = reinterpret_cast<ID2D1DeviceContext* const*>(this)[25];
			}
			return context;
		}
		ID2D1PrivateCompositorDeviceContext* GetPrivateCompositorDeviceContext() const
		{
			ID2D1PrivateCompositorDeviceContext* context{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				context = reinterpret_cast<ID2D1PrivateCompositorDeviceContext* const*>(this)[31];
			}
			else
			{
				context = reinterpret_cast<ID2D1PrivateCompositorDeviceContext* const*>(this)[26];
			}
			return context;
		}
		const D2D1_RECT_F* GetWorldClip() const
		{
			const D2D1_RECT_F* clip{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				clip = reinterpret_cast<D2D1_RECT_F const*>(this) + 28;
			}
			else
			{
				clip = reinterpret_cast<D2D1_RECT_F const*>(this) + 25;
			}

			return clip;
		}
		D2D1_ANTIALIAS_MODE GetAntialiasMode() const
		{
			D2D1_ANTIALIAS_MODE mode{ D2D1_ANTIALIAS_MODE_ALIASED };

			if (g_buildNumber < os::build_w11_21h2)
			{
				mode = reinterpret_cast<D2D1_ANTIALIAS_MODE const*>(this)[116];
			}
			else
			{
				mode = reinterpret_cast<D2D1_ANTIALIAS_MODE const*>(this)[104];
			}

			return mode;
		}
	};
	struct IBitmapResource;
	struct IDeviceTarget;
	struct CDrawingContext
	{
		CD2DContext* GetD2DContext() const
		{
			CD2DContext* context{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				context = reinterpret_cast<CD2DContext* const*>(this)[48];
			}
			else
			{
				context = reinterpret_cast<CD2DContext*>(reinterpret_cast<ULONG_PTR const*>(this)[5] + 16);
			}

			// CDrawingContext -> CD3DDevice -> CD2DContext
			return context;
		}
		IDrawingContext* GetInterface() const
		{
			IDrawingContext* value{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				value = reinterpret_cast<IDrawingContext*>(reinterpret_cast<ULONG_PTR>(this));
			}
			else
			{
				value = reinterpret_cast<IDrawingContext*>(reinterpret_cast<ULONG_PTR>(this) + 16);
			}

			return value;
		}
		ID2DContextOwner* GetD2DContextOwner() const
		{
			ID2DContextOwner* value{ nullptr };

			if (g_buildNumber < os::build_w10_2004)
			{
				value = reinterpret_cast<ID2DContextOwner*>(reinterpret_cast<ULONG_PTR>(this) + 8);
			}
			else
			{
				value = reinterpret_cast<ID2DContextOwner*>(reinterpret_cast<ULONG_PTR>(this) + 24);
			}

			return value;
		}
		COcclusionContext* GetOcclusionContext() const
		{
			if (g_buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<COcclusionContext* const*>(this)[742];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				return reinterpret_cast<COcclusionContext* const*>(this)[993];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				return reinterpret_cast<COcclusionContext* const*>(this)[1009];
			}

			return reinterpret_cast<COcclusionContext* const*>(this)[995];
		}
		CVisualTree* GetCurrentVisualTree() const
		{
			if (g_buildNumber < os::build_w11_21h2)
			{
				return reinterpret_cast<CVisualTree* const*>(this)[741];
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				return reinterpret_cast<CVisualTree* const*>(this)[991];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				return reinterpret_cast<CVisualTree* const*>(this)[1007];
			}

			return reinterpret_cast<CVisualTree* const*>(this)[993];
		}
		
		HRESULT GetWorldTransform(CMILMatrix* matrix) const
		{
			CMatrixStack* stack{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 408);
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 368);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 400);
			}
			else 
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 288);
			}

			return stack->Top(matrix);
		}
		const CMILMatrix* GetWorldTransform() const
		{
			CMatrixStack* stack{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 408);
			}
			else if (g_buildNumber < os::build_w11_22h2)
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 368);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 400);
			}
			else
			{
				stack = reinterpret_cast<CMatrixStack*>(reinterpret_cast<ULONG_PTR>(this) + 288);
			}

			return stack->GetTopByReference();
		}
		CMILMatrix* GetDeviceTransform() const
		{
			CMILMatrix* deviceTransform{ nullptr };

			deviceTransform = reinterpret_cast<CMILMatrix*>(reinterpret_cast<ULONG_PTR>(this) + 96);

			return deviceTransform;
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
		DECLSPEC_PROJECTION CVisual* STDMETHODCALLTYPE GetCurrentVisual() const
		{
			return HANDLE_PROJECTION_FUNCTION(CDrawingContext::GetCurrentVisual);
		}

		winrt::com_ptr<ID2D1Bitmap1> AcquireRenderTargetBitmap(bool flush)
		{
			winrt::com_ptr<ID2D1Bitmap1> renderTargetBitmap{ nullptr };
			const auto d2dContext = GetD2DContext();
			// ensure deferred target bitmap in position
			d2dContext->EnsureBeginDraw();
			const auto context = d2dContext->GetDeviceContext();
			if (!context)
			{
				return renderTargetBitmap;
			}
			{
				winrt::com_ptr<ID2D1Image> targetImage{ nullptr };
				context->GetTarget(targetImage.put());
				if (!targetImage)
				{
					return renderTargetBitmap;
				}
				if (FAILED(targetImage->QueryInterface(renderTargetBitmap.put())))
				{
					return renderTargetBitmap;
				}
			}
			if (flush)
			{
				FlushD2D();
			}

			return renderTargetBitmap;
		}
		CVisual* GetCurrentVisualHelper() const
		{
			return (os::buildNumber >= os::build_w11_24h2 ? this : reinterpret_cast<dwmcore::CDrawingContext*>(GetD2DContextOwner()))->GetCurrentVisual();
		}
	};
	struct COcclusionContext : IDrawingContext
	{
		ULONGLONG GetFrameId() const
		{
			ULONGLONG frameId{};

			if (g_buildNumber < os::build_w11_21h2)
			{
				frameId = reinterpret_cast<ULONGLONG const*>(this)[2];
			}
			else
			{
				frameId = reinterpret_cast<ULONGLONG const*>(this)[3];
			}

			return frameId;
		}
		UINT GetCurrentZ() const
		{
			UINT z{};

			if (g_buildNumber < os::build_w11_21h2)
			{
				z = reinterpret_cast<UINT const*>(this)[364];
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				z = reinterpret_cast<UINT const*>(this)[357];
			}
			else
			{
				z = reinterpret_cast<UINT const*>(this)[329];
			}

			return z;
		}
		const CMILMatrix* GetWorldTransform() const
		{
			const CMILMatrix* worldTransform{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				worldTransform = reinterpret_cast<CMatrixStack const*>(reinterpret_cast<ULONG_PTR>(this) + 24)->GetTopByReference();
			}
			else
			{
				worldTransform = reinterpret_cast<CMatrixStack const*>(reinterpret_cast<ULONG_PTR>(this) + 32)->GetTopByReference();
			}

			return worldTransform;
		}
		const CMILMatrix* GetDeviceTransform() const
		{
			const CMILMatrix* deviceTransform{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				deviceTransform = reinterpret_cast<CMILMatrix const*>(reinterpret_cast<ULONG_PTR>(this) + 1248);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				deviceTransform = reinterpret_cast<CMILMatrix const*>(reinterpret_cast<ULONG_PTR>(this) + 1208);
			}
			else
			{
				deviceTransform = reinterpret_cast<CMILMatrix const*>(reinterpret_cast<ULONG_PTR>(this) + 1180);
			}

			return deviceTransform;
		}
		UINT* GetDeviceTransformFlag()
		{
			UINT* flag{ nullptr };

			if (g_buildNumber < os::build_w11_21h2)
			{
				flag = reinterpret_cast<UINT*>(&reinterpret_cast<bool*>(this)[1244]);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				flag = reinterpret_cast<UINT*>(&reinterpret_cast<bool*>(this)[1204]);
			}
			else
			{
				flag = reinterpret_cast<UINT*>(&reinterpret_cast<bool*>(this)[1172]);
			}

			return flag;
		}
		D2D1_RECT_F PageInPixelsRectToDeviceRect(const D2D1_RECT_F& pixelsRect)
		{
			auto result = pixelsRect;

			if (GetDeviceTransformFlag())
			{
				result = RectF::TransformRect(pixelsRect, GetDeviceTransform()->GetD2DMatrix());
			}

			return result;
		}
		CArrayBasedCoverageSet* GetArrayBasedCoverageSet() const
		{
			CArrayBasedCoverageSet* coverageSet{ nullptr };
			
			if (g_buildNumber < os::build_w11_21h2)
			{
				coverageSet = reinterpret_cast<CArrayBasedCoverageSet* const>(reinterpret_cast<ULONG_PTR>(this) + 408);
			}
			else if (g_buildNumber < os::build_w11_24h2)
			{
				coverageSet = reinterpret_cast<CArrayBasedCoverageSet* const>(reinterpret_cast<ULONG_PTR>(this) + 448);
			}
			else
			{
				coverageSet = reinterpret_cast<CArrayBasedCoverageSet* const>(reinterpret_cast<ULONG_PTR>(this) + 568);
			}

			return coverageSet;
		}
		D2D1_RECT_F STDMETHODCALLTYPE GetDestinationRect(const D2D1_RECT_F& inputRect) const
		{
			D2D1_RECT_F outputRect{};
			outputRect = RectF::TransformRect(inputRect, GetWorldTransform()->GetD2DMatrix());
			return outputRect;
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
		COcclusionContext* GetOcclusionContext() const
		{
			if (g_buildNumber < os::build_w11_24h2)
			{
				return reinterpret_cast<COcclusionContext* const>(reinterpret_cast<ULONG_PTR>(this) + 16);
			}

			return nullptr;
		}
	};
	struct CTreeDirty {};
	struct COverlayContext : CResource {};
	struct CWindowOcclusionInfo : CResource {};
	struct CWindowNode : CResource {};
	struct RenderTargetInfo;
	struct CCachedVisualImage : CResource
	{
		struct CCachedTarget : CResource
		{

		};
	};
	struct CDrawListCache : CResource {};

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

	inline CGlobalComposition** g_pComposition{ nullptr };
	inline CGlobalComposition* GetGlobalCompositionInstance()
	{
		return *g_pComposition;
	}

	inline auto g_projectionArray = make_projection_array(
		g_buildNumber,

		MAKE_FUNCTION_PROJECTION_TUPLE(CChannel::MatrixTransformUpdate, 0, os::build_w11_22h2),

		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CDesktopTree::vftable, "CDesktopTree::`vftable'", 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::GetDesktopTree, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CVisual::GetWorldTransform, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(CMILMatrix::Identity, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CMILMatrix::Multiply, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CMatrixStack::GetTopByReference, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CMatrixStack::Top, 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CShape::CopyShape, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CShape::Combine, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CShape::BuildFromRectFs, 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CRenderDataBuilder::DrawGeometry", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CRenderData::TryDrawCommandAsDrawList", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CGeometry::~CGeometry", 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CGeometry::GetShapeData, 0, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(CLegacyMilBrush::GetOpacity, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CSolidColorLegacyMilBrush::IsConstantOpaque, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE_BY_ALIAS(CSolidColorLegacyMilBrush::vftable, "CSolidColorLegacyMilBrush::`vftable'", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CArrayBasedCoverageSet::IsCovered", 0, os::build_w11_24h2),

		MAKE_FUNCTION_PROJECTION_TUPLE(CD2DContext::EnsureBeginDraw, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::PushTransformInternal, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::PopTransformInternal, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetUnOccludedWorldShape, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetClipBoundsWorld, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FillShapeWithBrush, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FillShapeWithSolidColor, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::FlushD2D, 0, 0),
		MAKE_FUNCTION_PROJECTION_TUPLE(CDrawingContext::GetCurrentVisual, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("CDrawingContext::DrawVisualTree", 0, 0),
		
		MAKE_FUNCTION_PROJECTION_TUPLE(COcclusionContext::SetDeviceTransform, 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::IsOccluded", os::build_w11_24h2, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::Compute", 0, 0),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::PageInPixelsRectToDeviceRect", 0, os::build_w11_24h2),
		MAKE_EMPTY_PROJECTION_TUPLE("COcclusionContext::~COcclusionContext", 0, 0),

		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetUnOccludedDirtyRect", os::build_min, os::build_w11_21h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CDirtyRegion::GetOptimizedRect", os::build_w11_21h2, os::build_w11_24h2),
		MAKE_EMPTY_PROJECTION_TUPLE("CTreeDirty::GetOptimizedRect", os::build_w11_24h2, 0),

		MAKE_FUNCTION_PROJECTION_TUPLE(GetCurrentFrameId, 0, 0),
		
		MAKE_VARIABLE_PROJECTION_TUPLE(CCommonRegistryData::m_dwOverlayTestMode, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(CCommonRegistryData::m_backdropBlurCachingThrottleQPCTimeDelta, 0, 0),

		MAKE_VARIABLE_PROJECTION_TUPLE(g_DeviceManager, 0, 0),
		MAKE_VARIABLE_PROJECTION_TUPLE(g_pComposition, 0, 0)
	);
	
	inline bool ParserCallback(PSYMBOL_INFO info, [[maybe_unused]] ULONG size)
	{
		CHAR symbolName[128]{};
		UnDecorateSymbolName(info->Name, symbolName, std::size(symbolName), UNDNAME_NAME_ONLY);

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