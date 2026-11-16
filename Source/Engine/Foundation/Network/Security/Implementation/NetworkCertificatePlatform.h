/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkError.h>
#include <Foundation/Strings/StringView.h>

XII_FOUNDATION_INTERNAL_HEADER

namespace xiiNetworkSecurityInternal
{
  void ClearSecret(xiiArrayPtr<xiiUInt8> bytes);
  // Opaque borrowed SSL_CTX interop keeps native certificate-store headers separate from OpenSSL.
  xiiNetworkError LoadSystemTrust(void* pContext);
  xiiNetworkError LoadDefaultTrust(void* pContext);
  xiiNetworkError AddTrustCertificate(void* pContext, xiiArrayPtr<const xiiUInt8> bytes);
  xiiNetworkError LoadCredentialFile(xiiStringView sPath, xiiUInt32 uiMaximumBytes, xiiDynamicArray<xiiUInt8>& out_bytes);
} // namespace xiiNetworkSecurityInternal
