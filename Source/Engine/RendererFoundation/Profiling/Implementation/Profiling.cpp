#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

#if XII_ENABLED(XII_USE_PROFILING)

struct GPUTimingScope
{
  XII_DECLARE_POD_TYPE();

  xiiGALTimestampHandle m_BeginTimestamp;
  xiiGALTimestampHandle m_EndTimestamp;
  char                  m_szName[48];
};

class GPUProfilingSystem
{
public:
  static void ProcessTimestamps(const xiiGALDeviceEvent& e)
  {
    if (e.m_Type != xiiGALDeviceEvent::AfterEndFrame)
      return;

    while (!s_TimingScopes.IsEmpty())
    {
      auto& timingScope = s_TimingScopes.PeekFront();

      xiiTime endTime;
      if (e.m_pDevice->GetTimestampResult(timingScope.m_EndTimestamp, endTime).Succeeded())
      {
        xiiTime beginTime;
        XII_VERIFY(e.m_pDevice->GetTimestampResult(timingScope.m_BeginTimestamp, beginTime).Succeeded(),
                   "Begin timestamp should be finished before end timestamp");

        if (!beginTime.IsZero() && !endTime.IsZero())
        {
#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
          static bool warnOnRingBufferOverun = true;
          if (warnOnRingBufferOverun && endTime < beginTime)
          {
            warnOnRingBufferOverun = false;
            xiiLog::Error("Profiling end is before start, the timestamp ring buffer was probably overrun.");
          }
#  endif
          xiiProfilingSystem::AddGPUScope(timingScope.m_szName, beginTime, endTime);
        }

        s_TimingScopes.PopFront();
      }
      else
      {
        // Timestamps are not available yet
        break;
      }
    }
  }

  static GPUTimingScope& AllocateScope() { return s_TimingScopes.ExpandAndGetRef(); }

private:
  static void OnEngineStartup() { xiiGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(&GPUProfilingSystem::ProcessTimestamps); }

  static void OnEngineShutdown()
  {
    s_TimingScopes.Clear();
    xiiGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static xiiDeque<GPUTimingScope, xiiStaticAllocatorWrapper> s_TimingScopes;

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, GPUProfilingSystem);
};

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, GPUProfilingSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    GPUProfilingSystem::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    GPUProfilingSystem::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiDeque<GPUTimingScope, xiiStaticAllocatorWrapper> GPUProfilingSystem::s_TimingScopes;

//////////////////////////////////////////////////////////////////////////

GPUTimingScope* xiiProfilingScopeAndMarker::Start(xiiGALCommandEncoder* pCommandEncoder, const char* szName)
{
  pCommandEncoder->PushMarker(szName);

  auto& timingScope            = GPUProfilingSystem::AllocateScope();
  timingScope.m_BeginTimestamp = pCommandEncoder->InsertTimestamp();
  xiiStringUtils::Copy(timingScope.m_szName, XII_ARRAY_SIZE(timingScope.m_szName), szName);

  return &timingScope;
}

void xiiProfilingScopeAndMarker::Stop(xiiGALCommandEncoder* pCommandEncoder, GPUTimingScope*& pTimingScope)
{
  pCommandEncoder->PopMarker();
  pTimingScope->m_EndTimestamp = pCommandEncoder->InsertTimestamp();
  pTimingScope                 = nullptr;
}

xiiProfilingScopeAndMarker::xiiProfilingScopeAndMarker(xiiGALCommandEncoder* pCommandEncoder, const char* szName) :
  xiiProfilingScope(szName, nullptr, xiiTime::Zero()), m_pCommandEncoder(pCommandEncoder)
{
  m_pTimingScope = Start(pCommandEncoder, szName);
}

xiiProfilingScopeAndMarker::~xiiProfilingScopeAndMarker()
{
  Stop(m_pCommandEncoder, m_pTimingScope);
}

#endif

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_Profiling);
