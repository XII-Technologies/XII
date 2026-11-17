/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Diagnostics/TelemetryProtocol.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>

namespace
{
  xiiNetworkAttribute* BoundedField(xiiUInt64 uiId, xiiUInt32 uiElements)
  {
    auto* pAttribute                = new xiiNetworkAttribute(uiId);
    pAttribute->m_uiMaximumElements = uiElements;
    return pAttribute;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetrySourceInfo, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetrySourceInfo>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_uiId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Category", m_sCategory)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Encoding", m_uiEncodingVersion)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("MinimumInterval", m_uiMinimumIntervalMilliseconds)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("Permission", m_uiPermission)->AddAttributes(new xiiNetworkAttribute(6)),
    XII_MEMBER_PROPERTY("Snapshot", m_bSnapshot)->AddAttributes(new xiiNetworkAttribute(7)),
    XII_MEMBER_PROPERTY("LatestOnly", m_bLatestOnly)->AddAttributes(new xiiNetworkAttribute(8)),
    XII_MEMBER_PROPERTY("CollectionWindow", m_uiCollectionWindowMilliseconds)->AddAttributes(new xiiNetworkAttribute(9)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000001ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetryHello, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetryHello>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Instance", m_Instance)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Application", m_sApplication)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Process", m_uiProcessId)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Permissions", m_uiPermissions)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_ARRAY_MEMBER_PROPERTY("Sources", m_Sources)->AddAttributes(BoundedField(5, 64)),
    XII_MEMBER_PROPERTY("CatalogRevision", m_uiCatalogRevision)->AddAttributes(new xiiNetworkAttribute(6)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000002ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetrySubscription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetrySubscription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Request", m_uiRequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Source", m_uiSource)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Interval", m_uiIntervalMilliseconds)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("Snapshot", m_bSnapshot)->AddAttributes(new xiiNetworkAttribute(6)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000003ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetryRecord, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetryRecord>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Fields", m_Fields)->AddAttributes(BoundedField(1, 256)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000004ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetrySample, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetrySample>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Source", m_uiSource)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Sequence", m_uiSequence)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Dropped", m_uiDropped)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("Kind", m_uiKind)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("Encoding", m_uiEncodingVersion)->AddAttributes(new xiiNetworkAttribute(6)),
    XII_MEMBER_PROPERTY("TimestampUs", m_fTimestampMicroseconds)->AddAttributes(new xiiNetworkAttribute(7)),
    XII_MEMBER_PROPERTY("Fields", m_Fields)->AddAttributes(BoundedField(8, 256)),
    XII_MEMBER_PROPERTY("Data", m_Data)->AddAttributes(BoundedField(9, 16 * 1024 * 1024)),
    XII_ARRAY_MEMBER_PROPERTY("Records", m_Records)->AddAttributes(BoundedField(10, 4096)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000005ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTelemetrySubscriptionResult, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTelemetrySubscriptionResult>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Request", m_uiRequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Source", m_uiSource)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Interval", m_uiIntervalMilliseconds)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("Error", m_uiError)->AddAttributes(new xiiNetworkAttribute(5)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494432000006ULL) } XII_END_ATTRIBUTES;
} XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(Foundation, Foundation_Diagnostics_Implementation_TelemetryProtocol);
