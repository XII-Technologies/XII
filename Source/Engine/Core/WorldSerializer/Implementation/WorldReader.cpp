#include <Core/CorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/Progress.h>

xiiWorldReader::FindComponentTypeCallback xiiWorldReader::s_FindComponentTypeCallback;

thread_local xiiWorldReader::InstantiationContextBase* tl_pReaderContext = nullptr;

xiiWorldReader::xiiWorldReader()  = default;
xiiWorldReader::~xiiWorldReader() = default;

xiiResult xiiWorldReader::ReadWorldDescription(xiiStreamReader& ref_stream, bool bWarningOnUnknownSkip)
{
  m_pReadStream = &ref_stream;

  m_uiVersion = 0;
  ref_stream >> m_uiVersion;

  if (m_uiVersion < 8 || m_uiVersion > 10)
  {
    xiiLog::Error("Invalid world version (got {}).", m_uiVersion);
    return XII_FAILURE;
  }

  // destroy old context first
  m_pStringDedupReadContext = nullptr;
  m_pStringDedupReadContext = XII_DEFAULT_NEW(xiiStringDeduplicationReadContext, ref_stream);

  if (m_uiVersion == 8)
  {
    // add tags from the stream
    XII_SUCCEED_OR_RETURN(xiiTagRegistry::GetGlobalRegistry().Load(ref_stream));
  }

  xiiUInt32 uiNumRootObjects = 0;
  ref_stream >> uiNumRootObjects;

  xiiUInt32 uiNumChildObjects = 0;
  ref_stream >> uiNumChildObjects;

  xiiUInt32 uiNumComponentTypes = 0;
  ref_stream >> uiNumComponentTypes;

  if (uiNumComponentTypes > xiiMath::MaxValue<xiiUInt16>())
  {
    xiiLog::Error("World description has too many component types, got {0} - maximum allowed are {1}", uiNumComponentTypes, xiiMath::MaxValue<xiiUInt16>());
    return XII_FAILURE;
  }

  m_RootObjectsToCreate.Reserve(uiNumRootObjects);
  m_ChildObjectsToCreate.Reserve(uiNumChildObjects);

  for (xiiUInt32 i = 0; i < uiNumRootObjects; ++i)
  {
    ReadGameObjectDesc(m_RootObjectsToCreate.ExpandAndGetRef());
  }

  for (xiiUInt32 i = 0; i < uiNumChildObjects; ++i)
  {
    ReadGameObjectDesc(m_ChildObjectsToCreate.ExpandAndGetRef());
  }

  m_ComponentTypes.SetCount(uiNumComponentTypes);
  m_ComponentTypeVersions.Reserve(uiNumComponentTypes);
  for (xiiUInt32 i = 0; i < uiNumComponentTypes; ++i)
  {
    ReadComponentTypeInfo(i);
  }

  // read all component data
  ReadComponentDataToMemStream(bWarningOnUnknownSkip);
  m_pStringDedupReadContext->SetActive(false);

  return XII_SUCCESS;
}

xiiUniquePtr<xiiWorldReader::InstantiationContextBase> xiiWorldReader::InstantiateWorld(xiiWorld& ref_world, const xiiUInt16* pOverrideTeamID, xiiTime maxStepTime, xiiProgress* pProgress)
{
  xiiPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = pOverrideTeamID;
  options.m_MaxStepTime     = maxStepTime;
  options.m_pProgress       = pProgress;
  options.m_RandomSeedMode  = xiiPrefabInstantiationOptions::RandomSeedMode::FixedFromSerialization;

  return Instantiate(ref_world, false, xiiTransform(), options);
}

xiiUniquePtr<xiiWorldReader::InstantiationContextBase> xiiWorldReader::InstantiatePrefab(xiiWorld& ref_world, const xiiTransform& rootTransform, const xiiPrefabInstantiationOptions& options)
{
  return Instantiate(ref_world, true, rootTransform, options);
}

xiiStreamReader& xiiWorldReader::GetStream() const
{
  xiiWorldReader::InstantiationContext* pContext = ((xiiWorldReader::InstantiationContext*)tl_pReaderContext);

  return pContext->m_CurrentReader;
}

xiiGameObjectHandle xiiWorldReader::ReadGameObjectHandle()
{
  xiiWorldReader::InstantiationContext* pContext = ((xiiWorldReader::InstantiationContext*)tl_pReaderContext);

  xiiUInt32 idx = 0;
  pContext->m_CurrentReader >> idx;

  return pContext->m_IndexToGameObjectHandle[idx];
}

