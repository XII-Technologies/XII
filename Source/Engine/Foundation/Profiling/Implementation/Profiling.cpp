#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/DataTransfer.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadUtils.h>

#if XII_ENABLED(XII_USE_PROFILING)

class xiiProfileCaptureDataTransfer : public xiiDataTransfer
{
private:
  virtual void OnTransferRequest() override
  {
    xiiDataTransferObject dto(*this, "Capture", "application/json", "json");

    xiiProfilingSystem::ProfilingData profilingData;
    xiiProfilingSystem::Capture(profilingData);
    profilingData.Write(dto.GetWriter()).IgnoreResult();

    dto.Transmit();
  }
};

static xiiProfileCaptureDataTransfer s_ProfileCaptureDataTransfer;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    xiiProfilingSystem::Initialize();
    s_ProfileCaptureDataTransfer.EnableDataTransfer("Profiling Capture");
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_ProfileCaptureDataTransfer.DisableDataTransfer();
    xiiProfilingSystem::Reset();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  enum
  {
    BUFFER_SIZE_OTHER_THREAD = 1024 * 1024,
    BUFFER_SIZE_MAIN_THREAD  = BUFFER_SIZE_OTHER_THREAD * 4 ///< Typically the main thread allocated a lot more profiling events than other threads
  };

  enum
  {
    BUFFER_SIZE_FRAMES = 120 * 60,
  };

  using GPUScopesBuffer = xiiStaticRingBuffer<xiiProfilingSystem::GPUScope, BUFFER_SIZE_OTHER_THREAD / sizeof(xiiProfilingSystem::GPUScope)>;

  static xiiUInt64 s_MainThreadId = 0;

  struct CpuScopesBufferBase
  {
    virtual ~CpuScopesBufferBase() = default;

    xiiUInt64 m_uiThreadId = 0;
    bool      IsMainThread() const { return m_uiThreadId == s_MainThreadId; }
  };

  template <xiiUInt32 SizeInBytes>
  struct CpuScopesBuffer : public CpuScopesBufferBase
  {
    xiiStaticRingBuffer<xiiProfilingSystem::CPUScope, SizeInBytes / sizeof(xiiProfilingSystem::CPUScope)> m_Data;
  };

  CpuScopesBuffer<BUFFER_SIZE_MAIN_THREAD>* CastToMainThreadEventBuffer(CpuScopesBufferBase* pEventBuffer)
  {
    XII_ASSERT_DEV(pEventBuffer->IsMainThread(), "Implementation error");
    return static_cast<CpuScopesBuffer<BUFFER_SIZE_MAIN_THREAD>*>(pEventBuffer);
  }

  CpuScopesBuffer<BUFFER_SIZE_OTHER_THREAD>* CastToOtherThreadEventBuffer(CpuScopesBufferBase* pEventBuffer)
  {
    XII_ASSERT_DEV(!pEventBuffer->IsMainThread(), "Implementation error");
    return static_cast<CpuScopesBuffer<BUFFER_SIZE_OTHER_THREAD>*>(pEventBuffer);
  }

  xiiCVarFloat cvar_ProfilingDiscardThresholdMS("Profiling.DiscardThresholdMS", 0.1f, xiiCVarFlags::Default, "Discard profiling scopes if their duration is shorter than this in milliseconds.");

  xiiStaticRingBuffer<xiiTime, BUFFER_SIZE_FRAMES> s_FrameStartTimes;
  xiiUInt64                                        s_uiFrameCount = 0;

  static xiiHybridArray<xiiProfilingSystem::ThreadInfo, 16> s_ThreadInfos;
  static xiiHybridArray<xiiUInt64, 16>                      s_DeadThreadIDs;
  static xiiMutex                                           s_ThreadInfosMutex;

#  if XII_ENABLED(XII_PLATFORM_64BIT)
  XII_CHECK_AT_COMPILETIME(sizeof(xiiProfilingSystem::CPUScope) == 72);
  XII_CHECK_AT_COMPILETIME(sizeof(xiiProfilingSystem::GPUScope) == 64);
