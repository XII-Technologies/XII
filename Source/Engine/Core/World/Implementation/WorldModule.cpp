/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/World/World.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiWorldModule::xiiWorldModule(xiiWorld* pWorld) :
  m_pWorld(pWorld)
{
}

xiiWorldModule::~xiiWorldModule() = default;

xiiUInt32 xiiWorldModule::GetWorldIndex() const
{
  return GetWorld()->GetIndex();
}

// protected methods

void xiiWorldModule::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void xiiWorldModule::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

xiiAllocator* xiiWorldModule::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

xiiInternal::WorldLargeBlockAllocator* xiiWorldModule::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

bool xiiWorldModule::GetWorldSimulationEnabled() const
{
  return m_pWorld->GetWorldSimulationEnabled();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, WorldModuleFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiWorldModuleFactory::PluginEventHandler);
    xiiWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiWorldModuleFactory::PluginEventHandler);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

static xiiWorldModuleTypeId                  s_uiNextTypeId = 0;
static xiiDynamicArray<xiiWorldModuleTypeId> s_freeTypeIds;
static constexpr xiiWorldModuleTypeId        s_InvalidWorldModuleTypeId = xiiWorldModuleTypeId(-1);

xiiWorldModuleFactory::xiiWorldModuleFactory() = default;

// static
xiiWorldModuleFactory* xiiWorldModuleFactory::GetInstance()
{
  static xiiWorldModuleFactory* pInstance = new xiiWorldModuleFactory();
  return pInstance;
}

