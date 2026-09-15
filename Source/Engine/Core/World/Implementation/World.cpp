/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

static xiiGameObjectHandle DefaultGameObjectReferenceResolver(const void* pData, xiiComponentHandle hThis, xiiStringView sProperty)
{
  XII_IGNORE_UNUSED(hThis);
  XII_IGNORE_UNUSED(sProperty);

  const char* szRef = reinterpret_cast<const char*>(pData);

  if (xiiStringUtils::IsNullOrEmpty(szRef))
    return xiiGameObjectHandle();

  // this is a convention used by xiiPrefabReferenceComponent:
  // a string starting with this means a 'global game object reference', ie a reference that is valid within the current world
  // what follows is an integer that is the internal storage of a xiiGameObjectHandle
  // thus parsing the int and casting it to a xiiGameObjectHandle gives the desired result
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

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiWorld, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(DeleteObjectDelayed, In, "GameObject", In, "DeleteEmptyParents")->AddAttributes(new xiiFunctionArgumentAttributes(1, new xiiDefaultValueAttribute(true))),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_TryGetObjectWithGlobalKey, In, "GlobalKey")->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_GetClock)->AddFlags(xiiPropertyFlags::PureFunction),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SearchForObject, In, "SearchPath", In, "ReferenceObject")->AddFlags(xiiPropertyFlags::PureFunction),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiWorld::xiiWorld(xiiWorldDescription& ref_description) :
  m_Data(ref_description)
{
  m_pUpdateTask                                     = XII_DEFAULT_NEW(xiiDelegateTask<void>, "WorldUpdate", xiiTaskNesting::Never, xiiMakeDelegate(&xiiWorld::UpdateFromThread, this));
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  xiiStringBuilder sb = ref_description.m_sName.GetString();
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
  SetWorldSimulationEnabled(false);

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

  xiiEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(this);
}

void xiiWorld::SetCoordinateSystemProvider(const xiiSharedPtr<xiiCoordinateSystemProvider>& pProvider)
{
  XII_ASSERT_DEV(pProvider != nullptr, "Coordinate System Provider must not be null");

  m_Data.m_pCoordinateSystemProvider                = pProvider;
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;
}

// A very simple, but also efficient random number generator.
inline static xiiUInt32 NextStableRandomSeed(xiiUInt32& ref_uiSeed)
{
  ref_uiSeed = 214013L * ref_uiSeed + 2531011L;
  return ((ref_uiSeed >> 16) & 0x7FFFF);
}

xiiGameObjectHandle xiiWorld::CreateObject(const xiiGameObjectDescription& desc, xiiGameObject*& out_pObject)
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
  pTransformationData->m_pObject         = pNewObject;
  pTransformationData->m_pParentData     = pParentData;
  pTransformationData->m_localPosition   = xiiSimdConversion::ToVec3(desc.m_LocalPosition);
  pTransformationData->m_localRotation   = xiiSimdConversion::ToQuat(desc.m_LocalRotation);
  pTransformationData->m_localScaling    = xiiSimdConversion::ToVec4(desc.m_LocalScaling.GetAsVec4(desc.m_LocalUniformScaling));
  pTransformationData->m_globalTransform = xiiSimdTransform::MakeIdentity();
#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  pTransformationData->m_lastGlobalTransform                = xiiSimdTransform::MakeIdentity();
  pTransformationData->m_uiLastGlobalTransformUpdateCounter = xiiInvalidIndex;
#endif
  pTransformationData->m_localBounds = xiiSimdBBoxSphere::MakeInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(xiiSimdFloat::MakeZero());
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

  pTransformationData->UpdateGlobalTransformNonRecursive(0);

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
  PostMessage(hObject, msg, xiiTime::MakeZero());
}

