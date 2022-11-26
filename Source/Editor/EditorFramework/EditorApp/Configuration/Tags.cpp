#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

xiiStatus xiiQtEditorApp::SaveTagRegistry()
{
  XII_LOG_BLOCK("xiiQtEditorApp::SaveTagRegistry()");

  xiiStringBuilder sPath;
  sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("Tags.ddl");

  xiiDeferredFileWriter file;
  file.SetOutput(sPath);

  xiiToolsTagRegistry::WriteToDDL(file);

  if (file.Close().Failed())
  {
    return xiiStatus(xiiFmt("Could not open tags config file '{0}' for writing", sPath));
  }
  return xiiStatus(XII_SUCCESS);
}

void xiiQtEditorApp::ReadTagRegistry()
{
  XII_LOG_BLOCK("xiiQtEditorApp::ReadTagRegistry");

  xiiToolsTagRegistry::Clear();

  xiiStringBuilder sPath;
  sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("Tags.ddl");

  xiiFileReader file;
  if (file.Open(sPath).Failed())
  {
    xiiLog::Warning("Could not open tags config file '{0}'", sPath);

    xiiStatus res = SaveTagRegistry();
    if (res.m_Result.Failed())
    {
      xiiLog::Error("{0}", res.m_sMessage);
    }
  }
  else
  {
    xiiStatus res = xiiToolsTagRegistry::ReadFromDDL(file);
    if (res.m_Result.Failed())
    {
      xiiLog::Error("{0}", res.m_sMessage);
    }
  }


  // TODO: Add default tags
  xiiToolsTag tag;
  tag.m_sName     = "EditorHidden";
  tag.m_sCategory = "Editor";
  xiiToolsTagRegistry::AddTag(tag);
}