xiiWorldModuleTypeId xiiWorldModuleFactory::GetTypeId(const xiiRTTI* pRtti)
{
  xiiWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  m_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

xiiWorldModule* xiiWorldModuleFactory::CreateWorldModule(xiiWorldModuleTypeId typeId, xiiWorld* pWorld)
{
  if (typeId < m_CreatorFuncs.GetCount())
  {
    CreatorFunc func = m_CreatorFuncs[typeId].m_Func;
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

void xiiWorldModuleFactory::RegisterInterfaceImplementation(xiiStringView sInterfaceName, xiiStringView sImplementationName)
{
  m_InterfaceImplementations.Insert(sInterfaceName, sImplementationName);

  xiiStringBuilder sTemp          = sInterfaceName;
  const xiiRTTI*   pInterfaceRtti = xiiRTTI::FindTypeByName(sTemp);

  sTemp                              = sImplementationName;
  const xiiRTTI* pImplementationRtti = xiiRTTI::FindTypeByName(sTemp);

  if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
  {
    m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    return;
  }

  // Clear existing mapping if it maps to the wrong type
  xiiUInt16 uiTypeId;
  if (pInterfaceRtti != nullptr && m_TypeToId.TryGetValue(pInterfaceRtti, uiTypeId))
  {
    if (m_CreatorFuncs[uiTypeId].m_pRtti->GetTypeName() != sImplementationName)
    {
      XII_ASSERT_DEV(pImplementationRtti == nullptr, "Implementation error");
      m_TypeToId.Remove(pInterfaceRtti);
    }
  }
}
xiiWorldModuleTypeId xiiWorldModuleFactory::RegisterWorldModule(const xiiRTTI* pRtti, CreatorFunc creatorFunc)
{
  XII_ASSERT_DEV(pRtti != xiiGetStaticRTTI<xiiWorldModule>(), "Trying to register a world module that is not reflected!");
  XII_ASSERT_DEV(m_TypeToId.GetCount() < xiiWorld::GetMaxNumWorldModules(), "Max number of world modules reached: {}", xiiWorld::GetMaxNumWorldModules());

  xiiWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  if (m_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  if (s_freeTypeIds.IsEmpty())
  {
    XII_ASSERT_DEV(s_uiNextTypeId < XII_MAX_WORLD_MODULE_TYPES - 1, "World module id overflow!");

    uiTypeId = s_uiNextTypeId++;
  }
  else
  {
    uiTypeId = s_freeTypeIds.PeekBack();
    s_freeTypeIds.PopBack();
  }

  m_TypeToId.Insert(pRtti, uiTypeId);

  m_CreatorFuncs.EnsureCount(uiTypeId + 1);

  auto& creatorFuncContext   = m_CreatorFuncs[uiTypeId];
  creatorFuncContext.m_Func  = creatorFunc;
  creatorFuncContext.m_pRtti = pRtti;

  return uiTypeId;
}

// static
void xiiWorldModuleFactory::PluginEventHandler(const xiiPluginEvent& EventData)
{
  if (EventData.m_EventType == xiiPluginEvent::AfterLoadingBeforeInit)
  {
    xiiWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  if (EventData.m_EventType == xiiPluginEvent::AfterUnloading)
  {
    xiiWorldModuleFactory::GetInstance()->ClearUnloadedTypeToIDs();
  }
}

namespace
{
  struct NewEntry
  {
    XII_DECLARE_POD_TYPE();

    const xiiRTTI*       m_pRtti;
    xiiWorldModuleTypeId m_uiTypeId;
  };
} // namespace

void xiiWorldModuleFactory::AdjustBaseTypeId(const xiiRTTI* pParentRtti, const xiiRTTI* pRtti, xiiUInt16 uiParentTypeId)
{
  xiiDynamicArray<xiiPlugin::PluginInfo> infos;
  xiiPlugin::GetAllPluginInfos(infos);

  auto HasManualDependency = [&](xiiStringView sPluginName) -> bool {
    for (const auto& p : infos)
    {
      if (p.m_sName == sPluginName)
      {
        return !p.m_LoadFlags.IsSet(xiiPluginLoadFlags::CustomDependency);
      }
    }

    return false;
  };

  xiiStringView szPlugin1 = m_CreatorFuncs[uiParentTypeId].m_pRtti->GetPluginName();
  xiiStringView szPlugin2 = pRtti->GetPluginName();

  const bool bPrio1 = HasManualDependency(szPlugin1);
  const bool bPrio2 = HasManualDependency(szPlugin2);

  if (bPrio1 && !bPrio2)
  {
    // keep the previous one
    return;
  }

  if (!bPrio1 && bPrio2)
  {
    // take the new one
    m_TypeToId[pParentRtti] = m_TypeToId[pRtti];
    return;
  }

  xiiLog::Error("Interface '{}' is already implemented by '{}'. Specify which implementation should be used via RegisterInterfaceImplementation() or WorldModules.ddl config file.", pParentRtti->GetTypeName(), m_CreatorFuncs[uiParentTypeId].m_pRtti->GetTypeName());
}

void xiiWorldModuleFactory::FillBaseTypeIds()
{
  // m_TypeToId contains RTTI types for xiiWorldModules and xiiComponents
  // m_TypeToId[xiiComponent] maps to TypeID for its respective xiiComponentManager
  // m_TypeToId[xiiWorldModule] maps to TypeID for itself OR in case of an interface to the derived type that implements the interface
  // after types are registered we only have a mapping for m_TypeToId[xiiWorldModule(impl)] and now we want to add
  // the mapping for m_TypeToId[xiiWorldModule(interface)], such that querying the TypeID for the interface works as well
  // and yields the implementation

  xiiTemporaryHybridArray<NewEntry, 64> newEntries;
  const xiiRTTI*                        pModuleRtti = xiiGetStaticRTTI<xiiWorldModule>(); // base type where we want to stop iterating upwards

  // explicit mappings
  for (auto it = m_InterfaceImplementations.GetIterator(); it.IsValid(); ++it)
  {
    const xiiRTTI* pInterfaceRtti      = xiiRTTI::FindTypeByName(it.Key());
    const xiiRTTI* pImplementationRtti = xiiRTTI::FindTypeByName(it.Value());

    if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
    {
      m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    }
  }

  // automatic mappings
  for (auto it = m_TypeToId.GetIterator(); it.IsValid(); ++it)
  {
    const xiiRTTI* pRtti = it.Key();

    // ignore components, we only want to fill out mappings for the base types of world modules
    if (!pRtti->IsDerivedFrom<xiiWorldModule>())
      continue;

    const xiiWorldModuleTypeId uiTypeId = it.Value();

    for (const xiiRTTI* pParentRtti = pRtti->GetParentType(); pParentRtti != pModuleRtti; pParentRtti = pParentRtti->GetParentType())
    {
      // we are only interested in parent types that are pure interfaces
      if (!pParentRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
        continue;

      // skip if we have an explicit mapping for this interface, they are already handled above
      if (m_InterfaceImplementations.GetValue(pParentRtti->GetTypeName()) != nullptr)
        continue;


      if (xiiUInt16* pParentTypeId = m_TypeToId.GetValue(pParentRtti))
      {
        if (*pParentTypeId != uiTypeId)
        {
          AdjustBaseTypeId(pParentRtti, pRtti, *pParentTypeId);
        }
      }
      else
      {
        auto& newEntry      = newEntries.ExpandAndGetRef();
        newEntry.m_pRtti    = pParentRtti;
        newEntry.m_uiTypeId = uiTypeId;
      }
    }
  }

  // delayed insertion to not interfere with the iteration above
  for (auto& newEntry : newEntries)
  {
    m_TypeToId.Insert(newEntry.m_pRtti, newEntry.m_uiTypeId);
  }
}

void xiiWorldModuleFactory::ClearUnloadedTypeToIDs()
{
  xiiSet<const xiiRTTI*> allRttis;
  xiiRTTI::ForEachType([&](const xiiRTTI* pRtti) { allRttis.Insert(pRtti); });

  xiiSet<xiiWorldModuleTypeId> mappedIdsToRemove;

  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const xiiRTTI*             pRtti    = it.Key();
    const xiiWorldModuleTypeId uiTypeId = it.Value();

    if (!allRttis.Contains(pRtti))
    {
      // type got removed, clear it from the map
      it = m_TypeToId.Remove(it);

      // and record that all other types that map to the same typeId also must be removed
      mappedIdsToRemove.Insert(uiTypeId);
    }
    else
    {
      ++it;
    }
  }

  // now remove all mappings that map to an invalid typeId
  // this can be more than one, since we can map multiple (interface) types to the same implementation
  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const xiiWorldModuleTypeId uiTypeId = it.Value();

    if (mappedIdsToRemove.Contains(uiTypeId))
    {
      it = m_TypeToId.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  // Finally, adding all invalid typeIds to the free list for reusing later
  for (xiiWorldModuleTypeId removedId : mappedIdsToRemove)
  {
    s_freeTypeIds.PushBack(removedId);
  }
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_WorldModule);
