#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/Log.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiGlobalEvent);

xiiGlobalEvent::EventMap xiiGlobalEvent::s_KnownEvents;

xiiGlobalEvent::EventData::EventData()
{
  m_uiNumTimesFired           = 0;
  m_uiNumEventHandlersOnce    = 0;
  m_uiNumEventHandlersRegular = 0;
}

xiiGlobalEvent::xiiGlobalEvent(const char* szEventName, XII_GLOBAL_EVENT_HANDLER Handler, bool bOnlyOnce)
{
  m_szEventName   = szEventName;
  m_bOnlyOnce     = bOnlyOnce;
  m_bHasBeenFired = false;
  m_EventHandler  = Handler;
}

void xiiGlobalEvent::Broadcast(const char* szEventName, xiiVariant p1, xiiVariant p2, xiiVariant p3, xiiVariant p4)
{
  xiiGlobalEvent* pHandler = xiiGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    if (xiiStringUtils::IsEqual(pHandler->m_szEventName, szEventName))
    {
      if (!pHandler->m_bOnlyOnce || !pHandler->m_bHasBeenFired)
      {
        pHandler->m_bHasBeenFired = true;

        pHandler->m_EventHandler(p1, p2, p3, p4);
      }
    }

    pHandler = pHandler->GetNextInstance();
  }


  EventData& ed = s_KnownEvents[szEventName]; // this will make sure to record all fired events, even if there are no handlers for them
  ed.m_uiNumTimesFired++;
}

void xiiGlobalEvent::UpdateGlobalEventStatistics()
{
  for (EventMap::Iterator it = s_KnownEvents.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_uiNumEventHandlersRegular = 0;
    it.Value().m_uiNumEventHandlersOnce    = 0;
  }

  xiiGlobalEvent* pHandler = xiiGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    EventData& ed = s_KnownEvents[pHandler->m_szEventName];

    if (pHandler->m_bOnlyOnce)
      ++ed.m_uiNumEventHandlersOnce;
    else
      ++ed.m_uiNumEventHandlersRegular;

    pHandler = pHandler->GetNextInstance();
  }
}

void xiiGlobalEvent::PrintGlobalEventStatistics()
{
  UpdateGlobalEventStatistics();

  XII_LOG_BLOCK("Global Event Statistics");

  EventMap::Iterator it = s_KnownEvents.GetIterator();

  while (it.IsValid())
  {
    xiiLog::Info("Event: '{0}', Num Handlers Regular / Once: {1} / {2}, Num Times Fired: {3}", it.Key(), it.Value().m_uiNumEventHandlersRegular,
                 it.Value().m_uiNumEventHandlersOnce, it.Value().m_uiNumTimesFired);

    ++it;
  }
}



XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_GlobalEvent);
