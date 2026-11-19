/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Logging/Log.h>
#include <MCPPlugin/Registry.h>

#ifdef XII_MCP_HAS_RENDERING
#  include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#  include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#endif

class xiiMCPRuntimeDiagnostics final
{
public:
  explicit xiiMCPRuntimeDiagnostics(xiiMCPRegistry& registry);
  ~xiiMCPRuntimeDiagnostics();
  void Update();

private:
  void      OnLog(const xiiLoggingEventData& log);
  xiiStatus GetLogs(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result);

  xiiMCPRegistry&        m_Registry;
  xiiMutex               m_Mutex;
  xiiVariantArray        m_Logs;
  xiiEventSubscriptionID m_LogSubscription = 0;
#ifdef XII_MCP_HAS_RENDERING
  void      OnGraph(const xiiRenderGraphDebug::ExecutionEvent& event);
  void      OnTiming(const xiiRenderGraphTimestampProfiler::TimingSample& sample);
  xiiStatus GetGraph(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result);

  xiiVariantDictionary   m_Graph;
  xiiVariantArray        m_Timings;
  xiiString              m_sRequestedView;
  xiiTime                m_CaptureUntil;
  xiiEventSubscriptionID m_GraphSubscription  = 0;
  xiiEventSubscriptionID m_TimingSubscription = 0;
#endif
};
