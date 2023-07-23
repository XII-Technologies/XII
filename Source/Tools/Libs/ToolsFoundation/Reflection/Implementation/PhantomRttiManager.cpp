#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

xiiCopyOnBroadcastEvent<const xiiPhantomRttiManagerEvent&> xiiPhantomRttiManager::s_Events;

xiiHashTable<xiiStringView, xiiPhantomRTTI*> xiiPhantomRttiManager::s_NameToPhantom;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPhantomRttiManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPhantomRttiManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiPhantomRttiManager public functions
////////////////////////////////////////////////////////////////////////

const xiiRTTI* xiiPhantomRttiManager::RegisterType(xiiReflectedTypeDescriptor& ref_desc)
{
  XII_PROFILE_SCOPE("RegisterType");
  const xiiRTTI*  pType    = xiiRTTI::FindTypeByName(ref_desc.m_sTypeName);
  xiiPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(ref_desc.m_sTypeName, pPhantom);

  // concrete type !
  if (pPhantom == nullptr && pType != nullptr)
  {
    return pType;
  }

  if (pPhantom != nullptr && pPhantom->IsEqualToDescriptor(ref_desc))
    return pPhantom;

  if (pPhantom == nullptr)
  {
    pPhantom = XII_DEFAULT_NEW(xiiPhantomRTTI, ref_desc.m_sTypeName.GetData(), xiiRTTI::FindTypeByName(ref_desc.m_sParentTypeName), 0,
                               ref_desc.m_uiTypeVersion, xiiVariantType::Invalid, ref_desc.m_Flags, ref_desc.m_sPluginName.GetData());

    pPhantom->SetProperties(ref_desc.m_Properties);
    pPhantom->SetAttributes(ref_desc.m_Attributes);
    pPhantom->SetFunctions(ref_desc.m_Functions);
    pPhantom->SetupParentHierarchy();

    s_NameToPhantom[pPhantom->GetTypeName()] = pPhantom;

    xiiPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type         = xiiPhantomRttiManagerEvent::Type::TypeAdded;
    s_Events.Broadcast(msg, 1); /// \todo Had to increase the recursion depth to allow registering phantom types that are based on actual
                                /// types coming from the engine process
  }
  else
  {
    pPhantom->UpdateType(ref_desc);

    xiiPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type         = xiiPhantomRttiManagerEvent::Type::TypeChanged;
    s_Events.Broadcast(msg, 1);
  }

  return pPhantom;
}

bool xiiPhantomRttiManager::UnregisterType(const xiiRTTI* pRtti)
{
  xiiPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(pRtti->GetTypeName(), pPhantom);

  if (pPhantom == nullptr)
    return false;

  {
    xiiPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type         = xiiPhantomRttiManagerEvent::Type::TypeRemoved;
    s_Events.Broadcast(msg);
  }

  s_NameToPhantom.Remove(pPhantom->GetTypeName());

  XII_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// xiiPhantomRttiManager private functions
////////////////////////////////////////////////////////////////////////

void xiiPhantomRttiManager::PluginEventHandler(const xiiPluginEvent& e)
{
  if (e.m_EventType == xiiPluginEvent::Type::BeforeUnloading)
  {
    while (!s_NameToPhantom.IsEmpty())
    {
      UnregisterType(s_NameToPhantom.GetIterator().Value());
    }

    XII_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "xiiPhantomRttiManager::Shutdown: Removal of types failed!");
  }
}

void xiiPhantomRttiManager::Startup()
{
  xiiPlugin::Events().AddEventHandler(&xiiPhantomRttiManager::PluginEventHandler);
}


void xiiPhantomRttiManager::Shutdown()
{
  xiiPlugin::Events().RemoveEventHandler(&xiiPhantomRttiManager::PluginEventHandler);

  while (!s_NameToPhantom.IsEmpty())
  {
    UnregisterType(s_NameToPhantom.GetIterator().Value());
  }

  XII_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "xiiPhantomRttiManager::Shutdown: Removal of types failed!");
}
