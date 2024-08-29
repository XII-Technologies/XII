#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiApplicationFileSystemConfig, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiApplicationFileSystemConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("DataDirs", m_DataDirs),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiApplicationFileSystemConfig_DataDirConfig, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiApplicationFileSystemConfig_DataDirConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RelativePath", m_sDataDirSpecialPath),
    XII_MEMBER_PROPERTY("Writable", m_bWritable),
    XII_MEMBER_PROPERTY("RootName", m_sRootName),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiApplicationFileSystemConfig::Save(xiiStringView sPath)
{
  xiiFileWriter file;
  if (file.Open(sPath).Failed())
    return XII_FAILURE;

  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);

  for (xiiUInt32 i = 0; i < m_DataDirs.GetCount(); ++i)
  {
    writer.BeginObject("DataDir");

    xiiOpenDdlUtils::StoreString(writer, m_DataDirs[i].m_sDataDirSpecialPath, "Path");
    xiiOpenDdlUtils::StoreString(writer, m_DataDirs[i].m_sRootName, "RootName");
    xiiOpenDdlUtils::StoreBool(writer, m_DataDirs[i].m_bWritable, "Writable");

    writer.EndObject();
  }

  return XII_SUCCESS;
}

void xiiApplicationFileSystemConfig::Load(xiiStringView sPath)
{
  XII_LOG_BLOCK("xiiApplicationFileSystemConfig::Load()");

  m_DataDirs.Clear();

  xiiFileReader file;
  if (file.Open(sPath).Failed())
  {
    xiiLog::Dev("File-system config file '{0}' does not exist.", sPath);
    return;
  }

  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse file-system config file '{0}'", sPath);
    return;
  }

  const xiiOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pDirs = pTree->GetFirstChild(); pDirs != nullptr; pDirs = pDirs->GetSibling())
  {
    if (!pDirs->IsCustomType("DataDir"))
      continue;

    DataDirConfig cfg;
    cfg.m_bWritable = false;

    const xiiOpenDdlReaderElement* pPath  = pDirs->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Path");
    const xiiOpenDdlReaderElement* pRoot  = pDirs->FindChildOfType(xiiOpenDdlPrimitiveType::String, "RootName");
    const xiiOpenDdlReaderElement* pWrite = pDirs->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "Writable");

    if (pPath)
      cfg.m_sDataDirSpecialPath = pPath->GetPrimitivesString()[0];
    if (pRoot)
      cfg.m_sRootName = pRoot->GetPrimitivesString()[0];
    if (pWrite)
      cfg.m_bWritable = pWrite->GetPrimitivesBool()[0];

    /// \todo Temp fix for backwards compatibility
    {
      if (cfg.m_sRootName == "project")
      {
        cfg.m_sDataDirSpecialPath = ">project/";
      }
      else if (cfg.m_sDataDirSpecialPath.StartsWith_NoCase(":project/"))
      {
        xiiStringBuilder temp(">project/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath.GetData() + 9);
        cfg.m_sDataDirSpecialPath = temp;
      }
      else if (cfg.m_sDataDirSpecialPath.StartsWith_NoCase(":sdk/"))
      {
        xiiStringBuilder temp(">sdk/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath.GetData() + 5);
        cfg.m_sDataDirSpecialPath = temp;
      }
      else if (!cfg.m_sDataDirSpecialPath.StartsWith_NoCase(">sdk/"))
      {
        xiiStringBuilder temp(">sdk/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath);
        cfg.m_sDataDirSpecialPath = temp;
      }
    }

    m_DataDirs.PushBack(cfg);
  }
}

void xiiApplicationFileSystemConfig::Apply()
{
  XII_LOG_BLOCK("xiiApplicationFileSystemConfig::Apply");

  // xiiStringBuilder s;

  // Make sure previous calls to Apply do not accumulate
  Clear();

  for (const auto& var : m_DataDirs)
  {
    // if (xiiFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Succeeded())
    {
      xiiFileSystem::AddDataDirectory(var.m_sDataDirSpecialPath, "AppFileSystemConfig", var.m_sRootName, (!var.m_sRootName.IsEmpty() && var.m_bWritable) ? xiiDataDirUsage::AllowWrites : xiiDataDirUsage::ReadOnly).IgnoreResult();
    }
  }
}


void xiiApplicationFileSystemConfig::Clear()
{
  xiiFileSystem::RemoveDataDirectoryGroup("AppFileSystemConfig");
}

xiiResult xiiApplicationFileSystemConfig::CreateDataDirStubFiles()
{
  XII_LOG_BLOCK("xiiApplicationFileSystemConfig::CreateDataDirStubFiles");

  xiiStringBuilder s;
  xiiResult        res = XII_SUCCESS;

  for (const auto& var : m_DataDirs)
  {
    if (xiiFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Failed())
    {
      xiiLog::Error("Failed to get special directory '{0}'", var.m_sDataDirSpecialPath);
      res = XII_FAILURE;
      continue;
    }

    s.AppendPath("DataDir.xiiManifest");

    xiiOSFile file;
    if (file.Open(s, xiiFileOpenMode::Write).Failed())
    {
      xiiLog::Error("Failed to create stub file '{0}'", s);
      res = XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_FileSystemConfig);
