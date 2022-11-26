#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Stats.h>

xiiStaticArray<xiiWorld*, xiiWorld::GetMaxNumWorlds()> xiiWorld::s_Worlds;

static xiiGameObjectHandle DefaultGameObjectReferenceResolver(const void* pData, xiiComponentHandle hThis, const char* szProperty)
{
  const char* szRef = reinterpret_cast<const char*>(pData);

  if (xiiStringUtils::IsNullOrEmpty(szRef))
    return xiiGameObjectHandle();

  // this is a convention used by xiiPrefabReferenceComponent:
  // a string starting with this means a 'global game object reference', ie a reference that is valid within the current world
  // what follows is an integer that is the internal storage of an xiiGameObjectHandle
  // thus parsing the int and casting it to an xiiGameObjectHandle gives the desired result
  if (xiiStringUtils::StartsWith(szRef, "#!GGOR-"))
  {
    xiiInt64 id;
    if (xiiConversionUtils::StringToInt64(szRef + 7, id).Succeeded())
    {
      return xiiGameObjectHandle(xiiGameObjectId(reinterpret_cast<xiiUInt64&>(id)));
    }
  }

  return xiiGameObjectHandle();
}

xiiWorld::xiiWorld(xiiWorldDesc& desc) :
  m_Data(desc)
{
  m_pUpdateTask                                     = XII_DEFAULT_NEW(xiiDelegateTask<void>, "", xiiMakeDelegate(&xiiWorld::UpdateFromThread, this));
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  xiiStringBuilder sb = desc.m_sName.GetString();
  sb.Append(".Update");
  m_pUpdateTask->ConfigureTask(sb, xiiTaskNesting::Maybe);

  m_uiIndex = xiiInvalidIndex;

  // find a free world slot
  const xiiUInt32 uiWorldCount = s_Worlds.GetCount();
  for (xiiUInt32 i = 0; i < uiWorldCount; i++)
  {
    if (s_Worlds[i] == nullptr)
    {
      s_Worlds[i] = this;
      m_uiIndex   = i;
      break;
    }
  }

  if (m_uiIndex == xiiInvalidIndex)
  {
    m_uiIndex = s_Worlds.GetCount();
    XII_ASSERT_DEV(m_uiIndex < GetMaxNumWorlds(), "Max world index reached: {}", GetMaxNumWorlds());
    static_assert(GetMaxNumWorlds() == XII_MAX_WORLDS);

    s_Worlds.PushBack(this);
  }

  SetGameObjectReferenceResolver(DefaultGameObjectReferenceResolver);
}

xiiWorld::~xiiWorld()
{
  XII_LOCK(GetWriteMarker());
  m_Data.Clear();

  s_Worlds[m_uiIndex] = nullptr;
  m_uiIndex           = xiiInvalidIndex;
}


void xiiWorld::Clear()
{
  CheckForWriteAccess();

  while (GetObjectCount() > 0)
  {
    for (auto it = GetObjects(); it.IsValid(); ++it)
    {
      DeleteObjectNow(it->GetHandle());
    }

    if (GetObjectCount() > 0)
    {
      xiiLog::Dev("Remaining objects after xiiWorld::Clear: {}", GetObjectCount());
    }
  }

  for (xiiWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      pModule->WorldClear();
    }
  }

  // make sure all dead objects and components are cleared right now
  DeleteDeadObjects();
  DeleteDeadComponents();
}

void xiiWorld::SetCoordinateSystemProvider(const xiiSharedPtr<xiiCoordinateSystemProvider>& pProvider)
{
  XII_ASSERT_DEV(pProvider != nullptr, "Coordinate System Provider must not be null");

  m_Data.m_pCoordinateSystemProvider                = pProvider;
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;
}

void xiiWorld::SetGameObjectReferenceResolver(const ReferenceResolver& resolver)
{
  m_Data.m_GameObjectReferenceResolver = resolver;
}

const xiiWorld::ReferenceResolver& xiiWorld::GetGameObjectReferenceResolver() const
{
  return m_Data.m_GameObjectReferenceResolver;
}

// a super simple, but also efficient random number generator
inline static xiiUInt32 NextStableRandomSeed(xiiUInt32& seed)
{
  seed = 214013L * seed + 2531011L;
  return ((seed >> 16) & 0x7FFFF);
}

