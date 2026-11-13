/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <JoltPlugin/System/JoltPhysicsProfileConfig.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltPhysicsProfileConfig, 1, xiiRTTIDefaultAllocator<xiiJoltPhysicsProfileConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Gravity", m_vGravity)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.0f, 0.0f, -9.81f))),
    XII_MEMBER_PROPERTY("FixedStepSeconds", m_fFixedStep)->AddAttributes(new xiiDefaultValueAttribute(xiiJoltPhysicsProfileConfig::s_fDefaultFixedStep), new xiiClampValueAttribute(0.000001, 1.0)),
    XII_MEMBER_PROPERTY("MaxCatchUpSteps", m_uiMaxCatchUpSteps)->AddAttributes(new xiiDefaultValueAttribute(xiiJoltPhysicsProfileConfig::s_uiDefaultMaxCatchUpSteps), new xiiClampValueAttribute(1u, 256u)),
    XII_MEMBER_PROPERTY("MaxBodies", m_uiMaxBodies)->AddAttributes(new xiiDefaultValueAttribute(xiiJoltPhysicsProfileConfig::s_uiDefaultMaxBodies), new xiiClampValueAttribute(1u, 1048576u)),
    XII_MEMBER_PROPERTY("MaxBodyPairs", m_uiMaxBodyPairs)->AddAttributes(new xiiDefaultValueAttribute(xiiJoltPhysicsProfileConfig::s_uiDefaultMaxBodyPairs), new xiiClampValueAttribute(1u, 1048576u)),
    XII_MEMBER_PROPERTY("MaxContactConstraints", m_uiMaxContactConstraints)->AddAttributes(new xiiDefaultValueAttribute(xiiJoltPhysicsProfileConfig::s_uiDefaultMaxContactConstraints), new xiiClampValueAttribute(1u, 1048576u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiJoltPhysicsProfileConfig::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  return m_vGravity.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

void xiiJoltPhysicsProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& ref_stream) const
{
  if (Validate().Failed())
  {
    xiiLog::Error("Cannot export invalid Jolt physics settings.");
    return;
  }
  ref_stream.BeginChunk("JoltPhysics", 1);
  xiiReflectionSerializer::WriteObjectToDDL(ref_stream, GetDynamicRTTI(), this);
  ref_stream.EndChunk();
}

void xiiJoltPhysicsProfileConfig::LoadRuntimeData(xiiChunkStreamReader& ref_stream)
{
  if (ref_stream.GetCurrentChunk().m_sChunkName == "JoltPhysics" &&
      (ref_stream.GetCurrentChunk().m_uiChunkVersion != 1 || ReadFromDDL(ref_stream).Failed()))
    xiiLog::Error("Invalid Jolt physics profile settings. Keeping the previous settings.");
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_Implementation_JoltPhysicsProfileConfig);
