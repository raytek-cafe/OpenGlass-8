#include "pch.h"
#include "Shared.hpp"
#include "uDWMProjection.hpp"
#include "ReflectionRealizer.hpp"

using namespace OpenGlass;

HRESULT CReflectionRealizer::LoadTexture(ID2D1DeviceContext* context)
{
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

	RETURN_IF_FAILED(
		context->CreateBitmapFromWicBitmap(
			wicConverter.get(),
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			),
			m_reflectionBitmap.put()
		)
	);

	return S_OK;
}

HRESULT CReflectionRealizer::Render(
	ID2D1DeviceContext* context,
	const ReflectionInput& input
)
{
	if (!m_reflectionBitmap)
	{
		RETURN_IF_FAILED(LoadTexture(context));
	}

	const auto reflectionSize = m_reflectionBitmap->GetSize();
	const auto visualWorldTransform2D = input.visualWorldTransform->GetD2DMatrix();
	
	static ULONGLONG s_frameId{};
	static D2D1_SIZE_F s_desktopSize{};
	static D2D1_POINT_2F s_desktopOrigin{};
	if (const auto frameId = dwmcore::GetCurrentFrameId(); s_frameId != frameId)
	{
		s_desktopSize =
		{
			static_cast<float>(GetSystemMetrics(SM_CXVIRTUALSCREEN)),
			static_cast<float>(GetSystemMetrics(SM_CYVIRTUALSCREEN))
		};
		s_desktopOrigin =
		{
			static_cast<float>(GetSystemMetrics(SM_XVIRTUALSCREEN)),
			static_cast<float>(GetSystemMetrics(SM_YVIRTUALSCREEN))
		};
		s_frameId = frameId;
	}

	// if the visual is rtl, the rect is mirrored
	const auto reflectionDesktopRect = RectF::TransformRect(*input.shapeLocalBounds, visualWorldTransform2D);
	D2D1_RECT_F clippedReflectionDesktopRect
	{
		std::max(reflectionDesktopRect.left, 0.f),
		std::max(reflectionDesktopRect.top, 0.f),
		std::min(reflectionDesktopRect.right, s_desktopOrigin.x + s_desktopSize.width),
		std::min(reflectionDesktopRect.bottom, s_desktopOrigin.x + s_desktopSize.height)
	};

	D2D1_RECT_F sourceRect
	{
		clippedReflectionDesktopRect.left / s_desktopSize.width * reflectionSize.width,
		clippedReflectionDesktopRect.top / s_desktopSize.height * reflectionSize.height,
		clippedReflectionDesktopRect.right / s_desktopSize.width * reflectionSize.width,
		clippedReflectionDesktopRect.bottom / s_desktopSize.height * reflectionSize.height
	};

	const auto worldTransform2D = input.worldTransform->GetD2DMatrix();
	auto shapeLocalBounds = *input.shapeLocalBounds;
	shapeLocalBounds.left += clippedReflectionDesktopRect.left - reflectionDesktopRect.left;
	shapeLocalBounds.top += clippedReflectionDesktopRect.top - reflectionDesktopRect.top;
	shapeLocalBounds.right += clippedReflectionDesktopRect.right - reflectionDesktopRect.right;
	shapeLocalBounds.bottom += clippedReflectionDesktopRect.bottom - reflectionDesktopRect.bottom;

	if (input.reflectionParallaxIntensity)
	{
		const auto width = wil::rect_width(sourceRect);
		sourceRect.left -= input.reflectionParallaxIntensity * (reflectionDesktopRect.left / s_desktopSize.width * reflectionSize.width);
		sourceRect.right = sourceRect.left + width;
	}

	const auto worldTransform3D = input.worldTransform->GetD3DMatrix();
	context->SetTransform(worldTransform2D);
	context->DrawBitmap(
		m_reflectionBitmap.get(),
		&shapeLocalBounds,
		input.reflectionIntensity,
		D2D1_INTERPOLATION_MODE_LINEAR,
		&sourceRect,
		nullptr
	);
	context->SetTransform(D2D1::IdentityMatrix());

	return S_OK;
}