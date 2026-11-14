/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkAddress.h>

#include <Foundation/Strings/StringBuilder.h>

#include <cstring>

xiiNetworkAddress xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily family)
{
  xiiNetworkAddress address;
  if (family == xiiNetworkAddressFamily::Ipv4 || family == xiiNetworkAddressFamily::Ipv6)
  {
    address.m_Family = family;
  }
  return address;
}

xiiNetworkAddress xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily family)
{
  xiiNetworkAddress address = MakeWildcard(family);
  if (family == xiiNetworkAddressFamily::Ipv4)
  {
    address.m_uiBytes[0] = 127;
    address.m_uiBytes[3] = 1;
  }
  else if (family == xiiNetworkAddressFamily::Ipv6)
  {
    address.m_uiBytes[15] = 1;
  }
  return address;
}

xiiNetworkError xiiNetworkAddress::FromBytes(xiiArrayPtr<const xiiUInt8> bytes, xiiUInt32 uiScopeId, xiiNetworkAddress& out_address)
{
  if ((bytes.GetCount() != 4 && bytes.GetCount() != 16) || (bytes.GetCount() == 4 && uiScopeId != 0))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  xiiNetworkAddress address;
  address.m_Family    = bytes.GetCount() == 4 ? xiiNetworkAddressFamily::Ipv4 : xiiNetworkAddressFamily::Ipv6;
  address.m_uiScopeId = uiScopeId;
  std::memcpy(address.m_uiBytes, bytes.GetPtr(), bytes.GetCount());
  out_address = address;
  return {};
}

xiiArrayPtr<const xiiUInt8> xiiNetworkAddress::GetBytes() const
{
  return {m_uiBytes, m_Family == xiiNetworkAddressFamily::Ipv4 ? 4U : (m_Family == xiiNetworkAddressFamily::Ipv6 ? 16U : 0U)};
}

bool xiiNetworkAddress::IsWildcard() const
{
  if (!IsValid())
  {
    return false;
  }
  for (xiiUInt8 uiByte : GetBytes())
  {
    if (uiByte != 0)
    {
      return false;
    }
  }
  return true;
}

bool xiiNetworkAddress::IsLoopback() const
{
  if (m_Family == xiiNetworkAddressFamily::Ipv4)
  {
    return m_uiBytes[0] == 127;
  }
  if (m_Family != xiiNetworkAddressFamily::Ipv6 || m_uiBytes[15] != 1)
  {
    return false;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 15; ++uiIndex)
  {
    if (m_uiBytes[uiIndex] != 0)
    {
      return false;
    }
  }
  return true;
}

bool xiiNetworkAddress::IsMulticast() const
{
  return (m_Family == xiiNetworkAddressFamily::Ipv4 && (m_uiBytes[0] & 0xF0) == 0xE0) ||
    (m_Family == xiiNetworkAddressFamily::Ipv6 && m_uiBytes[0] == 0xFF);
}

bool xiiNetworkAddress::operator==(const xiiNetworkAddress& other) const
{
  return m_Family == other.m_Family && m_uiScopeId == other.m_uiScopeId && std::memcmp(m_uiBytes, other.m_uiBytes, sizeof(m_uiBytes)) == 0;
}

xiiUInt32 xiiNetworkAddress::ComputeHash() const
{
  // Hash fields explicitly: padding must not affect value equality.
  xiiUInt32 uiHash = xiiHashingUtils::xxHash32(m_uiBytes, sizeof(m_uiBytes));
  uiHash           = xiiHashingUtils::CombineHashValues32(uiHash, m_uiScopeId);
  return xiiHashingUtils::CombineHashValues32(uiHash, static_cast<xiiUInt32>(m_Family));
}

void xiiNetworkAddress::ToString(xiiStringBuilder& out_sText) const
{
  out_sText.Clear();
  if (m_Family == xiiNetworkAddressFamily::Ipv4)
  {
    out_sText.SetFormat("{}.{}.{}.{}", m_uiBytes[0], m_uiBytes[1], m_uiBytes[2], m_uiBytes[3]);
  }
  else if (m_Family == xiiNetworkAddressFamily::Ipv6)
  {
    // RFC 5952: compress the first longest run of at least two zero words, lowercase hex.
    xiiUInt16 uiWords[8];
    xiiUInt32 uiBestStart  = 8;
    xiiUInt32 uiBestLength = 1;
    for (xiiUInt32 uiIndex = 0; uiIndex < 8; ++uiIndex)
    {
      uiWords[uiIndex] = static_cast<xiiUInt16>((m_uiBytes[uiIndex * 2] << 8) | m_uiBytes[uiIndex * 2 + 1]);
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < 8;)
    {
      if (uiWords[uiIndex] != 0)
      {
        ++uiIndex;
        continue;
      }
      const xiiUInt32 uiStart = uiIndex;
      while (uiIndex < 8 && uiWords[uiIndex] == 0)
      {
        ++uiIndex;
      }
      if (uiIndex - uiStart > uiBestLength)
      {
        uiBestStart  = uiStart;
        uiBestLength = uiIndex - uiStart;
      }
    }
    for (xiiUInt32 uiIndex = 0; uiIndex < 8;)
    {
      if (uiIndex == uiBestStart)
      {
        out_sText.Append("::");
        uiIndex += uiBestLength;
        continue;
      }
      if (uiIndex > 0 && uiIndex != uiBestStart + uiBestLength)
      {
        out_sText.Append(":");
      }
      out_sText.AppendFormat("{0}", xiiArgU(uiWords[uiIndex], 1, false, 16, false));
      ++uiIndex;
    }
    if (m_uiScopeId != 0)
    {
      out_sText.AppendFormat("%%{}", m_uiScopeId);
    }
  }
}
