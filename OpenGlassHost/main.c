#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <strsafe.h>
#include <pathcch.h>
#include "ServiceHandler.h"

typedef VOID(WINAPI* PFN_ServiceMain)(DWORD dwNumServicesArgs, LPWSTR* lpServiceArgVectors);

HMODULE g_openGlassModule = NULL;
PFN_ServiceMain g_openGlassServiceMain = NULL;

// Verifies Authenticode signature using an already-opened file HANDLE.
// Keeping the handle open with restrictive share mode mitigates TOCTOU between verification and load.
HRESULT VerifyAuthenticodeSignatureWithHandle(_In_ HANDLE fileHandle, _In_opt_ PCWSTR filePath)
{
	GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_FILE_INFO fileInfo = { 0 };
	WINTRUST_DATA trustData = { 0 };
	LONG status;

	if (!fileHandle || fileHandle == INVALID_HANDLE_VALUE)
	{
		return E_INVALIDARG;
	}

	fileInfo.cbStruct = sizeof(fileInfo);
	fileInfo.pcwszFilePath = filePath;
	fileInfo.hFile = fileHandle;

	trustData.cbStruct = sizeof(trustData);
	trustData.dwUIChoice = WTD_UI_NONE;
	trustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
	trustData.dwUnionChoice = WTD_CHOICE_FILE;
	trustData.pFile = &fileInfo;
	trustData.dwStateAction = WTD_STATEACTION_VERIFY;
	trustData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;

	status = WinVerifyTrust(NULL, &policyGUID, &trustData);

	trustData.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust(NULL, &policyGUID, &trustData);

	if (status == ERROR_SUCCESS)
	{
		return S_OK;
	}

	return (HRESULT)status;
}

static HRESULT ReadAllBytesFromFileHandle(
	_In_ HANDLE fileHandle,
	_Outptr_result_bytebuffer_(*cbData) BYTE** pbData,
	_Out_ DWORD* cbData
)
{
	LARGE_INTEGER fileSize;
	LARGE_INTEGER zero;
	BYTE* buffer;
	DWORD totalRead;

	if (!pbData || !cbData)
	{
		return E_INVALIDARG;
	}
	*pbData = NULL;
	*cbData = 0;

	if (!fileHandle || fileHandle == INVALID_HANDLE_VALUE)
	{
		return E_INVALIDARG;
	}

	if (!GetFileSizeEx(fileHandle, &fileSize))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (fileSize.QuadPart <= 0)
	{
		return E_FAIL;
	}
	if (fileSize.QuadPart > (LONGLONG)MAXDWORD)
	{
		return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
	}

	buffer = (BYTE*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)fileSize.QuadPart);
	if (!buffer)
	{
		return E_OUTOFMEMORY;
	}

	zero.QuadPart = 0;
	if (!SetFilePointerEx(fileHandle, zero, NULL, FILE_BEGIN))
	{
		HeapFree(GetProcessHeap(), 0, buffer);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	totalRead = 0;
	while (totalRead < (DWORD)fileSize.QuadPart)
	{
		DWORD bytesRead = 0;
		DWORD toRead = (DWORD)fileSize.QuadPart - totalRead;
		if (!ReadFile(fileHandle, buffer + totalRead, toRead, &bytesRead, NULL))
		{
			HeapFree(GetProcessHeap(), 0, buffer);
			return HRESULT_FROM_WIN32(GetLastError());
		}
		if (bytesRead == 0)
		{
			HeapFree(GetProcessHeap(), 0, buffer);
			return E_FAIL;
		}
		totalRead += bytesRead;
	}

	*pbData = buffer;
	*cbData = (DWORD)fileSize.QuadPart;
	return S_OK;
}

