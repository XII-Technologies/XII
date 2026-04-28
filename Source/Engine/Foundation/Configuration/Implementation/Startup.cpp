/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiSubSystem);

bool                           xiiStartup::s_bPrintAllSubSystems = true;
xiiStartupStage::Enum          xiiStartup::s_CurrentState        = xiiStartupStage::None;
xiiDynamicArray<xiiStringView> xiiStartup::s_ApplicationTags;


void xiiStartup::AddApplicationTag(xiiStringView sTag)
{
  s_ApplicationTags.PushBack(sTag);
}

bool xiiStartup::HasApplicationTag(xiiStringView sTag)
{
  for (xiiUInt32 i = 0; i < s_ApplicationTags.GetCount(); ++i)
  {
    if (s_ApplicationTags[i].IsEqual_NoCase(sTag))
      return true;
  }

  return false;
}

void xiiStartup::PrintAllSubsystems()
{
  XII_LOG_BLOCK("Available Subsystems");

  xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

  while (pSub)
  {
    xiiLog::Debug("Subsystem: '{0}::{1}'", pSub->GetGroupName(), pSub->GetSubSystemName());

    if (pSub->GetDependency(0) == nullptr)
    {
      xiiLog::Debug("  <no dependencies>");
    }
    else
    {
      for (xiiInt32 i = 0; pSub->GetDependency(i) != nullptr; ++i)
      {
        xiiLog::Debug("  depends on '{0}'", pSub->GetDependency(i));
      }
    }

    xiiLog::Debug("");

    pSub = pSub->GetNextInstance();
  }
}

void xiiStartup::AssignSubSystemPlugin(xiiStringView sPluginName)
{
  // iterates over all existing subsystems and finds those that have no plugin name yet
  // assigns the given name to them

  xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->m_sPluginName.IsEmpty())
    {
      pSub->m_sPluginName = sPluginName;
    }

    pSub = pSub->GetNextInstance();
  }
}

void xiiStartup::PluginEventHandler(const xiiPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case xiiPluginEvent::BeforeLoading:
    {
      AssignSubSystemPlugin("Static");
    }
    break;

    case xiiPluginEvent::AfterLoadingBeforeInit:
    {
      AssignSubSystemPlugin(EventData.m_sPluginBinary);
    }
    break;

    case xiiPluginEvent::StartupShutdown:
    {
      xiiStartup::UnloadPluginSubSystems(EventData.m_sPluginBinary);
    }
    break;

    case xiiPluginEvent::AfterPluginChanges:
    {
      xiiStartup::ReinitToCurrentState();
    }
    break;

    default:
      break;
  }
}

static bool IsGroupName(xiiStringView sName)
{
  xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

  bool bGroup     = false;
  bool bSubSystem = false;

  while (pSub)
  {
    if (pSub->GetGroupName() == sName)
      bGroup = true;

    if (pSub->GetSubSystemName() == sName)
      bSubSystem = true;

    pSub = pSub->GetNextInstance();
  }

  XII_ASSERT_ALWAYS(!bGroup || !bSubSystem, "There cannot be a SubSystem AND a Group called '{0}'.", sName);

  return bGroup;
}

static xiiStringView GetGroupSubSystems(xiiStringView sGroup, xiiInt32 iSubSystem)
{
  xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->GetGroupName() == sGroup)
    {
      if (iSubSystem == 0)
        return pSub->GetSubSystemName();

      --iSubSystem;
    }

    pSub = pSub->GetNextInstance();
  }

  return nullptr;
}

void xiiStartup::ComputeOrder(xiiDeque<xiiSubSystem*>& Order)
{
  Order.Clear();
  xiiSet<xiiString> sSystemsInited;

  bool bCouldInitAny = true;

  while (bCouldInitAny)
  {
    bCouldInitAny = false;

    xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!sSystemsInited.Find(pSub->GetSubSystemName()).IsValid())
      {
        bool     bAllDependsFulfilled = true;
        xiiInt32 iDep                 = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (IsGroupName(pSub->GetDependency(iDep)))
          {
            xiiInt32      iSubSystemIndex = 0;
            xiiStringView sNextSubSystem  = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            while (sNextSubSystem.IsValid())
            {
              if (!sSystemsInited.Find(sNextSubSystem).IsValid())
              {
                bAllDependsFulfilled = false;
                break;
              }

              ++iSubSystemIndex;
              sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            }
          }
          else
          {
            if (!sSystemsInited.Find(pSub->GetDependency(iDep)).IsValid())
            {
              bAllDependsFulfilled = false;
              break;
            }
          }

          ++iDep;
        }

        if (bAllDependsFulfilled)
        {
          bCouldInitAny = true;
          Order.PushBack(pSub);
          sSystemsInited.Insert(pSub->GetSubSystemName());
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }
}

