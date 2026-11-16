/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Reflection/NetworkSchema.h>
#include <Foundation/Network/Serialization/NetworkSerializationContext.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/SharedPtr.h>

class xiiRTTI;
class xiiNetworkTypeRegistryState;
class xiiNetworkRegistryAccess;

/// Trusted extension. Decode must reserve its allocation budget before allocating and return exactly
/// the registered value type. The bounded reader/writer always enforce encoded-size limits.
class XII_FOUNDATION_DLL xiiNetworkCustomCodec : public xiiRefCounted
{
public:
  [[nodiscard]] virtual const xiiRTTI*      GetType() const                                                                                     = 0;
  [[nodiscard]] virtual xiiUInt64           GetWireId() const                                                                                   = 0;
  [[nodiscard]] virtual xiiUInt32           GetVersion() const                                                                                  = 0;
  [[nodiscard]] virtual xiiNetworkDataError Encode(const xiiVariant& value, xiiNetworkBinaryWriter& writer) const                               = 0;
  [[nodiscard]] virtual xiiNetworkDataError Decode(xiiNetworkBinaryReader& reader, xiiNetworkDecodeBudget& budget, xiiVariant& out_value) const = 0;
};

struct XII_FOUNDATION_DLL xiiNetworkSchemaStatistics
{
  xiiUInt32 m_uiSchemas            = 0;
  xiiUInt32 m_uiValidationFailures = 0;
  xiiTime   m_ConstructionTime;
  xiiUInt64 m_uiGeneration                = 0;
  xiiUInt64 m_uiSerializationOperations   = 0;
  xiiUInt64 m_uiDeserializationOperations = 0;
  xiiUInt64 m_uiSerializationFailures     = 0;
  xiiUInt64 m_uiDeserializationFailures   = 0;
  xiiUInt64 m_uiBytesEncoded              = 0;
  xiiUInt64 m_uiBytesDecoded              = 0;
  xiiUInt32 m_uiPeakEncodedBytes          = 0;
};

/// Automatically discovers attributed RTTI. Separate instances have independent codec policy/caches.
/// Serialization and lookups may be concurrent. Refresh/codec changes drain active metadata users;
/// do not initiate registry mutation or plugin unload from a serialization/dispatch callback.
/// Plugin changes pause admission, drain operations and invalidate metadata before unloading code.
/// Manually created RTTI must be removed only between Invalidate() and Refresh(), with no live objects.
/// Destroy the registry after all serializers/endpoints stop accessing it.
class XII_FOUNDATION_DLL xiiNetworkTypeRegistry final
{
public:
  xiiNetworkTypeRegistry();
  ~xiiNetworkTypeRegistry();
  xiiNetworkTypeRegistry(const xiiNetworkTypeRegistry&)                             = delete;
  xiiNetworkTypeRegistry&                  operator=(const xiiNetworkTypeRegistry&) = delete;
  [[nodiscard]] xiiNetworkDataError        Refresh();
  void                                     Invalidate();
  [[nodiscard]] xiiNetworkDataError        GetSchema(const xiiRTTI* pType, xiiSharedPtr<const xiiNetworkSchema>& out_schema) const;
  [[nodiscard]] xiiNetworkDataError        GetSchema(xiiUInt64 uiTypeId, xiiSharedPtr<const xiiNetworkSchema>& out_schema) const;
  [[nodiscard]] xiiNetworkDataError        RegisterCodec(xiiSharedPtr<xiiNetworkCustomCodec> pCodec);
  [[nodiscard]] xiiNetworkDataError        RemoveCodec(xiiUInt64 uiCodecId);
  [[nodiscard]] xiiNetworkSchemaStatistics GetStatistics() const;

private:
  friend class xiiNetworkRegistryAccess;
  xiiNetworkTypeRegistryState* m_pState;
};
