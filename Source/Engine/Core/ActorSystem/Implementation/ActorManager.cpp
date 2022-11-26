#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorApiService.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/UniquePtr.h>

//////////////////////////////////////////////////////////////////////////

static xiiUniquePtr<xiiActorManager> s_pActorManager;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, xiiActorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pActorManager = XII_DEFAULT_NEW(xiiActorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pActorManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pActorManager)
    {
      s_pActorManager->DestroyAllActors(nullptr, xiiActorManager::DestructionMode::Immediate);
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

struct xiiActorManagerImpl
{
  xiiMutex                                            m_Mutex;
  xiiHybridArray<xiiUniquePtr<xiiActor>, 8>           m_AllActors;
  xiiHybridArray<xiiUniquePtr<xiiActorApiService>, 8> m_AllApiServices;
};

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SINGLETON(xiiActorManager);

xiiCopyOnBroadcastEvent<const xiiActorEvent&> xiiActorManager::s_ActorEvents;

xiiActorManager::xiiActorManager() :
  m_SingletonRegistrar(this)
{
  m_pImpl = XII_DEFAULT_NEW(xiiActorManagerImpl);
}

xiiActorManager::~xiiActorManager()
{
  Shutdown();
}

void xiiActorManager::Shutdown()
{
  XII_LOCK(m_pImpl->m_Mutex);

  DestroyAllActors(nullptr, DestructionMode::Immediate);
  DestroyAllApiServices();

  s_ActorEvents.Clear();
}

void xiiActorManager::AddActor(xiiUniquePtr<xiiActor>&& pActor)
{
  XII_LOCK(m_pImpl->m_Mutex);

  XII_ASSERT_DEV(pActor != nullptr, "Actor must exist to be added.");
  m_pImpl->m_AllActors.PushBack(std::move(pActor));

  xiiActorEvent e;
  e.m_Type   = xiiActorEvent::Type::AfterActorCreation;
  e.m_pActor = m_pImpl->m_AllActors.PeekBack().Borrow();
  s_ActorEvents.Broadcast(e);
}

void xiiActorManager::DestroyActor(xiiActor* pActor, DestructionMode mode)
{
  XII_LOCK(m_pImpl->m_Mutex);

  pActor->m_State = xiiActor::State::QueuedForDestruction;

  if (mode == DestructionMode::Immediate && m_bForceQueueActorDestruction == false)
  {
    for (xiiUInt32 i = 0; i < m_pImpl->m_AllActors.GetCount(); ++i)
    {
      if (m_pImpl->m_AllActors[i] == pActor)
      {
        xiiActorEvent e;
        e.m_Type   = xiiActorEvent::Type::BeforeActorDestruction;
        e.m_pActor = pActor;
        s_ActorEvents.Broadcast(e);

        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
        break;
      }
    }
  }
}

void xiiActorManager::DestroyAllActors(const void* pCreatedBy, DestructionMode mode)
{
  XII_LOCK(m_pImpl->m_Mutex);

  for (xiiUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const xiiUInt32 i      = i0 - 1;
    xiiActor*       pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pCreatedBy == nullptr || pActor->GetCreatedBy() == pCreatedBy)
    {
      pActor->m_State = xiiActor::State::QueuedForDestruction;

      if (mode == DestructionMode::Immediate && m_bForceQueueActorDestruction == false)
      {
        xiiActorEvent e;
        e.m_Type   = xiiActorEvent::Type::BeforeActorDestruction;
        e.m_pActor = pActor;
        s_ActorEvents.Broadcast(e);

        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
      }
    }
  }
}

void xiiActorManager::GetAllActors(xiiHybridArray<xiiActor*, 8>& out_AllActors)
{
  XII_LOCK(m_pImpl->m_Mutex);

  out_AllActors.Clear();

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    out_AllActors.PushBack(pActor.Borrow());
  }
}

void xiiActorManager::AddApiService(xiiUniquePtr<xiiActorApiService>&& pApiService)
{
  XII_LOCK(m_pImpl->m_Mutex);

  XII_ASSERT_DEV(pApiService != nullptr, "Invalid API service");
  XII_ASSERT_DEV(pApiService->m_State == xiiActorApiService::State::New, "Actor API service already in use");

  for (auto& pExisting : m_pImpl->m_AllApiServices)
  {
    XII_ASSERT_ALWAYS(pApiService->GetDynamicRTTI() != pExisting->GetDynamicRTTI() || pExisting->m_State == xiiActorApiService::State::QueuedForDestruction, "An actor API service of this type has already been added");
  }

  m_pImpl->m_AllApiServices.PushBack(std::move(pApiService));
}