#  endif

  static thread_local CpuScopesBufferBase*        s_CpuScopes = nullptr;
  static xiiDynamicArray<CpuScopesBufferBase*>    s_AllCpuScopes;
  static xiiMutex                                 s_AllCpuScopesMutex;
  static xiiProfilingSystem::ScopeTimeoutDelegate s_ScopeTimeoutCallback;

  static xiiDynamicArray<xiiUniquePtr<GPUScopesBuffer>> s_GPUScopes;

  static xiiEventSubscriptionID s_PluginEventSubscription = 0;
  void                          PluginEvent(const xiiPluginEvent& e)
  {
    if (e.m_EventType == xiiPluginEvent::AfterUnloading)
    {
      // When a plugin is unloaded we need to clear all profiling data
      // since they can contain pointers to function names that don't exist anymore.
      xiiProfilingSystem::Clear();
    }
  }
} // namespace

void xiiProfilingSystem::ProfilingData::Clear()
{
  m_uiFramesThreadID = 0;
  m_uiProcessID      = 0;
  m_uiFrameCount     = 0;

  m_AllEventBuffers.Clear();
  m_FrameStartTimes.Clear();
  m_GPUScopes.Clear();
  m_ThreadInfos.Clear();
}

void xiiProfilingSystem::ProfilingData::Merge(ProfilingData& out_merged, xiiArrayPtr<const ProfilingData*> inputs)
{
  out_merged.Clear();

  if (inputs.IsEmpty())
    return;

  out_merged.m_uiProcessID      = inputs[0]->m_uiProcessID;
  out_merged.m_uiFramesThreadID = inputs[0]->m_uiFramesThreadID;

  // concatenate m_FrameStartTimes and m_GPUScopes and m_uiFrameCount
  {
    xiiUInt32 uiNumFrameStartTimes = 0;
    xiiUInt32 uiNumGpuScopes       = 0;

    for (const auto& pd : inputs)
    {
      out_merged.m_uiFrameCount += pd->m_uiFrameCount;

      uiNumFrameStartTimes += pd->m_FrameStartTimes.GetCount();
      uiNumGpuScopes += pd->m_GPUScopes.GetCount();
    }

    out_merged.m_FrameStartTimes.Reserve(uiNumFrameStartTimes);
    out_merged.m_GPUScopes.Reserve(uiNumGpuScopes);

    for (const auto& pd : inputs)
    {
      out_merged.m_FrameStartTimes.PushBackRange(pd->m_FrameStartTimes);
      out_merged.m_GPUScopes.PushBackRange(pd->m_GPUScopes);
    }
  }

  // merge m_ThreadInfos
  {
    auto threadInfoAlreadyKnown = [out_merged](xiiUInt64 uiThreadId) -> bool {
      for (const auto& ti : out_merged.m_ThreadInfos)
      {
        if (ti.m_uiThreadId == uiThreadId)
          return true;
      }

      return false;
    };

    for (const auto& pd : inputs)
    {
      for (const auto& ti : pd->m_ThreadInfos)
      {
        if (!threadInfoAlreadyKnown(ti.m_uiThreadId))
        {
          out_merged.m_ThreadInfos.PushBack(ti);
        }
      }
    }
  }

  // merge m_AllEventBuffers
  {
    struct CountAndIndex
    {
      xiiUInt32 m_uiCount = 0;
      xiiUInt32 m_uiIndex = 0xFFFFFFFF;
    };

    xiiMap<xiiUInt64, CountAndIndex> eventBufferInfos;

    // gather info about required size of the output array
    for (const auto& pd : inputs)
    {
      for (const auto& eb : pd->m_AllEventBuffers)
      {
        auto& ebInfo = eventBufferInfos[eb.m_uiThreadId];

        ebInfo.m_uiIndex = xiiMath::Min(ebInfo.m_uiIndex, eventBufferInfos.GetCount() - 1);
        ebInfo.m_uiCount += eb.m_Data.GetCount();
      }
    }

    // reserve the output array
    {
      out_merged.m_AllEventBuffers.SetCount(eventBufferInfos.GetCount());

      for (auto ebinfoIt : eventBufferInfos)
      {
        auto& neb        = out_merged.m_AllEventBuffers[ebinfoIt.Value().m_uiIndex];
        neb.m_uiThreadId = ebinfoIt.Key();
        neb.m_Data.Reserve(ebinfoIt.Value().m_uiCount);
      }
    }

    // fill the output array
    for (const auto& pd : inputs)
    {
      for (const auto& eb : pd->m_AllEventBuffers)
      {
        const auto& ebInfo = eventBufferInfos[eb.m_uiThreadId];

        out_merged.m_AllEventBuffers[ebInfo.m_uiIndex].m_Data.PushBackRange(eb.m_Data);
      }
    }
  }
}

