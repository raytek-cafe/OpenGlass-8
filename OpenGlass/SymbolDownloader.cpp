#include "pch.h"
#include "Util.hpp"
#include "SymbolDownloader.hpp"
#include <winrt/Windows.Storage.Streams.h>

namespace OpenGlass
{
	// https://deplinenoise.wordpress.com/2013/06/14/getting-your-pdb-name-from-a-running-executable-windows/
	struct PdbInfo
	{
		DWORD     Signature;
		GUID      Guid;
		DWORD     Age;
		char      PdbFileName[1];
	};
}
namespace winrt
{
	using namespace Windows::Web::Http;
	using namespace Windows::Foundation;
}

OpenGlass::CSymbolDownloader::CSymbolDownloader() : m_client{ winrt::HttpClient{} } {}

winrt::IAsyncOperation<int> OpenGlass::CSymbolDownloader::DownloadAsync(
	const winrt::hstring& url,
	const winrt::hstring& destinationPath,
	Callback* progressCallback
) try
{
	// Notify UI that we started.
	if (progressCallback)
	{
		progressCallback(CDownloadProgress{ -1.0 });
	}

	const winrt::Uri uri{ url };
	const winrt::HttpRequestMessage request{ winrt::HttpMethod::Get(), uri };
	const winrt::HttpResponseMessage response = co_await m_client.SendRequestAsync(request, winrt::HttpCompletionOption::ResponseHeadersRead);
	response.EnsureSuccessStatusCode();

	uint64_t totalBytes = 0;
	if (auto length = response.Content().Headers().ContentLength())
	{
		totalBytes = length.Value();
	}

	const auto stream = co_await response.Content().ReadAsInputStreamAsync();
	winrt::Windows::Storage::Streams::Buffer buffer{ 128 * 1024 };
	
	if (PathFileExistsW(destinationPath.c_str()))
	{
		std::filesystem::remove_all(destinationPath.c_str());
	}
	auto file = wil::open_or_truncate_existing_file(
		destinationPath.c_str(),
		GENERIC_WRITE,
		0
	);

	// weird...
	bool completed = false;
	const auto cleanup = wil::scope_exit([&]
	{
		file.reset();
		if (!completed)
		{
			std::filesystem::remove(destinationPath.c_str());
		}
	});

	uint64_t downloaded = 0;
	for (;;)
	{
		const auto read = co_await stream.ReadAsync(buffer, buffer.Capacity(), winrt::Windows::Storage::Streams::InputStreamOptions::Partial);
		if (read.Length() == 0)
		{
			break;
		}

		DWORD written = 0;
		THROW_IF_WIN32_BOOL_FALSE(WriteFile(file.get(), read.data(), read.Length(), &written, nullptr));

		downloaded += read.Length();

		double pct = -1.0;
		if (totalBytes > 0)
		{
			pct = (static_cast<double>(downloaded) / static_cast<double>(totalBytes)) * 100.0;
		}

		if (progressCallback)
		{
			progressCallback(CDownloadProgress{
				pct,
				totalBytes,
				downloaded,
				static_cast<uint32_t>(read.Length())
			});
		}
	}

	completed = true;
	co_return S_OK;
}
catch (...)
{
	co_return wil::ResultFromCaughtException();
}

winrt::Windows::Foundation::IAsyncOperation<int> OpenGlass::DownloadSymbolForModuleAsync(
	CSymbolDownloader& downloader,
	HMODULE moduleHandle,
	LPCWSTR symbolServerBase,
	LPCWSTR destinationRoot,
	CDownloadContext** contextReceiver,
	CSymbolDownloader::Callback* progressCallback
) try
{
	PdbInfo* pdbInfo{ nullptr };
	WCHAR url[MAX_PATH]{};
	std::unique_ptr<WCHAR[]> pdbFileName{};
	CDownloadContext context{ url };
	wcscpy_s(url, symbolServerBase);

	{
		std::wstring filePath{};
		THROW_IF_FAILED((wil::GetModuleFileNameW<std::wstring, MAX_PATH>(moduleHandle, filePath)));

		wil::unique_hfile file
		{
			CreateFileW(
				filePath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			)
		};
		THROW_LAST_ERROR_IF(!file.is_valid());

		wil::unique_handle fileMapping
		{
			CreateFileMappingW(
				file.get(),
				nullptr,
				PAGE_READONLY,
				0,
				0,
				nullptr
			)
		};
		THROW_LAST_ERROR_IF_NULL(fileMapping);

		wil::unique_mapview_ptr<void> imageBase
		{
			MapViewOfFile(
				fileMapping.get(),
				FILE_MAP_READ,
				0,
				0,
				0
			)
		};
		THROW_LAST_ERROR_IF_NULL(imageBase);

		ULONG size = 0;
		const auto debugDirectory = reinterpret_cast<PIMAGE_DEBUG_DIRECTORY>(
			ImageDirectoryEntryToDataEx(
				imageBase.get(),
				FALSE,
				IMAGE_DIRECTORY_ENTRY_DEBUG,
				&size,
				nullptr
			)
		);
		for (uint32_t i = 0; i < size / sizeof(IMAGE_DEBUG_DIRECTORY); ++i)
		{
			if (debugDirectory[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
			{
				pdbInfo = reinterpret_cast<PdbInfo*>(reinterpret_cast<ULONG_PTR>(imageBase.get()) + debugDirectory[i].PointerToRawData);

				if (constexpr DWORD RSDS = 'SDSR'; *reinterpret_cast<DWORD const*>(&pdbInfo->Signature) == RSDS)
				{
					THROW_IF_FAILED(
						Util::MB2WC(
							pdbFileName,
							pdbInfo->PdbFileName
						)
					);
					wcscat_s(url, MAX_PATH, pdbFileName.get());
					wcscat_s(url, L"/");
					// https://stackoverflow.com/questions/1672677/print-a-guid-variable
					swprintf_s(
						url + wcslen(url),
						33,
						L"%08lX%04hX%04hX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
						pdbInfo->Guid.Data1,
						pdbInfo->Guid.Data2,
						pdbInfo->Guid.Data3,
						pdbInfo->Guid.Data4[0],
						pdbInfo->Guid.Data4[1],
						pdbInfo->Guid.Data4[2],
						pdbInfo->Guid.Data4[3],
						pdbInfo->Guid.Data4[4],
						pdbInfo->Guid.Data4[5],
						pdbInfo->Guid.Data4[6],
						pdbInfo->Guid.Data4[7]
					);
					swprintf_s(
						url + wcslen(url),
						4,
						L"%x/",
						pdbInfo->Age
					);
					wcscat_s(url, MAX_PATH, pdbFileName.get());
					break;
				}
			}
		}
	}

	WCHAR pdbFilePath[MAX_PATH]{};
	THROW_IF_FAILED(
		PathCchCombine(
			pdbFilePath,
			MAX_PATH,
			destinationRoot,
			pdbFileName.get()
		)
	);

	const auto contextScope = wil::scope_exit([contextReceiver]
	{
		if (contextReceiver)
		{
			*contextReceiver = nullptr;
		}
	});
	if (contextReceiver)
	{
		context.pdbFileName = pdbFileName.get();
		context.url = url;
		*contextReceiver = &context;
	}
	HRESULT hr = co_await downloader.DownloadAsync(url, pdbFilePath, progressCallback);
	co_return hr;
}
catch (...)
{
	co_return wil::ResultFromCaughtException();
}