void xiiActorManager::DestroyApiService(xiiActorApiService* pApiService, DestructionMode mode /*= DestructionMode::Immediate*/)
{
  XII_LOCK(m_pImpl->m_Mutex);

  XII_ASSERT_DEV(pApiService != nullptr, "Invalid API service");

  pApiService->m_State = xiiActorApiService::State::QueuedForDestruction;

  if (mode == DestructionMode::Immediate)
  {
    for (xiiUInt32 i = 0; i < m_pImpl->m_AllApiServices.GetCount(); ++i)
    {
      if (m_pImpl->m_AllApiServices[i] == pApiService)
      {
        m_pImpl->m_AllApiServices.RemoveAtAndCopy(i);
        break;
      }
    }
  }
}

void xiiActorManager::DestroyAllApiServices(DestructionMode mode /*= DestructionMode::Immediate*/)
{
  XII_LOCK(m_pImpl->m_Mutex);

  for (xiiUInt32 i0 = m_pImpl->m_AllApiServices.GetCount(); i0 > 0; --i0)
  {
    const xiiUInt32     i           = i0 - 1;
    xiiActorApiService* pApiService = m_pImpl->m_AllApiServices[i].Borrow();

    pApiService->m_State = xiiActorApiService::State::QueuedForDestruction;

    if (mode == DestructionMode::Immediate)
    {
      m_pImpl->m_AllApiServices.RemoveAtAndCopy(i);
    }
  }
}

void xiiActorManager::ActivateQueuedApiServices()
{
  XII_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllApiServices)
  {
    if (pManager->m_State == xiiActorApiService::State::New)
    {
      pManager->Activate();
      pManager->m_State = xiiActorApiService::State::Active;
    }
  }
}

xiiActorApiService* xiiActorManager::GetApiService(const xiiRTTI* pType)
{
  XII_LOCK(m_pImpl->m_Mutex);

  XII_ASSERT_DEV(pType->IsDerivedFrom<xiiActorApiService>(), "The queried type has to derive from xiiActorApiService");

  for (auto& pApiService : m_pImpl->m_AllApiServices)
  {
    if (pApiService->GetDynamicRTTI()->IsDerivedFrom(pType) && pApiService->m_State != xiiActorApiService::State::QueuedForDestruction)
      return pApiService.Borrow();
  }

  return nullptr;
}

void xiiActorManager::UpdateAllApiServices()
{
  XII_LOCK(m_pImpl->m_Mutex);

  for (auto& pApiService : m_pImpl->m_AllApiServices)
  {
    if (pApiService->m_State == xiiActorApiService::State::Active)
    {
      pApiService->Update();
    }
  }
}

void xiiActorManager::UpdateAllActors()
{
  XII_LOCK(m_pImpl->m_Mutex);

  m_bForceQueueActorDestruction = true;
  XII_SCOPE_EXIT(m_bForceQueueActorDestruction = false);

  for (xiiUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const xiiUInt32 i      = i0 - 1;
    xiiActor*       pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pActor->m_State == xiiActor::State::New)
    {
      pActor->m_State = xiiActor::State::Active;

      pActor->Activate();

      xiiActorEvent e;
      e.m_Type   = xiiActorEvent::Type::AfterActorActivation;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);
    }

    if (pActor->m_State == xiiActor::State::Active)
    {
      pActor->Update();
    }
  }
}

void xiiActorManager::DestroyQueuedActors()
{
  XII_LOCK(m_pImpl->m_Mutex);

  XII_ASSERT_DEV(!m_bForceQueueActorDestruction, "Cannot execute this function right now");

  for (xiiUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const xiiUInt32 i      = i0 - 1;
    xiiActor*       pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pActor->m_State == xiiActor::State::QueuedForDestruction)
    {
      xiiActorEvent e;
      e.m_Type   = xiiActorEvent::Type::BeforeActorDestruction;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndCopy(i);
    }
  }
}

void xiiActorManager::DestroyQueuedActorApiServices()
{
  XII_LOCK(m_pImpl->m_Mutex);

  for (xiiUInt32 i0 = m_pImpl->m_AllApiServices.GetCount(); i0 > 0; --i0)
  {
    const xiiUInt32     i           = i0 - 1;
    xiiActorApiService* pApiService = m_pImpl->m_AllApiServices[i].Borrow();

    if (pApiService->m_State == xiiActorApiService::State::QueuedForDestruction)
    {
      m_pImpl->m_AllApiServices.RemoveAtAndCopy(i);
    }
  }
}

void xiiActorManager::Update()
{
  XII_LOCK(m_pImpl->m_Mutex);

  DestroyQueuedActorApiServices();
  DestroyQueuedActors();
  ActivateQueuedApiServices();
  UpdateAllApiServices();
  UpdateAllActors();
}


XII_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorManager);