xiiResult xiiProfilingSystem::ProfilingData::Write(xiiStreamWriter& ref_outputStream) const
{
  xiiStandardJSONWriter writer;
  writer.SetWhitespaceMode(xiiJSONWriter::WhitespaceMode::None);
  writer.SetOutputStream(&ref_outputStream);

  writer.BeginObject();
  {
    writer.BeginArray("traceEvents");

    // Process metadata
    {
      xiiApplication::GetApplicationInstance()->GetApplicationName();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "process_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", xiiApplication::GetApplicationInstance() ? xiiApplication::GetApplicationInstance()->GetApplicationName().GetView() : "XII");
        writer.EndObject();
      }
      writer.EndObject();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "process_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", m_uiProcessSortIndex);
        writer.EndObject();
      }
      writer.EndObject();
    }

    // Frames thread metadata
    {
      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableString("name", "Frames");
        writer.EndObject();
      }
      writer.EndObject();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", -1);
        writer.EndObject();
      }
      writer.EndObject();

      if (writer.HadWriteError())
      {
        return XII_FAILURE;
      }
    }

    const xiiUInt32 uiGpuCount = m_GPUScopes.GetCount();
    // GPU thread metadata
    // Since there are no actual threads, we assign 1..uiGpuCount as the respective threadID
    for (xiiUInt32 gpuIndex = 1; gpuIndex <= uiGpuCount; ++gpuIndex)
    {
      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_name");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", gpuIndex);
        writer.AddVariableString("ph", "M");

        xiiStringBuilder gpuNameBuilder;
        gpuNameBuilder.AppendFormat("GPU {}", gpuIndex - 1);

        writer.BeginObject("args");
        writer.AddVariableString("name", gpuNameBuilder);
        writer.EndObject();
      }
      writer.EndObject();

      writer.BeginObject();
      {
        writer.AddVariableString("name", "thread_sort_index");
        writer.AddVariableString("cat", "__metadata");
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", gpuIndex);
        writer.AddVariableString("ph", "M");

        writer.BeginObject("args");
        writer.AddVariableInt32("sort_index", -2);
        writer.EndObject();
      }
      writer.EndObject();
      if (writer.HadWriteError())
      {
        return XII_FAILURE;
      }
    }

    // thread metadata
    {
      for (xiiUInt32 threadIndex = 0; threadIndex < m_ThreadInfos.GetCount(); ++threadIndex)
      {
        const ThreadInfo& info = m_ThreadInfos[threadIndex];
        writer.BeginObject();
        {
          writer.AddVariableString("name", "thread_name");
          writer.AddVariableString("cat", "__metadata");
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", info.m_uiThreadId + uiGpuCount + 1);
          writer.AddVariableString("ph", "M");

          writer.BeginObject("args");
          writer.AddVariableString("name", info.m_sName);
          writer.EndObject();
        }
        writer.EndObject();

        writer.BeginObject();
        {
          writer.AddVariableString("name", "thread_sort_index");
          writer.AddVariableString("cat", "__metadata");
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", info.m_uiThreadId + uiGpuCount + 1);
          writer.AddVariableString("ph", "M");

          writer.BeginObject("args");
          writer.AddVariableInt32("sort_index", threadIndex);
          writer.EndObject();
        }
        writer.EndObject();

        if (writer.HadWriteError())
        {
          return XII_FAILURE;
        }
      }
    }

    // scoped events
    xiiDynamicArray<CPUScope> sortedScopes;
    for (const auto& eventBuffer : m_AllEventBuffers)
    {
      // Since we introduced fake thread IDs via the GPUs, we simply shift all real thread IDs to be in a different range to avoid collisions.
      const xiiUInt64 uiThreadId = eventBuffer.m_uiThreadId + uiGpuCount + 1;

      // It seems that chrome does a stable sort by scope begin time. Now that we write complete scopes at the end of a scope
      // we actually write nested scopes before their corresponding parent scope to the file. If both start at the same quantized time stamp
      // chrome prints the nested scope first and then scrambles everything.
      // So we sort by duration to make sure that parent scopes are written first in the json file.
      sortedScopes = eventBuffer.m_Data;
      sortedScopes.Sort([](const CPUScope& a, const CPUScope& b) { return (a.m_EndTime - a.m_BeginTime) > (b.m_EndTime - b.m_BeginTime); });

      for (const CPUScope& e : sortedScopes)
      {
        writer.BeginObject();
        writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", uiThreadId);
        writer.AddVariableUInt64("ts", static_cast<xiiUInt64>(e.m_BeginTime.GetMicroseconds()));
        writer.AddVariableString("ph", "B");

        if (e.m_sFunctionName != nullptr)
        {
          writer.BeginObject("args");
          writer.AddVariableString("function", e.m_sFunctionName);
          writer.EndObject();
        }

        writer.EndObject();

        if (e.m_EndTime.IsPositive())
        {
          writer.BeginObject();
          writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", uiThreadId);
          writer.AddVariableUInt64("ts", static_cast<xiiUInt64>(e.m_EndTime.GetMicroseconds()));
          writer.AddVariableString("ph", "E");
          writer.EndObject();
        }

        if (writer.HadWriteError())
        {
          return XII_FAILURE;
        }
      }
    }

    // frame start/end
    {
      xiiStringBuilder sFrameName;

      const xiiUInt32 uiNumFrames = m_FrameStartTimes.GetCount();
      for (xiiUInt32 i = 1; i < uiNumFrames; ++i)
      {
        const xiiTime t0 = m_FrameStartTimes[i - 1];
        const xiiTime t1 = m_FrameStartTimes[i];

        const xiiUInt64 localFrameID = uiNumFrames - i - 1;
        sFrameName.SetFormat("Frame {}", m_uiFrameCount - localFrameID);

        writer.BeginObject();
        writer.AddVariableString("name", sFrameName);
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableUInt64("ts", static_cast<xiiUInt64>(t0.GetMicroseconds()));
        writer.AddVariableString("ph", "B");
        writer.EndObject();

        writer.BeginObject();
        writer.AddVariableString("name", sFrameName);
        writer.AddVariableUInt32("pid", m_uiProcessID);
        writer.AddVariableUInt64("tid", m_uiFramesThreadID);
        writer.AddVariableUInt64("ts", static_cast<xiiUInt64>(t1.GetMicroseconds()));
        writer.AddVariableString("ph", "E");
        writer.EndObject();
        if (writer.HadWriteError())
        {
          return XII_FAILURE;
        }
      }
    }

    // GPU data
    // Since there are no actual threads, we assign 1..gpuCount as the respective threadID
    {
      // See comment on sortedScopes above.
      xiiDynamicArray<GPUScope> sortedGpuScopes;
      for (xiiUInt32 gpuIndex = 1; gpuIndex <= m_GPUScopes.GetCount(); ++gpuIndex)
      {
        sortedGpuScopes = m_GPUScopes[gpuIndex - 1];
        sortedGpuScopes.Sort([](const GPUScope& a, const GPUScope& b) { return (a.m_EndTime - a.m_BeginTime) > (b.m_EndTime - b.m_BeginTime); });

        for (xiiUInt32 i = 0; i < sortedGpuScopes.GetCount(); ++i)
        {
          const auto& e = sortedGpuScopes[i];

          writer.BeginObject();
          writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", gpuIndex);
          writer.AddVariableUInt64("ts", static_cast<xiiUInt64>(e.m_BeginTime.GetMicroseconds()));
          writer.AddVariableString("ph", "B");
          writer.EndObject();

          writer.BeginObject();
          writer.AddVariableString("name", static_cast<const char*>(e.m_szName));
          writer.AddVariableUInt32("pid", m_uiProcessID);
          writer.AddVariableUInt64("tid", gpuIndex);
          writer.AddVariableUInt64("ts", static_cast<xiiUInt64>(e.m_EndTime.GetMicroseconds()));
          writer.AddVariableString("ph", "E");
          writer.EndObject();
          if (writer.HadWriteError())
          {
            return XII_FAILURE;
          }
        }
      }
    }

    writer.EndArray();
  }

  writer.EndObject();

  return writer.HadWriteError() ? XII_FAILURE : XII_SUCCESS;
}

