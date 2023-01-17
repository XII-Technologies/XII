#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiJoltWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiCVarBool cvar_JoltSimulationPause("Jolt.Simulation.Pause", false, xiiCVarFlags::None, "Pauses the physics simulation.");

#ifdef JPH_DEBUG_RENDERER
xiiCVarBool cvar_JoltDebugDrawConstraints("Jolt.DebugDraw.Constraints", false, xiiCVarFlags::None, "Visualize physics constraints.");
xiiCVarBool cvar_JoltDebugDrawConstraintLimits("Jolt.DebugDraw.ConstraintLimits", false, xiiCVarFlags::None, "Visualize physics constraint limits.");
xiiCVarBool cvar_JoltDebugDrawConstraintFrames("Jolt.DebugDraw.ConstraintFrames", false, xiiCVarFlags::None, "Visualize physics constraint frames.");
xiiCVarBool cvar_JoltDebugDrawBodies("Jolt.DebugDraw.Bodies", false, xiiCVarFlags::None, "Visualize physics bodies.");
#endif

xiiJoltWorldModule::xiiJoltWorldModule(xiiWorld* pWorld) :
  xiiPhysicsWorldModuleInterface(pWorld)
//, m_FreeObjectFilterIDs(xiiJolt::GetSingleton()->GetAllocator()) // could use a proxy allocator to bin those
{
  m_pSimulateTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "", xiiMakeDelegate(&xiiJoltWorldModule::Simulate, this));
  m_pSimulateTask->ConfigureTask("Jolt Simulate", xiiTaskNesting::Maybe);
}

xiiJoltWorldModule::~xiiJoltWorldModule() = default;

class xiiJoltBodyActivationListener : public JPH::BodyActivationListener
{
public:
  virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
  {
    const xiiJoltUserData* pUserData = reinterpret_cast<const xiiJoltUserData*>(inBodyUserData);
    if (xiiJoltActorComponent* pActor = xiiJoltUserData::GetActorComponent(pUserData))
    {
      m_pActiveActors->Insert(pActor, inBodyID.GetIndexAndSequenceNumber());
    }
  }

  virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
  {
    const xiiJoltUserData* pUserData = reinterpret_cast<const xiiJoltUserData*>(inBodyUserData);
    if (xiiJoltActorComponent* pActor = xiiJoltUserData::GetActorComponent(pUserData))
    {
      m_pActiveActors->Remove(pActor);
    }
  }

  xiiMap<xiiJoltActorComponent*, xiiUInt32>* m_pActiveActors = nullptr;
};

class xiiJoltGroupFilter : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& inGroup1, const JPH::CollisionGroup& inGroup2) const override
  {
    const xiiUInt64 id = static_cast<xiiUInt64>(inGroup1.GetGroupID()) << 32 | inGroup2.GetGroupID();

    return !m_IgnoreCollisions.Contains(id);
  }

  xiiHashSet<xiiUInt64> m_IgnoreCollisions;
};

class xiiJoltGroupFilterIgnoreSame : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& inGroup1, const JPH::CollisionGroup& inGroup2) const override
  {
    return inGroup1.GetGroupID() != inGroup2.GetGroupID();
  }
};

void xiiJoltWorldModule::Deinitialize()
{
  m_pSystem        = nullptr;
  m_pTempAllocator = nullptr;

  xiiJoltBodyActivationListener* pActivationListener = reinterpret_cast<xiiJoltBodyActivationListener*>(m_pActivationListener);
  XII_DEFAULT_DELETE(pActivationListener);
  m_pActivationListener = nullptr;

  xiiJoltContactListener* pContactListener = reinterpret_cast<xiiJoltContactListener*>(m_pContactListener);
  XII_DEFAULT_DELETE(pContactListener);
  m_pContactListener = nullptr;

  m_pGroupFilter->Release();
  m_pGroupFilterIgnoreSame->Release();
}

class xiiJoltTempAlloc : public JPH::TempAllocator
{
public:
  xiiJoltTempAlloc(const char* szName) :
    m_ProxyAlloc(szName, xiiFoundation::GetAlignedAllocator())
  {
    AddChunk(0);
    m_uiCurChunkIdx = 0;
  }

  ~xiiJoltTempAlloc()
  {
    for (xiiUInt32 i = 0; i < m_Chunks.GetCount(); ++i)
    {
      ClearChunk(i);
    }
  }

