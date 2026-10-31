/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/Containers/Set.h>
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
  xiiStringBuilder resolved;
  XII_SUCCEED_OR_RETURN(xiiProjectManifest::ResolveConfigPath(sPath, resolved).GetResult());
  xiiFileWriter file;
  if (file.Open(resolved).Failed())
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

xiiStatus xiiApplicationFileSystemConfig::Load(xiiStringView sPath)
{
  xiiStringBuilder resolved;
  XII_SUCCEED_OR_RETURN(xiiProjectManifest::ResolveConfigPath(sPath, resolved));
  xiiHybridArray<DataDirConfig, 4> directories;
  xiiFileReader                    file;
  if (file.Open(resolved).Failed())
  {
    if (xiiFileSystem::ExistsFile(resolved))
      return xiiStatus(xiiFmt("Cannot read data-directory configuration '{}'.", resolved));
    m_DataDirs.Clear(); // An absent file uses the caller's project / SDK mounts.
    return XII_SUCCESS;
  }
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    return xiiStatus(xiiFmt("Invalid data-directory configuration '{}'.", resolved));
  xiiSet<xiiString> roots;
  for (auto* dir = reader.GetRootElement()->GetFirstChild(); dir; dir = dir->GetSibling())
  {
    if (!dir->IsCustomType("DataDir"))
      continue;
    DataDirConfig     cfg;
    xiiSet<xiiString> names;
    for (auto* field = dir->GetFirstChild(); field; field = field->GetSibling())
    {
      const auto name = field->GetName();
      if (!field->HasName() || names.Contains(name))
        return xiiStatus("Data-directory fields must have unique names.");
      names.Insert(name);
      if (name == "Path" || name == "RootName")
      {
        if (field->GetPrimitivesType() != xiiOpenDdlPrimitiveType::String || field->GetNumPrimitives() != 1)
          return xiiStatus("Data-directory Path and RootName require one string.");
        (name == "Path" ? cfg.m_sDataDirSpecialPath : cfg.m_sRootName) = field->GetPrimitivesString()[0];
      }
      else if (name == "Writable")
      {
        if (field->GetPrimitivesType() != xiiOpenDdlPrimitiveType::Bool || field->GetNumPrimitives() != 1)
          return xiiStatus("Data-directory Writable requires one bool.");
        cfg.m_bWritable = field->GetPrimitivesBool()[0];
      }
    }
    if (cfg.m_sDataDirSpecialPath.IsEmpty())
      return xiiStatus("Data-directory Path cannot be empty.");
    xiiStringBuilder root(cfg.m_sRootName);
    root.ToLower();
    if (!root.IsEmpty() && roots.Contains(root))
      return xiiStatus(xiiFmt("Duplicate data-directory root '{}'.", root));
    roots.Insert(root);
    directories.PushBack(std::move(cfg));
  }
  m_DataDirs = std::move(directories);
  return XII_SUCCESS;
}

xiiStatus xiiApplicationFileSystemConfig::Apply()
{
  XII_LOG_BLOCK("xiiApplicationFileSystemConfig::Apply");

  // xiiStringBuilder s;

  // Make sure previous calls to Apply do not accumulate
  Clear();

  for (const auto& var : m_DataDirs)
  {
    // if (xiiFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Succeeded())
    {
      if (xiiFileSystem::AddDataDirectory(var.m_sDataDirSpecialPath, "AppFileSystemConfig", var.m_sRootName, (!var.m_sRootName.IsEmpty() && var.m_bWritable) ? xiiDataDirUsage::AllowWrites : xiiDataDirUsage::ReadOnly).Failed())
      {
        Clear();
        return xiiStatus(xiiFmt("Could not mount required data directory '{}' (root '{}').", var.m_sDataDirSpecialPath, var.m_sRootName));
      }
    }
  }
  return XII_SUCCESS;
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

  return res;
}



XII_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_FileSystemConfig);
