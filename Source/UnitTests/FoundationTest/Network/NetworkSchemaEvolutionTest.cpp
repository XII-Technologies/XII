/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Serialization/NetworkSerializer.h>

namespace
{
  struct EvolutionValue
  {
    xiiInt32 m_iRequired = 0;
    xiiInt32 m_iOptional = 0;
    double   m_fChanged  = 0;
  };

  xiiNetworkDataError EvolutionSchema(xiiNetworkTypeRegistry& registry, xiiUInt32 uiVersion, bool bOptional, bool bChanged,
                                      xiiStringView sFieldName, EvolutionValue& value, xiiArrayPtr<const xiiUInt8> input, xiiDynamicArray<xiiUInt8>& out_bytes)
  {
    registry.Invalidate();
    using OwnType = EvolutionValue;
    xiiHybridArray<const xiiAbstractProperty*, 2> properties;
    if (bOptional)
    {
      properties.PushBack(XII_MEMBER_PROPERTY("Optional", m_iOptional)->AddAttributes(new xiiNetworkAttribute(2, true)));
    }
    properties.PushBack(bChanged ? XII_MEMBER_PROPERTY("Changed", m_fChanged)->AddAttributes(new xiiNetworkAttribute(1)) :
                                   XII_MEMBER_PROPERTY(sFieldName, m_iRequired)->AddAttributes(new xiiNetworkAttribute(1)));
    xiiRTTIDefaultAllocator<EvolutionValue> allocator;
    const xiiPropertyAttribute*             attributes[] = {new xiiNetworkIdentityAttribute(0x4D3456455253494FULL)};
    xiiNetworkDataError                     error;
    {
      xiiRTTI dynamic(uiVersion == 1 ? "NetworkEvolutionOld" : "NetworkEvolutionNew", nullptr, sizeof(EvolutionValue), uiVersion,
                      xiiVariantType::Invalid, xiiTypeFlags::Class, &allocator, properties, {}, xiiMakeArrayPtr(attributes), {}, {}, nullptr);
      error = registry.Refresh();
      xiiNetworkSerializationContext context;
      if (error.Succeeded() && !input.IsEmpty())
      {
        error = xiiNetworkSerializer::Deserialize(registry, &dynamic, &value, input, context);
      }
      if (error.Succeeded())
      {
        xiiArrayPtr<const xiiUInt8> bytes;
        error = xiiNetworkSerializer::Serialize(registry, &dynamic, &value, context, bytes);
        if (error.Succeeded())
        {
          out_bytes = bytes;
        }
      }
      registry.Invalidate();
    }
    registry.Refresh().IgnoreResult();
    return error;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(NetworkReflection, TaggedSchemaEvolution)
{
  xiiNetworkTypeRegistry registry;
  EvolutionValue         old;
  old.m_iRequired = 42;
  xiiDynamicArray<xiiUInt8> oldBytes, newBytes, roundTrip;
  XII_TEST_BOOL(EvolutionSchema(registry, 1, false, false, "Required", old, {}, oldBytes).Succeeded());
  EvolutionValue newer;
  newer.m_iOptional = 17;
  XII_TEST_BOOL(EvolutionSchema(registry, 2, true, false, "Renamed", newer, oldBytes, newBytes).Succeeded());
  XII_TEST_INT(newer.m_iRequired, 42);
  XII_TEST_INT(newer.m_iOptional, 17);
  EvolutionValue older;
  XII_TEST_BOOL(EvolutionSchema(registry, 1, false, false, "Required", older, newBytes, roundTrip).Succeeded());
  XII_TEST_INT(older.m_iRequired, 42);
  XII_TEST_BOOL(oldBytes == roundTrip);
  EvolutionValue changed;
  changed.m_iRequired = -1;
  changed.m_fChanged  = 0.5;
  XII_TEST_BOOL(EvolutionSchema(registry, 3, true, true, "Required", changed, oldBytes, roundTrip).m_Code == xiiNetworkDataErrorCode::InvalidFieldType);
  XII_TEST_FLOAT(changed.m_fChanged, 0.5, 0.00001);
  EvolutionValue renamed;
  XII_TEST_BOOL(EvolutionSchema(registry, 1, false, false, "Renamed", renamed, oldBytes, roundTrip).Succeeded());
  XII_TEST_BOOL(oldBytes == roundTrip);
}
