#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiApplicationPluginConfig, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiApplicationPluginConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Plugins", m_Plugins),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiApplicationPluginConfig_PluginConfig, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiApplicationPluginConfig_PluginConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RelativePath", m_sAppDirRelativePath),
    XII_MEMBER_PROPERTY("LoadCopy", m_bLoadCopy),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiApplicationPluginConfig::PluginConfig::operator<(const PluginConfig& rhs) const
{
  return m_sAppDirRelativePath < rhs.m_sAppDirRelativePath;
}

bool xiiApplicationPluginConfig::AddPlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (xiiUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      return false;
    }
  }

  m_Plugins.PushBack(cfg);
  return true;
}

bool xiiApplicationPluginConfig::RemovePlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (xiiUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      m_Plugins.RemoveAtAndSwap(i);
      return true;
    }
  }

  return false;
}

xiiApplicationPluginConfig::xiiApplicationPluginConfig() = default;

xiiResult xiiApplicationPluginConfig::Save(xiiStringView sConfigPath) const
{
  m_Plugins.Sort();

  xiiDeferredFileWriter file;
  file.SetOutput(sConfigPath, true);

  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);

  for (xiiUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    writer.BeginObject("Plugin");

    xiiOpenDdlUtils::StoreString(writer, m_Plugins[i].m_sAppDirRelativePath, "Path");
    xiiOpenDdlUtils::StoreBool(writer, m_Plugins[i].m_bLoadCopy, "LoadCopy");

    writer.EndObject();
  }

  return file.Close();
}

void xiiApplicationPluginConfig::Load(xiiStringView sConfigPath)
{
  XII_LOG_BLOCK("xiiApplicationPluginConfig::Load()");

  m_Plugins.Clear();

  xiiFileReader file;
  if (file.Open(sConfigPath).Failed())
  {
    xiiLog::Warning("Could not open plugins config file '{0}'", sConfigPath);
    return;
  }

  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse plugins config file '{0}'", sConfigPath);
    return;
  }

  const xiiOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pPlugin = pTree->GetFirstChild(); pPlugin != nullptr; pPlugin = pPlugin->GetSibling())
  {
    if (!pPlugin->IsCustomType("Plugin"))
      continue;

    PluginConfig cfg;

    const xiiOpenDdlReaderElement* pPath = pPlugin->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Path");
    const xiiOpenDdlReaderElement* pCopy = pPlugin->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "LoadCopy");

    if (pPath)
    {
      cfg.m_sAppDirRelativePath = pPath->GetPrimitivesString()[0];
    }

    if (pCopy)
    {
      cfg.m_bLoadCopy = pCopy->GetPrimitivesBool()[0];
    }

    // this prevents duplicates
    AddPlugin(cfg);
  }
}

void xiiApplicationPluginConfig::Apply()
{
  XII_LOG_BLOCK("xiiApplicationPluginConfig::Apply");

  for (const auto& var : m_Plugins)
  {
    xiiBitflags<xiiPluginLoadFlags> flags;
    flags.AddOrRemove(xiiPluginLoadFlags::LoadCopy, var.m_bLoadCopy);
    flags.AddOrRemove(xiiPluginLoadFlags::CustomDependency, false);

    xiiPlugin::LoadPlugin(var.m_sAppDirRelativePath, flags).IgnoreResult();
  }
}



XII_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_PluginConfig);
