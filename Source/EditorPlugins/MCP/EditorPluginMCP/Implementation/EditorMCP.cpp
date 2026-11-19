/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorPluginMCP/EditorMCP.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

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
    m_AllowedProject = xiiConversionUtils::ConvertStringToUuid(sProject);
  }
  xiiUInt8 uiPermissions = static_cast<xiiUInt8>(xiiMCPPermission::Read);
  if (m_AllowedProject.IsValid() && pOptions->GetBoolOption("-MCPAllowEdit"))
  {
    uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
  }
  if (m_AllowedProject.IsValid() && pOptions->GetBoolOption("-MCPAllowExecute"))
  {
    uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
  }

  if (RegisterProject().Failed() || RegisterDocuments().Failed() || RegisterScenes().Failed() || RegisterAssets().Failed())
  {
    return xiiStatus("Could not register Editor MCP capabilities.");
  }

  if (pOptions->GetBoolOption("-MCP"))
  {
    const auto status = m_Bridge.StartServer(m_Registry, "XII Editor", pOptions->GetStringOption("-MCPLocal"), uiPermissions, !pOptions->GetBoolOption("-MCPNoDiscovery"));
    if (status.Failed())
    {
      return status;
    }
  }

  const auto sListen = pOptions->GetStringOption("-MCPListen");
  if (!sListen.IsEmpty())
  {
    xiiMCPHttpConfiguration http;
    http.m_sBearerToken  = xiiEnvironmentVariableUtils::GetValueString("XII_MCP_TOKEN");
    http.m_uiPermissions = uiPermissions;
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
    m_Bridge.Update();
    m_Http.Update();
  });
  m_Timer.start(10);
  return XII_SUCCESS;
}

XII_PLUGIN_DEPENDENCY(xiiMCPPlugin);

XII_PLUGIN_ON_LOADED()
{
  const auto* pOptions = xiiCommandLineUtils::GetGlobalInstance();
  if (QCoreApplication::instance() == nullptr || (!pOptions->GetBoolOption("-MCP") && pOptions->GetStringOption("-MCPListen").IsEmpty()))
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