  virtual void* Allocate(JPH::uint inSize) override
  {
    if (inSize == 0)
      return nullptr;

    const xiiUInt32 uiNeeded = xiiMemoryUtils::AlignSize(inSize, 16u);

    while (true)
    {
      const xiiUInt32 uiRemaining = m_Chunks[m_uiCurChunkIdx].m_uiSize - m_Chunks[m_uiCurChunkIdx].m_uiLastOffset;

      if (uiRemaining >= uiNeeded)
        break;

      AddChunk(uiNeeded);
    }

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];

    void* pRes = xiiMemoryUtils::AddByteOffset(lastAlloc.m_pPtr, lastAlloc.m_uiLastOffset);
    lastAlloc.m_uiLastOffset += uiNeeded;
    return pRes;
  }

  virtual void Free(void* inAddress, JPH::uint inSize) override
  {
    if (inAddress == nullptr)
      return;

    const xiiUInt32 uiAllocSize = xiiMemoryUtils::AlignSize(inSize, 16u);

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];
    lastAlloc.m_uiLastOffset -= uiAllocSize;

    if (lastAlloc.m_uiLastOffset == 0 && m_uiCurChunkIdx > 0)
    {
      // move back to the previous chunk
      --m_uiCurChunkIdx;
    }
  }

  struct Chunk
  {
    void*     m_pPtr         = nullptr;
    xiiUInt32 m_uiSize       = 0;
    xiiUInt32 m_uiLastOffset = 0;
  };

  void AddChunk(xiiUInt32 uiSize)
  {
    ++m_uiCurChunkIdx;

    if (m_uiCurChunkIdx < m_Chunks.GetCount())
      return;

    uiSize = xiiMath::Max(uiSize, 1024u * 1024u);

    auto& alloc    = m_Chunks.ExpandAndGetRef();
    alloc.m_pPtr   = XII_NEW_RAW_BUFFER(&m_ProxyAlloc, xiiUInt8, uiSize);
    alloc.m_uiSize = uiSize;
  }

  void ClearChunk(xiiUInt32 uiChunkIdx)
  {
    XII_DELETE_RAW_BUFFER(&m_ProxyAlloc, m_Chunks[uiChunkIdx].m_pPtr);
    m_Chunks[uiChunkIdx].m_pPtr         = nullptr;
    m_Chunks[uiChunkIdx].m_uiSize       = 0;
    m_Chunks[uiChunkIdx].m_uiLastOffset = 0;
  }

  xiiUInt32                 m_uiCurChunkIdx = 0;
  xiiHybridArray<Chunk, 16> m_Chunks;
  xiiProxyAllocator         m_ProxyAlloc;
};


void xiiJoltWorldModule::Initialize()
{
  // TODO: it would be better if this were in OnSimulationStarted() to guarantee that the system is always initialized with the latest values
  // however, that doesn't work because xiiJoltWorldModule is only created by calls to GetOrCreateWorldModule, where Initialize is called, but OnSimulationStarted
  // is queued and executed later

  // ensure the first element is reserved for 'invalid' objects
  m_AllocatedUserData.SetCount(1);

  UpdateSettingsCfg();

  xiiStringBuilder tmp("Jolt-", GetWorld()->GetName());
  m_pTempAllocator = std::make_unique<xiiJoltTempAlloc>(tmp);

  const uint32_t cMaxBodies             = m_Settings.m_uiMaxBodies;
  const uint32_t cMaxContactConstraints = m_Settings.m_uiMaxBodies * 4;
  const uint32_t cMaxBodyPairs          = cMaxContactConstraints * 10;
  const uint32_t cNumBodyMutexes        = 0;

  m_pSystem = std::make_unique<JPH::PhysicsSystem>();
  m_pSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_ObjectToBroadphase, m_ObjectVsBroadphaseFilter, m_ObjectLayerPairFilter);

  {
    xiiJoltBodyActivationListener* pListener = XII_DEFAULT_NEW(xiiJoltBodyActivationListener);
    m_pActivationListener                    = pListener;
    pListener->m_pActiveActors               = &m_ActiveActors;
    m_pSystem->SetBodyActivationListener(pListener);
  }

  {
    xiiJoltContactListener* pListener = XII_DEFAULT_NEW(xiiJoltContactListener);
    pListener->m_pWorld               = GetWorld();
    m_pContactListener                = pListener;
    m_pSystem->SetContactListener(pListener);
  }

  {
    m_pGroupFilter = new xiiJoltGroupFilter();
    m_pGroupFilter->AddRef();
  }

  {
    m_pGroupFilterIgnoreSame = new xiiJoltGroupFilterIgnoreSame();
    m_pGroupFilterIgnoreSame->AddRef();
  }
}