// static
void xiiProfilingSystem::Clear()
{
  {
    XII_LOCK(s_AllCpuScopesMutex);
    for (auto pEventBuffer : s_AllCpuScopes)
    {
      if (pEventBuffer->IsMainThread())
      {
        CastToMainThreadEventBuffer(pEventBuffer)->m_Data.Clear();
      }
      else
      {
        CastToOtherThreadEventBuffer(pEventBuffer)->m_Data.Clear();
      }
    }
  }

  s_FrameStartTimes.Clear();

  for (auto& gpuScopes : s_GPUScopes)
  {
    if (gpuScopes != nullptr)
    {
      gpuScopes->Clear();
    }
  }
}

// static
void xiiProfilingSystem::Capture(xiiProfilingSystem::ProfilingData& ref_profilingData, bool bClearAfterCapture)
{
  ref_profilingData.Clear();

  ref_profilingData.m_uiFramesThreadID = 0;
#  if XII_ENABLED(XII_SUPPORTS_PROCESSES)
  ref_profilingData.m_uiProcessID = xiiProcess::GetCurrentProcessID();
#  else
  ref_profilingData.m_uiProcessID = 0;
#  endif

  {
    XII_LOCK(s_ThreadInfosMutex);

    if (bClearAfterCapture)
    {
      ref_profilingData.m_ThreadInfos = std::move(s_ThreadInfos);
    }
    else
    {
      ref_profilingData.m_ThreadInfos = s_ThreadInfos;
    }
  }

  {
    XII_LOCK(s_AllCpuScopesMutex);

    ref_profilingData.m_AllEventBuffers.Reserve(s_AllCpuScopes.GetCount());
    for (xiiUInt32 i = 0; i < s_AllCpuScopes.GetCount(); ++i)
    {
      const auto&          sourceEventBuffer = s_AllCpuScopes[i];
      CPUScopesBufferFlat& targetEventBuffer = ref_profilingData.m_AllEventBuffers.ExpandAndGetRef();

      targetEventBuffer.m_uiThreadId = sourceEventBuffer->m_uiThreadId;

      xiiUInt32 uiSourceCount = sourceEventBuffer->IsMainThread() ? CastToMainThreadEventBuffer(sourceEventBuffer)->m_Data.GetCount() : CastToOtherThreadEventBuffer(sourceEventBuffer)->m_Data.GetCount();
      targetEventBuffer.m_Data.SetCountUninitialized(uiSourceCount);
      for (xiiUInt32 j = 0; j < uiSourceCount; ++j)
      {
        const CPUScope& sourceEvent = sourceEventBuffer->IsMainThread() ? CastToMainThreadEventBuffer(sourceEventBuffer)->m_Data[j] : CastToOtherThreadEventBuffer(sourceEventBuffer)->m_Data[j];

        CPUScope& copiedEvent       = targetEventBuffer.m_Data[j];
        copiedEvent.m_sFunctionName = sourceEvent.m_sFunctionName;
        copiedEvent.m_BeginTime     = sourceEvent.m_BeginTime;
        copiedEvent.m_EndTime       = sourceEvent.m_EndTime;
        xiiStringUtils::Copy(copiedEvent.m_szName, CPUScope::NAME_SIZE, sourceEvent.m_szName);
      }
    }
  }

  ref_profilingData.m_uiFrameCount = s_uiFrameCount;

  ref_profilingData.m_FrameStartTimes.SetCountUninitialized(s_FrameStartTimes.GetCount());
  for (xiiUInt32 i = 0; i < s_FrameStartTimes.GetCount(); ++i)
  {
    ref_profilingData.m_FrameStartTimes[i] = s_FrameStartTimes[i];
  }

  if (!s_GPUScopes.IsEmpty())
  {
    for (const auto& gpuScopes : s_GPUScopes)
    {
      if (gpuScopes != nullptr)
      {
        xiiDynamicArray<GPUScope>& gpuScopesCopy = ref_profilingData.m_GPUScopes.ExpandAndGetRef();
        gpuScopesCopy.SetCountUninitialized(gpuScopes->GetCount());
        for (xiiUInt32 i = 0; i < gpuScopes->GetCount(); ++i)
        {
          const GPUScope& sourceGpuDat = (*gpuScopes)[i];

          GPUScope& copiedGpuData   = gpuScopesCopy[i];
          copiedGpuData.m_BeginTime = sourceGpuDat.m_BeginTime;
          copiedGpuData.m_EndTime   = sourceGpuDat.m_EndTime;
          xiiStringUtils::Copy(copiedGpuData.m_szName, GPUScope::NAME_SIZE, sourceGpuDat.m_szName);
        }
      }
    }
  }

  if (bClearAfterCapture)
  {
    Clear();
  }
}

