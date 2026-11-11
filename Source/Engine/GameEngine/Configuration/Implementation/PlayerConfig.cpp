/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GameEngine/Configuration/PlayerConfig.h>

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

xiiStatus xiiPlayerLaunchConfig::ValidateRuntimeArguments(xiiArrayPtr<const xiiString> arguments)
{
  for (xiiUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    const auto& arg     = arguments[i];
    bool        integer = true;
    double      minimum = 0.0;
    double      maximum = 0.0;
    if (arg.IsEqual_NoCase("-width"))
    {
      minimum = 320;
      maximum = 16384;
    }
    else if (arg.IsEqual_NoCase("-height"))
    {
      minimum = 200;
      maximum = 16384;
    }
    else if (arg.IsEqual_NoCase("-display"))
    {
      minimum = -1;
      maximum = 127;
    }
    else if (arg.IsEqual_NoCase("-App.FrameLimit"))
    {
      minimum = 0;
      maximum = 1000;
    }
    else if (arg.IsEqual_NoCase("-background-fps"))
    {
      minimum = 1;
      maximum = 1000;
    }
    else if (arg.IsEqual_NoCase("-volume"))
    {
      minimum = 0;
      maximum = 1.0;
      integer = false;
    }
    else if (arg.IsEqual_NoCase("-port") || arg.IsEqual_NoCase("-TelemetryPort"))
    {
      minimum = 0;
      maximum = 65535;
    }
    else if (arg.IsEqual_NoCase("-window-x") || arg.IsEqual_NoCase("-window-y"))
    {
      minimum = -32768;
      maximum = 32767;
    }
    else if (arg.IsEqual_NoCase("-window-mode"))
    {
      if (i + 1 >= arguments.GetCount() || (arguments[i + 1] != "window" && arguments[i + 1] != "borderless" && arguments[i + 1] != "fullscreen"))
        return xiiStatus("Window mode must be window, borderless or fullscreen.");
      continue;
    }
    else
      continue;
    if (i + 1 >= arguments.GetCount())
      return xiiStatus(xiiFmt("Missing value for '{}'.", arg));
    double      number = 0;
    const char* end    = nullptr;
    xiiResult   parsed = XII_FAILURE;
    if (integer)
    {
      xiiInt32 value = 0;
      parsed         = xiiConversionUtils::StringToInt(arguments[i + 1], value, &end);
      number         = value;
    }
    else
      parsed = xiiConversionUtils::StringToFloat(arguments[i + 1], number, &end);
    if (parsed.Failed() || end != arguments[i + 1].GetData() + arguments[i + 1].GetElementCount() || !xiiMath::IsFinite(number) || number < minimum || number > maximum)
      return xiiStatus(xiiFmt("Invalid value '{}' for '{}'; expected {} to {}.", arguments[i + 1], arg, minimum, maximum));
  }
  return xiiStatus(XII_SUCCESS);
}
