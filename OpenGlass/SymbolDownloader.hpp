#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Storage.Streams.h>

namespace OpenGlass
{
	struct CDownloadProgress
	{
		double percent{ -1.0 };              // -1 when Content-Length is unknown.
		uint64_t totalBytes{ 0 };            // 0 when unknown.
		uint64_t downloadedBytes{ 0 };       // Cumulative bytes written so far.
		uint32_t lastChunkBytes{ 0 };        // Size of the most recent block.
	};

	class CSymbolDownloader
	{
		winrt::Windows::Web::Http::HttpClient m_client{ nullptr };
	public:
		CSymbolDownloader();

		using Callback = void(const CDownloadProgress&);
		winrt::Windows::Foundation::IAsyncOperation<int> DownloadAsync(
			const winrt::hstring& url,
			const winrt::hstring& destinationPath,
			Callback* progressCallback
		);
	};

	struct CDownloadContext
	{
		LPCWSTR pdbFileName;
		LPCWSTR url;
	};

	winrt::Windows::Foundation::IAsyncOperation<int> DownloadSymbolForModuleAsync(
		CSymbolDownloader& downloader,
		HMODULE moduleHandle,
		LPCWSTR symbolServerBase,
		LPCWSTR destinationRoot,
		CDownloadContext** contextReceiver,
		CSymbolDownloader::Callback* progressCallback
	);
}