xiiComponentInitBatchHandle xiiWorld::CreateComponentInitBatch(xiiStringView sBatchName, bool bMustFinishWithinOneFrame /*= true*/)
{
  auto pInitBatch = XII_NEW(GetAllocator(), xiiInternal::WorldData::InitBatch, GetAllocator(), sBatchName, bMustFinishWithinOneFrame);
  return xiiComponentInitBatchHandle(m_Data.m_InitBatches.Insert(pInitBatch));
}

void xiiWorld::DeleteComponentInitBatch(const xiiComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  XII_IGNORE_UNUSED(pInitBatch);
  XII_ASSERT_DEV(pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty(), "Init batch has not been completely processed");
  m_Data.m_InitBatches.Remove(hBatch.GetInternalID());
}

void xiiWorld::BeginAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& hBatch)
{
  XII_ASSERT_DEV(m_Data.m_pCurrentInitBatch == m_Data.m_pDefaultInitBatch, "Nested init batches are not supported");
  m_Data.m_pCurrentInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()].Borrow();
}

void xiiWorld::EndAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& hBatch)
{
  XII_ASSERT_DEV(m_Data.m_InitBatches[hBatch.GetInternalID()] == m_Data.m_pCurrentInitBatch, "Init batch with id {} is currently not active", hBatch.GetInternalID().m_Data);
  XII_IGNORE_UNUSED(hBatch);
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

void xiiWorld::SubmitComponentInitBatch(const xiiComponentInitBatchHandle& hBatch)
{
  m_Data.m_InitBatches[hBatch.GetInternalID()]->m_bIsReady = true;
  m_Data.m_pCurrentInitBatch                               = m_Data.m_pDefaultInitBatch;
}

bool xiiWorld::IsComponentInitBatchCompleted(const xiiComponentInitBatchHandle& hBatch, double* pCompletionFactor /*= nullptr*/)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  XII_ASSERT_DEV(pInitBatch->m_bIsReady, "Batch is not submitted yet");

  if (pCompletionFactor != nullptr)
  {
    if (pInitBatch->m_ComponentsToInitialize.IsEmpty())
    {
      if (m_Data.m_bSimulateWorld)
      {
        double fStartSimCompletion = pInitBatch->m_ComponentsToStartSimulation.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToStartSimulation / pInitBatch->m_ComponentsToStartSimulation.GetCount();
        *pCompletionFactor         = fStartSimCompletion * 0.5 + 0.5;
      }
      else
      {
        *pCompletionFactor = 1.0;

        XII_ASSERT_DEV(m_Data.m_pDefaultInitBatch != pInitBatch, "");

        m_Data.m_pDefaultInitBatch->m_ComponentsToStartSimulation.PushBackRange(pInitBatch->m_ComponentsToStartSimulation);
        pInitBatch->m_ComponentsToStartSimulation.Clear();
        return true;
      }
    }
    else
    {
      double fInitCompletion = pInitBatch->m_ComponentsToInitialize.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToInitialize / pInitBatch->m_ComponentsToInitialize.GetCount();

      if (m_Data.m_bSimulateWorld)
      {
        *pCompletionFactor = fInitCompletion * 0.5;
      }
      else
      {
        *pCompletionFactor = fInitCompletion;
      }
    }
  }

  return pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty();
}