void xiiWorldReader::ReadComponentHandle(xiiComponentHandle& out_hComponent)
{
  xiiWorldReader::InstantiationContext* pContext = ((xiiWorldReader::InstantiationContext*)tl_pReaderContext);

  xiiUInt16 uiTypeIndex = 0;
  xiiUInt32 uiIndex     = 0;

  pContext->m_CurrentReader >> uiTypeIndex;
  pContext->m_CurrentReader >> uiIndex;

  out_hComponent.Invalidate();

  if (uiTypeIndex < m_ComponentTypes.GetCount())
  {
    auto& indexToHandle = pContext->m_ComponentTypeStates[uiTypeIndex].m_ComponentIndexToHandle;
    if (uiIndex < indexToHandle.GetCount())
    {
      out_hComponent = indexToHandle[uiIndex];
    }
  }
}

xiiUInt32 xiiWorldReader::GetComponentTypeVersion(const xiiRTTI* pRtti) const
{
  xiiUInt32 uiVersion = 0xFFFFFFFF;
  m_ComponentTypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}

bool xiiWorldReader::HasComponentOfType(const xiiRTTI* pRtti) const
{
  return m_ComponentTypeVersions.Contains(pRtti);
}

void xiiWorldReader::ClearAndCompact()
{
  // m_IndexToGameObjectHandle.Clear();
  // m_IndexToGameObjectHandle.Compact();

  m_RootObjectsToCreate.Clear();
  m_RootObjectsToCreate.Compact();

  m_ChildObjectsToCreate.Clear();
  m_ChildObjectsToCreate.Compact();

  m_ComponentTypes.Clear();
  m_ComponentTypes.Compact();

  m_ComponentTypeVersions.Clear();
  m_ComponentTypeVersions.Compact();

  m_ComponentCreationStream.Clear();
  m_ComponentCreationStream.Compact();

  m_ComponentDataStream.Clear();
  m_ComponentDataStream.Compact();
}

xiiUInt64 xiiWorldReader::GetHeapMemoryUsage() const
{
  return /*m_IndexToGameObjectHandle.GetHeapMemoryUsage() +*/ m_RootObjectsToCreate.GetHeapMemoryUsage() + m_ChildObjectsToCreate.GetHeapMemoryUsage() + m_ComponentTypes.GetHeapMemoryUsage() + m_ComponentTypeVersions.GetHeapMemoryUsage() + m_ComponentCreationStream.GetHeapMemoryUsage() + m_ComponentDataStream.GetHeapMemoryUsage();
}

xiiUInt32 xiiWorldReader::GetRootObjectCount() const
{
  return m_RootObjectsToCreate.GetCount();
}


xiiUInt32 xiiWorldReader::GetChildObjectCount() const
{
  return m_ChildObjectsToCreate.GetCount();
}

void xiiWorldReader::SetMaxStepTime(InstantiationContextBase* pContext, xiiTime maxStepTime)
{
  return static_cast<InstantiationContext*>(pContext)->SetMaxStepTime(maxStepTime);
}

xiiTime xiiWorldReader::GetMaxStepTime(InstantiationContextBase* pContext)
{
  return static_cast<InstantiationContext*>(pContext)->GetMaxStepTime();
}

void xiiWorldReader::ReadGameObjectDesc(GameObjectToCreate& godesc)
{
  xiiGameObjectDesc& desc = godesc.m_Desc;
  xiiStringBuilder   sName, sGlobalKey;

  *m_pReadStream >> godesc.m_uiParentHandleIdx;
  *m_pReadStream >> sName;

  *m_pReadStream >> sGlobalKey;
  godesc.m_sGlobalKey = sGlobalKey;

  *m_pReadStream >> desc.m_LocalPosition;
  *m_pReadStream >> desc.m_LocalRotation;
  *m_pReadStream >> desc.m_LocalScaling;
  *m_pReadStream >> desc.m_LocalUniformScaling;

  *m_pReadStream >> desc.m_bActiveFlag;
  *m_pReadStream >> desc.m_bDynamic;

  desc.m_Tags.Load(*m_pReadStream, xiiTagRegistry::GetGlobalRegistry());

  *m_pReadStream >> desc.m_uiTeamID;

  desc.m_sName.Assign(sName.GetData());

  if (m_uiVersion >= 10)
  {
    *m_pReadStream >> desc.m_uiStableRandomSeed;
  }
}

