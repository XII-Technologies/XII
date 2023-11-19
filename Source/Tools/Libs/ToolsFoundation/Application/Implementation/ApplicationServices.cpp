#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_IMPLEMENT_SINGLETON(xiiApplicationServices);

static xiiApplicationServices g_instance;

xiiApplicationServices::xiiApplicationServices() :
  m_SingletonRegistrar(this)
{
}

xiiString xiiApplicationServices::GetApplicationUserDataFolder() const
{
  xiiStringBuilder path = xiiOSFile::GetUserDataFolder();
  path.AppendPath("XII", xiiApplication::GetApplicationInstance()->GetApplicationName());
  path.MakeCleanPath();

  return path;
}

xiiString xiiApplicationServices::GetApplicationDataFolder() const
{
  xiiStringBuilder sAppDir(">sdk/Data/Tools/", xiiApplication::GetApplicationInstance()->GetApplicationName());

  xiiStringBuilder result;
  xiiFileSystem::ResolveSpecialDirectory(sAppDir, result).IgnoreResult();
  result.MakeCleanPath();

  return result;
}

xiiString xiiApplicationServices::GetApplicationPreferencesFolder() const
{
  return GetApplicationUserDataFolder();
}

xiiString xiiApplicationServices::GetProjectPreferencesFolder() const
{
  return GetProjectPreferencesFolder(xiiToolsProject::GetSingleton()->GetProjectDirectory());
}

xiiString xiiApplicationServices::GetProjectPreferencesFolder(xiiStringView sProjectFilePath) const
{
  xiiStringBuilder path = GetApplicationUserDataFolder();

  sProjectFilePath.TrimWordEnd("xiiProject");
  sProjectFilePath.TrimWordEnd("xiiRemoteProject");
  sProjectFilePath.Trim("/\\");

  xiiStringBuilder ProjectName = sProjectFilePath;

  xiiStringBuilder ProjectPath = ProjectName;
  ProjectPath.PathParentDirectory();

  const xiiUInt64 uiPathHash = xiiHashingUtils::StringHash(ProjectPath.GetView());

  ProjectName = ProjectName.GetFileName();

  path.AppendFormat("/Projects/{}_{}", uiPathHash, ProjectName);

  path.MakeCleanPath();
  return path;
}

xiiString xiiApplicationServices::GetDocumentPreferencesFolder(const xiiDocument* pDocument) const
{
  xiiStringBuilder path = GetProjectPreferencesFolder();

  xiiStringBuilder sGuid;
  xiiConversionUtils::ToString(pDocument->GetGuid(), sGuid);

  path.AppendPath(sGuid);

  path.MakeCleanPath();
  return path;
}

xiiString xiiApplicationServices::GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const
{
  xiiStringBuilder sPath = xiiOSFile::GetApplicationDirectory();

  if (bUsePrecompiledTools)
  {
    sPath.AppendPath("../../../Data/Tools/Precompiled");
  }

  sPath.MakeCleanPath();

  return sPath;
}

xiiString xiiApplicationServices::GetSampleProjectsFolder() const
{
  xiiStringBuilder sPath = xiiOSFile::GetApplicationDirectory();

  sPath.AppendPath("../../../Data/Samples");

  sPath.MakeCleanPath();

  return sPath;
}
