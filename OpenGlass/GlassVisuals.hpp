#pragma once
#include "resource.h"
#include "uDWMProjection.hpp"
#include "dcompPrivates.hpp"
#include "Shared.hpp"

namespace OpenGlass
{
	class CGlassReflectionResources
	{
		winrt::CompositionDrawingSurface m_reflectionSurface{ nullptr };
		winrt::CompositionGraphicsDevice m_graphicsDevice{ nullptr };
		winrt::event_token m_token{};
		std::atomic_bool m_recreateSurface{};
	public:
		~CGlassReflectionResources()
		{
			if (m_token)
			{
				m_graphicsDevice.RenderingDeviceReplaced(m_token);
			}
		}

		HRESULT EnsureSurface()
		{
			if (!m_graphicsDevice)
			{
				winrt::com_ptr<IDCompositionDesktopDevicePartner> dcompDevicePartner{ nullptr };
				RETURN_IF_FAILED(uDWM::CDesktopManager::GetInstance()->GetDCompDevice()->QueryInterface(dcompDevicePartner.put()));
				const auto compositor = dcompDevicePartner.as<winrt::Compositor>();

				RETURN_IF_FAILED(
					compositor.as<abi::ICompositorInterop>()->CreateGraphicsDevice(
						uDWM::CDesktopManager::GetInstance()->GetD2DDevice(),
						reinterpret_cast<abi::ICompositionGraphicsDevice**>(winrt::put_abi(m_graphicsDevice))
					)
				);
				m_token = m_graphicsDevice.RenderingDeviceReplaced([this](const winrt::CompositionGraphicsDevice&, const winrt::RenderingDeviceReplacedEventArgs&)
				{
					m_reflectionSurface = nullptr;
					RETURN_IF_FAILED(
						m_graphicsDevice.as<abi::ICompositionGraphicsDeviceInterop>()->SetRenderingDevice(
							uDWM::CDesktopManager::GetInstance()->GetD2DDevice()
						)
					);
					RETURN_IF_FAILED(EnsureSurface());
					return S_OK;
				});
			}

			if (m_recreateSurface)
			{
				m_reflectionSurface = nullptr;
				m_recreateSurface = false;
			}
			if (m_reflectionSurface)
			{
				return S_OK;
			}

			winrt::com_ptr<IStream> stream{ nullptr };
			if (
				Shared::g_reflectionTexturePath.empty() ||
				!PathFileExistsW(Shared::g_reflectionTexturePath.data())
			)
			{
				const auto currentModule = wil::GetModuleInstanceHandle();
				const auto resourceHandle = FindResourceW(currentModule, MAKEINTRESOURCE(IDB_REFLECTION), L"PNG");
				RETURN_LAST_ERROR_IF_NULL(resourceHandle);
				const auto globalHandle = LoadResource(currentModule, resourceHandle);
				RETURN_LAST_ERROR_IF_NULL(globalHandle);
				const auto cleanup = wil::scope_exit([=]
				{
					if (globalHandle)
					{
						UnlockResource(globalHandle);
						FreeResource(globalHandle);
					}
				});
				const auto resourceSize = SizeofResource(currentModule, resourceHandle);
				RETURN_LAST_ERROR_IF(resourceSize == 0);
				const auto resourceAddress = reinterpret_cast<PBYTE>(LockResource(globalHandle));
				stream = { SHCreateMemStream(resourceAddress, resourceSize), winrt::take_ownership_from_abi };
			}
			else
			{
				wil::unique_hfile file{ CreateFileW(Shared::g_reflectionTexturePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0) };
				RETURN_LAST_ERROR_IF(!file.is_valid());

				LARGE_INTEGER fileSize{};
				RETURN_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &fileSize));