void xiiWorldReader::ReadComponentTypeInfo(xiiUInt32 uiComponentTypeIdx)
{
  xiiStreamReader& s = *m_pReadStream;

  xiiStringBuilder sRttiName;
  xiiUInt32        uiRttiVersion = 0;

  s >> sRttiName;
  s >> uiRttiVersion;

  const xiiRTTI* pRtti = nullptr;

  if (s_FindComponentTypeCallback.IsValid())
  {
    pRtti = s_FindComponentTypeCallback(sRttiName);
  }
  else
  {
    pRtti = xiiRTTI::FindTypeByName(sRttiName);

    if (pRtti == nullptr)
    {
      xiiLog::Error("Unknown component type '{0}'. Components of this type will be skipped.", sRttiName);
    }
  }

  m_ComponentTypes[uiComponentTypeIdx].m_pRtti = pRtti;
  m_ComponentTypeVersions[pRtti]               = uiRttiVersion;
}

void xiiWorldReader::ReadComponentDataToMemStream(bool warningOnUnknownSkip)
{
  auto WriteToMemStream = [&](xiiMemoryStreamWriter& ref_writer, bool bReadNumComponents) {
    xiiUInt8 Temp[4096];
    for (auto& compTypeInfo : m_ComponentTypes)
    {
      xiiUInt32 uiAllComponentsSize = 0;
      *m_pReadStream >> uiAllComponentsSize;

      if (compTypeInfo.m_pRtti == nullptr)
      {
        if (warningOnUnknownSkip)
        {
          xiiLog::Warning("Skipping components of unknown type");
        }

        m_pReadStream->SkipBytes(uiAllComponentsSize);
      }
      else
      {
        if (bReadNumComponents)
        {
          *m_pReadStream >> compTypeInfo.m_uiNumComponents;
          uiAllComponentsSize -= sizeof(xiiUInt32);

          m_uiTotalNumComponents += compTypeInfo.m_uiNumComponents;
        }

        compTypeInfo.m_uiComponentDataSize = uiAllComponentsSize;

        while (uiAllComponentsSize > 0)
        {
          const xiiUInt64 uiRead = m_pReadStream->ReadBytes(Temp, xiiMath::Min<xiiUInt32>(uiAllComponentsSize, XII_ARRAY_SIZE(Temp)));

          ref_writer.WriteBytes(Temp, uiRead).IgnoreResult();

          uiAllComponentsSize -= (xiiUInt32)uiRead;
        }
      }
    }
  };

  {
    xiiMemoryStreamWriter writer(&m_ComponentCreationStream);
    WriteToMemStream(writer, true);
  }

  {
    xiiMemoryStreamWriter writer(&m_ComponentDataStream);
    WriteToMemStream(writer, false);
  }
}

xiiUniquePtr<xiiWorldReader::InstantiationContextBase> xiiWorldReader::Instantiate(xiiWorld& world, bool bUseTransform, const xiiTransform& rootTransform, const xiiPrefabInstantiationOptions& options)
{
  if (options.m_MaxStepTime <= xiiTime::MakeZero())
  {
    InstantiationContext context = InstantiationContext(*this, &world, bUseTransform, rootTransform, options);

    XII_VERIFY(context.Step() == InstantiationContextBase::StepResult::Finished, "Instantiation should be completed after this call");
    return nullptr;
  }

  xiiUniquePtr<InstantiationContext> pContext = XII_DEFAULT_NEW(InstantiationContext, *this, &world, bUseTransform, rootTransform, options);

  return std::move(pContext);
}

