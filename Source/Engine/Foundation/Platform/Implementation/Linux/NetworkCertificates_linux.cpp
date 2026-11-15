/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Foundation/Network/Security/Implementation/NetworkCertificatePlatform.h>

namespace xiiNetworkSecurityInternal
{
  xiiNetworkError LoadSystemTrust(void* pContext)
  {
    return LoadDefaultTrust(pContext);
  }
} // namespace xiiNetworkSecurityInternal

#endif