// static
void xiiProfilingSystem::SetDiscardThreshold(xiiTime threshold)
{
  cvar_ProfilingDiscardThresholdMS = static_cast<float>(threshold.GetMilliseconds());
}

void xiiProfilingSystem::SetScopeTimeoutCallback(ScopeTimeoutDelegate callback)
{
  s_ScopeTimeoutCallback = callback;
}

// static
xiiUInt64 xiiProfilingSystem::GetFrameCount()
{
  return s_uiFrameCount;
}

// static
void xiiProfilingSystem::StartNewFrame()
{
  ++s_uiFrameCount;

  if (!s_FrameStartTimes.CanAppend())
  {
    s_FrameStartTimes.PopFront();
  }

  s_FrameStartTimes.PushBack(xiiTime::Now());
}

// static
void xiiProfilingSystem::AddCPUScope(xiiStringView sName, xiiStringView sFunctionName, xiiTime beginTime, xiiTime endTime, xiiTime scopeTimeout)
{
  const xiiTime duration = endTime - beginTime;

  // discard?
  if (duration < xiiTime::Milliseconds(cvar_ProfilingDiscardThresholdMS))
    return;

  ::CpuScopesBufferBase* pScopes = s_CpuScopes;

  if (pScopes == nullptr)
  {
    if (xiiThreadUtils::IsMainThread())
    {
      pScopes = XII_DEFAULT_NEW(::CpuScopesBuffer<BUFFER_SIZE_MAIN_THREAD>);
    }
    else
    {
      pScopes = XII_DEFAULT_NEW(::CpuScopesBuffer<BUFFER_SIZE_OTHER_THREAD>);
    }

    pScopes->m_uiThreadId = (xiiUInt64)xiiThreadUtils::GetCurrentThreadID();
    s_CpuScopes           = pScopes;

    {
      XII_LOCK(s_AllCpuScopesMutex);
      s_AllCpuScopes.PushBack(pScopes);
    }
  }

  CPUScope scope;
  scope.m_sFunctionName = sFunctionName;
  scope.m_BeginTime     = beginTime;
  scope.m_EndTime       = endTime;
  xiiStringUtils::Copy(scope.m_szName, XII_ARRAY_SIZE(scope.m_szName), sName.GetStartPointer(), sName.GetEndPointer());

  if (xiiThreadUtils::IsMainThread())
  {
    auto pMainThreadBuffer = CastToMainThreadEventBuffer(pScopes);
    if (!pMainThreadBuffer->m_Data.CanAppend())
    {
      pMainThreadBuffer->m_Data.PopFront();
    }

    pMainThreadBuffer->m_Data.PushBack(scope);
  }
  else
  {
    auto pOtherThreadBuffer = CastToOtherThreadEventBuffer(pScopes);
    if (!pOtherThreadBuffer->m_Data.CanAppend())
    {
      pOtherThreadBuffer->m_Data.PopFront();
    }

    pOtherThreadBuffer->m_Data.PushBack(scope);
  }

  if (scopeTimeout.IsPositive() && duration > scopeTimeout && s_ScopeTimeoutCallback.IsValid())
  {
    s_ScopeTimeoutCallback(sName, sFunctionName, duration);
  }
}

