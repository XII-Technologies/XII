#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/Thread.h>

class xiiTelemetryThread : public xiiThread
{
public:
  xiiTelemetryThread() :
    xiiThread("xiiTelemetryThread")
  {
    m_bKeepRunning = true;
  }

  volatile bool m_bKeepRunning;

private:
  virtual xiiUInt32 Run()
  {
    xiiTime LastPing;

    while (m_bKeepRunning)
    {
      xiiTelemetry::UpdateNetwork();

      // Send a Ping every once in a while
      if (xiiTelemetry::s_ConnectionMode == xiiTelemetry::Client)
      {
        xiiTime tNow = xiiTime::Now();

        if (tNow - LastPing > xiiTime::MakeFromMilliseconds(500))
        {
          LastPing = tNow;

          xiiTelemetry::UpdateServerPing();
        }
      }

      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }

    return 0;
  }
};

static xiiTelemetryThread* g_pBroadcastThread = nullptr;
xiiMutex                   xiiTelemetry::s_TelemetryMutex;


xiiMutex& xiiTelemetry::GetTelemetryMutex()
{
  return s_TelemetryMutex;
}

void xiiTelemetry::StartTelemetryThread()
{
  if (!g_pBroadcastThread)
  {
    g_pBroadcastThread = XII_DEFAULT_NEW(xiiTelemetryThread);
    g_pBroadcastThread->Start();
  }
}

void xiiTelemetry::StopTelemetryThread()
{
  if (g_pBroadcastThread)
  {
    g_pBroadcastThread->m_bKeepRunning = false;
    g_pBroadcastThread->Join();

    XII_DEFAULT_DELETE(g_pBroadcastThread);
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_TelemetryThread);