xiiWorldReader::InstantiationContext::InstantiationContext(xiiWorldReader& ref_worldReader, xiiWorld* pWorld, bool bUseTransform, const xiiTransform& rootTransform, const xiiPrefabInstantiationOptions& options) :
  m_WorldReader(ref_worldReader), m_bUseTransform(bUseTransform), m_RootTransform(rootTransform), m_Options(options)
{
  m_Phase = Phase::CreateRootObjects;

  m_pWorld = pWorld;

  m_IndexToGameObjectHandle.PushBack(xiiGameObjectHandle());

  m_ComponentTypeStates.SetCount(ref_worldReader.m_ComponentTypes.GetCount());
  for (auto& ct : m_ComponentTypeStates)
  {
    ct.m_ComponentIndexToHandle.PushBack(xiiComponentHandle());
  }

  if (m_Options.m_MaxStepTime.IsZeroOrNegative())
  {
    m_Options.m_MaxStepTime = xiiTime::MakeFromHours(24 * 365);
  }

  if (options.m_MaxStepTime.IsPositive())
  {
    m_hComponentInitBatch = m_pWorld->CreateComponentInitBatch("WorldReaderBatch", options.m_MaxStepTime.IsPositive() ? false : true);
  }

  if (options.m_pProgress != nullptr)
  {
    m_pOverallProgressRange = XII_DEFAULT_NEW(xiiProgressRange, "Instantiate", Phase::Count, false, options.m_pProgress);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateRootObjects, m_WorldReader.m_RootObjectsToCreate.GetCount() / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateChildObjects, m_WorldReader.m_ChildObjectsToCreate.GetCount() / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateComponents, m_WorldReader.m_uiTotalNumComponents / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::DeserializeComponents, m_WorldReader.m_uiTotalNumComponents / 100.0f);
    // Ten times more weight since init components takes way longer than the rest
    m_pOverallProgressRange->SetStepWeighting(Phase::InitComponents, m_WorldReader.m_uiTotalNumComponents / 10.0f);

    m_pOverallProgressRange->BeginNextStep("CreateRootObjects");
  }
}

xiiWorldReader::InstantiationContext::~InstantiationContext()
{
  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->DeleteComponentInitBatch(m_hComponentInitBatch);
    m_hComponentInitBatch.Invalidate();
  }
}

xiiWorldReader::InstantiationContext::StepResult xiiWorldReader::InstantiationContext::Step()
{
  XII_ASSERT_DEV(m_Phase != Phase::Invalid, "InstantiationContext cannot be re-used.");

  XII_PROFILE_SCOPE("xiiWorldReader::InstContext::Step");

  XII_LOCK(m_pWorld->GetWriteMarker());

  xiiTime endTime = xiiTime::Now() + m_Options.m_MaxStepTime;

  if (m_Phase == Phase::CreateRootObjects)
  {
    if (!m_Options.m_ReplaceNamedRootWithParent.IsEmpty())
    {
      XII_ASSERT_DEBUG(!m_Options.m_hParent.IsInvalidated(), "Parent must be provided when m_ReplaceNamedRootWithParent is specified.");

      if (m_WorldReader.m_RootObjectsToCreate.GetCount() == 1 && m_WorldReader.m_RootObjectsToCreate[0].m_Desc.m_sName == m_Options.m_ReplaceNamedRootWithParent)
      {
        m_uiCurrentIndex = 1;
        m_IndexToGameObjectHandle.PushBack(m_Options.m_hParent);

        xiiGameObject* pParent = nullptr;
        if (m_pWorld->TryGetObject(m_Options.m_hParent, pParent))
        {
          if (m_Options.m_pCreatedRootObjectsOut)
          {
            m_Options.m_pCreatedRootObjectsOut->PushBack(pParent);
          }

          if (m_WorldReader.m_RootObjectsToCreate[0].m_Desc.m_bDynamic)
          {
            pParent->MakeDynamic();
          }
        }
      }
    }

    if (m_bUseTransform)
    {
      if (!CreateGameObjects<true>(m_WorldReader.m_RootObjectsToCreate, m_Options.m_hParent, m_Options.m_pCreatedRootObjectsOut, endTime))
        return StepResult::Continue;
    }
    else
    {
      if (!CreateGameObjects<false>(m_WorldReader.m_RootObjectsToCreate, m_Options.m_hParent, m_Options.m_pCreatedRootObjectsOut, endTime))
        return StepResult::Continue;
    }

    m_Phase = Phase::CreateChildObjects;
    BeginNextProgressStep("CreateChildObjects");
  }

  if (m_Phase == Phase::CreateChildObjects)
  {
    if (!CreateGameObjects<false>(m_WorldReader.m_ChildObjectsToCreate, xiiGameObjectHandle(), m_Options.m_pCreatedChildObjectsOut, endTime))
      return StepResult::Continue;

    m_CurrentReader.SetStorage(&m_WorldReader.m_ComponentCreationStream);
    m_Phase = Phase::CreateComponents;
    BeginNextProgressStep("CreateComponents");
  }

  if (m_Phase == Phase::CreateComponents)
  {
    if (m_WorldReader.m_ComponentCreationStream.GetStorageSize64() > 0)
    {
      m_WorldReader.m_pStringDedupReadContext->SetActive(true);
      tl_pReaderContext = this;

      // xiiStreamReader* pPrevReader = m_WorldReader.m_pStream;
      // m_WorldReader.m_pStream = &m_CurrentReader;

      XII_SCOPE_EXIT(/*m_WorldReader.m_pStream = pPrevReader; */ m_WorldReader.m_pStringDedupReadContext->SetActive(false); tl_pReaderContext = nullptr;);

      if (!CreateComponents(endTime))
        return StepResult::Continue;
    }

    m_CurrentReader.SetStorage(&m_WorldReader.m_ComponentDataStream);
    m_Phase = Phase::DeserializeComponents;
    BeginNextProgressStep("DeserializeComponents");
  }

  if (m_Phase == Phase::DeserializeComponents)
  {
    if (m_WorldReader.m_ComponentDataStream.GetStorageSize64() > 0)
    {
      m_WorldReader.m_pStringDedupReadContext->SetActive(true);
      tl_pReaderContext = this;

      // xiiStreamReader* pPrevReader = m_WorldReader.m_pStream;
      // m_WorldReader.m_pStream = &m_CurrentReader;

      XII_SCOPE_EXIT(/*m_WorldReader.m_pStream = pPrevReader;*/ m_WorldReader.m_pStringDedupReadContext->SetActive(false); tl_pReaderContext = nullptr;);

      if (!DeserializeComponents(endTime))
        return StepResult::Continue;
    }

    m_CurrentReader.SetStorage(nullptr);
    m_Phase = Phase::AddComponentsToBatch;
    BeginNextProgressStep("AddComponentsToBatch");
  }

  if (m_Phase == Phase::AddComponentsToBatch)
  {
    if (!AddComponentsToBatch(endTime))
      return StepResult::Continue;

    m_Phase = Phase::InitComponents;
    BeginNextProgressStep("InitComponents");
  }

  if (m_Phase == Phase::InitComponents)
  {
    if (!m_hComponentInitBatch.IsInvalidated())
    {
      double fCompletionFactor = 0.0;
      if (!m_pWorld->IsComponentInitBatchCompleted(m_hComponentInitBatch, &fCompletionFactor))
      {
        SetSubProgressCompletion(fCompletionFactor);
        return StepResult::ContinueNextFrame;
      }
    }

    m_Phase                 = Phase::Invalid;
    m_pSubProgressRange     = nullptr;
    m_pOverallProgressRange = nullptr;
  }

  return StepResult::Finished;
}

