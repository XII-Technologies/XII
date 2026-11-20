/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <MCPPlugin/Implementation/RuntimeDiagnostics.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>
#include <MCPPlugin/Runtime.h>

namespace
{
  xiiMCPRuntime* s_pRuntime = nullptr;

  void UpdateRuntime(const xiiGameApplicationExecutionEvent& event)
  {
    if (event.m_Type == xiiGameApplicationExecutionEvent::Type::AfterWorldUpdates && s_pRuntime != nullptr)
    {
      s_pRuntime->Update();
    }
  }

  void StartRuntime()
  {
    auto*       pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance();
    const auto* pCommandLine = xiiCommandLineUtils::GetGlobalInstance();
    const auto  sListen      = pCommandLine->GetStringOption("-MCPListen");
    if (pApplication == nullptr || pCommandLine->GetBoolOption("-MCPDisable"))
    {
      return;
    }

    xiiMCPRuntimeConfiguration configuration;
    configuration.m_sApplication   = pApplication->GetApplicationName();
    configuration.m_sLocalEndpoint = pCommandLine->GetStringOption("-MCPLocal");
    configuration.m_bNative        = pCommandLine->GetBoolOption("-MCP", sListen.IsEmpty());
    configuration.m_bAdvertise     = !pCommandLine->GetBoolOption("-MCPNoDiscovery");
    if (pCommandLine->GetBoolOption("-MCPAllowEdit"))
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
    }
    if (pCommandLine->GetBoolOption("-MCPAllowCapture"))
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Capture);
    }
    if (pCommandLine->GetBoolOption("-MCPAllowExecute"))
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
    }
    if (!sListen.IsEmpty() && xiiNetworkEndpoint::Parse(sListen, configuration.m_Http.m_Endpoint).Failed())
    {
      xiiLog::Error("Invalid -MCPListen endpoint.");
      return;
    }
    configuration.m_Http.m_sBearerToken  = xiiEnvironmentVariableUtils::GetValueString("XII_MCP_TOKEN");
    configuration.m_Http.m_uiPermissions = configuration.m_uiPermissions;

    xiiUniquePtr<xiiMCPRuntime> pRuntime = XII_DEFAULT_NEW(xiiMCPRuntime);
    const auto                  status   = pRuntime->Start(configuration);
    if (status.Failed())
    {
      xiiLog::Error("Could not start MCP: {}", status.GetMessageString());
      return;
    }

    s_pRuntime = pRuntime.Release();
    pApplication->m_ExecutionEvents.AddEventHandler(&UpdateRuntime);
  }

  void StopRuntime()
  {
    if (s_pRuntime != nullptr)
    {
      if (auto* pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance())
      {
        pApplication->m_ExecutionEvents.RemoveEventHandler(&UpdateRuntime);
      }
      XII_DEFAULT_DELETE(s_pRuntime);
    }
  }
} // namespace

xiiMCPRuntime::xiiMCPRuntime() :
  m_Protocol(m_Registry),
  m_Http(m_Protocol),
  m_pDiagnostics(XII_DEFAULT_NEW(xiiMCPRuntimeDiagnostics, m_Registry))
{
  xiiMCPRuntimeProviders::RegisterApplication(m_Registry);
  xiiMCPRuntimeProviders::RegisterWorld(m_Registry);
  xiiMCPRuntimeProviders::RegisterResources(m_Registry);
  xiiMCPRuntimeProviders::RegisterReflection(m_Registry);
}

xiiMCPRuntime::~xiiMCPRuntime()
{
  Stop();
}

xiiStatus xiiMCPRuntime::Start(const xiiMCPRuntimeConfiguration& configuration)
{
  if (configuration.m_bNative)
  {
    const auto status = m_Bridge.StartServer(m_Registry, configuration.m_sApplication, configuration.m_sLocalEndpoint, configuration.m_uiPermissions, configuration.m_bAdvertise);
    if (status.Failed())
    {
      return status;
    }
  }

  if (configuration.m_Http.m_Endpoint.GetAddress().IsValid())
  {
    const auto status = m_Http.Start(configuration.m_Http);
    if (status.Failed())
    {
      m_Bridge.Stop();
      return status;
    }
  }

  return XII_SUCCESS;
}

void xiiMCPRuntime::Update()
{
  m_pDiagnostics->Update();
  xiiMCPRuntimeProviders::UpdatePhysics(m_Registry, m_bPhysicsRegistered);
  m_Bridge.Update();
  m_Http.Update();
}

void xiiMCPRuntime::Stop()
{
  m_Http.Stop();
  m_Bridge.Stop();
}

XII_BEGIN_SUBSYSTEM_DECLARATION(MCPPlugin, MCPRuntime)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    StartRuntime();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    StopRuntime();
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_STATICLINK_FILE(MCPPlugin, MCPPlugin_Implementation_Runtime);
