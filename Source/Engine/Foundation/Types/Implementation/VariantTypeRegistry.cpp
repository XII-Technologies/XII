#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/VariantTypeRegistry.h>

XII_IMPLEMENT_SINGLETON(xiiVariantTypeRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, VariantTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiVariantTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiVariantTypeRegistry * pDummy = xiiVariantTypeRegistry::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiVariantTypeRegistry::xiiVariantTypeRegistry() :
  m_SingletonRegistrar(this)
{
  xiiPlugin::Events().AddEventHandler(xiiMakeDelegate(&xiiVariantTypeRegistry::PluginEventHandler, this));

  UpdateTypes();
}

xiiVariantTypeRegistry::~xiiVariantTypeRegistry()
{
  xiiPlugin::Events().RemoveEventHandler(xiiMakeDelegate(&xiiVariantTypeRegistry::PluginEventHandler, this));
}

const xiiVariantTypeInfo* xiiVariantTypeRegistry::FindVariantTypeInfo(const xiiRTTI* pType) const
{
  const xiiVariantTypeInfo* pTypeInfo = nullptr;
  m_TypeInfos.TryGetValue(pType, pTypeInfo);
  return pTypeInfo;
}

void xiiVariantTypeRegistry::PluginEventHandler(const xiiPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case xiiPluginEvent::AfterLoadingBeforeInit:
    case xiiPluginEvent::AfterUnloading:
      UpdateTypes();
      break;
    default:
      break;
  }
}

void xiiVariantTypeRegistry::UpdateTypes()
{
  m_TypeInfos.Clear();
  xiiVariantTypeInfo* pInstance = xiiVariantTypeInfo::GetFirstInstance();

  while (pInstance)
  {
    XII_ASSERT_DEV(pInstance->GetType()->GetAllocator()->CanAllocate(), "Custom type '{0}' needs to be allocatable.", pInstance->GetType()->GetTypeName());

    m_TypeInfos.Insert(pInstance->GetType(), pInstance);
    pInstance = pInstance->GetNextInstance();
  }
}

//////////////////////////////////////////////////////////////////////////

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiVariantTypeInfo);

xiiVariantTypeInfo::xiiVariantTypeInfo() = default;


XII_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VariantTypeRegistry);