void xiiStartup::Startup(xiiStartupStage::Enum stage)
{
  if (stage == xiiStartupStage::BaseSystems)
  {
    xiiFoundation::Initialize();
  }

  const char* szStartup[] = {"Startup Base", "Startup Core", "Startup Engine"};

  if (stage == xiiStartupStage::CoreSystems)
  {
    Startup(xiiStartupStage::BaseSystems);

    xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_STARTUP_CORESYSTEMS_BEGIN);

    if (s_bPrintAllSubSystems)
    {
      s_bPrintAllSubSystems = false;
      PrintAllSubsystems();
    }
  }

  if (stage == xiiStartupStage::HighLevelSystems)
  {
    Startup(xiiStartupStage::CoreSystems);

    xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_BEGIN);
  }

  XII_LOG_BLOCK(szStartup[stage]);

  xiiDeque<xiiSubSystem*> Order;
  ComputeOrder(Order);

  for (xiiUInt32 i = 0; i < Order.GetCount(); ++i)
  {
    if (!Order[i]->m_bStartupDone[stage])
    {
      Order[i]->m_bStartupDone[stage] = true;

      switch (stage)
      {
        case xiiStartupStage::BaseSystems:
          xiiLog::Debug("Executing 'Base' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnBaseSystemsStartup();
          break;
        case xiiStartupStage::CoreSystems:
          xiiLog::Debug("Executing 'Core' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnCoreSystemsStartup();
          break;
        case xiiStartupStage::HighLevelSystems:
          xiiLog::Debug("Executing 'Engine' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnHighLevelSystemsStartup();
          break;

        default:
          break;
      }
    }
  }

  // now everything should be started
  {
    XII_LOG_BLOCK("Failed SubSystems");

    xiiSet<xiiString> sSystemsFound;

    xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

    while (pSub)
    {
      sSystemsFound.Insert(pSub->GetSubSystemName());
      pSub = pSub->GetNextInstance();
    }

    pSub = xiiSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!pSub->m_bStartupDone[stage])
      {
        xiiInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (!sSystemsFound.Find(pSub->GetDependency(iDep)).IsValid())
          {
            xiiLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' is unknown.", pSub->GetGroupName(), pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }
          else
          {
            xiiLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' has not been initialized.", pSub->GetGroupName(), pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }

          ++iDep;
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }

  switch (stage)
  {
    case xiiStartupStage::BaseSystems:
      break;
    case xiiStartupStage::CoreSystems:
      xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_STARTUP_CORESYSTEMS_END);
      break;
    case xiiStartupStage::HighLevelSystems:
      xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState == xiiStartupStage::None)
  {
    xiiPlugin::Events().AddEventHandler(PluginEventHandler);
  }

  s_CurrentState = stage;
}

void xiiStartup::Shutdown(xiiStartupStage::Enum stage)
{
  // without that we cannot function, so make sure it is up and running
  xiiFoundation::Initialize();

  {
    const char* szStartup[] = {"Shutdown Base", "Shutdown Core", "Shutdown Engine"};

    if (stage == xiiStartupStage::BaseSystems)
    {
      Shutdown(xiiStartupStage::CoreSystems);
    }

    if (stage == xiiStartupStage::CoreSystems)
    {
      Shutdown(xiiStartupStage::HighLevelSystems);
      s_bPrintAllSubSystems = true;

      xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_BEGIN);
    }

    if (stage == xiiStartupStage::HighLevelSystems)
    {
      xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_BEGIN);
    }

    XII_LOG_BLOCK(szStartup[stage]);

    xiiDeque<xiiSubSystem*> Order;
    ComputeOrder(Order);

    for (xiiInt32 i = (xiiInt32)Order.GetCount() - 1; i >= 0; --i)
    {
      if (Order[i]->m_bStartupDone[stage])
      {
        switch (stage)
        {
          case xiiStartupStage::CoreSystems:
            xiiLog::Debug("Executing 'Core' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnCoreSystemsShutdown();
            break;

          case xiiStartupStage::HighLevelSystems:
            xiiLog::Debug("Executing 'Engine' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnHighLevelSystemsShutdown();
            break;

          default:
            break;
        }

        Order[i]->m_bStartupDone[stage] = false;
      }
    }
  }

  switch (stage)
  {
    case xiiStartupStage::CoreSystems:
      xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_END);
      break;

    case xiiStartupStage::HighLevelSystems:
      xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState != xiiStartupStage::None)
  {
    s_CurrentState = (xiiStartupStage::Enum)(((xiiInt32)stage) - 1);

    if (s_CurrentState == xiiStartupStage::None)
    {
      xiiPlugin::Events().RemoveEventHandler(PluginEventHandler);
    }
  }
}

bool xiiStartup::HasDependencyOnPlugin(xiiSubSystem* pSubSystem, xiiStringView sModule)
{
  if (pSubSystem->m_sPluginName == sModule)
    return true;

  for (xiiUInt32 i = 0; pSubSystem->GetDependency(i) != nullptr; ++i)
  {
    xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();
    while (pSub)
    {
      if (pSub->GetSubSystemName() == pSubSystem->GetDependency(i))
      {
        if (HasDependencyOnPlugin(pSub, sModule))
          return true;

        break;
      }

      pSub = pSub->GetNextInstance();
    }
  }

  return false;
}

void xiiStartup::UnloadPluginSubSystems(xiiStringView sPluginName)
{
  XII_LOG_BLOCK("Unloading Plugin SubSystems", sPluginName);
  xiiLog::Dev("Plugin to unload: '{0}'", sPluginName);

  xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN, xiiVariant(sPluginName));

  xiiDeque<xiiSubSystem*> Order;
  ComputeOrder(Order);

  for (xiiInt32 i = (xiiInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[xiiStartupStage::HighLevelSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      xiiLog::Info("Engine shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnHighLevelSystemsShutdown();
      Order[i]->m_bStartupDone[xiiStartupStage::HighLevelSystems] = false;
    }
  }

  for (xiiInt32 i = (xiiInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[xiiStartupStage::CoreSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      xiiLog::Info("Core shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnCoreSystemsShutdown();
      Order[i]->m_bStartupDone[xiiStartupStage::CoreSystems] = false;
    }
  }

  xiiGlobalEvent::Broadcast(XII_GLOBALEVENT_UNLOAD_PLUGIN_END, xiiVariant(sPluginName));
}

void xiiStartup::ReinitToCurrentState()
{
  if (s_CurrentState != xiiStartupStage::None)
    Startup(s_CurrentState);
}

XII_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Startup);
