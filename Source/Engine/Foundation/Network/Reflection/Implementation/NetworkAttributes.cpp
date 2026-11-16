/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Reflection/NetworkAttributes.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNetworkAttribute, 1, xiiRTTIDefaultAllocator<xiiNetworkAttribute>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("FieldId", m_uiFieldId),
      XII_MEMBER_PROPERTY("Optional", m_bOptional),
      XII_MEMBER_PROPERTY("MaximumElements", m_uiMaximumElements),
      XII_MEMBER_PROPERTY("MaximumStringBytes", m_uiMaximumStringBytes),
      XII_MEMBER_PROPERTY("CodecId", m_uiCodecId),
      XII_MEMBER_PROPERTY("QuantizationBits", m_uiQuantizationBits),
      XII_MEMBER_PROPERTY("Minimum", m_fMinimum),
      XII_MEMBER_PROPERTY("Maximum", m_fMaximum),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNetworkIdentityAttribute, 1, xiiRTTIDefaultAllocator<xiiNetworkIdentityAttribute>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("TypeId", m_uiTypeId),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNetworkRpcAttribute, 1, xiiRTTIDefaultAllocator<xiiNetworkRpcAttribute>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("MethodId", m_uiMethodId),
      XII_MEMBER_PROPERTY("MaximumArgumentBytes", m_uiMaximumArgumentBytes),
      XII_MEMBER_PROPERTY("TimeoutMilliseconds", m_uiTimeoutMilliseconds),
      XII_MEMBER_PROPERTY("AllowFireAndForget", m_bAllowFireAndForget),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