void xiiWorldReader::InstantiationContext::Cancel()
{
  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->CancelComponentInitBatch(m_hComponentInitBatch);
  }

  m_Phase                 = Phase::Invalid;
  m_pSubProgressRange     = nullptr;
  m_pOverallProgressRange = nullptr;
}

// a super simple, but also efficient random number generator
inline static xiiUInt32 NextStableRandomSeed(xiiUInt32& ref_uiSeed)
{
  ref_uiSeed = 214013L * ref_uiSeed + 2531011L;
  return ((ref_uiSeed >> 16) & 0x7FFFF);
}

template <bool UseTransform>
bool xiiWorldReader::InstantiationContext::CreateGameObjects(const xiiDynamicArray<GameObjectToCreate>& objects, xiiGameObjectHandle hParent, xiiDynamicArray<xiiGameObject*>* out_pCreatedObjects, xiiTime endTime)
{
  XII_PROFILE_SCOPE("xiiWorldReader::CreateGameObjects");

  while (m_uiCurrentIndex < objects.GetCount())
  {
    auto& godesc = objects[m_uiCurrentIndex];

    xiiGameObjectDesc desc = godesc.m_Desc; // make a copy
    desc.m_hParent         = hParent.IsInvalidated() ? m_IndexToGameObjectHandle[godesc.m_uiParentHandleIdx] : hParent;
    desc.m_bDynamic |= m_Options.m_bForceDynamic;

    switch (m_Options.m_RandomSeedMode)
    {
      case xiiPrefabInstantiationOptions::RandomSeedMode::DeterministicFromParent:
        desc.m_uiStableRandomSeed = 0xFFFFFFFF; // xiiWorld::CreateObject() will either derive a deterministic value from the parent object, or assign a random value, if no parent exists
        break;

      case xiiPrefabInstantiationOptions::RandomSeedMode::CompletelyRandom:
        desc.m_uiStableRandomSeed = 0; // xiiWorld::CreateObject() will assign a random value to this object
        break;

      case xiiPrefabInstantiationOptions::RandomSeedMode::FixedFromSerialization:
        // keep deserialized value
        break;

      case xiiPrefabInstantiationOptions::RandomSeedMode::CustomRootValue:
        // we use the given seed root value to assign a deterministic (but different) value to each game object
        desc.m_uiStableRandomSeed = NextStableRandomSeed(m_Options.m_uiCustomRandomSeedRootValue);
        break;
    }

    if (m_Options.m_pOverrideTeamID != nullptr)
    {
      desc.m_uiTeamID = *m_Options.m_pOverrideTeamID;
    }

    if (UseTransform)
    {
      xiiTransform tChild(desc.m_LocalPosition, desc.m_LocalRotation, desc.m_LocalScaling);
      xiiTransform tFinal;
      tFinal = xiiTransform::MakeGlobalTransform(m_RootTransform, tChild);

      desc.m_LocalPosition = tFinal.m_vPosition;
      desc.m_LocalRotation = tFinal.m_qRotation;
      desc.m_LocalScaling  = tFinal.m_vScale;
    }

    xiiGameObject* pObject = nullptr;
    m_IndexToGameObjectHandle.PushBack(m_pWorld->CreateObject(desc, pObject));

    if (!godesc.m_sGlobalKey.IsEmpty())
    {
      pObject->SetGlobalKey(godesc.m_sGlobalKey);
    }

    if (out_pCreatedObjects)
    {
      out_pCreatedObjects->PushBack(pObject);
    }

    ++m_uiCurrentIndex;

    // exit here to ensure that we at least did some work
    if (xiiTime::Now() >= endTime)
    {
      SetSubProgressCompletion(static_cast<double>(m_uiCurrentIndex) / objects.GetCount());
      return false;
    }
  }

  m_uiCurrentIndex = 0;

  return true;
}

