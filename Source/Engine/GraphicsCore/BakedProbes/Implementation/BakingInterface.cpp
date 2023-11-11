#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/BakedProbes/BakingInterface.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBakingSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiBakingSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProbeSpacing", m_vProbeSpacing)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(4)), new xiiClampValueAttribute(xiiVec3(0.1f), xiiVariant())),
    XII_MEMBER_PROPERTY("NumSamplesPerProbe", m_uiNumSamplesPerProbe)->AddAttributes(new xiiDefaultValueAttribute(128), new xiiClampValueAttribute(32, 1024)),
    XII_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new xiiDefaultValueAttribute(1000), new xiiClampValueAttribute(1, xiiVariant())),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

static xiiTypeVersion s_BakingSettingsVersion = 1;
xiiResult             xiiBakingSettings::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BakingSettingsVersion);

  inout_stream << m_vProbeSpacing;
  inout_stream << m_uiNumSamplesPerProbe;
  inout_stream << m_fMaxRayDistance;

  return XII_SUCCESS;
}

xiiResult xiiBakingSettings::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiTypeVersion version = inout_stream.ReadVersion(s_BakingSettingsVersion);
  XII_IGNORE_UNUSED(version);

  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_uiNumSamplesPerProbe;
  inout_stream >> m_fMaxRayDistance;

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_BakedProbes_Implementation_BakingInterface);