HRESULT GetSignerCertThumbprintSha1WithHandle(
	_In_ HANDLE fileHandle,
	_Out_writes_(thumbprintSize) BYTE* thumbprint,
	_In_ DWORD thumbprintSize
)
{
	HCERTSTORE hStore = NULL;
	HCRYPTMSG hMsg = NULL;
	DWORD dwEncoding = 0;
	DWORD dwContentType = 0;
	DWORD dwFormatType = 0;
	DWORD cbSignerInfo = 0;
	PCMSG_SIGNER_INFO pSignerInfo = NULL;
	CERT_INFO certInfo = { 0 };
	PCCERT_CONTEXT pCertContext = NULL;
	BYTE* fileBytes = NULL;
	DWORD fileBytesSize = 0;
	CRYPT_DATA_BLOB blob;
	BOOL ok;

	if (!fileHandle || fileHandle == INVALID_HANDLE_VALUE || !thumbprint || thumbprintSize < 20)
	{
		return E_INVALIDARG;
	}

	if (FAILED(ReadAllBytesFromFileHandle(fileHandle, &fileBytes, &fileBytesSize)))
	{
		return E_FAIL;
	}

	blob.cbData = fileBytesSize;
	blob.pbData = fileBytes;

	ok = CryptQueryObject(
		CERT_QUERY_OBJECT_BLOB,
		&blob,
		CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
		CERT_QUERY_FORMAT_FLAG_BINARY,
		0,
		&dwEncoding,
		&dwContentType,
		&dwFormatType,
		&hStore,
		&hMsg,
		NULL
	);
	UNREFERENCED_PARAMETER(dwEncoding);
	UNREFERENCED_PARAMETER(dwContentType);
	UNREFERENCED_PARAMETER(dwFormatType);
	if (!ok)
	{
		HeapFree(GetProcessHeap(), 0, fileBytes);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	ok = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &cbSignerInfo);
	if (!ok || !cbSignerInfo)
	{
		CertCloseStore(hStore, 0);
		CryptMsgClose(hMsg);
		HeapFree(GetProcessHeap(), 0, fileBytes);
		return E_FAIL;
	}

	pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, cbSignerInfo);
	if (!pSignerInfo)
	{
		CertCloseStore(hStore, 0);
		CryptMsgClose(hMsg);
		HeapFree(GetProcessHeap(), 0, fileBytes);
		return E_OUTOFMEMORY;
	}

	ok = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, pSignerInfo, &cbSignerInfo);
	if (!ok)
	{
		LocalFree(pSignerInfo);
		CertCloseStore(hStore, 0);
		CryptMsgClose(hMsg);
		HeapFree(GetProcessHeap(), 0, fileBytes);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	certInfo.Issuer = pSignerInfo->Issuer;
	certInfo.SerialNumber = pSignerInfo->SerialNumber;
	pCertContext = CertFindCertificateInStore(
		hStore,
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		0,
		CERT_FIND_SUBJECT_CERT,
		&certInfo,
		NULL
	);
	if (!pCertContext)
	{
		LocalFree(pSignerInfo);
		CertCloseStore(hStore, 0);
		CryptMsgClose(hMsg);
		HeapFree(GetProcessHeap(), 0, fileBytes);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	{
		DWORD cbHash = thumbprintSize;
		ok = CryptHashCertificate(
			0,
			CALG_SHA1,
			0,
			pCertContext->pbCertEncoded,
			pCertContext->cbCertEncoded,
			thumbprint,
			&cbHash
		);
		if (!ok || cbHash != 20)
		{
			CertFreeCertificateContext(pCertContext);
			LocalFree(pSignerInfo);
			CertCloseStore(hStore, 0);
			CryptMsgClose(hMsg);
			HeapFree(GetProcessHeap(), 0, fileBytes);
			return E_FAIL;
		}
	}

	CertFreeCertificateContext(pCertContext);
	LocalFree(pSignerInfo);
	CertCloseStore(hStore, 0);
	CryptMsgClose(hMsg);
	HeapFree(GetProcessHeap(), 0, fileBytes);
	return S_OK;
}

HRESULT LoadAndValidateOpenGlass(void)
{
	WCHAR dllPath[MAX_PATH];
	DWORD len;
	HRESULT hr;
	HANDLE dllFileHandle = INVALID_HANDLE_VALUE;

#if defined(OPENGLASS_ENFORCE_SIGNING) && (OPENGLASS_ENFORCE_SIGNING)
	BYTE exeThumb[20];
	BYTE dllThumb[20];
#endif

	// Build absolute path: <OpenGlassHost.exe dir>\OpenGlass.dll
	len = GetModuleFileNameW(NULL, dllPath, ARRAYSIZE(dllPath));
	if (!len || len >= ARRAYSIZE(dllPath))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	hr = PathCchRemoveFileSpec(dllPath, ARRAYSIZE(dllPath));
	if (FAILED(hr))
	{
		return hr;
	}
	hr = PathCchAppend(dllPath, ARRAYSIZE(dllPath), L"OpenGlass.dll");
	if (FAILED(hr))
	{
		return hr;
	}

	// Lock the DLL on disk to mitigate TOCTOU: prevent rename/replace while we verify and load.
	// Share mode intentionally excludes WRITE and DELETE.
	dllFileHandle = CreateFileW(
		dllPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (dllFileHandle == INVALID_HANDLE_VALUE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

#if defined(OPENGLASS_ENFORCE_SIGNING) && (OPENGLASS_ENFORCE_SIGNING)
	// ReleaseSigned: require a valid Authenticode signature.
	hr = VerifyAuthenticodeSignatureWithHandle(dllFileHandle, dllPath);
	if (FAILED(hr))
	{
		CloseHandle(dllFileHandle);
		return hr;
	}

	// If the host EXE itself is signed/valid, bind the DLL to the same signer certificate.
	// This avoids loading an unrelated (but validly signed) OpenGlass.dll.
	{
		WCHAR exePath[MAX_PATH];
		DWORD exeLen;
		HANDLE exeFileHandle = INVALID_HANDLE_VALUE;
		exeLen = GetModuleFileNameW(NULL, exePath, ARRAYSIZE(exePath));
		if (exeLen && exeLen < ARRAYSIZE(exePath))
		{
			exeFileHandle = CreateFileW(
				exePath,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (exeFileHandle != INVALID_HANDLE_VALUE && SUCCEEDED(VerifyAuthenticodeSignatureWithHandle(exeFileHandle, exePath)))
			{
				if (FAILED(GetSignerCertThumbprintSha1WithHandle(exeFileHandle, exeThumb, ARRAYSIZE(exeThumb))))
				{
					CloseHandle(exeFileHandle);
					CloseHandle(dllFileHandle);
					return E_FAIL;
				}
				if (FAILED(GetSignerCertThumbprintSha1WithHandle(dllFileHandle, dllThumb, ARRAYSIZE(dllThumb))))
				{
					CloseHandle(exeFileHandle);
					CloseHandle(dllFileHandle);
					return E_FAIL;
				}
				if (memcmp(exeThumb, dllThumb, 20) != 0)
				{
					CloseHandle(exeFileHandle);
					CloseHandle(dllFileHandle);
					return TRUST_E_SUBJECT_NOT_TRUSTED;
				}

				CloseHandle(exeFileHandle);
				exeFileHandle = INVALID_HANDLE_VALUE;
			}
			else if (exeFileHandle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(exeFileHandle);
				exeFileHandle = INVALID_HANDLE_VALUE;
			}
		}
	}
#endif

	g_openGlassModule = LoadLibraryExW(dllPath, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	// Safe to release the lock after the loader has opened/mapped the image.
	CloseHandle(dllFileHandle);
	dllFileHandle = INVALID_HANDLE_VALUE;
	if (!g_openGlassModule)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	g_openGlassServiceMain = (PFN_ServiceMain)GetProcAddress(g_openGlassModule, "ServiceMain");
	if (!g_openGlassServiceMain)
	{
		FreeLibrary(g_openGlassModule);
		g_openGlassModule = NULL;
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

VOID NTAPI tls_callback(
	PVOID DllHandle,
	DWORD Reason,
	PVOID Reserved
)
{
	UNREFERENCED_PARAMETER(DllHandle);
	UNREFERENCED_PARAMETER(Reserved);

	if (DLL_PROCESS_ATTACH == Reason)
	{
		BOOL isLocalSystem = FALSE;
		PSID sid = NULL;
		SID_IDENTIFIER_AUTHORITY authority = { SECURITY_NT_AUTHORITY };
		if (!AllocateAndInitializeSid(&authority, 1ul, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &sid))
		{
			__fastfail(HRESULT_FROM_WIN32(GetLastError()));
		}
		if (!CheckTokenMembership(NULL, sid, &isLocalSystem))
		{
			FreeSid(sid);
			sid = NULL;

			__fastfail(HRESULT_FROM_WIN32(GetLastError()));
		}

		FreeSid(sid);
		sid = NULL;

		if (!isLocalSystem)
		{
			__fastfail(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
		}

		{
			PROCESS_MITIGATION_IMAGE_LOAD_POLICY policy = { 0 };
			policy.NoLowMandatoryLabelImages = 1;
			policy.NoRemoteImages = 1;
			policy.PreferSystem32Images = 1;
			SetProcessMitigationPolicy(ProcessImageLoadPolicy, &policy, sizeof(policy));
		}
		{
			PROCESS_MITIGATION_DYNAMIC_CODE_POLICY policy = { 0 };
			policy.ProhibitDynamicCode = 1;
			SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &policy, sizeof(policy));
		}
		{
			PROCESS_MITIGATION_CHILD_PROCESS_POLICY policy = { 0 };
			policy.NoChildProcessCreation = 1;
			SetProcessMitigationPolicy(ProcessChildProcessPolicy, &policy, sizeof(policy));
		}
		{
			PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY policy = { 0 };
			policy.DisallowFsctlSystemCalls = 1;
			SetProcessMitigationPolicy(ProcessSystemCallDisablePolicy, &policy, sizeof(policy));
		}
		{
			PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY policy = { 0 };
			policy.HandleExceptionsPermanentlyEnabled = 1;
			policy.RaiseExceptionOnInvalidHandleReference = 1;
			SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &policy, sizeof(policy));
		}
	}
}

ULONG _tls_index = 0;
PIMAGE_TLS_CALLBACK _tls_callback[] = { tls_callback, NULL };

#pragma const_seg(".rdata$T")
EXTERN_C const IMAGE_TLS_DIRECTORY64 _tls_used =
{
	0,							// StartAddressOfRawData
	0,							// EndAddressOfRawData
	(ULONG_PTR)&_tls_index,     // AddressOfIndex
	(ULONG_PTR)_tls_callback,   // AddressOfCallBacks
	0,                          // SizeOfZeroFill
	0                           // Characteristics
};
#pragma const_seg()

#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:_tls_callback")

int main(void)
{
	HRESULT hr = S_OK;

	hr = LoadAndValidateOpenGlass();
	if (FAILED(hr))
	{
		__fastfail(hr);
	}

	// Service dispatch table: maps service name to entry point
	SERVICE_TABLE_ENTRYW serverTableEntry[] =
	{
		{ SERVICE_NAME, g_openGlassServiceMain },
		{ NULL, NULL }
	};

	// Start the service control dispatcher
	// This call blocks until the service is stopped
	if (!StartServiceCtrlDispatcherW(serverTableEntry))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		__fastfail(hr);
	}

	ExitProcess(S_OK);
}