bool xiiWorldReader::InstantiationContext::CreateComponents(xiiTime endTime)
{
  XII_PROFILE_SCOPE("xiiWorldReader::CreateComponents");

  xiiStreamReader& s = m_CurrentReader;

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    const auto& compTypeInfo  = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    auto&       compTypeState = m_ComponentTypeStates[m_uiCurrentComponentTypeIndex];

    // will be the case for all abstract component types
    if (compTypeInfo.m_pRtti == nullptr || compTypeInfo.m_uiNumComponents == 0)
      continue;

    xiiComponentManagerBase* pManager = m_pWorld->GetOrCreateManagerForComponentType(compTypeInfo.m_pRtti);
    XII_ASSERT_DEV(pManager != nullptr, "Cannot create components of type '{0}', manager is not available.", compTypeInfo.m_pRtti->GetTypeName());

    while (m_uiCurrentIndex < compTypeInfo.m_uiNumComponents)
    {
      const xiiGameObjectHandle hOwner = m_WorldReader.ReadGameObjectHandle();

      xiiUInt32 uiComponentIdx = 0;
      s >> uiComponentIdx;

      bool bActive = true;
      s >> bActive;

      xiiUInt8 userFlags = 0;
      s >> userFlags;

      xiiGameObject* pOwnerObject = nullptr;
      if (!m_pWorld->TryGetObject(hOwner, pOwnerObject))
      {
        XII_REPORT_FAILURE("Owner object must be not null");
      }

      xiiComponent* pComponent = nullptr;
      auto          hComponent = pManager->CreateComponentNoInit(pOwnerObject, pComponent);

      pComponent->SetActiveFlag(bActive);

      for (xiiUInt8 j = 0; j < 8; ++j)
      {
        pComponent->SetUserFlag(j, (userFlags & XII_BIT(j)) != 0);
      }

      XII_ASSERT_DEBUG(uiComponentIdx == compTypeState.m_ComponentIndexToHandle.GetCount(), "Component index doesn't match");
      compTypeState.m_ComponentIndexToHandle.PushBack(hComponent);

      ++m_uiCurrentIndex;
      ++m_uiCurrentNumComponentsProcessed;

      // exit here to ensure that we at least did some work
      if (xiiTime::Now() >= endTime)
      {
        SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);
        return false;
      }
    }

    m_uiCurrentIndex = 0;
  }

  m_uiCurrentIndex                  = 0;
  m_uiCurrentComponentTypeIndex     = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