// static
void xiiProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");

  s_MainThreadId = (xiiUInt64)xiiThreadUtils::GetCurrentThreadID();

  s_PluginEventSubscription = xiiPlugin::Events().AddEventHandler(&PluginEvent);
}

// static
void xiiProfilingSystem::Reset()
{
  XII_LOCK(s_ThreadInfosMutex);
  XII_LOCK(s_AllCpuScopesMutex);
  for (xiiUInt32 i = 0; i < s_DeadThreadIDs.GetCount(); i++)
  {
    xiiUInt64 uiThreadId = s_DeadThreadIDs[i];
    for (xiiUInt32 k = 0; k < s_ThreadInfos.GetCount(); k++)
    {
      if (s_ThreadInfos[k].m_uiThreadId == uiThreadId)
      {
        // Don't use swap as a thread ID could be re-used and so we might delete the
        // info for an actual thread in the next loop instead of the remnants of the thread
        // that existed before.
        s_ThreadInfos.RemoveAtAndCopy(k);
        break;
      }
    }
    for (xiiUInt32 k = 0; k < s_AllCpuScopes.GetCount(); k++)
    {
      CpuScopesBufferBase* pEventBuffer = s_AllCpuScopes[k];
      if (pEventBuffer->m_uiThreadId == uiThreadId)
      {
        XII_DEFAULT_DELETE(pEventBuffer);
        // Forward order and no swap important, see comment above.
        s_AllCpuScopes.RemoveAtAndCopy(k);
      }
    }
  }
  s_DeadThreadIDs.Clear();

  xiiPlugin::Events().RemoveEventHandler(s_PluginEventSubscription);
}

