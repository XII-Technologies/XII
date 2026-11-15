/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Network/Reflection/NetworkTypeRegistry.h>

struct NetworkSchemaBase
{
  xiiInt32  m_iBase    = 1;
  xiiString m_sPrivate = "local";
};
struct NetworkSchemaChild : NetworkSchemaBase
{
  xiiUInt64                   m_uiSequence = 2;
  xiiHybridArray<float, 4>    m_Samples;
  xiiMap<xiiString, xiiInt32> m_Values;
  float                       GetValue() const { return m_fValue; }
  void                        SetValue(float fValue) { m_fValue = fValue; }
  float                       m_fValue = 3;
};
struct NetworkSchemaOrdinary
{
  xiiInt32 m_iValue = 0;
};
struct NetworkSchemaBad
{
  NetworkSchemaBase* m_pValue = nullptr;
};
struct NetworkSchemaDuplicate
{
  xiiInt32 m_iFirst  = 0;
  xiiInt32 m_iSecond = 0;
};
struct NetworkSchemaCollisionA
{
  xiiInt32 m_iValue = 0;
};
struct NetworkSchemaCollisionB
{
  xiiInt32 m_iValue = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaBase);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaChild);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaOrdinary);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaBad);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaDuplicate);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaCollisionA);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, NetworkSchemaCollisionB);

XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaBase, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaBase>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Base", m_iBase)->AddAttributes(new xiiNetworkAttribute(1)),
      XII_MEMBER_PROPERTY("Private", m_sPrivate),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaChild, NetworkSchemaBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaChild>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Sequence", m_uiSequence)->AddAttributes(new xiiNetworkAttribute(2)),
      XII_ARRAY_MEMBER_PROPERTY("Samples", m_Samples)->AddAttributes(new xiiNetworkAttribute(3)),
      XII_MAP_MEMBER_PROPERTY("Values", m_Values)->AddAttributes(new xiiNetworkAttribute(4)),
      XII_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new xiiNetworkAttribute(5)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaOrdinary, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaOrdinary>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Value", m_iValue)} XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaBad, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaBad>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Value", m_pValue)->AddAttributes(new xiiNetworkAttribute())} XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaDuplicate, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaDuplicate>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("First", m_iFirst)->AddAttributes(new xiiNetworkAttribute(7)),
      XII_MEMBER_PROPERTY("Second", m_iSecond)->AddAttributes(new xiiNetworkAttribute(7)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaCollisionA, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaCollisionA>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Value", m_iValue)->AddAttributes(new xiiNetworkAttribute())} XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiNetworkIdentityAttribute(0x987654321ULL)} XII_END_ATTRIBUTES;
  }
XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_STATIC_REFLECTED_TYPE(NetworkSchemaCollisionB, xiiNoBase, 1, xiiRTTIDefaultAllocator<NetworkSchemaCollisionB>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Value", m_iValue)->AddAttributes(new xiiNetworkAttribute())} XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiNetworkIdentityAttribute(0x987654321ULL)} XII_END_ATTRIBUTES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(NetworkReflection, AutomaticSchemaDiscovery)
{
  xiiNetworkTypeRegistry               registry;
  xiiSharedPtr<const xiiNetworkSchema> schema;
  if (!XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkSchemaChild>(), schema).Succeeded()))
  {
    return;
  }
  XII_TEST_INT(schema->m_Fields.GetCount(), 5);
  XII_TEST_BOOL(schema->m_uiTypeId == xiiNetworkStableId("NetworkSchemaChild"));
  for (xiiUInt32 uiIndex = 0; uiIndex < schema->m_Fields.GetCount(); ++uiIndex)
  {
    XII_TEST_INT(schema->m_Fields[uiIndex].m_uiId, uiIndex + 1);
    XII_TEST_BOOL(schema->m_Fields[uiIndex].m_sName != "Private");
  }
  XII_TEST_BOOL(schema->m_Fields[2].m_Category == xiiNetworkFieldCategory::Array);
  XII_TEST_BOOL(schema->m_Fields[3].m_Category == xiiNetworkFieldCategory::Map);
  const auto                           uiFingerprint = schema->m_uiFingerprint;
  const auto                           uiGeneration  = registry.GetStatistics().m_uiGeneration;
  xiiSharedPtr<const xiiNetworkSchema> second;
  XII_TEST_BOOL(registry.GetSchema(schema->m_uiTypeId, second).Succeeded());
  XII_TEST_BOOL(schema.Borrow() == second.Borrow());
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkSchemaOrdinary>(), second).m_Code == xiiNetworkDataErrorCode::UnknownType);
  registry.Invalidate();
  XII_TEST_BOOL(registry.GetSchema(schema->m_uiTypeId, second).m_Code == xiiNetworkDataErrorCode::RegistryUnavailable);
  XII_TEST_INT(schema->m_Fields.GetCount(), 5); // Portable snapshots survive invalidation.
  XII_TEST_BOOL(registry.Refresh().Succeeded());
  XII_TEST_BOOL(registry.GetSchema(schema->m_uiTypeId, second).Succeeded());
  XII_TEST_BOOL(second->m_uiFingerprint == uiFingerprint);
  XII_TEST_BOOL(registry.GetStatistics().m_uiGeneration > uiGeneration);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, SchemaRejectsInvalidMetadata)
{
  xiiNetworkTypeRegistry               registry;
  xiiSharedPtr<const xiiNetworkSchema> schema;
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkSchemaBad>(), schema).m_Code == xiiNetworkDataErrorCode::UnsupportedProperty);
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkSchemaDuplicate>(), schema).m_Code == xiiNetworkDataErrorCode::IdentifierCollision);
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkSchemaCollisionA>(), schema).m_Code == xiiNetworkDataErrorCode::IdentifierCollision);
  XII_TEST_BOOL(registry.GetSchema(xiiGetStaticRTTI<NetworkSchemaCollisionB>(), schema).m_Code == xiiNetworkDataErrorCode::IdentifierCollision);
}

XII_CREATE_SIMPLE_TEST(NetworkReflection, DynamicSchemaLifetime)
{
  xiiNetworkTypeRegistry               registry;
  xiiSharedPtr<const xiiNetworkSchema> snapshot;
  registry.Invalidate();
  {
    // A local dynamic RTTI borrows existing property metadata; no manual network registration occurs.
    xiiDynamicArray<const xiiAbstractProperty*> properties;
    for (const auto* pProperty : xiiGetStaticRTTI<NetworkSchemaBase>()->GetProperties())
    {
      properties.PushBack(pProperty);
    }
    xiiRTTIDefaultAllocator<NetworkSchemaBase> allocator;
    xiiRTTI                                    dynamic("NetworkSchemaDynamic", nullptr, sizeof(NetworkSchemaBase), 1, xiiVariantType::Invalid,
                                                       xiiTypeFlags::Class, &allocator, properties, {}, {}, {}, {}, nullptr);
    XII_TEST_BOOL(registry.Refresh().Succeeded());
    XII_TEST_BOOL(registry.GetSchema(&dynamic, snapshot).Succeeded());
    registry.Invalidate();
  }
  XII_TEST_BOOL(registry.Refresh().Succeeded());
  xiiSharedPtr<const xiiNetworkSchema> current;
  XII_TEST_BOOL(registry.GetSchema(snapshot->m_uiTypeId, current).m_Code == xiiNetworkDataErrorCode::UnknownType);
  XII_TEST_STRING(snapshot->m_sTypeName, "NetworkSchemaDynamic");
  XII_TEST_INT(snapshot->m_Fields.GetCount(), 1);
}
