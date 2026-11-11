/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/Configuration/GameApplicationSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameApplicationSettings, 1, xiiRTTIDefaultAllocator<xiiGameApplicationSettings>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Platform),
    new xiiCategoryAttribute("Presentation"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VSync", m_bVSync)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("FrameLimit", m_uiFrameLimit)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiClampValueAttribute(0, 1000)),
    XII_MEMBER_PROPERTY("BackgroundFrameLimit", m_uiBackgroundFrameLimit)->AddAttributes(new xiiDefaultValueAttribute(30), new xiiClampValueAttribute(1, 1000)),
    XII_MEMBER_PROPERTY("ShowFrameStats", m_bShowFrameStats)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("MasterVolume", m_fMasterVolume)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("MuteInBackground", m_bMuteInBackground)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiGameApplicationSettings::Load(xiiStringView sPath, bool bLegacyVSync)
{
  if (!xiiFileSystem::ExistsFile(sPath))
  {
    m_bVSync = bLegacyVSync;
    return XII_SUCCESS;
  }
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sPath));
  return ReadFromDDL(file);
}

xiiResult xiiGameApplicationSettings::Save(xiiStringView sPath) const
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

XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_GameApplicationSettings);