// static
void xiiProfilingSystem::SetThreadName(xiiStringView sThreadName)
{
  XII_LOCK(s_ThreadInfosMutex);

  ThreadInfo& info  = s_ThreadInfos.ExpandAndGetRef();
  info.m_uiThreadId = (xiiUInt64)xiiThreadUtils::GetCurrentThreadID();
  info.m_sName      = sThreadName;
}

// static
void xiiProfilingSystem::RemoveThread()
{
  XII_LOCK(s_ThreadInfosMutex);

  s_DeadThreadIDs.PushBack((xiiUInt64)xiiThreadUtils::GetCurrentThreadID());
}

// static
void xiiProfilingSystem::InitializeGPUData(xiiUInt32 uiGpuCount)
{
  if (s_GPUScopes.GetCount() < uiGpuCount)
  {
    s_GPUScopes.SetCount(uiGpuCount);
  }

  for (auto& gpuScopes : s_GPUScopes)
  {
    if (gpuScopes == nullptr)
    {
      gpuScopes = XII_DEFAULT_NEW(GPUScopesBuffer);
    }
  }
}

void xiiProfilingSystem::AddGPUScope(xiiStringView sName, xiiTime beginTime, xiiTime endTime, xiiUInt32 uiGpuIndex)
{
  // discard?
  if (endTime - beginTime < xiiTime::Milliseconds(cvar_ProfilingDiscardThresholdMS))
    return;

  if (!s_GPUScopes[uiGpuIndex]->CanAppend())
  {
    s_GPUScopes[uiGpuIndex]->PopFront();
  }

  GPUScope scope;
  scope.m_BeginTime = beginTime;
  scope.m_EndTime   = endTime;
  xiiStringUtils::Copy(scope.m_szName, XII_ARRAY_SIZE(scope.m_szName), sName.GetStartPointer(), sName.GetEndPointer());

  s_GPUScopes[uiGpuIndex]->PushBack(scope);
}

