/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void xiiQtEditorApp::StoreEnginePluginModificationTimes()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    xiiPluginBundle& plugin = it.Value();

    for (const xiiString& rt : plugin.m_RuntimePlugins)
    {
      xiiStringBuilder sPath, sCopy;
      xiiPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      xiiFileStats stats;
      if (xiiOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, xiiTimestamp::CompareMode::Newer))
        {
          // store the maximum (latest) modification timestamp
          plugin.m_LastModificationTime = stats.m_LastModificationTime;
        }
      }
    }
  }
}

bool xiiQtEditorApp::CheckForEnginePluginModifications()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    xiiPluginBundle& plugin = it.Value();

    if (plugin.m_bMissing)
    {
      DetectAvailablePluginBundles(xiiOSFile::GetApplicationDirectory());

      xiiCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        xiiQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(xiiCppProject::GetPluginSourceDir(cppSettings));
      }

      break;
    }
  }

  for (auto it : m_PluginBundles.m_Plugins)
  {
    xiiPluginBundle& plugin = it.Value();

    if (!plugin.m_bSelected || !plugin.m_bLoadCopy)
      continue;

    for (const xiiString& rt : plugin.m_RuntimePlugins)
    {
      xiiStringBuilder sPath, sCopy;
      xiiPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      xiiFileStats stats;
      if (xiiOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, xiiTimestamp::CompareMode::Newer))
        {
          return true;
        }
      }
    }
  }

  return false;
}

void xiiQtEditorApp::RestartEngineProcessIfPluginsChanged(bool bForce)
{
  if (!xiiToolsProject::IsProjectOpen())
    return;

  if (!bForce)
  {
    if (m_LastPluginModificationCheck + xiiTime::MakeFromSeconds(2) > xiiTime::Now())
      return;
  }

  m_LastPluginModificationCheck = xiiTime::Now();

  for (auto pMan : xiiDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->xiiDocumentManager::GetAllOpenDocuments())
    {
      if (!pDoc->CanEngineProcessBeRestarted())
      {
        // not allowed to restart at the moment
        return;
      }
    }
  }

  if (!CheckForEnginePluginModifications())
    return;

  xiiLog::Info("Engine plugins have changed, restarting engine process.");

  StoreEnginePluginModificationTimes();
  xiiEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));
  xiiEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}
