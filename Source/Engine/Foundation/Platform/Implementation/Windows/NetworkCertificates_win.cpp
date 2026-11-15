/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Network/Security/Implementation/NetworkCertificatePlatform.h>
#  include <Foundation/Strings/StringConversion.h>

#  include <wincrypt.h>

#  include <cstring>
#  include <utility>

namespace xiiNetworkSecurityInternal
{
  xiiNetworkError LoadSystemTrust(void* pContext)
  {
    xiiUInt32 uiImported = 0;
    for (DWORD uiLocation : {CERT_SYSTEM_STORE_CURRENT_USER, CERT_SYSTEM_STORE_LOCAL_MACHINE})
    {
      HCERTSTORE hStore = ::CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, uiLocation | CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, L"ROOT");
      if (hStore == nullptr)
      {
        return xiiNetworkError(xiiNetworkErrorCode::InitializationFailed, static_cast<xiiInt32>(::GetLastError()), xiiNetworkErrorDomain::CredentialFile);
      }
      PCCERT_CONTEXT  pCertificate = nullptr;
      xiiNetworkError error;
      while ((pCertificate = ::CertEnumCertificatesInStore(hStore, pCertificate)) != nullptr)
      {
        error = AddTrustCertificate(pContext, {pCertificate->pbCertEncoded, pCertificate->cbCertEncoded});
        if (error.Failed())
        {
          ::CertFreeCertificateContext(pCertificate);
          break;
        }
        ++uiImported;
      }
      ::CertCloseStore(hStore, 0);
      if (error.Failed())
      {
        return error;
      }
    }
    return uiImported != 0 ? xiiNetworkError() : xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
  }

  xiiNetworkError LoadCredentialFile(xiiStringView sPath, xiiUInt32 uiMaximumBytes, xiiDynamicArray<xiiUInt8>& out_bytes)
  {
    if (sPath.IsEmpty() || std::memchr(sPath.GetStartPointer(), 0, sPath.GetElementCount()) != nullptr)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    HANDLE hFile = ::CreateFileW(xiiStringWChar(sPath).GetData(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
      return xiiNetworkError(xiiNetworkErrorCode::PermissionDenied, static_cast<xiiInt32>(::GetLastError()), xiiNetworkErrorDomain::CredentialFile);
    }
    LARGE_INTEGER size = {};
    if (::GetFileType(hFile) != FILE_TYPE_DISK || !::GetFileSizeEx(hFile, &size) || size.QuadPart <= 0 || static_cast<xiiUInt64>(size.QuadPart) > uiMaximumBytes)
    {
      ::CloseHandle(hFile);
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(static_cast<xiiUInt32>(size.QuadPart));
    xiiUInt32 uiRead = 0;
    while (uiRead < bytes.GetCount())
    {
      DWORD uiCount = 0;
      if (!::ReadFile(hFile, bytes.GetData() + uiRead, bytes.GetCount() - uiRead, &uiCount, nullptr) || uiCount == 0)
      {
        const auto error = ::GetLastError();
        ::SecureZeroMemory(bytes.GetData(), bytes.GetCount());
        ::CloseHandle(hFile);
        return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument, static_cast<xiiInt32>(error), xiiNetworkErrorDomain::CredentialFile);
      }
      uiRead += uiCount;
    }
    ::CloseHandle(hFile);
    out_bytes = std::move(bytes);
    return {};
  }
} // namespace xiiNetworkSecurityInternal

#endif
