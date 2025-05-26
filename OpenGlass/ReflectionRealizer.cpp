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

	const auto worldTransform3D = input.worldTransform->GetD3DMatrix();
	context->DrawBitmap(
		m_reflectionBitmap.get(),
		input.viewport,
		input.intensity,
		D2D1_INTERPOLATION_MODE_LINEAR,
		nullptr,
		&worldTransform3D
	);

	return S_OK;
}