void xiiJoltWorldModule::OnSimulationStarted()
{
  {
    auto startSimDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltWorldModule::StartSimulation, this);
    startSimDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }

  xiiJoltCollisionFiltering::LoadCollisionFilters();

  UpdateSettingsCfg();
  ApplySettingsCfg();

  m_AccumulatedTimeSinceUpdate.SetZero();
}

xiiUInt32 xiiJoltWorldModule::CreateObjectFilterID()
{
  if (!m_FreeObjectFilterIDs.IsEmpty())
  {
    xiiUInt32 uiObjectFilterID = m_FreeObjectFilterIDs.PeekBack();
    m_FreeObjectFilterIDs.PopBack();

    return uiObjectFilterID;
  }

  return m_uiNextObjectFilterID++;
}

void xiiJoltWorldModule::DeleteObjectFilterID(xiiUInt32& uiObjectFilterID)
{
  if (uiObjectFilterID == xiiInvalidIndex)
    return;

  m_FreeObjectFilterIDs.PushBack(uiObjectFilterID);

  uiObjectFilterID = xiiInvalidIndex;
}

xiiUInt32 xiiJoltWorldModule::AllocateUserData(xiiJoltUserData*& out_pUserData)
{
  if (!m_FreeUserData.IsEmpty())
  {
    xiiUInt32 uiIndex = m_FreeUserData.PeekBack();
    m_FreeUserData.PopBack();

    out_pUserData = &m_AllocatedUserData[uiIndex];
    return uiIndex;
  }

  out_pUserData = &m_AllocatedUserData.ExpandAndGetRef();
  return m_AllocatedUserData.GetCount() - 1;
}

void xiiJoltWorldModule::DeallocateUserData(xiiUInt32& uiUserDataId)
{
  if (uiUserDataId == xiiInvalidIndex)
    return;

  m_AllocatedUserData[uiUserDataId].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(uiUserDataId);

  uiUserDataId = xiiInvalidIndex;
}

const xiiJoltUserData& xiiJoltWorldModule::GetUserData(xiiUInt32 uiUserDataId) const
{
  XII_ASSERT_DEBUG(uiUserDataId != xiiInvalidIndex, "Invalid xiiJoltUserData ID");

  return m_AllocatedUserData[uiUserDataId];
}

