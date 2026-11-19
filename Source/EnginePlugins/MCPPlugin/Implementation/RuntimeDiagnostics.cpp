/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>
#include <MCPPlugin/Implementation/RuntimeDiagnostics.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Reflection.h>

#ifdef XII_MCP_HAS_RENDERING
#  include <Core/GameApplication/GameApplicationBase.h>
#  include <Core/Graphics/Camera.h>
#  include <Core/System/WindowManager.h>
#  include <Core/World/World.h>
#  include <Foundation/Utilities/ConversionUtils.h>
#  include <GraphicsCore/Pipeline/RenderGraph.h>
#  include <GraphicsCore/Pipeline/RenderWorldModule.h>
#  include <GraphicsCore/Pipeline/View.h>
#  include <GraphicsFoundation/Device/Device.h>
#endif

namespace
{
  xiiStatus GetDiagnostics(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
  {
    xiiDynamicArray<xiiPlugin::PluginInfo> pluginInfos;
    xiiPlugin::GetAllPluginInfos(pluginInfos);
    xiiVariantArray plugins;
    for (const auto& plugin : pluginInfos)
    {
      if (plugins.GetCount() >= 128)
      {
        break;
      }
      plugins.PushBack(plugin.m_sName);
    }
    out_result["plugins"] = std::move(plugins);

    xiiVariantArray allocators;
    for (auto it = xiiMemoryTracker::GetIterator(); it.IsValid() && allocators.GetCount() < 128; ++it)
    {
      xiiVariantDictionary row;
      row["name"]        = xiiString(it.Name());
      row["bytes"]       = it.Stats().m_uiAllocationSize;
      row["allocations"] = it.Stats().m_uiAllocationCount;
      allocators.PushBack(std::move(row));
    }
    out_result["allocators"] = std::move(allocators);

    xiiStats::MapType counters;
    out_result["omittedCounters"] = xiiStats::Capture(counters, 128);
    xiiVariantDictionary values;
    for (auto it = counters.GetIterator(); it.IsValid(); ++it)
    {
      xiiVariant json;
      if (!it.Value().IsA<xiiString>() && xiiMCPReflection::ConvertValue(it.Value(), json))
      {
        values[it.Key()] = std::move(json);
      }
    }
    out_result["counters"] = std::move(values);
    return XII_SUCCESS;
  }

  xiiStatus GetCVars(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto      sFilter = xiiMCPJson::GetString(arguments, "filter");
    xiiVariantArray rows;
    for (const auto* pVariable = xiiCVar::GetFirstInstance(); pVariable != nullptr && rows.GetCount() < 128; pVariable = pVariable->GetNextInstance())
    {
      if (!sFilter.IsEmpty() && pVariable->GetName().FindSubString_NoCase(sFilter) == nullptr)
      {
        continue;
      }
      xiiVariantDictionary row;
      row["name"]   = xiiString(pVariable->GetName());
      row["plugin"] = xiiString(pVariable->GetPluginName());
      switch (pVariable->GetType())
      {
        case xiiCVarType::Bool:
          row["value"] = static_cast<const xiiCVarBool*>(pVariable)->GetValue();
          break;
        case xiiCVarType::Int:
          row["value"] = static_cast<const xiiCVarInt*>(pVariable)->GetValue();
          break;
        case xiiCVarType::Float:
          row["value"] = static_cast<const xiiCVarFloat*>(pVariable)->GetValue();
          break;
        case xiiCVarType::Double:
          row["value"] = static_cast<const xiiCVarDouble*>(pVariable)->GetValue();
          break;
        case xiiCVarType::String:
          row["redacted"] = true;
          break;
        default:
          break;
      }
      rows.PushBack(std::move(row));
    }
    out_result["variables"] = std::move(rows);
    return XII_SUCCESS;
  }

#ifdef XII_MCP_HAS_RENDERING
  xiiStatus RequestScreenshot(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
  {
    auto* pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance();
    auto* pWindows     = xiiWindowManager::GetSingleton();
    if (pApplication == nullptr || pWindows == nullptr || pApplication->GetActiveGameState() == nullptr)
    {
      return xiiStatus("No active game window supports screenshot capture.");
    }

    xiiDynamicArray<xiiRegisteredWindowHandle> windows;
    pWindows->GetRegistered(windows, pApplication->GetActiveGameState());
    if (windows.IsEmpty())
    {
      return xiiStatus("No active game window supports screenshot capture.");
    }

    pApplication->TakeScreenshot();
    out_result["requested"]       = true;
    out_result["outputDirectory"] = xiiString(":appdata/Screenshots");
    return XII_SUCCESS;
  }

  xiiStatus GetGraphics(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
  {
    auto pDevice = xiiGALDevice::GetDefaultDevice();
    if (!pDevice)
    {
      return xiiStatus("No graphics device is active.");
    }
    const auto& adapter               = pDevice->GetGraphicsDeviceAdapterProperties();
    const auto& features              = pDevice->GetFeatures();
    out_result["adapter"]             = adapter.m_sAdapterName;
    out_result["vendorId"]            = adapter.m_uiVendorID;
    out_result["deviceId"]            = adapter.m_uiDeviceID;
    out_result["computeShaders"]      = features.m_ComputeShaders != xiiGALDeviceFeatureState::Disabled;
    out_result["meshShaders"]         = features.m_MeshShaders != xiiGALDeviceFeatureState::Disabled;
    out_result["rayTracing"]          = features.m_RayTracing != xiiGALDeviceFeatureState::Disabled;
    out_result["variableRateShading"] = features.m_VariableRateShading != xiiGALDeviceFeatureState::Disabled;
    xiiDynamicArray<xiiGALMemoryHeapStatistics> heaps;
    pDevice->GetMemoryHeapStatistics(heaps);
    xiiVariantArray rows;
    for (const auto& heap : heaps)
    {
      xiiVariantDictionary row;
      row["capacityBytes"]  = heap.m_uiCapacityBytes;
      row["budgetBytes"]    = heap.m_uiBudgetBytes;
      row["usageBytes"]     = heap.m_uiUsageBytes;
      row["committedBytes"] = heap.m_uiCommittedBytes;
      row["allocatedBytes"] = heap.m_uiAllocatedBytes;
      row["deviceLocal"]    = heap.m_bDeviceLocal;
      row["hostVisible"]    = heap.m_bHostVisible;
      row["nativeBudget"]   = heap.m_bNativeBudget;
      rows.PushBack(std::move(row));
    }
    out_result["memoryHeaps"] = std::move(rows);
    return XII_SUCCESS;
  }

  xiiStatus ListRenderViews(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto sWorld = xiiMCPJson::GetString(arguments, "world");
    if (!xiiConversionUtils::IsStringUuid(sWorld))
    {
      return xiiStatus("Use a world ID returned by runtime.worlds.");
    }
    const auto worldId = xiiConversionUtils::ConvertStringToUuid(sWorld);
    for (xiiUInt32 uiSlot = 0; uiSlot < xiiWorld::GetWorldCount(); ++uiSlot)
    {
      const auto* pWorld = xiiWorld::GetWorld(uiSlot);
      if (pWorld == nullptr || pWorld->GetInstanceId() != worldId)
      {
        continue;
      }
      XII_LOCK(pWorld->GetReadMarker());
      const auto* pRender = pWorld->GetModule<xiiRenderWorldModule>();
      if (pRender == nullptr)
      {
        return xiiStatus("The selected world has no rendering module.");
      }

      xiiVariantArray views;
      for (xiiUInt32 uiHint = 0; uiHint < xiiCameraUsageHint::ENUM_COUNT; ++uiHint)
      {
        if (context.HasExpired())
        {
          return xiiStatus("Render view inspection exceeded its deadline.");
        }
        const auto* pView = pRender->GetViewByUsageHint(static_cast<xiiCameraUsageHint::Enum>(uiHint));
        if (pView == nullptr)
        {
          continue;
        }
        const auto&          viewport = pView->GetViewport();
        xiiVariantDictionary row;
        row["name"]            = xiiString(pView->GetName());
        row["usageHint"]       = uiHint;
        row["viewportWidth"]   = viewport.width;
        row["viewportHeight"]  = viewport.height;
        row["renderWidth"]     = pView->GetRenderResolutionWidth();
        row["renderHeight"]    = pView->GetRenderResolutionHeight();
        row["renderScale"]     = pView->GetRenderScale();
        row["resolutionScale"] = pView->GetRenderResolutionScale();
        row["renderMode"]      = pView->GetViewRenderMode().GetValue();
        row["renderPath"]      = pView->GetRenderPath().GetValue();
        if (pView->GetRenderPipelineResource().IsValid())
        {
          row["pipeline"] = xiiString(pView->GetRenderPipelineResource().GetResourceID());
        }
        if (const auto* pCamera = pView->GetCamera())
        {
          if (xiiVariant position; xiiMCPReflection::ConvertValue(pCamera->GetPosition(), position))
          {
            row["cameraPosition"] = position;
          }
          if (xiiVariant forward; xiiMCPReflection::ConvertValue(pCamera->GetDirForwards(), forward))
          {
            row["cameraForward"] = forward;
          }
        }
        views.PushBack(row);
      }
      out_result["views"] = views;
      return XII_SUCCESS;
    }
    return xiiStatus("The selected world no longer exists.");
  }
#endif
} // namespace

xiiMCPRuntimeDiagnostics::xiiMCPRuntimeDiagnostics(xiiMCPRegistry& registry) :
  m_Registry(registry)
{
  m_Registry.RegisterTool("runtime.diagnostics", "Read bounded allocator, plugin and performance counter snapshots.", R"({"type":"object","additionalProperties":false})",
                          xiiMCPPermission::Read, &GetDiagnostics)
    .IgnoreResult();
  m_Registry.RegisterTool("runtime.cvars", "List matching CVars and numeric or boolean values. String values are redacted.", R"({"type":"object","properties":{"filter":{"type":"string"}}})",
                          xiiMCPPermission::Read, &GetCVars)
    .IgnoreResult();
  m_Registry.RegisterTool("runtime.logs", "Read the most recent engine warnings and errors captured since MCP began.", R"({"type":"object","additionalProperties":false})",
                          xiiMCPPermission::Capture, xiiMakeDelegate(&xiiMCPRuntimeDiagnostics::GetLogs, this))
    .IgnoreResult();
  m_LogSubscription = xiiGlobalLog::AddLogWriter(xiiMakeDelegate(&xiiMCPRuntimeDiagnostics::OnLog, this));
}

xiiMCPRuntimeDiagnostics::~xiiMCPRuntimeDiagnostics()
{
#ifdef XII_MCP_HAS_RENDERING
  if (m_GraphSubscription != 0)
  {
    xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(m_GraphSubscription);
    xiiRenderGraphTimestampProfiler::s_TimingEvents.RemoveEventHandler(m_TimingSubscription);
  }
#endif
  if (m_LogSubscription != 0)
  {
    xiiGlobalLog::RemoveLogWriter(m_LogSubscription);
  }
}

void xiiMCPRuntimeDiagnostics::OnLog(const xiiLoggingEventData& log)
{
  if (log.m_EventType < xiiLogMsgType::ErrorMsg || log.m_EventType > xiiLogMsgType::WarningMsg)
  {
    return;
  }
  xiiVariantDictionary row;
  row["level"]            = xiiString(log.m_EventType == xiiLogMsgType::ErrorMsg ? "error" : "warning");
  row["text"]             = xiiString(log.m_sText.GetSubString(0, 512));
  row["tag"]              = xiiString(log.m_sTag);
  row["timestampSeconds"] = xiiTime::Now().GetSeconds();
  XII_LOCK(m_Mutex);
  if (m_Logs.GetCount() >= 64)
  {
    m_Logs.RemoveAtAndCopy(0);
  }
  m_Logs.PushBack(std::move(row));
}

xiiStatus xiiMCPRuntimeDiagnostics::GetLogs(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
{
  XII_LOCK(m_Mutex);
  out_result["entries"] = m_Logs;
  return XII_SUCCESS;
}

void xiiMCPRuntimeDiagnostics::Update()
{
#ifdef XII_MCP_HAS_RENDERING
  const bool bAvailable = xiiGALDevice::HasDefaultDevice();
  if (bAvailable && m_GraphSubscription == 0)
  {
    m_Registry.RegisterTool("runtime.screenshot", "Request a screenshot of the game application output. XII saves it through its normal screenshot pipeline.",
                            R"({"type":"object","additionalProperties":false})", xiiMCPPermission::Capture, &RequestScreenshot)
      .IgnoreResult();
    m_Registry.RegisterTool("runtime.graphics", "Read the active graphics adapter, optional features and heap allocations.", R"({"type":"object","additionalProperties":false})",
                            xiiMCPPermission::Read, &GetGraphics)
      .IgnoreResult();
    m_Registry.RegisterTool("runtime.render_views", "Inspect the first view for each camera usage in a world, including camera, pipeline, mode and render resolution.",
                            R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"}}})", xiiMCPPermission::Read, &ListRenderViews)
      .IgnoreResult();
    m_Registry.RegisterTool("runtime.render_graph", "Request a bounded render graph snapshot for a named view (or the next rendered view) and poll for its passes and available GPU timings.",
                            R"({"type":"object","properties":{"capture":{"type":"boolean","default":false},"view":{"type":"string","maxLength":128}}})",
                            xiiMCPPermission::Capture, xiiMakeDelegate(&xiiMCPRuntimeDiagnostics::GetGraph, this))
      .IgnoreResult();
    m_GraphSubscription  = xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(xiiMakeDelegate(&xiiMCPRuntimeDiagnostics::OnGraph, this));
    m_TimingSubscription = xiiRenderGraphTimestampProfiler::s_TimingEvents.AddEventHandler(xiiMakeDelegate(&xiiMCPRuntimeDiagnostics::OnTiming, this));
  }
  else if (!bAvailable && m_GraphSubscription != 0)
  {
    m_Registry.Remove(xiiMCPCapability::Tool, "runtime.render_graph");
    m_Registry.Remove(xiiMCPCapability::Tool, "runtime.graphics");
    m_Registry.Remove(xiiMCPCapability::Tool, "runtime.render_views");
    m_Registry.Remove(xiiMCPCapability::Tool, "runtime.screenshot");
    xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(m_GraphSubscription);
    xiiRenderGraphTimestampProfiler::s_TimingEvents.RemoveEventHandler(m_TimingSubscription);
    m_Graph.Clear();
    m_Timings.Clear();
    m_CaptureUntil = {};
  }
#endif
}

#ifdef XII_MCP_HAS_RENDERING
void xiiMCPRuntimeDiagnostics::OnGraph(const xiiRenderGraphDebug::ExecutionEvent& event)
{
  XII_LOCK(m_Mutex);
  if (!m_CaptureUntil.IsPositive() || xiiTime::Now() > m_CaptureUntil || !m_Graph.IsEmpty() ||
      (!m_sRequestedView.IsEmpty() && event.m_sViewName != m_sRequestedView))
  {
    return;
  }

  m_Graph["frame"]    = event.m_uiFrameIndex;
  m_Graph["view"]     = xiiString(event.m_sViewName);
  m_Graph["executed"] = event.m_bSucceeded;
  if (!event.m_bSucceeded)
  {
    m_Graph["error"] = xiiString(event.m_sError.GetSubString(0, 512));
  }
  xiiVariantArray passes;
  for (const auto& pass : event.m_Graph.GetCompiledPasses())
  {
    if (passes.GetCount() >= 128)
    {
      m_Graph["truncated"] = true;
      break;
    }
    xiiVariantDictionary row;
    row["name"]       = xiiString(pass.m_sName.GetView());
    row["index"]      = pass.m_uiPassIndex;
    row["queue"]      = pass.m_uiQueueIndex;
    row["submission"] = pass.m_uiSubmissionIndex;
    row["culled"]     = pass.m_bIsCulled;
    xiiVariantArray dependencies;
    for (const auto uiDependency : pass.m_DependencyPassIndices)
    {
      if (dependencies.GetCount() >= 32)
      {
        row["truncated"] = true;
        break;
      }
      dependencies.PushBack(uiDependency);
    }
    row["dependsOn"] = std::move(dependencies);
    passes.PushBack(std::move(row));
  }
  m_Graph["passes"] = std::move(passes);
}

void xiiMCPRuntimeDiagnostics::OnTiming(const xiiRenderGraphTimestampProfiler::TimingSample& sample)
{
  XII_LOCK(m_Mutex);
  const auto* pFrame = m_Graph.GetValue("frame");
  if (pFrame == nullptr || pFrame->Get<xiiUInt64>() != sample.m_uiFrameIndex || m_Timings.GetCount() >= 128)
  {
    return;
  }
  xiiVariantDictionary row;
  row["graph"]      = sample.m_uiGraphId;
  row["name"]       = xiiString(sample.m_sName.GetView());
  row["passIndex"]  = sample.m_uiPassIndex;
  row["queue"]      = sample.m_uiQueueIndex;
  row["submission"] = sample.m_bSubmission;
  row["available"]  = sample.m_bAvailable;
  if (sample.m_bAvailable)
  {
    row["durationMs"] = sample.m_fDurationMs;
  }
  m_Timings.PushBack(std::move(row));
}

xiiStatus xiiMCPRuntimeDiagnostics::GetGraph(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto* pCapture = arguments.GetValue("capture");
  const auto  sView    = xiiMCPJson::GetString(arguments, "view");
  if ((pCapture != nullptr && !pCapture->IsA<bool>()) || sView.GetElementCount() > 128)
  {
    return xiiStatus("Provide a boolean capture flag and an optional view name up to 128 bytes.");
  }
  XII_LOCK(m_Mutex);
  if (pCapture != nullptr && pCapture->Get<bool>())
  {
    m_Graph.Clear();
    m_Timings.Clear();
    m_sRequestedView = sView;
    m_CaptureUntil   = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  }
  out_result                       = m_Graph;
  out_result["pending"]            = m_Graph.IsEmpty() && xiiTime::Now() <= m_CaptureUntil;
  out_result["gpuTimingsForFrame"] = m_Timings;
  return XII_SUCCESS;
}
#endif