bool xiiWorldReader::InstantiationContext::DeserializeComponents(xiiTime endTime)
{
  XII_PROFILE_SCOPE("xiiWorldReader::DeserializeComponents");

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    const auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    if (compTypeInfo.m_pRtti == nullptr)
      continue;

    auto& compTypeState = m_ComponentTypeStates[m_uiCurrentComponentTypeIndex];

    if (m_uiCurrentIndex == 0)
    {
      compTypeState.m_uiDataReadOffset = m_CurrentReader.GetReadPosition();
    }

    while (m_uiCurrentIndex < compTypeState.m_ComponentIndexToHandle.GetCount())
    {
      xiiComponent* pComponent = nullptr;
      if (m_pWorld->TryGetComponent(compTypeState.m_ComponentIndexToHandle[m_uiCurrentIndex++], pComponent))
      {
        pComponent->DeserializeComponent(m_WorldReader);

        ++m_uiCurrentNumComponentsProcessed;

        // exit here to ensure that we at least did some work
        if (xiiTime::Now() >= endTime)
        {
          SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);
          return false;
        }
      }
    }

    const xiiUInt64 uiBytesRead = m_CurrentReader.GetReadPosition() - compTypeState.m_uiDataReadOffset;

    if (uiBytesRead != compTypeInfo.m_uiComponentDataSize)
    {
      XII_REPORT_FAILURE("Component type '{}' (version {}) deserialized {} of the stored {} bytes.\nCheck that the serialization and deserialization functions assume the same data layout.", compTypeInfo.m_pRtti->GetTypeName(), compTypeInfo.m_pRtti->GetTypeVersion(), uiBytesRead, compTypeInfo.m_uiComponentDataSize);
    }

    m_uiCurrentIndex = 0;
  }

  m_uiCurrentIndex                  = 0;
  m_uiCurrentComponentTypeIndex     = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

bool xiiWorldReader::InstantiationContext::AddComponentsToBatch(xiiTime endTime)
{
  XII_PROFILE_SCOPE("xiiWorldReader::AddComponentsToBatch");

  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->BeginAddingComponentsToInitBatch(m_hComponentInitBatch);
  }

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    const auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    if (compTypeInfo.m_pRtti == nullptr)
      continue;

    const auto& compTypeState = m_ComponentTypeStates[m_uiCurrentComponentTypeIndex];

    while (m_uiCurrentIndex < compTypeState.m_ComponentIndexToHandle.GetCount())
    {
      xiiComponent* pComponent = nullptr;
      if (m_pWorld->TryGetComponent(compTypeState.m_ComponentIndexToHandle[m_uiCurrentIndex++], pComponent))
      {
        pComponent->GetOwningManager()->InitializeComponent(pComponent);

        ++m_uiCurrentNumComponentsProcessed;

        // exit here to ensure that we at least did some work
        if (xiiTime::Now() >= endTime)
        {
          SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);

          if (!m_hComponentInitBatch.IsInvalidated())
          {
            m_pWorld->EndAddingComponentsToInitBatch(m_hComponentInitBatch);
          }
          return false;
        }
      }
    }

    m_uiCurrentIndex = 0;
  }

  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_pWorld->SubmitComponentInitBatch(m_hComponentInitBatch);
  }

  m_uiCurrentIndex                  = 0;
  m_uiCurrentComponentTypeIndex     = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

void xiiWorldReader::InstantiationContext::SetMaxStepTime(xiiTime stepTime)
{
  m_Options.m_MaxStepTime = stepTime;
}

xiiTime xiiWorldReader::InstantiationContext::GetMaxStepTime() const
{
  return m_Options.m_MaxStepTime;
}

void xiiWorldReader::InstantiationContext::BeginNextProgressStep(xiiStringView sName)
{
  if (m_pOverallProgressRange != nullptr)
  {
    m_pOverallProgressRange->BeginNextStep(sName);
    m_pSubProgressRange = nullptr;
    m_pSubProgressRange = XII_DEFAULT_NEW(xiiProgressRange, sName, false, m_pOverallProgressRange->GetProgressbar());
  }
}

void xiiWorldReader::InstantiationContext::SetSubProgressCompletion(double fCompletion)
{
  if (m_pSubProgressRange != nullptr)
  {
    m_pSubProgressRange->SetCompletion(fCompletion);
  }
}

XII_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_WorldReader);
