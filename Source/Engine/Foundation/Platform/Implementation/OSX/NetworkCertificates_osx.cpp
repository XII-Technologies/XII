/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Network/Security/Implementation/NetworkCertificatePlatform.h>

#  include <Security/Security.h>

#  include <climits>

namespace xiiNetworkSecurityInternal
{
  xiiNetworkError LoadSystemTrust(void* pContext)
  {
    CFArrayRef     pAnchors = nullptr;
    const OSStatus iStatus  = ::SecTrustCopyAnchorCertificates(&pAnchors);
    if (iStatus != errSecSuccess || pAnchors == nullptr)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InitializationFailed, iStatus, xiiNetworkErrorDomain::CredentialFile);
    }
    xiiNetworkError error;
    const CFIndex   iCount = ::CFArrayGetCount(pAnchors);
    for (CFIndex iIndex = 0; iIndex < iCount; ++iIndex)
    {
      const auto pCertificate = reinterpret_cast<SecCertificateRef>(const_cast<void*>(::CFArrayGetValueAtIndex(pAnchors, iIndex)));
      CFDataRef  pData        = ::SecCertificateCopyData(pCertificate);
      if (pData == nullptr)
      {
        error = xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
        break;
      }
      const CFIndex iLength = ::CFDataGetLength(pData);
      error                 = iLength > 0 && iLength <= INT_MAX ? AddTrustCertificate(pContext, {::CFDataGetBytePtr(pData), static_cast<xiiUInt32>(iLength)}) : xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
      ::CFRelease(pData);
      if (error.Failed())
      {
        break;
      }
    }
    ::CFRelease(pAnchors);
    return iCount > 0 ? error : xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
  }
} // namespace xiiNetworkSecurityInternal

#endif
