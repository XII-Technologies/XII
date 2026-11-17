/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkLocalEndpoint.h>

#include <cstring>

xiiNetworkError xiiNetworkLocalEndpoint::Make(xiiStringView sName, xiiNetworkLocalEndpoint& out_endpoint)
{
  xiiNetworkLocalEndpoint endpoint;
  if (sName.IsEmpty() || sName.GetElementCount() >= sizeof(endpoint.m_szName))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }

  for (xiiUInt32 uiIndex = 0; uiIndex < sName.GetElementCount(); ++uiIndex)
  {
    const char c = sName.GetStartPointer()[uiIndex];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
    }
  }

  std::memcpy(endpoint.m_szName, sName.GetStartPointer(), sName.GetElementCount());
  out_endpoint = endpoint;
  return {};
}