//////////////////////////////////////////////////////////////////////////

xiiProfilingScope::xiiProfilingScope(xiiStringView sName, xiiStringView sFunctionName, xiiTime timeout) :
  m_sName(sName), m_sFunction(sFunctionName), m_BeginTime(xiiTime::Now()), m_Timeout(timeout)
{
}

xiiProfilingScope::~xiiProfilingScope()
{
  xiiProfilingSystem::AddCPUScope(m_sName, m_sFunction, m_BeginTime, xiiTime::Now(), m_Timeout);
}

//////////////////////////////////////////////////////////////////////////

thread_local xiiProfilingListScope* xiiProfilingListScope::s_pCurrentList = nullptr;

xiiProfilingListScope::xiiProfilingListScope(xiiStringView sListName, xiiStringView sFirstSectionName, xiiStringView sFunctionName) :
  m_sListName(sListName), m_sListFunction(sFunctionName), m_ListBeginTime(xiiTime::Now()), m_sCurSectionName(sFirstSectionName), m_CurSectionBeginTime(m_ListBeginTime)
{
  m_pPreviousList = s_pCurrentList;
  s_pCurrentList  = this;
}

xiiProfilingListScope::~xiiProfilingListScope()
{
  xiiTime now = xiiTime::Now();
  xiiProfilingSystem::AddCPUScope(m_sCurSectionName, nullptr, m_CurSectionBeginTime, now, xiiTime::MakeZero());
  xiiProfilingSystem::AddCPUScope(m_sListName, m_sListFunction, m_ListBeginTime, now, xiiTime::MakeZero());

  s_pCurrentList = m_pPreviousList;
}

// static
void xiiProfilingListScope::StartNextSection(xiiStringView sNextSectionName)
{
  xiiProfilingListScope* pCurScope = s_pCurrentList;

  xiiTime now = xiiTime::Now();
  xiiProfilingSystem::AddCPUScope(pCurScope->m_sCurSectionName, nullptr, pCurScope->m_CurSectionBeginTime, now, xiiTime::MakeZero());

  pCurScope->m_sCurSectionName     = sNextSectionName;
  pCurScope->m_CurSectionBeginTime = now;
}

#else

xiiResult xiiProfilingSystem::ProfilingData::Write(xiiStreamWriter& outputStream) const
{
  return XII_FAILURE;
}

void xiiProfilingSystem::Clear() {}

void xiiProfilingSystem::Capture(xiiProfilingSystem::ProfilingData& out_Capture, bool bClearAfterCapture) {}

void xiiProfilingSystem::SetDiscardThreshold(xiiTime threshold) {}

void xiiProfilingSystem::StartNewFrame() {}

void xiiProfilingSystem::AddCPUScope(xiiStringView sName, xiiStringView sFunctionName, xiiTime beginTime, xiiTime endTime, xiiTime scopeTimeout) {}

void xiiProfilingSystem::Initialize() {}

void xiiProfilingSystem::Reset() {}

void xiiProfilingSystem::SetThreadName(xiiStringView sThreadName) {}

void xiiProfilingSystem::RemoveThread() {}

void xiiProfilingSystem::InitializeGPUData(xiiUInt32 uiGpuCount) {}

void xiiProfilingSystem::AddGPUScope(xiiStringView sName, xiiTime beginTime, xiiTime endTime, xiiUInt32 gpuIndex) {}

void xiiProfilingSystem::ProfilingData::Merge(ProfilingData& out_Merged, xiiArrayPtr<const ProfilingData*> inputs) {}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);