				auto buffer{ std::make_unique<BYTE[]>(static_cast<size_t>(fileSize.QuadPart)) };
				RETURN_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), buffer.get(), static_cast<DWORD>(fileSize.QuadPart), nullptr, nullptr));
				stream = { SHCreateMemStream(buffer.get(), static_cast<UINT>(fileSize.QuadPart)), winrt::take_ownership_from_abi };
			}
			RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);

			winrt::com_ptr<IWICImagingFactory2> wicFactory{ nullptr };
			wicFactory.copy_from(uDWM::CDesktopManager::GetInstance()->GetWICFactory());
			winrt::com_ptr<IWICBitmapDecoder> wicDecoder{ nullptr };
			RETURN_IF_FAILED(wicFactory->CreateDecoderFromStream(stream.get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, wicDecoder.put()));
			winrt::com_ptr<IWICBitmapFrameDecode> wicFrame{ nullptr };
			RETURN_IF_FAILED(wicDecoder->GetFrame(0, wicFrame.put()));
			winrt::com_ptr<IWICFormatConverter> wicConverter{ nullptr };
			RETURN_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
			RETURN_IF_FAILED(
				wicConverter->Initialize(
					wicFrame.get(),
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					nullptr,
					0,
					WICBitmapPaletteTypeCustom
				)
			);

			UINT width{ 0 }, height{ 0 };
			RETURN_IF_FAILED(wicConverter->GetSize(&width, &height));

			if (!m_reflectionSurface)
			{
				m_reflectionSurface = m_graphicsDevice.CreateDrawingSurface2(
					{ static_cast<int>(width), static_cast<int>(height) },
					winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
					winrt::DirectXAlphaMode::Premultiplied
				);
			}

			const auto drawingSurfaceInterop = m_reflectionSurface.as<abi::ICompositionDrawingSurfaceInterop>();
			POINT offset{};
			winrt::com_ptr<ID2D1DeviceContext> context{ nullptr };
			RETURN_IF_FAILED(
				drawingSurfaceInterop->BeginDraw(nullptr, IID_PPV_ARGS(context.put()), &offset)
			);
			context->Clear();
			winrt::com_ptr<ID2D1Bitmap1> bitmap{ nullptr };
			RETURN_IF_FAILED(
				context->CreateBitmapFromWicBitmap(
					wicConverter.get(),
					D2D1::BitmapProperties1(
						D2D1_BITMAP_OPTIONS_NONE,
						D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
					),
					bitmap.put()
				)
			);
			context->DrawBitmap(bitmap.get());
			RETURN_IF_FAILED(
				drawingSurfaceInterop->EndDraw()
			);

			return S_OK;
		}
		auto GetSurface() const
		{
			return m_reflectionSurface;
		}
		void Reload()
		{
			m_recreateSurface = true;
		}
	};
	class CGlassReflectionVisual : public uDWM::CVisual
	{
		winrt::com_ptr<uDWM::CBaseGeometryProxy> m_geometry{ nullptr };
		winrt::SpriteVisual m_rootVisual{ nullptr };
		winrt::com_ptr<IInteropVisualTarget> m_visualTarget{ nullptr };

		inline static PVOID vftable[32]{};
		inline static std::unordered_set<CGlassReflectionVisual*> s_visualSet{};
		inline static std::unordered_map<uDWM::CVisual*, CGlassReflectionVisual*> s_visualParentMap{};
		inline static CGlassReflectionResources s_reflectionResources{};

		static void BuildVirtualFunctionTable()
		{
			PVOID CVisual_Initialize_Org{ nullptr };
			PVOID CVisual_SetParent_Org{ nullptr };
			PVOID CVisual_SetSize_Org{ nullptr };
			PVOID CVisual_SetOpacity_Org{ nullptr };
			PVOID CVisual_CloneVisualTree_Org{ nullptr };
			PVOID CVisual_ValidateVisual_Org{ nullptr };
			uDWM::g_projectionArray.ApplyToVariable("CVisual::Initialize", CVisual_Initialize_Org);
			uDWM::g_projectionArray.ApplyToVariable("CVisual::SetParent", CVisual_SetParent_Org);
			uDWM::g_projectionArray.ApplyToVariable("CVisual::SetSize", CVisual_SetSize_Org);
			uDWM::g_projectionArray.ApplyToVariable("CVisual::SetOpacity", CVisual_SetOpacity_Org);
			uDWM::g_projectionArray.ApplyToVariable("CVisual::CloneVisualTree", CVisual_CloneVisualTree_Org);
			uDWM::g_projectionArray.ApplyToVariable("CVisual::ValidateVisual", CVisual_ValidateVisual_Org);

			for (int i = 1; i < std::size(CGlassReflectionVisual::vftable); i++)
			{
				CGlassReflectionVisual::vftable[i] = uDWM::CVisual::vftable[i];
				if (CGlassReflectionVisual::vftable[i] == CVisual_Initialize_Org)
				{
					CGlassReflectionVisual::vftable[i] = Util::force_cast_from(&CGlassReflectionVisual::Initialize);
				}
				else if (CGlassReflectionVisual::vftable[i] == CVisual_SetParent_Org)
				{
					CGlassReflectionVisual::vftable[i] = Util::force_cast_from(&CGlassReflectionVisual::SetParent);
				}
				else if (CGlassReflectionVisual::vftable[i] == CVisual_SetSize_Org)
				{
					CGlassReflectionVisual::vftable[i] = Util::force_cast_from(&CGlassReflectionVisual::SetSize);
				}
				else if (CGlassReflectionVisual::vftable[i] == CVisual_SetOpacity_Org)
				{
					CGlassReflectionVisual::vftable[i] = Util::force_cast_from(&CGlassReflectionVisual::SetOpacity);
				}
				else if (CGlassReflectionVisual::vftable[i] == CVisual_CloneVisualTree_Org)
				{
					CGlassReflectionVisual::vftable[i] = Util::force_cast_from(&CGlassReflectionVisual::CloneVisualTree);
				}
				else if (CGlassReflectionVisual::vftable[i] == CVisual_ValidateVisual_Org)
				{
					CGlassReflectionVisual::vftable[i] = Util::force_cast_from(&CGlassReflectionVisual::ValidateVisual);
				}
			}
		}
	public:
		#pragma warning (suppress : 26495)
		CGlassReflectionVisual()
		{
			if (!CGlassReflectionVisual::vftable[0])
			{
				CGlassReflectionVisual::vftable[0] = HookHelper::vftbl_of(this)[0];
				BuildVirtualFunctionTable();
			}
			*reinterpret_cast<PVOID*>(this) = &CGlassReflectionVisual::vftable;
			s_visualSet.insert(this);
		}
		~CGlassReflectionVisual()
		{
			if (const auto currentParent = this->GetTransformParent(); currentParent)
			{
				s_visualParentMap.erase(currentParent);
			}
			s_visualSet.erase(this);
		}

		HRESULT STDMETHODCALLTYPE Initialize()
		{
			winrt::com_ptr<IDCompositionDesktopDevicePartner> dcompDevicePartner{ nullptr };
			RETURN_IF_FAILED(uDWM::CDesktopManager::GetInstance()->GetDCompDevice()->QueryInterface(dcompDevicePartner.put()));

			try
			{
				const auto compositor = dcompDevicePartner.as<winrt::Compositor>();
				m_rootVisual = compositor.CreateSpriteVisual();
				//m_rootVisual.Brush(compositor.CreateColorBrush({ 120, 112, 184, 252 }));
				const auto surfaceBrush = compositor.CreateSurfaceBrush();
				surfaceBrush.Stretch(winrt::CompositionStretch::None);
				surfaceBrush.HorizontalAlignmentRatio(0.f);
				surfaceBrush.VerticalAlignmentRatio(0.f);
				surfaceBrush.BitmapInterpolationMode(winrt::CompositionBitmapInterpolationMode::Linear);
				m_rootVisual.Brush(surfaceBrush);
			}
			CATCH_RETURN()
			RETURN_IF_FAILED(
				dcompDevicePartner->CreateSharedResource(
					IID_PPV_ARGS(m_visualTarget.put())
				)
			);
			RETURN_IF_FAILED(
				m_visualTarget.as<IVisualTargetPartner>()->SetRoot(
					m_rootVisual.as<abi::IVisual>().get()
				)
			);
			RETURN_IF_FAILED(dcompDevicePartner->Commit());

			wil::unique_handle resourceHandle{ nullptr };
			RETURN_IF_FAILED(
				dcompDevicePartner->OpenSharedResourceHandle(
					m_visualTarget.get(), 
					resourceHandle.put()
				)
			);
			RETURN_IF_FAILED(CVisual::InitializeFromSharedHandle(resourceHandle.get()));
			CVisual::SetInsetFromParent({});

			return S_OK;
		}
		static HRESULT STDMETHODCALLTYPE Create(CGlassReflectionVisual** visual)
		{
			auto cleanup = wil::scope_exit([visual]
			{
				if (visual)
				{
					(*visual)->Release();
					*visual = nullptr;
				}
			});
			RETURN_HR_IF_NULL(E_INVALIDARG, visual);
			winrt::com_ptr<CGlassReflectionVisual> createdVisual{ new(std::nothrow) CGlassReflectionVisual(), winrt::take_ownership_from_abi};
			RETURN_LAST_ERROR_IF_NULL(createdVisual);
			RETURN_IF_FAILED(createdVisual->Initialize());
			cleanup.release();
			*visual = createdVisual.detach();
			return S_OK;
		}

		void STDMETHODCALLTYPE SetParent(CVisual* parent)
		{
			if (const auto currentParent = this->GetTransformParent(); currentParent)
			{
				s_visualParentMap.erase(currentParent);
			}
			if (parent)
			{
				s_visualParentMap[parent] = this;
			}

			CVisual::SetParent(parent);
		}
		void STDMETHODCALLTYPE SetSize(const SIZE* size)
		{
			CVisual::SetSize(size);
			try
			{
				m_rootVisual.Size({ static_cast<float>(size->cx), static_cast<float>(size->cy) });
			}
			catch (...) {}
		}
		void STDMETHODCALLTYPE SetOpacity(double opacity)
		{
			CVisual::SetOpacity(opacity);
			try
			{
				m_rootVisual.Opacity(static_cast<float>(opacity));
			}
			catch (...) {}
		}
		HRESULT STDMETHODCALLTYPE InitializeVisualTreeClone(
			CGlassReflectionVisual* clonedVisual,
			UINT cloneOption
		)
		{
			RETURN_IF_FAILED(CVisual::InitializeVisualTreeClone(clonedVisual, cloneOption));
			RETURN_IF_FAILED(clonedVisual->SetClip(m_geometry.get()));

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE CloneVisualTree(
			CGlassReflectionVisual** clonedVisual,
			UINT cloneOption
		)
		{
			auto cleanup = wil::scope_exit([clonedVisual]
			{
				if (clonedVisual)
				{
					(*clonedVisual)->Release();
					*clonedVisual = nullptr;
				}
			});
			RETURN_IF_FAILED(CGlassReflectionVisual::Create(clonedVisual));
			RETURN_IF_FAILED(CGlassReflectionVisual::InitializeVisualTreeClone(*clonedVisual, cloneOption));
			cleanup.release();
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE ValidateVisual()
		{
			RETURN_IF_FAILED(CVisual::ValidateVisual());

			RETURN_IF_FAILED(s_reflectionResources.EnsureSurface());
			const auto surfaceBrush = m_rootVisual.Brush().as<winrt::CompositionSurfaceBrush>();
			if (const auto surface = s_reflectionResources.GetSurface(); surfaceBrush.Surface() != surface)
			{
				surfaceBrush.Surface(surface);
			}

			// TO-Do: ...
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE SetClip(uDWM::CBaseGeometryProxy* geometry)
		{
			m_rootVisual.IsVisible(geometry != nullptr);
			if (m_geometry.get() != geometry)
			{
				m_geometry.copy_from(geometry);
				return GetVisualProxy()->SetClip(geometry);
			}

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE SetClip()
		{
			m_geometry = nullptr;
			m_rootVisual.IsVisible(true);
			return GetVisualProxy()->SetClip(nullptr);
		}

		static CGlassReflectionVisual* GetChildOf(CVisual* parent)
		{
			const auto it = s_visualParentMap.find(parent);
			return it == s_visualParentMap.end() ? nullptr : it->second;
		}
		static auto& Resources()
		{
			return s_reflectionResources;
		}
		static void RemoveAll()
		{
			const auto visualSet = std::move(s_visualSet);
			for (const auto& visual : visualSet)
			{
				const auto parent = visual->GetTransformParent();
				if (parent)
				{
					parent->GetVisualCollection()->Remove(visual);
				}
			}
			s_visualParentMap.clear();
		}
	};
}