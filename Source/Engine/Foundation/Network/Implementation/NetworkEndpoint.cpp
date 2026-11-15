/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkEndpoint.h>

#include <Foundation/Strings/StringBuilder.h>

xiiNetworkError xiiNetworkEndpoint::Parse(xiiStringView sText, xiiNetworkEndpoint& out_endpoint)
{
  if (sText.IsEmpty())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  const char* pStart      = sText.GetStartPointer();
  const char* pEnd        = sText.GetEndPointer();
  const char* pAddressEnd = nullptr;
  const char* pPort       = nullptr;
  const bool  bBracketed  = *pStart == '[';
  if (bBracketed)
  {
    ++pStart;
    for (const char* pCharacter = pStart; pCharacter != pEnd; ++pCharacter)
    {
      if (*pCharacter == ']')
      {
        pAddressEnd = pCharacter;
        if (pEnd - pCharacter >= 3 && pCharacter[1] == ':')
        {
          pPort = pCharacter + 2;
        }
        break;
      }
    }
  }
  else
  {
    for (const char* pCharacter = pStart; pCharacter != pEnd; ++pCharacter)
    {
      if (*pCharacter == ':')
      {
        pAddressEnd = pCharacter;
        pPort       = pCharacter + 1;
        break;
      }
    }
  }
  if (pPort == nullptr || pPort == pEnd || pAddressEnd == pStart)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  xiiUInt32 uiPort = 0;
  for (const char* pCharacter = pPort; pCharacter != pEnd; ++pCharacter)
  {
    if (*pCharacter < '0' || *pCharacter > '9')
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    uiPort = uiPort * 10 + static_cast<xiiUInt32>(*pCharacter - '0');
    if (uiPort > 65535)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
  }
  xiiNetworkAddress     address;
  const xiiNetworkError error = xiiNetworkAddress::Parse(xiiStringView(pStart, pAddressEnd), address);
  if (error.Failed())
  {
    return error;
  }
  if (bBracketed != (address.GetFamily() == xiiNetworkAddressFamily::Ipv6))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  out_endpoint = xiiNetworkEndpoint(address, static_cast<xiiUInt16>(uiPort));
  return {};
}

void xiiNetworkEndpoint::ToString(xiiStringBuilder& out_sText) const
{
  xiiStringBuilder sAddress;
  m_Address.ToString(sAddress);
  out_sText.Clear();
  if (m_Address.GetFamily() == xiiNetworkAddressFamily::Ipv6)
  {
    out_sText.SetFormat("[{}]:{}", sAddress, m_uiPort);
  }
  else if (IsValid())
  {
    out_sText.SetFormat("{}:{}", sAddress, m_uiPort);
  }
}

xiiUInt32 xiiNetworkEndpoint::ComputeHash() const
{
  return xiiHashingUtils::CombineHashValues32(m_Address.ComputeHash(), m_uiPort);
}
