/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/Reflection/Implementation/NetworkSchemaInternal.h>
#include <Foundation/Network/Serialization/NetworkSerializer.h>

XII_FOUNDATION_INTERNAL_HEADER

class xiiNetworkSerializationAccess
{
public:
  static xiiDynamicArray<xiiUInt8>& Scratch(xiiNetworkSerializationContext& context) { return context.m_Scratch; }
  static void                       TrackBudget(xiiNetworkSerializationContext& context, xiiUInt64 uiBytes) { context.m_uiPeakDecodedBytes = xiiMath::Max(context.m_uiPeakDecodedBytes, uiBytes); }
};

namespace xiiNetworkSerializationInternal
{
  using namespace xiiNetworkReflectionInternal;
  const xiiRTTI*      ValueType(const xiiVariant& value);
  xiiNetworkDataError WriteBorrowedValue(const CompiledValue& value, const void* pObject, const xiiNetworkFieldSchema& policy,
                                         xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth);
  xiiNetworkDataError WriteValue(const CompiledValue& value, const xiiVariant& object, const xiiNetworkFieldSchema& policy,
                                 xiiNetworkBinaryWriter& writer, xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth);
  xiiNetworkDataError ReadValue(const CompiledValue& value, xiiVariant& ref_object, const xiiNetworkFieldSchema& policy,
                                xiiNetworkBinaryReader& reader, xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth);
  xiiNetworkDataError WriteObject(const CompiledSchema& schema, const void* pObject, xiiNetworkBinaryWriter& writer,
                                  xiiNetworkEncoding encoding, const xiiNetworkSerializationLimits& limits, xiiUInt32 uiDepth);
  xiiNetworkDataError ReadObject(const CompiledSchema& schema, void* pObject, xiiNetworkBinaryReader& reader,
                                 xiiNetworkEncoding encoding, xiiNetworkDecodeBudget& budget, xiiUInt32 uiDepth, bool bRequireFinished = false);
} // namespace xiiNetworkSerializationInternal
