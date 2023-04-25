#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiComponentManagerBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiComponentManagerBase::xiiComponentManagerBase(xiiWorld* pWorld) :
  xiiWorldModule(pWorld), m_Components(pWorld->GetAllocator())
{
}

xiiComponentManagerBase::~xiiComponentManagerBase() {}

xiiComponentHandle xiiComponentManagerBase::CreateComponent(xiiGameObject* pOwnerObject)
{
  xiiComponent* pDummy;
  return CreateComponent(pOwnerObject, pDummy);
}

void xiiComponentManagerBase::DeleteComponent(const xiiComponentHandle& hComponent)
{
  xiiComponent* pComponent = nullptr;
  if (!m_Components.TryGetValue(hComponent, pComponent))
    return;

  DeleteComponent(pComponent);
}

void xiiComponentManagerBase::DeleteComponent(xiiComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  DeinitializeComponent(pComponent);

  m_Components.Remove(pComponent->m_InternalId);

  pComponent->m_InternalId.Invalidate();
  pComponent->m_ComponentFlags.Remove(xiiObjectFlags::ActiveFlag | xiiObjectFlags::ActiveState);

  GetWorld()->m_Data.m_DeadComponents.Insert(pComponent);
}

void xiiComponentManagerBase::Deinitialize()
{
  for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
  {
    DeinitializeComponent(it.Value());
  }

  SUPER::Deinitialize();
}

xiiComponentHandle xiiComponentManagerBase::CreateComponentNoInit(xiiGameObject* pOwnerObject, xiiComponent*& out_pComponent)
{
  XII_ASSERT_DEV(m_Components.GetCount() < xiiWorld::GetMaxNumComponentsPerType(), "Max number of components per type reached: {}",
                 xiiWorld::GetMaxNumComponentsPerType());

  xiiComponent* pComponent = CreateComponentStorage();
  if (pComponent == nullptr)
  {
    return xiiComponentHandle();
  }

  xiiComponentId newId = m_Components.Insert(pComponent);
  newId.m_WorldIndex   = GetWorldIndex();
  newId.m_TypeId       = pComponent->GetTypeId();

  pComponent->m_pManager   = this;
  pComponent->m_InternalId = newId;
  pComponent->m_ComponentFlags.AddOrRemove(xiiObjectFlags::Dynamic, pComponent->GetMode() == xiiComponentMode::Dynamic);

  // In Editor we add components via reflection so it is fine to have a nullptr here.
  // We check for a valid owner before the Initialize() callback.
  if (pOwnerObject != nullptr)
  {
    // AddComponent will update the active state internally
    pOwnerObject->AddComponent(pComponent);
  }
  else
  {
    pComponent->UpdateActiveState(true);
  }

  out_pComponent = pComponent;
  return pComponent->GetHandle();
}

void xiiComponentManagerBase::InitializeComponent(xiiComponent* pComponent)
{
  GetWorld()->AddComponentToInitialize(pComponent->GetHandle());
}

void xiiComponentManagerBase::DeinitializeComponent(xiiComponent* pComponent)
{
  if (pComponent->IsInitialized())
  {
    pComponent->Deinitialize();
    pComponent->m_ComponentFlags.Remove(xiiObjectFlags::Initialized);
  }

  if (xiiGameObject* pOwner = pComponent->GetOwner())
  {
    pOwner->RemoveComponent(pComponent);
  }
}

void xiiComponentManagerBase::PatchIdTable(xiiComponent* pComponent)
{
  xiiComponentId id = pComponent->m_InternalId;
  if (id.m_InstanceIndex != xiiComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = pComponent;
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);