void xiiJoltWorldModule::SetGravity(const xiiVec3& objectGravity, const xiiVec3& characterGravity)
{
  m_Settings.m_vObjectGravity    = objectGravity;
  m_Settings.m_vCharacterGravity = characterGravity;

  if (m_pSystem)
  {
    m_pSystem->SetGravity(xiiJoltConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

void xiiJoltWorldModule::AddStaticCollisionBox(xiiGameObject* pObject, xiiVec3 boxSize)
{
  xiiJoltStaticActorComponent* pActor = nullptr;
  xiiJoltStaticActorComponent::CreateComponent(pObject, pActor);

  xiiJoltShapeBoxComponent* pBox;
  xiiJoltShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetHalfExtents(boxSize * 0.5f);
}

void xiiJoltWorldModule::QueueBodyToAdd(JPH::Body* pBody, bool bAwake)
{
  if (bAwake)
    m_BodiesToAddAndActivate.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
  else
    m_BodiesToAdd.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
}

void xiiJoltWorldModule::EnableJoinedBodiesCollisions(xiiUInt32 uiObjectFilterID1, xiiUInt32 uiObjectFilterID2, bool bEnable)
{
  xiiJoltGroupFilter* pFilter = static_cast<xiiJoltGroupFilter*>(m_pGroupFilter);

  const xiiUInt64 uiMask1 = static_cast<xiiUInt64>(uiObjectFilterID1) << 32 | uiObjectFilterID2;
  const xiiUInt64 uiMask2 = static_cast<xiiUInt64>(uiObjectFilterID2) << 32 | uiObjectFilterID1;

  if (bEnable)
  {
    pFilter->m_IgnoreCollisions.Remove(uiMask1);
    pFilter->m_IgnoreCollisions.Remove(uiMask2);
  }
  else
  {
    pFilter->m_IgnoreCollisions.Insert(uiMask1);
    pFilter->m_IgnoreCollisions.Insert(uiMask2);
  }
}

void xiiJoltWorldModule::ActivateCharacterController(xiiJoltCharacterControllerComponent* pCharacter, bool bActivate)
{
  if (bActivate)
  {
    XII_ASSERT_DEBUG(!m_ActiveCharacters.Contains(pCharacter), "xiiJoltCharacterControllerComponent was activated more than once.");

    m_ActiveCharacters.PushBack(pCharacter);
  }
  else
  {
    if (!m_ActiveCharacters.RemoveAndSwap(pCharacter))
    {
      XII_ASSERT_DEBUG(false, "xiiJoltCharacterControllerComponent was deactivated more than once.");
    }
  }
}

void xiiJoltWorldModule::FreeUserDataAfterSimulationStep()
{
  m_FreeUserData.PushBackRange(m_FreeUserDataAfterSimulationStep);
  m_FreeUserDataAfterSimulationStep.Clear();
}

void xiiJoltWorldModule::StartSimulation(const xiiWorldModule::UpdateContext& context)
{
  if (cvar_JoltSimulationPause)
    return;

  if (!m_BodiesToAdd.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAdd.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(xiiUInt32));

    xiiUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAdd.GetCount())
    {
      const xiiUInt32 uiCount = m_BodiesToAdd.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAdd[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::DontActivate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAdd.Clear();
  }

  if (!m_BodiesToAddAndActivate.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAddAndActivate.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(xiiUInt32));

    xiiUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAddAndActivate.GetCount())
    {
      const xiiUInt32 uiCount = m_BodiesToAddAndActivate.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAddAndActivate[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::Activate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAddAndActivate.Clear();
  }

  if (m_uiBodiesAddedSinceOptimize > 128)
  {
    // TODO: not clear whether this could be multi-threaded or done more efficiently somehow
    m_pSystem->OptimizeBroadPhase();
    m_uiBodiesAddedSinceOptimize = 0;
  }

  UpdateSettingsCfg();

  m_SimulatedTimeStep = CalculateUpdateSteps();

  if (m_UpdateSteps.IsEmpty())
    return;

  if (xiiJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<xiiJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateKinematicActors(m_SimulatedTimeStep);
  }

  if (xiiJoltQueryShapeActorComponentManager* pQueryShapesManager = GetWorld()->GetComponentManager<xiiJoltQueryShapeActorComponentManager>())
  {
    pQueryShapesManager->UpdateMovingQueryShapes();
  }

  if (xiiJoltTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<xiiJoltTriggerComponentManager>())
  {
    pTriggerManager->UpdateMovingTriggers();
  }

  UpdateConstraints();

  m_SimulateTaskGroupId = xiiTaskSystem::StartSingleTask(m_pSimulateTask, xiiTaskPriority::EarlyThisFrame);
}

void xiiJoltWorldModule::FetchResults(const xiiWorldModule::UpdateContext& context)
{
  XII_PROFILE_SCOPE("FetchResults");

  {
    XII_PROFILE_SCOPE("Wait for Simulate Task");
    xiiTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

#ifdef JPH_DEBUG_RENDERER
  if (cvar_JoltDebugDrawConstraints)
    m_pSystem->DrawConstraints(xiiJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintLimits)
    m_pSystem->DrawConstraintLimits(xiiJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintFrames)
    m_pSystem->DrawConstraintReferenceFrame(xiiJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawBodies)
  {
    JPH::BodyManager::DrawSettings opt;
    opt.mDrawShape          = true;
    opt.mDrawShapeWireframe = true;
    m_pSystem->DrawBodies(opt, xiiJoltCore::s_pDebugRenderer.get());
  }

  xiiJoltCore::DebugDraw(GetWorld());
#endif

  // Nothing to fetch if no simulation step was executed
  if (m_UpdateSteps.IsEmpty())
    return;

  if (xiiJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<xiiJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateDynamicActors();
  }

  for (auto pCharacter : m_ActiveCharacters)
  {
    pCharacter->Update(m_SimulatedTimeStep);
  }

  if (xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView, GetWorld()))
  {
    reinterpret_cast<xiiJoltContactListener*>(m_pContactListener)->m_ContactEvents.m_vMainCameraPosition = pView->GetCamera()->GetPosition();
  }

  reinterpret_cast<xiiJoltContactListener*>(m_pContactListener)->m_ContactEvents.SpawnPhysicsImpactReactions();
  reinterpret_cast<xiiJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsSlideReactions();
  reinterpret_cast<xiiJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsRollReactions();

  //  HandleBrokenConstraints();

  FreeUserDataAfterSimulationStep();
}

// void xiiJoltWorldModule::HandleBrokenConstraints()
//{
//   XII_PROFILE_SCOPE("HandleBrokenConstraints");
//
//   for (auto pConstraint : m_pSimulationEventCallback->m_BrokenConstraints)
//   {
//     auto it = m_BreakableJoints.Find(pConstraint);
//     if (it.IsValid())
//     {
//       xiiJoltConstraintComponent* pJoint = nullptr;
//
//       if (m_pWorld->TryGetComponent(it.Value(), pJoint))
//       {
//         xiiMsgPhysicsJointBroke msg;
//         msg.m_hJointObject = pJoint->GetOwner()->GetHandle();
//
//         pJoint->GetOwner()->PostEventMessage(msg, pJoint, xiiTime::Zero());
//       }
//
//       // it can't break twice
//       m_BreakableJoints.Remove(it);
//     }
//   }
//
//   m_pSimulationEventCallback->m_BrokenConstraints.Clear();
// }

xiiTime xiiJoltWorldModule::CalculateUpdateSteps()
{
  xiiTime tSimulatedTimeStep = xiiTime::Zero();
  m_AccumulatedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();
  m_UpdateSteps.Clear();

  if (m_Settings.m_SteppingMode == xiiJoltSteppingMode::Variable)
  {
    // always do a single step with the entire time
    m_UpdateSteps.PushBack(m_AccumulatedTimeSinceUpdate);

    tSimulatedTimeStep = m_AccumulatedTimeSinceUpdate;
    m_AccumulatedTimeSinceUpdate.SetZero();
  }
  else if (m_Settings.m_SteppingMode == xiiJoltSteppingMode::Fixed)
  {
    const xiiTime tFixedStep = xiiTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);

    xiiUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      m_UpdateSteps.PushBack(tFixedStep);
      ++uiNumSubSteps;

      tSimulatedTimeStep += tFixedStep;
      m_AccumulatedTimeSinceUpdate -= tFixedStep;
    }
  }
  else if (m_Settings.m_SteppingMode == xiiJoltSteppingMode::SemiFixed)
  {
    xiiTime       tFixedStep = xiiTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);
    const xiiTime tMinStep   = tFixedStep * 0.25;

    if (tFixedStep * m_Settings.m_uiMaxSubSteps < m_AccumulatedTimeSinceUpdate) // in case too much time has passed
    {
      // if taking N steps isn't sufficient to catch up to the passed time, increase the fixed time step accordingly
      tFixedStep = m_AccumulatedTimeSinceUpdate / (double)m_Settings.m_uiMaxSubSteps;
    }

    while (m_AccumulatedTimeSinceUpdate > tMinStep)
    {
      // prefer fixed time steps
      // but if at the end there is still more than tMinStep time left, do another step with the remaining time
      const xiiTime tDeltaTime = xiiMath::Min(tFixedStep, m_AccumulatedTimeSinceUpdate);

      m_UpdateSteps.PushBack(tDeltaTime);

      tSimulatedTimeStep += tDeltaTime;
      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }

  return tSimulatedTimeStep;
}

void xiiJoltWorldModule::Simulate()
{
  if (m_UpdateSteps.IsEmpty())
    return;

  XII_PROFILE_SCOPE("Physics Simulation");

  xiiTime   tDelta  = m_UpdateSteps[0];
  xiiUInt32 uiSteps = 1;

  for (xiiUInt32 i = 1; i < m_UpdateSteps.GetCount(); ++i)
  {
    XII_PROFILE_SCOPE("Physics Sim Step");

    if (m_UpdateSteps[i] == tDelta)
    {
      ++uiSteps;
    }
    else
    {
      // do a single Update call with multiple sub-steps, if possible
      // this saves a bit of time compared to just doing multiple Update calls

      m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, 1, m_pTempAllocator.get(), xiiJoltCore::GetJoltJobSystem());

      tDelta  = m_UpdateSteps[i];
      uiSteps = 1;
    }
  }

  m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, 1, m_pTempAllocator.get(), xiiJoltCore::GetJoltJobSystem());
}

void xiiJoltWorldModule::UpdateSettingsCfg()
{
  if (xiiJoltSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<xiiJoltSettingsComponentManager>())
  {
    xiiJoltSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();

    if (pSettings != nullptr && pSettings->IsModified())
    {
      m_Settings = pSettings->GetSettings();
      pSettings->ResetModified();

      ApplySettingsCfg();
    }
  }
}

void xiiJoltWorldModule::ApplySettingsCfg()
{
  SetGravity(m_Settings.m_vObjectGravity, m_Settings.m_vCharacterGravity);
}

void xiiJoltWorldModule::UpdateConstraints()
{
  if (m_RequireUpdate.IsEmpty())
    return;

  xiiJoltConstraintComponent* pComponent;
  for (auto& hComponent : m_RequireUpdate)
  {
    if (this->m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      pComponent->ApplySettings();
    }
  }

  m_RequireUpdate.Clear();
}