xiiGameObjectHandle xiiWorld::CreateObject(const xiiGameObjectDesc& desc, xiiGameObject*& out_pObject)
{
  CheckForWriteAccess();

  XII_ASSERT_DEV(m_Data.m_Objects.GetCount() < GetMaxNumGameObjects(), "Max number of game objects reached: {}", GetMaxNumGameObjects());

  xiiGameObject*                     pParentObject    = nullptr;
  xiiGameObject::TransformationData* pParentData      = nullptr;
  xiiUInt32                          uiParentIndex    = 0;
  xiiUInt64                          uiHierarchyLevel = 0;
  bool                               bDynamic         = desc.m_bDynamic;

  if (TryGetObject(desc.m_hParent, pParentObject))
  {
    pParentData      = pParentObject->m_pTransformationData;
    uiParentIndex    = desc.m_hParent.m_InternalId.m_InstanceIndex;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1; // if there is a parent hierarchy level is parent level + 1
    XII_ASSERT_DEV(uiHierarchyLevel < GetMaxNumHierarchyLevels(), "Max hierarchy level reached: {}", GetMaxNumHierarchyLevels());
    bDynamic |= pParentObject->IsDynamic();
  }

  // get storage for the transformation data
  xiiGameObject::TransformationData* pTransformationData = m_Data.CreateTransformationData(bDynamic, static_cast<xiiUInt32>(uiHierarchyLevel));

  // get storage for the object itself
  xiiGameObject* pNewObject = m_Data.m_ObjectStorage.Create();

  // insert the new object into the id mapping table
  xiiGameObjectId newId = m_Data.m_Objects.Insert(pNewObject);
  newId.m_WorldIndex    = xiiGameObjectId::StorageType(m_uiIndex & (XII_MAX_WORLDS - 1));

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags      = xiiObjectFlags::None;
  pNewObject->m_Flags.AddOrRemove(xiiObjectFlags::Dynamic, bDynamic);
  pNewObject->m_Flags.AddOrRemove(xiiObjectFlags::ActiveFlag, desc.m_bActiveFlag);
  pNewObject->m_sName         = desc.m_sName;
  pNewObject->m_uiParentIndex = uiParentIndex;
  pNewObject->m_Tags          = desc.m_Tags;
  pNewObject->m_uiTeamID      = desc.m_uiTeamID;

  static_assert((GetMaxNumHierarchyLevels() - 1) <= xiiMath::MaxValue<xiiUInt16>());
  pNewObject->m_uiHierarchyLevel = static_cast<xiiUInt16>(uiHierarchyLevel);

  // fill out the transformation data
  pTransformationData->m_pObject       = pNewObject;
  pTransformationData->m_pParentData   = pParentData;
  pTransformationData->m_localPosition = xiiSimdConversion::ToVec3(desc.m_LocalPosition);
  pTransformationData->m_localRotation = xiiSimdConversion::ToQuat(desc.m_LocalRotation);
  pTransformationData->m_localScaling  = xiiSimdConversion::ToVec4(desc.m_LocalScaling.GetAsVec4(desc.m_LocalUniformScaling));
  pTransformationData->m_globalTransform.SetIdentity();
#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  pTransformationData->m_velocity.SetZero();
#endif
  pTransformationData->m_localBounds.SetInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(xiiSimdFloat::Zero());
  pTransformationData->m_globalBounds = pTransformationData->m_localBounds;
  pTransformationData->m_hSpatialData.Invalidate();
  pTransformationData->m_uiSpatialDataCategoryBitmask = 0;
  pTransformationData->m_uiStableRandomSeed           = desc.m_uiStableRandomSeed;

  // if seed is set to 0xFFFFFFFF, use the parent's seed to create a deterministic value for this object
  if (pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF && pTransformationData->m_pParentData != nullptr)
  {
    xiiUInt32 seed = pTransformationData->m_pParentData->m_uiStableRandomSeed + pTransformationData->m_pParentData->m_pObject->GetChildCount();

    do
    {
      pTransformationData->m_uiStableRandomSeed = NextStableRandomSeed(seed);

    } while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF);
  }

  // if the seed is zero (or there was no parent to derive the seed from), assign a random value
  while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF)
  {
    pTransformationData->m_uiStableRandomSeed = GetRandomNumberGenerator().UInt();
  }

  pTransformationData->UpdateGlobalTransformNonRecursive();

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  pTransformationData->m_lastGlobalPosition = pTransformationData->m_globalTransform.m_Position;
#endif

  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  // fix links
  LinkToParent(pNewObject);

  pNewObject->UpdateActiveState(pParentObject == nullptr ? true : pParentObject->IsActive());

  out_pObject = pNewObject;
  return xiiGameObjectHandle(newId);
}

void xiiWorld::DeleteObjectNow(const xiiGameObjectHandle& hObject0, bool bAlsoDeleteEmptyParents /*= true*/)
{
  CheckForWriteAccess();

  xiiGameObject* pObject = nullptr;
  if (!m_Data.m_Objects.TryGetValue(hObject0, pObject))
    return;

  xiiGameObjectHandle hObject = hObject0;

  if (bAlsoDeleteEmptyParents)
  {
    xiiGameObject* pParent = pObject->GetParent();

    while (pParent)
    {
      if (pParent->GetChildCount() != 1 || pParent->GetComponents().GetCount() != 0)
        break;

      pObject = pParent;

      pParent = pParent->GetParent();
    }

    hObject = pObject->GetHandle();
  }

  // inform external systems that we are about to delete this object
  m_Data.m_ObjectDeletionEvent.Broadcast(pObject);

  // set object to inactive so components and children know that they shouldn't access the object anymore.
  pObject->m_Flags.Remove(xiiObjectFlags::ActiveFlag | xiiObjectFlags::ActiveState);

  // delete children
  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteObjectNow(it->GetHandle(), false);
  }

  // delete attached components
  while (!pObject->m_Components.IsEmpty())
  {
    xiiComponent* pComponent = pObject->m_Components[0];
    pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());
  }
  XII_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // remove from global key tables
  SetObjectGlobalKey(pObject, xiiHashedString());

  // invalidate (but preserve world index) and remove from id table
  pObject->m_InternalId.Invalidate();
  pObject->m_InternalId.m_WorldIndex = m_uiIndex;

  m_Data.m_DeadObjects.Insert(pObject);
  XII_VERIFY(m_Data.m_Objects.Remove(hObject), "Implementation error.");
}

void xiiWorld::DeleteObjectDelayed(const xiiGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents /*= true*/)
{
  xiiMsgDeleteGameObject msg;
  msg.m_bDeleteEmptyParents = bAlsoDeleteEmptyParents;
  PostMessage(hObject, msg, xiiTime::Zero());
}

xiiComponentInitBatchHandle xiiWorld::CreateComponentInitBatch(const char* szBatchName, bool bMustFinishWithinOneFrame /*= true*/)
{
  auto pInitBatch = XII_NEW(GetAllocator(), xiiInternal::WorldData::InitBatch, GetAllocator(), szBatchName, bMustFinishWithinOneFrame);
  return xiiComponentInitBatchHandle(m_Data.m_InitBatches.Insert(pInitBatch));
}

void xiiWorld::DeleteComponentInitBatch(const xiiComponentInitBatchHandle& batch)
{
  auto& pInitBatch = m_Data.m_InitBatches[batch.GetInternalID()];
  XII_ASSERT_DEV(pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty(), "Init batch has not been completely processed");
  m_Data.m_InitBatches.Remove(batch.GetInternalID());
}

