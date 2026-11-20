/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/Project/ProjectPreferences.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QCoreApplication>

namespace
{
  xiiEditorMCP* s_pEditorMCP = nullptr;
}

xiiEditorMCP::xiiEditorMCP() :
  m_Protocol(m_Registry),
  m_Http(m_Protocol)
{
}

xiiEditorMCP::~xiiEditorMCP()
{
  m_Timer.stop();
  DrainCppBuild();
  if (m_ProjectEvents != 0)
  {
    xiiToolsProject::s_Events.RemoveEventHandler(m_ProjectEvents);
  }
  if (m_ProjectRequests != 0)
  {
    xiiToolsProject::s_Requests.RemoveEventHandler(m_ProjectRequests);
  }
  if (m_DocumentEvents != 0)
  {
    xiiDocumentManager::s_Events.RemoveEventHandler(m_DocumentEvents);
  }
}

xiiStatus xiiEditorMCP::Start()
{
  const auto* pOptions = xiiCommandLineUtils::GetGlobalInstance();
  m_DocumentEvents     = xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiEditorMCP::DocumentEvent, this));

  const auto sProject = pOptions->GetStringOption("-MCPProject");
  if (!sProject.IsEmpty())
  {
    if (!xiiConversionUtils::IsStringUuid(sProject))
    {
      return xiiStatus("Invalid -MCPProject UUID.");
    }
    m_ArgumentProject = xiiConversionUtils::ConvertStringToUuid(sProject);
  }
  m_AllowedProject        = m_ArgumentProject;
  m_bAllowAssetProcessing = m_ArgumentProject.IsValid() && pOptions->GetBoolOption("-MCPAllowExecute");
  m_bAllowCppBuilds       = m_ArgumentProject.IsValid() && pOptions->GetBoolOption("-MCPAllowBuild");
  if (m_ArgumentProject.IsValid() && pOptions->GetBoolOption("-MCPAllowEdit"))
  {
    m_uiArgumentPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
  }
  if (m_bAllowAssetProcessing || m_bAllowCppBuilds)
  {
    m_uiArgumentPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
  }

  if (RegisterProject().Failed() || RegisterCppBuild().Failed() || RegisterDocuments().Failed() || RegisterScenes().Failed() || RegisterAssets().Failed() || RegisterGraphs().Failed() || RegisterViewport().Failed())
  {
    return xiiStatus("Could not register Editor MCP capabilities.");
  }

  ConnectCppBuildProcess();
  m_ProjectRequests = xiiToolsProject::s_Requests.AddEventHandler(xiiMakeDelegate(&xiiEditorMCP::OnProjectRequest, this));
  m_ProjectEvents   = xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiEditorMCP::OnProjectEvent, this));

  const auto sListen = pOptions->GetStringOption("-MCPListen");
  if (pOptions->GetBoolOption("-MCP", sListen.IsEmpty()))
  {
    const auto status = m_Bridge.StartServer(m_Registry, "XII Editor", pOptions->GetStringOption("-MCPLocal"), m_uiArgumentPermissions, !pOptions->GetBoolOption("-MCPNoDiscovery"));
    if (status.Failed())
    {
      return status;
    }
  }

  if (!sListen.IsEmpty())
  {
    xiiMCPHttpConfiguration http;
    http.m_sBearerToken  = xiiEnvironmentVariableUtils::GetValueString("XII_MCP_TOKEN");
    http.m_uiPermissions = m_uiArgumentPermissions;
    if (xiiNetworkEndpoint::Parse(sListen, http.m_Endpoint).Failed())
    {
      return xiiStatus("Invalid -MCPListen endpoint.");
    }
    const auto status = m_Http.Start(http);
    if (status.Failed())
    {
      return status;
    }
  }

  QObject::connect(&m_Timer, &QTimer::timeout, [this] {
    if (m_bUpdating)
    {
      return;
    }
    m_bUpdating = true;
    XII_SCOPE_EXIT(m_bUpdating = false);
    UpdateProjectPermissions();
    m_Bridge.Update();
    m_Http.Update();
  });
  m_Timer.start(10);
  return XII_SUCCESS;
}

void xiiEditorMCP::UpdateProjectPermissions()
{
  const auto now = xiiTime::Now();
  if (m_ArgumentProject.IsValid() || now < m_NextPermissionUpdate)
  {
    return;
  }
  m_NextPermissionUpdate  = now + xiiTime::MakeFromMilliseconds(100);
  auto uiPermissions      = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  m_AllowedProject        = {};
  m_bAllowAssetProcessing = false;
  m_bAllowCppBuilds       = false;
  if (xiiToolsProject::IsProjectOpen())
  {
    if (const auto* pPreferences = xiiEditorSettings::QuerySettings<xiiMCPProjectPreferences>())
    {
      if (pPreferences->m_bAllowEditorChanges)
      {
        uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
      }
      m_bAllowAssetProcessing = pPreferences->m_bAllowAssetProcessing;
      m_bAllowCppBuilds       = pPreferences->m_bAllowCppBuilds;
      if (m_bAllowAssetProcessing || m_bAllowCppBuilds)
      {
        uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
      }
      m_AllowedProject = xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID;
    }
  }
  m_Bridge.SetServerPermissions(uiPermissions);
}

XII_PLUGIN_DEPENDENCY(xiiMCPPlugin);

XII_PLUGIN_ON_LOADED()
{
  const auto* pOptions = xiiCommandLineUtils::GetGlobalInstance();
  const auto* pEditor  = xiiQtEditorApp::GetSingleton();
  if (QCoreApplication::instance() == nullptr || pEditor == nullptr || pEditor->IsInHeadlessMode() ||
      pEditor->IsBackgroundMode() || pEditor->IsInUnitTestMode() || pOptions->GetBoolOption("-MCPDisable"))
  {
    return;
  }
  s_pEditorMCP = XII_DEFAULT_NEW(xiiEditorMCP);
  if (s_pEditorMCP->Start().LogFailure())
  {
    XII_DEFAULT_DELETE(s_pEditorMCP);
  }
}

XII_PLUGIN_ON_UNLOADED()
{
  XII_DEFAULT_DELETE(s_pEditorMCP);
}

XII_STATICLINK_FILE(EditorPluginMCP, EditorPluginMCP_Implementation_EditorMCP);
