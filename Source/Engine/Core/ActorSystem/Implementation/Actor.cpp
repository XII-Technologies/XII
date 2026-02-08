#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

struct xiiActorImpl
{
  xiiString                                       m_sName;
  const void*                                     m_pCreatedBy = nullptr;
  xiiHybridArray<xiiUniquePtr<xiiActorPlugin>, 4> m_AllPlugins;
  xiiMap<const xiiRTTI*, xiiActorPlugin*>         m_PluginLookupCache;
};

xiiActor::xiiActor(xiiStringView sActorName, const void* pCreatedBy)
{
  m_pImpl = XII_DEFAULT_NEW(xiiActorImpl);

  m_pImpl->m_sName      = sActorName;
  m_pImpl->m_pCreatedBy = pCreatedBy;

  XII_ASSERT_DEV(!m_pImpl->m_sName.IsEmpty(), "Actor name must not be empty");
}

xiiActor::~xiiActor() = default;

xiiStringView xiiActor::GetName() const
{
  return m_pImpl->m_sName;
}

const void* xiiActor::GetCreatedBy() const
{
  return m_pImpl->m_pCreatedBy;
}

void xiiActor::AddPlugin(xiiUniquePtr<xiiActorPlugin>&& pPlugin)
{
  XII_ASSERT_DEV(pPlugin != nullptr, "Invalid actor plugin");
  XII_ASSERT_DEV(pPlugin->m_pOwningActor == nullptr, "Actor plugin already in use");

  pPlugin->m_pOwningActor = this;

  // register this plugin under its type and all its base types
  for (const xiiRTTI* pRtti = pPlugin->GetDynamicRTTI(); pRtti != xiiGetStaticRTTI<xiiActorPlugin>(); pRtti = pRtti->GetParentType())
  {
    m_pImpl->m_PluginLookupCache[pRtti] = pPlugin.Borrow();
  }

  m_pImpl->m_AllPlugins.PushBack(std::move(pPlugin));
}

xiiActorPlugin* xiiActor::GetPlugin(const xiiRTTI* pPluginType) const
{
  XII_ASSERT_DEV(pPluginType->IsDerivedFrom<xiiActorPlugin>(), "The queried type has to derive from xiiActorPlugin");

  return m_pImpl->m_PluginLookupCache.GetValueOrDefault(pPluginType, nullptr);
}

void xiiActor::DestroyPlugin(xiiActorPlugin* pPlugin)
{
  for (xiiUInt32 i = 0; i < m_pImpl->m_AllPlugins.GetCount(); ++i)
  {
    if (m_pImpl->m_AllPlugins[i] == pPlugin)
    {
      m_pImpl->m_AllPlugins.RemoveAtAndSwap(i);
      break;
    }
  }
}

void xiiActor::GetAllPlugins(xiiHybridArray<xiiActorPlugin*, 8>& out_allPlugins)
{
  out_allPlugins.Clear();

  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    out_allPlugins.PushBack(pPlugin.Borrow());
  }
}

void xiiActor::UpdateAllPlugins()
{
  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    pPlugin->Update();
  }
}

void xiiActor::Activate() {}

void xiiActor::Update()
{
  UpdateAllPlugins();
}


XII_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_Actor);