void xiiWorld::CancelComponentInitBatch(const xiiComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
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

  if (m_Data.m_ProcessingMessageQueue == queueType)
  {
    delay = xiiMath::Max(delay, xiiTime::MakeFromMilliseconds(1));
  }

  xiiRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, m_Data.m_LinearAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void xiiWorld::PostMessage(const xiiComponentHandle& hReceiverComponent, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.

  XII_ASSERT_DEBUG((hReceiverComponent.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in component id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = hReceiverComponent.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent       = true;
  metaData.m_uiRecursive                 = false;

  if (m_Data.m_ProcessingMessageQueue == queueType)
  {
    delay = xiiMath::Max(delay, xiiTime::MakeFromMilliseconds(1));
  }

  xiiRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    xiiMessage* pMsgCopy = pMsgRTTIAllocator->Clone<xiiMessage>(&msg, m_Data.m_LinearAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void xiiWorld::FindEventMsgHandlers(const xiiMessage& msg, xiiGameObject* pSearchObject, xiiDynamicArray<xiiComponent*>& out_components)
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void xiiWorld::FindEventMsgHandlers(const xiiMessage& msg, const xiiGameObject* pSearchObject, xiiDynamicArray<const xiiComponent*>& out_components) const
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void xiiWorld::Update()
{
  CheckForWriteAccess();

  XII_LOG_BLOCK(m_Data.m_sName.GetData());

  {
    xiiStringBuilder sStatName;
    sStatName.SetFormat("World Update/{0}/Game Object Count", m_Data.m_sName);

    xiiStringBuilder sStatValue;
    xiiStats::SetStat(sStatName, GetObjectCount());
  }

  ++m_Data.m_uiUpdateCounter;

  if (!m_Data.m_bSimulateWorld)
  {
    // Only change the pause mode temporarily, so that the user's choices do not get overridden.

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

  // Reload Resources.
  {
    XII_PROFILE_SCOPE("Reload Resources");
    ProcessResourceReloadFunctions();
  }

  // Initialization Phase.
  {
    XII_PROFILE_SCOPE("Initialize Phase");
    ProcessComponentsToInitialize();
    ProcessUpdateFunctionsToRegister();

    ProcessQueuedMessages(xiiObjectMsgQueueType::AfterInitialized);
  }

  // Pre-Asynchronous Phase.
  {
    XII_PROFILE_SCOPE("Pre-Async Phase");
    ProcessQueuedMessages(xiiObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[xiiWorldUpdatePhase::PreAsync]);
  }

  // Asynchronous Phase.
  {
    // Remove the write marker, but keep the read marker.
    // Thus, the world cannot be marked for writing, as only reading is permitted in the asynchronous phase.
    m_Data.m_WriteThreadID = (xiiThreadID)0;

    XII_PROFILE_SCOPE("Async Phase");
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = xiiThreadUtils::GetCurrentThreadID();
  }

  // Post-Asynchronous Phase.
  {
    XII_PROFILE_SCOPE("Post-Async Phase");
    ProcessQueuedMessages(xiiObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[xiiWorldUpdatePhase::PostAsync]);
  }

  // Delete dead objects and update the object hierarchy.
  {
    XII_PROFILE_SCOPE("Delete Dead Objects");
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // Update Transforms.
  {
    XII_PROFILE_SCOPE("Update Transforms");
    m_Data.UpdateGlobalTransforms();
  }

  // Post-Transform Phase.
  {
    XII_PROFILE_SCOPE("Post-Transform Phase");
    ProcessQueuedMessages(xiiObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[xiiWorldUpdatePhase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    XII_PROFILE_SCOPE("Initialize Phase 2");
    // Only process the default init batch here since it contains the components created at runtime.
    // Also make sure that all initialization is finished after this call by giving it enough time.
    ProcessInitializationBatch(*m_Data.m_pDefaultInitBatch, xiiTime::Now() + xiiTime::MakeFromHours(10000));

    ProcessQueuedMessages(xiiObjectMsgQueueType::AfterInitialized);
  }

  // Swap our double buffered stack allocator.
  m_Data.m_LinearAllocator.Swap();
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

    if (m_Data.m_bSimulateWorld)
    {
      pModule->OnSimulationStarted();
    }
    else
    {
      m_Data.m_ModulesToStartSimulation.PushBack(pModule);
    }
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

xiiGameObject* xiiWorld::Reflection_TryGetObjectWithGlobalKey(xiiTempHashedString sGlobalKey)
{
  xiiGameObject* pObject = nullptr;
  bool           res     = TryGetObjectWithGlobalKey(sGlobalKey, pObject);
  XII_IGNORE_UNUSED(res);
  return pObject;
}

xiiClock* xiiWorld::Reflection_GetClock()
{
  return &m_Data.m_Clock;
}

void xiiWorld::SetParent(xiiGameObject* pObject, xiiGameObject* pNewParent, xiiTransformPreservation::Enum preserve)
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

    if (pObject->m_Flags.IsSet(xiiObjectFlags::ParentChangesNotifications))
    {
      xiiMsgParentChanged msg;
      msg.m_Type    = xiiMsgParentChanged::Type::ParentLinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

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

    if (pObject->m_Flags.IsSet(xiiObjectFlags::ParentChangesNotifications))
    {
      xiiMsgParentChanged msg;
      msg.m_Type    = xiiMsgParentChanged::Type::ParentUnlinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

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

xiiStringView xiiWorld::GetObjectGlobalKey(const xiiGameObject* pObject) const
{
  const xiiUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  const xiiHashedString* pGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pGlobalKey))
  {
    return pGlobalKey->GetView();
  }

  return {};
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

    m_Data.m_ProcessingMessageQueue = queueType;
    for (xiiUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      ProcessQueuedMessage(queue[i]);

      // no need to deallocate these messages, they are allocated through a frame allocator
    }
    m_Data.m_ProcessingMessageQueue = xiiObjectMsgQueueType::COUNT;

    queue.Clear();
  }

  // timed messages
  {
    xiiInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    const xiiTime now = m_Data.m_Clock.GetAccumulatedTime();

    m_Data.m_ProcessingMessageQueue = queueType;
    while (!queue.IsEmpty())
    {
      auto& entry = queue.Peek();
      if (entry.m_MetaData.m_Due > now)
        break;

      ProcessQueuedMessage(entry);

      XII_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
    m_Data.m_ProcessingMessageQueue = xiiObjectMsgQueueType::COUNT;
  }
}

// static
template <typename World, typename GameObject, typename Component>
void xiiWorld::FindEventMsgHandlers(World& world, const xiiMessage& msg, GameObject pSearchObject, xiiDynamicArray<Component>& out_components)
{
  using EventMessageHandlerComponentType = typename std::conditional<std::is_const<World>::value, const xiiEventMessageHandlerComponent*, xiiEventMessageHandlerComponent*>::type;

  out_components.Clear();

  // walk the graph upwards until an object is found with at least one xiiComponent that handles this type of message
  {
    auto pCurrentObject = pSearchObject;

    while (pCurrentObject != nullptr)
    {
      bool bContinueSearch = true;
      for (auto pComponent : pCurrentObject->GetComponents())
      {
        if constexpr (std::is_const<World>::value == false)
        {
          pComponent->EnsureInitialized();
        }

        if (pComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pComponent);
          bContinueSearch = false;
        }
        else
        {
          if constexpr (std::is_const<World>::value)
          {
            if (pComponent->IsInitialized() == false)
            {
              xiiLog::Warning("Component of type '{}' was not initialized (yet) and thus might have reported an incorrect result in HandlesMessage(). "
                              "To allow this component to be automatically initialized at this point in time call the non-const variant of SendEventMessage.",
                              pComponent->GetDynamicRTTI()->GetTypeName());
            }
          }

          // only continue to search on parent objects if all event handlers on the current object have the "pass through unhandled events" flag set.
          if (auto pEventMessageHandlerComponent = xiiDynamicCast<EventMessageHandlerComponentType>(pComponent))
          {
            bContinueSearch &= pEventMessageHandlerComponent->GetPassThroughUnhandledEvents();
          }
        }
      }

      if (!bContinueSearch)
      {
        // stop searching as we found at least one xiiEventMessageHandlerComponent or one doesn't have the "pass through" flag set.
        return;
      }

      pCurrentObject = pCurrentObject->GetParent();
    }
  }

  // if no components have been found, check all event handler components that are registered as 'global event handlers'
  if (out_components.IsEmpty())
  {
    auto globalEventMessageHandler = xiiEventMessageHandlerComponent::GetAllGlobalEventHandler(&world);
    for (auto hEventMessageHandlerComponent : globalEventMessageHandler)
    {
      EventMessageHandlerComponentType pEventMessageHandlerComponent = nullptr;
      if (world.TryGetComponent(hEventMessageHandlerComponent, pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesMessage(msg))
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

  XII_ASSERT_DEV(desc.m_Phase != xiiWorldUpdatePhase::Async || desc.m_DependsOn.GetCount() == 0, "Asynchronous update functions must not have dependencies");
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

  for (xiiUInt32 phase = xiiWorldUpdatePhase::PreAsync; phase < xiiWorldUpdatePhase::COUNT; ++phase)
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

  xiiDynamicArrayBase<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[xiiWorldUpdatePhase::Async];

  xiiUInt32 uiCurrentTaskIndex = 0;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    xiiWorldModule*          pModule  = static_cast<xiiWorldModule*>(updateFunction.m_Function.GetClassInstance());
    xiiComponentManagerBase* pManager = xiiDynamicCast<xiiComponentManagerBase*>(pModule);

    // A world module can also register functions in the async phase so we want at least one task.
    const xiiUInt32 uiTotalCount  = pManager != nullptr ? pManager->GetComponentCount() : 1;
    const xiiUInt32 uiGranularity = (updateFunction.m_uiAsyncPhaseBatchSize != 0) ? updateFunction.m_uiAsyncPhaseBatchSize : uiTotalCount;

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

  // Ensure that all components that are created during this batch (e.g. from prefabs)
  // will also get initialized within this batch
  m_Data.m_pCurrentInitBatch = &batch;
  XII_SCOPE_EXIT(m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch);

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
      ProcessInitializationBatch(*pInitBatch, xiiTime::Now() + xiiTime::MakeFromHours(10000));
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

  updateFunctions.InsertAt(uiInsertionIndex, newFunction);

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

void xiiWorld::PatchHierarchyData(xiiGameObject* pObject, xiiTransformPreservation::Enum preserve)
{
  xiiGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == xiiTransformPreservation::Enum::PreserveGlobal)
  {
    // SetGlobalTransform will internally trigger bounds update for static objects
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    // Explicitly trigger transform AND bounds update, otherwise bounds would be outdated for static objects
    // Don't call pObject->UpdateGlobalTransformAndBounds() here since that would recursively update the parent global transform which is already up-to-date.
    pObject->m_pTransformationData->UpdateGlobalTransformNonRecursive(GetUpdateCounter());

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

void xiiWorld::ProcessResourceReloadFunctions()
{
  ResourceReloadContext context;
  context.m_pWorld = this;

  for (auto& hResource : m_Data.m_NeedReload)
  {
    if (m_Data.m_ReloadFunctions.TryGetValue(hResource, m_Data.m_TempReloadFunctions))
    {
      for (auto& data : m_Data.m_TempReloadFunctions)
      {
        XII_VERIFY(data.m_hComponent.IsInvalidated() || TryGetComponent(data.m_hComponent, context.m_pComponent), "Reload function called on dead component");
        context.m_pUserData = data.m_pUserData;

        data.m_Func(context);
      }
    }
  }

  m_Data.m_NeedReload.Clear();
}

void xiiWorld::SetMaxInitializationTimePerFrame(xiiTime maxInitTime)
{
  CheckForWriteAccess();

  m_Data.m_MaxInitializationTimePerFrame = maxInitTime;
}

void xiiWorld::SetGameObjectReferenceResolver(const ReferenceResolver& resolver)
{
  m_Data.m_GameObjectReferenceResolver = resolver;
}

const xiiWorld::ReferenceResolver& xiiWorld::GetGameObjectReferenceResolver() const
{
  return m_Data.m_GameObjectReferenceResolver;
}

void xiiWorld::AddResourceReloadFunction(xiiTypelessResourceHandle hResource, xiiComponentHandle hComponent, void* pUserData, ResourceReloadFunc function)
{
  CheckForWriteAccess();

  if (hResource.IsValid() == false)
    return;

  auto& data        = m_Data.m_ReloadFunctions[hResource].ExpandAndGetRef();
  data.m_hComponent = hComponent;
  data.m_pUserData  = pUserData;
  data.m_Func       = function;
}

void xiiWorld::RemoveResourceReloadFunction(xiiTypelessResourceHandle hResource, xiiComponentHandle hComponent, void* pUserData)
{
  CheckForWriteAccess();

  xiiInternal::WorldData::ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_Data.m_ReloadFunctions.TryGetValue(hResource, pReloadFunctions))
  {
    for (xiiUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      auto& data = (*pReloadFunctions)[i];
      if (data.m_hComponent == hComponent && data.m_pUserData == pUserData)
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

xiiGameObject* xiiWorld::SearchForObject(xiiStringView sSearchPath, xiiGameObject* pReferenceObject, const xiiRTTI* pExpectedComponent)
{
  // Possible paths:
  //
  // rel/path
  // ../rel/path
  // ..
  // G:key/rel/path
  // P:parent/rel/path
  // G:key/P:parent/rel/path
  // G:key/../rel/path
  // G:key/..
  // P:parent/../rel/path

  // if the search string starts with "G:", the next part of the path is the global key of an object
  // in this case, this object is not the reference object anymore, instead the object with that global key is the reference object
  if (sSearchPath.TrimWordStart("G:"))
  {
    xiiStringView sGlobalKey;

    if (const char* szSep = sSearchPath.FindSubString("/"))
    {
      sGlobalKey = xiiStringView(sSearchPath.GetStartPointer(), szSep);
      sSearchPath.SetStartPosition(szSep + 1);
    }
    else
    {
      sGlobalKey  = sSearchPath;
      sSearchPath = {};
    }

    if (!TryGetObjectWithGlobalKey(xiiTempHashedString(sGlobalKey), pReferenceObject))
    {
      return nullptr;
    }
  }

  if (pReferenceObject == nullptr)
    return nullptr;

  // if the search string starts with "P:", the next part of the path is an object name of a parent object
  // of the reference object, so we search upwards until we find the object with that name
  if (sSearchPath.TrimWordStart("P:"))
  {
    xiiStringView sParentName;

    if (const char* szSep = sSearchPath.FindSubString("/"))
    {
      sParentName = xiiStringView(sSearchPath.GetStartPointer(), szSep);
      sSearchPath.SetStartPosition(szSep + 1);
    }
    else
    {
      sParentName = sSearchPath;
      sSearchPath = {};
    }

    const xiiTempHashedString sStartName(sParentName);
    while (!pReferenceObject->HasName(sStartName))
    {
      pReferenceObject = pReferenceObject->GetParent();

      if (pReferenceObject == nullptr)
        return nullptr;
    }
  }

  // if the path contains "..", we go up one parent
  // this is only allowed at the start of the relative path section
  while (sSearchPath.TrimWordStart("../") || sSearchPath.TrimWordStart(".."))
  {
    pReferenceObject = pReferenceObject->GetParent();

    if (pReferenceObject == nullptr)
      return nullptr;
  }

  return pReferenceObject->SearchForChildByNameSequence(sSearchPath, pExpectedComponent);
}


const xiiGameObject* xiiWorld::SearchForObject(xiiStringView sSearchPath, const xiiGameObject* pReferenceObject /*= nullptr*/, const xiiRTTI* pExpectedComponent /*= nullptr*/) const
{
  xiiWorld*      pThis = const_cast<xiiWorld*>(this);
  xiiGameObject* pRef  = const_cast<xiiGameObject*>(pReferenceObject);
  return pThis->SearchForObject(sSearchPath, pRef, pExpectedComponent);
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_World);