void xiiWorld::BeginAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& batch)
{
  XII_ASSERT_DEV(m_Data.m_pCurrentInitBatch == m_Data.m_pDefaultInitBatch, "Nested init batches are not supported");
  m_Data.m_pCurrentInitBatch = m_Data.m_InitBatches[batch.GetInternalID()].Borrow();
}

void xiiWorld::EndAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& batch)
{
  XII_ASSERT_DEV(m_Data.m_InitBatches[batch.GetInternalID()] == m_Data.m_pCurrentInitBatch, "Init batch with id {} is currently not active", batch.GetInternalID().m_Data);
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

void xiiWorld::SubmitComponentInitBatch(const xiiComponentInitBatchHandle& batch)
{
  m_Data.m_InitBatches[batch.GetInternalID()]->m_bIsReady = true;
  m_Data.m_pCurrentInitBatch                              = m_Data.m_pDefaultInitBatch;
}

bool xiiWorld::IsComponentInitBatchCompleted(const xiiComponentInitBatchHandle& batch, double* pCompletionFactor /*= nullptr*/)
{
  auto& pInitBatch = m_Data.m_InitBatches[batch.GetInternalID()];
  XII_ASSERT_DEV(pInitBatch->m_bIsReady, "Batch is not submitted yet");

  if (pCompletionFactor != nullptr)
  {
    if (pInitBatch->m_ComponentsToInitialize.IsEmpty())
    {
      double fStartSimCompletion = pInitBatch->m_ComponentsToStartSimulation.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToStartSimulation / pInitBatch->m_ComponentsToStartSimulation.GetCount();
      *pCompletionFactor         = fStartSimCompletion * 0.5 + 0.5;
    }
    else
    {
      double fInitCompletion = pInitBatch->m_ComponentsToInitialize.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToInitialize / pInitBatch->m_ComponentsToInitialize.GetCount();
      *pCompletionFactor     = fInitCompletion * 0.5;
    }
  }

  return pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty();
}

void xiiWorld::CancelComponentInitBatch(const xiiComponentInitBatchHandle& batch)
{
  auto& pInitBatch = m_Data.m_InitBatches[batch.GetInternalID()];
  pInitBatch->m_ComponentsToInitialize.Clear();
  pInitBatch->m_ComponentsToStartSimulation.Clear();
}
void xiiWorld::PostMessage(const xiiGameObjectHandle& receiverObject, const xiiMessage& msg, xiiObjectMsgQueueType::Enum queueType, xiiTime delay, bool bRecursive) const
{
  // This method is allowed to be called from multiple threads.

  XII_ASSERT_DEBUG((receiverObject.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in object id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = receiverObject.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent       = false;
  metaData.m_uiRecursive                 = bRecursive;

  xiiRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.GetSeconds() > 0.0)
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void xiiWorld::PostMessage(const xiiComponentHandle& receiverComponent, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.

  XII_ASSERT_DEBUG((receiverComponent.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in component id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = receiverComponent.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent       = true;
  metaData.m_uiRecursive                 = false;

  xiiRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.GetSeconds() > 0.0)
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void xiiWorld::FindEventMsgHandlers(const xiiEventMessage& msg, xiiGameObject* pSearchObject, xiiDynamicArray<xiiComponent*>& out_components)
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void xiiWorld::FindEventMsgHandlers(const xiiEventMessage& msg, const xiiGameObject* pSearchObject, xiiDynamicArray<const xiiComponent*>& out_components) const
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void xiiWorld::Update()
{
  CheckForWriteAccess();

  XII_LOG_BLOCK(m_Data.m_sName.GetData());

  {
    xiiStringBuilder sStatName;
    sStatName.Format("World Update/{0}/Game Object Count", m_Data.m_sName);

    xiiStringBuilder sStatValue;
    xiiStats::SetStat(sStatName, GetObjectCount());
  }

  if (!m_Data.m_bSimulateWorld)
  {
    // only change the pause mode temporarily
    // so that user choices don't get overridden

    const bool bClockPaused = m_Data.m_Clock.GetPaused();
    m_Data.m_Clock.SetPaused(true);
    m_Data.m_Clock.Update();
    m_Data.m_Clock.SetPaused(bClockPaused);
  }
  else
  {
    m_Data.m_Clock.Update();
  }

  if (m_Data.m_pSpatialSystem != nullptr)
  {
    m_Data.m_pSpatialSystem->StartNewFrame();
  }

  // initialize phase
  {
    XII_PROFILE_SCOPE("Initialize Phase");
    ProcessComponentsToInitialize();
    ProcessUpdateFunctionsToRegister();

    ProcessQueuedMessages(xiiObjectMsgQueueType::AfterInitialized);
  }

  // pre-async phase
  {
    XII_PROFILE_SCOPE("Pre-Async Phase");
    ProcessQueuedMessages(xiiObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[xiiComponentManagerBase::UpdateFunctionDesc::Phase::PreAsync]);
  }

  // async phase
  {
    // remove write marker but keep the read marker. Thus no one can mark the world for writing now. Only reading is allowed in async phase.
    m_Data.m_WriteThreadID = (xiiThreadID)0;

    XII_PROFILE_SCOPE("Async Phase");
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = xiiThreadUtils::GetCurrentThreadID();
  }

  // post-async phase
  {
    XII_PROFILE_SCOPE("Post-Async Phase");
    ProcessQueuedMessages(xiiObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[xiiComponentManagerBase::UpdateFunctionDesc::Phase::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    XII_PROFILE_SCOPE("Delete Dead Objects");
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // update transforms
  {
    float fInvDelta = 0.0f;

    // when the clock is paused just use zero
    const float fDelta = (float)m_Data.m_Clock.GetTimeDiff().GetSeconds();
    if (fDelta > 0.0f)
      fInvDelta = 1.0f / fDelta;

    XII_PROFILE_SCOPE("Update Transforms");
    m_Data.UpdateGlobalTransforms(fInvDelta);
  }

  // post-transform phase
  {
    XII_PROFILE_SCOPE("Post-Transform Phase");
    ProcessQueuedMessages(xiiObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[xiiComponentManagerBase::UpdateFunctionDesc::Phase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    XII_PROFILE_SCOPE("Initialize Phase 2");
    // Only process the default init batch here since it contains the components created at runtime.
    // Also make sure that all initialization is finished after this call by giving it enough time.
    ProcessInitializationBatch(*m_Data.m_pDefaultInitBatch, xiiTime::Now() + xiiTime::Hours(10000));

    ProcessQueuedMessages(xiiObjectMsgQueueType::AfterInitialized);
  }

  // Swap our double buffered stack allocator
  m_Data.m_StackAllocator.Swap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

xiiWorldModule* xiiWorld::GetOrCreateModule(const xiiRTTI* pRtti)
{
  CheckForWriteAccess();

  const xiiWorldModuleTypeId uiTypeId = xiiWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId == 0xFFFF)
  {
    return nullptr;
  }

  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  xiiWorldModule* pModule = m_Data.m_Modules[uiTypeId];
  if (pModule == nullptr)
  {
    pModule = xiiWorldModuleFactory::GetInstance()->CreateWorldModule(uiTypeId, this);
    pModule->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

void xiiWorld::DeleteModule(const xiiRTTI* pRtti)
{
  CheckForWriteAccess();

  const xiiWorldModuleTypeId uiTypeId = xiiWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (xiiWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      pModule->Deinitialize();
      DeregisterUpdateFunctions(pModule);
      XII_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

xiiWorldModule* xiiWorld::GetModule(const xiiRTTI* pRtti)
{
  CheckForWriteAccess();

  const xiiWorldModuleTypeId uiTypeId = xiiWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

const xiiWorldModule* xiiWorld::GetModule(const xiiRTTI* pRtti) const
{
  CheckForReadAccess();

  const xiiWorldModuleTypeId uiTypeId = xiiWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

void xiiWorld::SetParent(xiiGameObject* pObject, xiiGameObject* pNewParent, xiiGameObject::TransformPreservation preserve)
{
  XII_ASSERT_DEV(pObject != pNewParent, "Object can't be its own parent!");
  XII_ASSERT_DEV(pNewParent == nullptr || pObject->IsDynamic() || pNewParent->IsStatic(), "Can't attach a static object to a dynamic parent!");
  CheckForWriteAccess();

  if (GetObjectUnchecked(pObject->m_uiParentIndex) == pNewParent)
    return;

  UnlinkFromParent(pObject);
  // UnlinkFromParent does not clear these as they are still needed in DeleteObjectNow to allow deletes while iterating.
  pObject->m_uiNextSiblingIndex = 0;
  pObject->m_uiPrevSiblingIndex = 0;
  if (pNewParent != nullptr)
  {
    // Ensure that the parent's global transform is up-to-date otherwise the object's local transform will be wrong afterwards.
    pNewParent->UpdateGlobalTransform();

    pObject->m_uiParentIndex = pNewParent->m_InternalId.m_InstanceIndex;
    LinkToParent(pObject);
  }

  PatchHierarchyData(pObject, preserve);

  // TODO: the functions above send messages such as xiiMsgChildrenChanged, which will not arrive for inactive components, is that a problem ?
  // 1) if a component was active before and now gets deactivated, it may not care about the message anymore anyway
  // 2) if a component was inactive before, it did not get the message, but upon activation it can update the state for which it needed the message
  // so probably it is fine, only components that were active and stay active need the message, and that will be the case
  pObject->UpdateActiveState(pNewParent == nullptr ? true : pNewParent->IsActive());
}

void xiiWorld::LinkToParent(xiiGameObject* pObject)
{
  XII_ASSERT_DEBUG(pObject->m_uiNextSiblingIndex == 0 && pObject->m_uiPrevSiblingIndex == 0, "Object is either still linked to another parent or data was not cleared.");
  if (xiiGameObject* pParentObject = pObject->GetParent())
  {
    const xiiUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (pParentObject->m_uiFirstChildIndex != 0)
    {
      pObject->m_uiPrevSiblingIndex                                               = pParentObject->m_uiLastChildIndex;
      GetObjectUnchecked(pParentObject->m_uiLastChildIndex)->m_uiNextSiblingIndex = uiIndex;
    }
    else
    {
      pParentObject->m_uiFirstChildIndex = uiIndex;
    }

    pParentObject->m_uiLastChildIndex = uiIndex;
    pParentObject->m_uiChildCount++;

    pObject->m_pTransformationData->m_pParentData = pParentObject->m_pTransformationData;

    if (pParentObject->m_Flags.IsSet(xiiObjectFlags::ChildChangesNotifications))
    {
      xiiMsgChildrenChanged msg;
      msg.m_Type    = xiiMsgChildrenChanged::Type::ChildAdded;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild  = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void xiiWorld::UnlinkFromParent(xiiGameObject* pObject)
{
  if (xiiGameObject* pParentObject = pObject->GetParent())
  {
    const xiiUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (uiIndex == pParentObject->m_uiFirstChildIndex)
      pParentObject->m_uiFirstChildIndex = pObject->m_uiNextSiblingIndex;

    if (uiIndex == pParentObject->m_uiLastChildIndex)
      pParentObject->m_uiLastChildIndex = pObject->m_uiPrevSiblingIndex;

    if (xiiGameObject* pNextObject = GetObjectUnchecked(pObject->m_uiNextSiblingIndex))
      pNextObject->m_uiPrevSiblingIndex = pObject->m_uiPrevSiblingIndex;

    if (xiiGameObject* pPrevObject = GetObjectUnchecked(pObject->m_uiPrevSiblingIndex))
      pPrevObject->m_uiNextSiblingIndex = pObject->m_uiNextSiblingIndex;

    pParentObject->m_uiChildCount--;
    pObject->m_uiParentIndex                      = 0;
    pObject->m_pTransformationData->m_pParentData = nullptr;

    // Note that the sibling indices must not be set to 0 here.
    // They are still needed if we currently iterate over child objects.

    if (pParentObject->m_Flags.IsSet(xiiObjectFlags::ChildChangesNotifications))
    {
      xiiMsgChildrenChanged msg;
      msg.m_Type    = xiiMsgChildrenChanged::Type::ChildRemoved;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild  = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void xiiWorld::SetObjectGlobalKey(xiiGameObject* pObject, const xiiHashedString& sGlobalKey)
{
  if (m_Data.m_GlobalKeyToIdTable.Contains(sGlobalKey.GetHash()))
  {
    xiiLog::Error("Can't set global key to '{0}' because an object with this global key already exists. Global keys have to be unique.", sGlobalKey);
    return;
  }

  const xiiUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  // Remove existing entry first.
  xiiHashedString* pOldGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pOldGlobalKey))
  {
    if (sGlobalKey == *pOldGlobalKey)
    {
      return;
    }

    XII_VERIFY(m_Data.m_GlobalKeyToIdTable.Remove(pOldGlobalKey->GetHash()), "Implementation error.");
    XII_VERIFY(m_Data.m_IdToGlobalKeyTable.Remove(uiId), "Implementation error.");
  }

  // Insert new one if key is valid.
  if (!sGlobalKey.IsEmpty())
  {
    m_Data.m_GlobalKeyToIdTable.Insert(sGlobalKey.GetHash(), pObject->m_InternalId);
    m_Data.m_IdToGlobalKeyTable.Insert(uiId, sGlobalKey);
  }
}

const char* xiiWorld::GetObjectGlobalKey(const xiiGameObject* pObject) const
{
  const xiiUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  const xiiHashedString* pGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pGlobalKey))
  {
    return pGlobalKey->GetData();
  }

  return "";
}

void xiiWorld::ProcessQueuedMessage(const xiiInternal::WorldData::MessageQueue::Entry& entry)
{
  if (entry.m_MetaData.m_uiReceiverIsComponent)
  {
    xiiComponentHandle hComponent(xiiComponentId(entry.m_MetaData.m_uiReceiverObjectOrComponent));

    xiiComponent* pReceiverComponent = nullptr;
    if (TryGetComponent(hComponent, pReceiverComponent))
    {
      pReceiverComponent->SendMessageInternal(*entry.m_pMessage, true);
    }
    else
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        xiiLog::Warning("xiiWorld::ProcessQueuedMessage: Receiver xiiComponent for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
  else
  {
    xiiGameObjectHandle hObject(xiiGameObjectId(entry.m_MetaData.m_uiReceiverObjectOrComponent));

    xiiGameObject* pReceiverObject = nullptr;
    if (TryGetObject(hObject, pReceiverObject))
    {
      if (entry.m_MetaData.m_uiRecursive)
      {
        pReceiverObject->SendMessageRecursiveInternal(*entry.m_pMessage, true);
      }
      else
      {
        pReceiverObject->SendMessageInternal(*entry.m_pMessage, true);
      }
    }
    else
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        xiiLog::Warning("xiiWorld::ProcessQueuedMessage: Receiver xiiGameObject for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
}

void xiiWorld::ProcessQueuedMessages(xiiObjectMsgQueueType::Enum queueType)
{
  XII_PROFILE_SCOPE("Process Queued Messages");

  struct MessageComparer
  {
    XII_FORCE_INLINE bool Less(const xiiInternal::WorldData::MessageQueue::Entry& a, const xiiInternal::WorldData::MessageQueue::Entry& b) const
    {
      if (a.m_MetaData.m_Due != b.m_MetaData.m_Due)
        return a.m_MetaData.m_Due < b.m_MetaData.m_Due;

      const xiiInt32 iKeyA = a.m_pMessage->GetSortingKey();
      const xiiInt32 iKeyB = b.m_pMessage->GetSortingKey();
      if (iKeyA != iKeyB)
        return iKeyA < iKeyB;

      if (a.m_pMessage->GetId() != b.m_pMessage->GetId())
        return a.m_pMessage->GetId() < b.m_pMessage->GetId();

      if (a.m_MetaData.m_uiReceiverData != b.m_MetaData.m_uiReceiverData)
        return a.m_MetaData.m_uiReceiverData < b.m_MetaData.m_uiReceiverData;

      if (a.m_uiMessageHash == 0)
      {
        a.m_uiMessageHash = a.m_pMessage->GetHash();
      }

      if (b.m_uiMessageHash == 0)
      {
        b.m_uiMessageHash = b.m_pMessage->GetHash();
      }

      return a.m_uiMessageHash < b.m_uiMessageHash;
    }
  };

  // regular messages
  {
    xiiInternal::WorldData::MessageQueue& queue = m_Data.m_MessageQueues[queueType];
    queue.Sort(MessageComparer());

    for (xiiUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      ProcessQueuedMessage(queue[i]);

      // no need to deallocate these messages, they are allocated through a frame allocator
    }

    queue.Clear();
  }

  // timed messages
  {
    xiiInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    const xiiTime now = m_Data.m_Clock.GetAccumulatedTime();

    while (!queue.IsEmpty())
    {
      auto& entry = queue.Peek();
      if (entry.m_MetaData.m_Due > now)
        break;

      ProcessQueuedMessage(entry);

      XII_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
  }
}

// static
template <typename World, typename GameObject, typename Component>
void xiiWorld::FindEventMsgHandlers(World& world, const xiiEventMessage& msg, GameObject pSearchObject, xiiDynamicArray<Component>& out_components)
{
  using EventMessageHandlerComponentType = typename std::conditional<std::is_const<World>::value, const xiiEventMessageHandlerComponent*, xiiEventMessageHandlerComponent*>::type;

  out_components.Clear();

  // walk the graph upwards until an object is found with an xiiEventMessageHandlerComponent that handles this type of message
  {
    auto pCurrentObject = pSearchObject;

    while (pCurrentObject != nullptr)
    {
      xiiHybridArray<EventMessageHandlerComponentType, 4> eventMessageHandlerComponents;
      pCurrentObject->TryGetComponentsOfBaseType(eventMessageHandlerComponents);

      if (eventMessageHandlerComponents.IsEmpty() == false)
      {
        bool bContinueSearch = true;

        for (auto pEventMessageHandlerComponent : eventMessageHandlerComponents)
        {
          if constexpr (std::is_const<World>::value == false)
          {
            pEventMessageHandlerComponent->EnsureInitialized();
          }

          if (pEventMessageHandlerComponent->HandlesEventMessage(msg))
          {
            out_components.PushBack(pEventMessageHandlerComponent);
            bContinueSearch = false;
          }
          else
          {
            if constexpr (std::is_const<World>::value)
            {
              if (pEventMessageHandlerComponent->IsInitialized() == false)
              {
                xiiLog::Warning("Potential event message handler component of type '{}' was not initialized (yet) and thus might have reported "
                                "an incorrect result in HandlesEventMessage(). "
                                "To allow this component to be automatically initialized at this point in time call the non-const variant of SendEventMessage.",
                                pEventMessageHandlerComponent->GetDynamicRTTI()->GetTypeName());
              }
            }

            // only continue to search on parent objects if all event handlers on the current object have the "pass through unhandled events" flag set.
            bContinueSearch &= pEventMessageHandlerComponent->GetPassThroughUnhandledEvents();
          }
        }

        if (!bContinueSearch)
        {
          // stop searching as we found at least one xiiEventMessageHandlerComponent or one doesn't have the "pass through" flag set.
          return;
        }
      }

      pCurrentObject = pCurrentObject->GetParent();
    }
  }

  // if no such object is found, check all objects that are registered as 'global event handlers'
  {
    auto globalEventMessageHandler = xiiEventMessageHandlerComponent::GetAllGlobalEventHandler(&world);
    for (auto hEventMessageHandlerComponent : globalEventMessageHandler)
    {
      EventMessageHandlerComponentType pEventMessageHandlerComponent = nullptr;
      if (world.TryGetComponent(hEventMessageHandlerComponent, pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesEventMessage(msg))
        {
          out_components.PushBack(pEventMessageHandlerComponent);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void xiiWorld::RegisterUpdateFunction(const xiiComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  XII_ASSERT_DEV(desc.m_Phase == xiiComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");
  XII_ASSERT_DEV(desc.m_Phase != xiiComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_DependsOn.GetCount() == 0, "Asynchronous update functions must not have dependencies");
  XII_ASSERT_DEV(desc.m_Function.IsComparable(), "Delegates with captures are not allowed as xiiWorld update functions.");

  m_Data.m_UpdateFunctionsToRegister.PushBack(desc);
}

void xiiWorld::DeregisterUpdateFunction(const xiiComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  xiiDynamicArrayBase<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];

  for (xiiUInt32 i = updateFunctions.GetCount(); i-- > 0;)
  {
    if (updateFunctions[i].m_Function.IsEqualIfComparable(desc.m_Function))
    {
      updateFunctions.RemoveAtAndCopy(i);
    }
  }
}

void xiiWorld::DeregisterUpdateFunctions(xiiWorldModule* pModule)
{
  CheckForWriteAccess();

  for (xiiUInt32 phase = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync; phase < xiiWorldModule::UpdateFunctionDesc::Phase::COUNT; ++phase)
  {
    xiiDynamicArrayBase<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[phase];

    for (xiiUInt32 i = updateFunctions.GetCount(); i-- > 0;)
    {
      if (updateFunctions[i].m_Function.GetClassInstance() == pModule)
      {
        updateFunctions.RemoveAtAndCopy(i);
      }
    }
  }
}

void xiiWorld::AddComponentToInitialize(xiiComponentHandle hComponent)
{
  m_Data.m_pCurrentInitBatch->m_ComponentsToInitialize.PushBack(hComponent);
}

void xiiWorld::UpdateFromThread()
{
  XII_LOCK(GetWriteMarker());

  Update();
}

void xiiWorld::UpdateSynchronous(const xiiArrayPtr<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  xiiWorldModule::UpdateContext context;
  context.m_uiFirstComponentIndex = 0;
  context.m_uiComponentCount      = xiiInvalidIndex;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    {
      XII_PROFILE_SCOPE(updateFunction.m_sFunctionName);
      updateFunction.m_Function(context);
    }
  }
}

void xiiWorld::UpdateAsynchronous()
{
  xiiTaskGroupID taskGroupId = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);

  xiiDynamicArrayBase<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[xiiComponentManagerBase::UpdateFunctionDesc::Phase::Async];

  xiiUInt32 uiCurrentTaskIndex = 0;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    xiiWorldModule*          pModule  = static_cast<xiiWorldModule*>(updateFunction.m_Function.GetClassInstance());
    xiiComponentManagerBase* pManager = xiiDynamicCast<xiiComponentManagerBase*>(pModule);

    // a world module can also register functions in the async phase so we want at least one task
    const xiiUInt32 uiTotalCount  = pManager != nullptr ? pManager->GetComponentCount() : 1;
    const xiiUInt32 uiGranularity = (updateFunction.m_uiGranularity != 0) ? updateFunction.m_uiGranularity : uiTotalCount;

    xiiUInt32 uiStartIndex = 0;
    while (uiStartIndex < uiTotalCount)
    {
      xiiSharedPtr<xiiInternal::WorldData::UpdateTask> pTask;
      if (uiCurrentTaskIndex < m_Data.m_UpdateTasks.GetCount())
      {
        pTask = m_Data.m_UpdateTasks[uiCurrentTaskIndex];
      }
      else
      {
        pTask = XII_NEW(&m_Data.m_Allocator, xiiInternal::WorldData::UpdateTask);
        m_Data.m_UpdateTasks.PushBack(pTask);
      }

      pTask->ConfigureTask(updateFunction.m_sFunctionName, xiiTaskNesting::Maybe);
      pTask->m_Function     = updateFunction.m_Function;
      pTask->m_uiStartIndex = uiStartIndex;
      pTask->m_uiCount      = (uiStartIndex + uiGranularity < uiTotalCount) ? uiGranularity : xiiInvalidIndex;
      xiiTaskSystem::AddTaskToGroup(taskGroupId, pTask);

      ++uiCurrentTaskIndex;
      uiStartIndex += uiGranularity;
    }
  }

  xiiTaskSystem::StartTaskGroup(taskGroupId);
  xiiTaskSystem::WaitForGroup(taskGroupId);
}

bool xiiWorld::ProcessInitializationBatch(xiiInternal::WorldData::InitBatch& batch, xiiTime endTime)
{
  CheckForWriteAccess();

  if (!batch.m_ComponentsToInitialize.IsEmpty())
  {
    xiiStringBuilder profileScopeName("Init ", batch.m_sName);
    XII_PROFILE_SCOPE(profileScopeName);

    // Reserve for later use
    batch.m_ComponentsToStartSimulation.Reserve(batch.m_ComponentsToInitialize.GetCount());

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToInitialize < batch.m_ComponentsToInitialize.GetCount(); ++batch.m_uiNextComponentToInitialize)
    {
      xiiComponentHandle hComponent = batch.m_ComponentsToInitialize[batch.m_uiNextComponentToInitialize];

      // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
      xiiComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      XII_ASSERT_DEBUG(pComponent->GetOwner() != nullptr, "Component must have a valid owner");

      // make sure the object's transform is up to date before the component is initialized.
      pComponent->GetOwner()->UpdateGlobalTransform();

      pComponent->EnsureInitialized();

      if (pComponent->IsActive())
      {
        pComponent->OnActivated();

        batch.m_ComponentsToStartSimulation.PushBack(hComponent);
      }

      // Check if there is still time left to initialize more components
      if (xiiTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToInitialize;
        return false;
      }
    }

    batch.m_ComponentsToInitialize.Clear();
    batch.m_uiNextComponentToInitialize = 0;
  }

  if (m_Data.m_bSimulateWorld)
  {
    xiiStringBuilder startSimName("Start Sim ", batch.m_sName);
    XII_PROFILE_SCOPE(startSimName);

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToStartSimulation < batch.m_ComponentsToStartSimulation.GetCount(); ++batch.m_uiNextComponentToStartSimulation)
    {
      xiiComponentHandle hComponent = batch.m_ComponentsToStartSimulation[batch.m_uiNextComponentToStartSimulation];

      // if it is in the editor, the component might have been added and already deleted,  without ever running the simulation
      xiiComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->EnsureSimulationStarted();
      }

      // Check if there is still time left to initialize more components
      if (xiiTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToStartSimulation;
        return false;
      }
    }

    batch.m_ComponentsToStartSimulation.Clear();
    batch.m_uiNextComponentToStartSimulation = 0;
  }

  return true;
}

void xiiWorld::ProcessComponentsToInitialize()
{
  CheckForWriteAccess();

  if (m_Data.m_bSimulateWorld)
  {
    XII_PROFILE_SCOPE("Modules Start Simulation");

    // Can't use foreach here because the array might be resized during iteration.
    for (xiiUInt32 i = 0; i < m_Data.m_ModulesToStartSimulation.GetCount(); ++i)
    {
      m_Data.m_ModulesToStartSimulation[i]->OnSimulationStarted();
    }

    m_Data.m_ModulesToStartSimulation.Clear();
  }

  XII_PROFILE_SCOPE("Initialize Components");

  xiiTime endTime = xiiTime::Now() + m_Data.m_MaxInitializationTimePerFrame;

  // First process all component init batches that have to finish within this frame
  for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
  {
    auto& pInitBatch = it.Value();
    if (pInitBatch->m_bIsReady && pInitBatch->m_bMustFinishWithinOneFrame)
    {
      ProcessInitializationBatch(*pInitBatch, xiiTime::Now() + xiiTime::Hours(10000));
    }
  }

  // If there is still time left process other component init batches
  if (xiiTime::Now() < endTime)
  {
    for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
    {
      auto& pInitBatch = it.Value();
      if (!pInitBatch->m_bIsReady || pInitBatch->m_bMustFinishWithinOneFrame)
        continue;

      if (!ProcessInitializationBatch(*pInitBatch, endTime))
        return;
    }
  }
}

void xiiWorld::ProcessUpdateFunctionsToRegister()
{
  CheckForWriteAccess();

  if (m_Data.m_UpdateFunctionsToRegister.IsEmpty())
    return;

  XII_PROFILE_SCOPE("Register update functions");

  while (!m_Data.m_UpdateFunctionsToRegister.IsEmpty())
  {
    const xiiUInt32 uiNumFunctionsToRegister = m_Data.m_UpdateFunctionsToRegister.GetCount();

    for (xiiUInt32 i = uiNumFunctionsToRegister; i-- > 0;)
    {
      if (RegisterUpdateFunctionInternal(m_Data.m_UpdateFunctionsToRegister[i]).Succeeded())
      {
        m_Data.m_UpdateFunctionsToRegister.RemoveAtAndCopy(i);
      }
    }

    XII_ASSERT_DEV(m_Data.m_UpdateFunctionsToRegister.GetCount() < uiNumFunctionsToRegister, "No functions have been registered because the dependencies could not be found.");
  }
}

xiiResult xiiWorld::RegisterUpdateFunctionInternal(const xiiWorldModule::UpdateFunctionDesc& desc)
{
  xiiDynamicArrayBase<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions  = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];
  xiiUInt32                                                              uiInsertionIndex = 0;

  for (xiiUInt32 i = 0; i < desc.m_DependsOn.GetCount(); ++i)
  {
    xiiUInt32 uiDependencyIndex = xiiInvalidIndex;

    for (xiiUInt32 j = 0; j < updateFunctions.GetCount(); ++j)
    {
      if (updateFunctions[j].m_sFunctionName == desc.m_DependsOn[i])
      {
        uiDependencyIndex = j;
        break;
      }
    }

    if (uiDependencyIndex == xiiInvalidIndex) // dependency not found
    {
      return XII_FAILURE;
    }
    else
    {
      uiInsertionIndex = xiiMath::Max(uiInsertionIndex, uiDependencyIndex + 1);
    }
  }

  xiiInternal::WorldData::RegisteredUpdateFunction newFunction;
  newFunction.FillFromDesc(desc);

  while (uiInsertionIndex < updateFunctions.GetCount())
  {
    const auto& existingFunction = updateFunctions[uiInsertionIndex];
    if (newFunction < existingFunction)
    {
      break;
    }

    ++uiInsertionIndex;
  }

  updateFunctions.Insert(newFunction, uiInsertionIndex);

  return XII_SUCCESS;
}

void xiiWorld::DeleteDeadObjects()
{
  while (!m_Data.m_DeadObjects.IsEmpty())
  {
    xiiGameObject* pObject = m_Data.m_DeadObjects.GetIterator().Key();

    if (!pObject->m_pTransformationData->m_hSpatialData.IsInvalidated())
    {
      m_Data.m_pSpatialSystem->DeleteSpatialData(pObject->m_pTransformationData->m_hSpatialData);
    }

    m_Data.DeleteTransformationData(pObject->IsDynamic(), pObject->m_uiHierarchyLevel, pObject->m_pTransformationData);

    xiiGameObject* pMovedObject = nullptr;
    m_Data.m_ObjectStorage.Delete(pObject, pMovedObject);

    if (pObject != pMovedObject)
    {
      // patch the id table: the last element in the storage has been moved to deleted object's location,
      // thus the pointer now points to another object
      xiiGameObjectId id = pObject->m_InternalId;
      if (id.m_InstanceIndex != xiiGameObjectId::INVALID_INSTANCE_INDEX)
        m_Data.m_Objects[id] = pObject;

      // The moved object might be deleted as well so we remove it from the dead objects set instead.
      // If that is not the case we remove the original object from the set.
      if (m_Data.m_DeadObjects.Remove(pMovedObject))
      {
        continue;
      }
    }

    m_Data.m_DeadObjects.Remove(pObject);
  }
}

void xiiWorld::DeleteDeadComponents()
{
  while (!m_Data.m_DeadComponents.IsEmpty())
  {
    xiiComponent* pComponent = m_Data.m_DeadComponents.GetIterator().Key();

    xiiComponentManagerBase* pManager        = pComponent->GetOwningManager();
    xiiComponent*            pMovedComponent = nullptr;
    pManager->DeleteComponentStorage(pComponent, pMovedComponent);

    // another component has been moved to the deleted component location
    if (pComponent != pMovedComponent)
    {
      pManager->PatchIdTable(pComponent);

      if (xiiGameObject* pOwner = pComponent->GetOwner())
      {
        pOwner->FixComponentPointer(pMovedComponent, pComponent);
      }

      // The moved component might be deleted as well so we remove it from the dead components set instead.
      // If that is not the case we remove the original component from the set.
      if (m_Data.m_DeadComponents.Remove(pMovedComponent))
      {
        continue;
      }
    }

    m_Data.m_DeadComponents.Remove(pComponent);
  }
}

void xiiWorld::PatchHierarchyData(xiiGameObject* pObject, xiiGameObject::TransformPreservation preserve)
{
  xiiGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == xiiGameObject::TransformPreservation::PreserveGlobal)
  {
    // SetGlobalTransform will internally trigger bounds update for static objects
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    // Explicitly trigger transform AND bounds update, otherwise bounds would be outdated for static objects
    // Don't call pObject->UpdateGlobalTransformAndBounds() here since that would recursively update the parent global transform which is already up-to-date.
    pObject->m_pTransformationData->UpdateGlobalTransformNonRecursive();

    pObject->m_pTransformationData->UpdateGlobalBounds(GetSpatialSystem());
  }

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    PatchHierarchyData(it, preserve);
  }
  XII_ASSERT_DEBUG(pObject->m_pTransformationData != pObject->m_pTransformationData->m_pParentData, "Hierarchy corrupted!");
}

void xiiWorld::RecreateHierarchyData(xiiGameObject* pObject, bool bWasDynamic)
{
  xiiGameObject* pParent = pObject->GetParent();

  const xiiUInt32 uiNewHierarchyLevel = pParent != nullptr ? pParent->m_uiHierarchyLevel + 1 : 0;
  const xiiUInt32 uiOldHierarchyLevel = pObject->m_uiHierarchyLevel;

  const bool bIsDynamic = pObject->IsDynamic();

  if (uiNewHierarchyLevel != uiOldHierarchyLevel || bIsDynamic != bWasDynamic)
  {
    xiiGameObject::TransformationData* pOldTransformationData = pObject->m_pTransformationData;

    xiiGameObject::TransformationData* pNewTransformationData = m_Data.CreateTransformationData(bIsDynamic, uiNewHierarchyLevel);
    xiiMemoryUtils::Copy(pNewTransformationData, pOldTransformationData, 1);

    pObject->m_uiHierarchyLevel    = static_cast<xiiUInt16>(uiNewHierarchyLevel);
    pObject->m_pTransformationData = pNewTransformationData;

    // fix parent transform data for children as well
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      xiiGameObject::TransformationData* pTransformData = it->m_pTransformationData;
      pTransformData->m_pParentData                     = pNewTransformationData;
    }

    m_Data.DeleteTransformationData(bWasDynamic, uiOldHierarchyLevel, pOldTransformationData);
  }
}

void xiiWorld::SetMaxInitializationTimePerFrame(xiiTime maxInitTime)
{
  CheckForWriteAccess();

  m_Data.m_MaxInitializationTimePerFrame = maxInitTime;
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_World);
