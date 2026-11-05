/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/Configuration/PlayerConfig.h>
#include <GameEngine/GameEnginePCH.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPlayerLaunchConfig, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPlayerLaunchConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Project", m_sProject),
    XII_MEMBER_PROPERTY("Application", m_sApplication),
    XII_MEMBER_PROPERTY("Scene", m_sScene),
    XII_MEMBER_PROPERTY("Role", m_sRole),
    XII_MEMBER_PROPERTY("Connect", m_sConnect),
    XII_MEMBER_PROPERTY("Instances", m_uiInstances),
    XII_MEMBER_PROPERTY("BasePort", m_uiBasePort),
    XII_MEMBER_PROPERTY("ArrangeWindows", m_bArrangeWindows),
    XII_ARRAY_MEMBER_PROPERTY("Arguments", m_Arguments),
    XII_ARRAY_MEMBER_PROPERTY("InstanceArguments", m_InstanceArguments),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayerConfig, 1, xiiRTTIDefaultAllocator<xiiPlayerConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MainScene", m_sMainScene),
    XII_MEMBER_PROPERTY("PreloadCollection", m_sPreloadCollection),
    XII_ARRAY_MEMBER_PROPERTY("LaunchConfigurations", m_LaunchConfigurations),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiPlayerLaunchConfig::Validate() const
{
  if (m_sName.IsEmpty() || m_uiInstances < 1 || m_uiInstances > 16 ||
      m_uiBasePort > 65535 - 2 * m_uiInstances || m_InstanceArguments.GetCount() > m_uiInstances)
    return XII_FAILURE;
  if (m_sRole != "standalone" && m_sRole != "server" && m_sRole != "client" && m_sRole != "server-clients")
    return XII_FAILURE;
  return XII_SUCCESS;
}

xiiResult xiiPlayerConfig::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  for (xiiUInt32 i = 0; i < m_LaunchConfigurations.GetCount(); ++i)
  {
    XII_SUCCEED_OR_RETURN(m_LaunchConfigurations[i].Validate());
    for (xiiUInt32 j = 0; j < i; ++j)
      if (m_LaunchConfigurations[i].m_sName == m_LaunchConfigurations[j].m_sName)
        return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiPlayerConfig::Load(xiiStringView sPath)
{
  if (!xiiFileSystem::ExistsFile(sPath))
    return XII_SUCCESS;
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sPath));
  return ReadFromDDL(file);
}

xiiResult xiiPlayerConfig::Save(xiiStringView sPath) const
{
  XII_SUCCEED_OR_RETURN(Validate());
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiReflectionSerializer::WriteObjectToDDL(writer, GetDynamicRTTI(), this);
  xiiDeferredFileWriter file;
  file.SetOutput(sPath, true);
  XII_SUCCEED_OR_RETURN(storage.CopyToStream(file));
  return file.Close();
}

const xiiPlayerLaunchConfig* xiiPlayerConfig::FindLaunch(xiiStringView sName) const
{
  for (const auto& launch : m_LaunchConfigurations)
    if (launch.m_sName == sName)
      return &launch;
  return nullptr;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_PlayerConfig);
