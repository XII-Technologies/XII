/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Serialization/NetworkSerializationContext.h>

xiiNetworkDataError xiiNetworkDecodeBudget::Reserve(xiiUInt64 uiBytes)
{
  if (m_uiUsedBytes > m_Limits.m_uiMaximumDecodedBytes || uiBytes > m_Limits.m_uiMaximumDecodedBytes - m_uiUsedBytes ||
      m_uiUsedBytes > m_Limits.m_uiMaximumTemporaryBytes || uiBytes > m_Limits.m_uiMaximumTemporaryBytes - m_uiUsedBytes)
  {
    return xiiNetworkDataError(xiiNetworkDataErrorCode::AllocationLimit);
  }
  m_uiUsedBytes += uiBytes;
  return {};
}
