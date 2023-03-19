
namespace xiiInternal
{
  // static
  XII_ALWAYS_INLINE WorldData::HierarchyType::Enum WorldData::GetHierarchyType(bool bIsDynamic)
  {
    return bIsDynamic ? HierarchyType::Dynamic : HierarchyType::Static;
  }

  // static
  template <typename VISITOR>
  XII_FORCE_INLINE xiiVisitorExecution::Enum WorldData::TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
  {
    for (WorldData::Hierarchy::DataBlock& block : blocks)
    {
      xiiGameObject::TransformationData* pCurrentData = block.m_pData;
      xiiGameObject::TransformationData* pEndData     = block.m_pData + block.m_uiCount;

      while (pCurrentData < pEndData)
      {
        xiiVisitorExecution::Enum execution = VISITOR::Visit(pCurrentData, pUserData);
        if (execution != xiiVisitorExecution::Continue)
          return execution;

        ++pCurrentData;
      }
    }

    return xiiVisitorExecution::Continue;
  }

  // static
  template <typename VISITOR>
  XII_FORCE_INLINE xiiVisitorExecution::Enum WorldData::TraverseHierarchyLevelMultiThreaded(
    Hierarchy::DataBlockArray& blocks,
    void*                      pUserData /* = nullptr*/)
  {
    xiiParallelForParams parallelForParams;
    parallelForParams.m_uiBinSize           = 100;
    parallelForParams.m_uiMaxTasksPerThread = 2;
    parallelForParams.m_pTaskAllocator      = m_StackAllocator.GetCurrentAllocator();

    xiiTaskSystem::ParallelFor(
      blocks.GetArrayPtr(),
      [pUserData](xiiArrayPtr<WorldData::Hierarchy::DataBlock> blocksSlice) {
        for (WorldData::Hierarchy::DataBlock& block : blocksSlice)
        {
          xiiGameObject::TransformationData* pCurrentData = block.m_pData;
          xiiGameObject::TransformationData* pEndData     = block.m_pData + block.m_uiCount;

          while (pCurrentData < pEndData)
          {
            VISITOR::Visit(pCurrentData, pUserData);
            ++pCurrentData;
          }
        }
      },
      "World DataBlock Traversal Task", parallelForParams);

    return xiiVisitorExecution::Continue;
  }

  // static
  XII_FORCE_INLINE void WorldData::UpdateGlobalTransform(xiiGameObject::TransformationData* pData, const xiiSimdFloat& fInvDeltaSeconds)
  {
    pData->UpdateGlobalTransformWithoutParent();
    pData->UpdateVelocity(fInvDeltaSeconds);
    pData->UpdateGlobalBounds();
  }

  // static
  XII_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParent(xiiGameObject::TransformationData* pData, const xiiSimdFloat& fInvDeltaSeconds)
  {
    pData->UpdateGlobalTransformWithParent();
    pData->UpdateVelocity(fInvDeltaSeconds);
    pData->UpdateGlobalBounds();
  }

  // static
  XII_FORCE_INLINE void WorldData::UpdateGlobalTransformAndSpatialData(
    xiiGameObject::TransformationData* pData,
    const xiiSimdFloat&                fInvDeltaSeconds,
    xiiSpatialSystem&                  spatialSystem)
  {
    pData->UpdateGlobalTransformWithoutParent();
    pData->UpdateVelocity(fInvDeltaSeconds);
    pData->UpdateGlobalBoundsAndSpatialData(spatialSystem);
  }

  // static
  XII_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParentAndSpatialData(
    xiiGameObject::TransformationData* pData,
    const xiiSimdFloat&                fInvDeltaSeconds,
    xiiSpatialSystem&                  spatialSystem)
  {
    pData->UpdateGlobalTransformWithParent();
    pData->UpdateVelocity(fInvDeltaSeconds);
    pData->UpdateGlobalBoundsAndSpatialData(spatialSystem);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  XII_ALWAYS_INLINE const xiiGameObject& WorldData::ConstObjectIterator::operator*() const { return *m_Iterator; }

  XII_ALWAYS_INLINE const xiiGameObject* WorldData::ConstObjectIterator::operator->() const { return m_Iterator; }

  XII_ALWAYS_INLINE WorldData::ConstObjectIterator::operator const xiiGameObject*() const { return m_Iterator; }

  XII_ALWAYS_INLINE void WorldData::ConstObjectIterator::Next()
  {
    m_Iterator.Next();

    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  XII_ALWAYS_INLINE bool WorldData::ConstObjectIterator::IsValid() const { return m_Iterator.IsValid(); }

  XII_ALWAYS_INLINE void WorldData::ConstObjectIterator::operator++() { Next(); }

  XII_ALWAYS_INLINE WorldData::ConstObjectIterator::ConstObjectIterator(ObjectStorage::ConstIterator iterator) :
    m_Iterator(iterator)
  {
    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  XII_ALWAYS_INLINE xiiGameObject& WorldData::ObjectIterator::operator*() { return *m_Iterator; }

  XII_ALWAYS_INLINE xiiGameObject* WorldData::ObjectIterator::operator->() { return m_Iterator; }

  XII_ALWAYS_INLINE WorldData::ObjectIterator::operator xiiGameObject*() { return m_Iterator; }

  XII_ALWAYS_INLINE void WorldData::ObjectIterator::Next()
  {
    m_Iterator.Next();

    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  XII_ALWAYS_INLINE bool WorldData::ObjectIterator::IsValid() const { return m_Iterator.IsValid(); }

  XII_ALWAYS_INLINE void WorldData::ObjectIterator::operator++() { Next(); }

  XII_ALWAYS_INLINE WorldData::ObjectIterator::ObjectIterator(ObjectStorage::Iterator iterator) :
    m_Iterator(iterator)
  {
    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  XII_FORCE_INLINE WorldData::InitBatch::InitBatch(xiiAllocatorBase* pAllocator, xiiStringView szName, bool bMustFinishWithinOneFrame) :
    m_bMustFinishWithinOneFrame(bMustFinishWithinOneFrame), m_ComponentsToInitialize(pAllocator), m_ComponentsToStartSimulation(pAllocator)
  {
    m_sName.Assign(szName);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  XII_FORCE_INLINE void WorldData::RegisteredUpdateFunction::FillFromDesc(const xiiWorldModule::UpdateFunctionDesc& desc)
  {
    m_Function                  = desc.m_Function;
    m_sFunctionName             = desc.m_sFunctionName;
    m_fPriority                 = desc.m_fPriority;
    m_uiGranularity             = desc.m_uiGranularity;
    m_bOnlyUpdateWhenSimulating = desc.m_bOnlyUpdateWhenSimulating;
  }

  XII_FORCE_INLINE bool WorldData::RegisteredUpdateFunction::operator<(const RegisteredUpdateFunction& other) const
  {
    // higher priority comes first
    if (m_fPriority != other.m_fPriority)
      return m_fPriority > other.m_fPriority;

    // sort by function name to ensure determinism
    xiiInt32 iNameComp = xiiStringUtils::Compare(m_sFunctionName, other.m_sFunctionName);
    XII_ASSERT_DEV(iNameComp != 0, "An update function with the same name and same priority is already registered. This breaks determinism.");
    return iNameComp < 0;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  XII_ALWAYS_INLINE WorldData::ReadMarker::ReadMarker(const WorldData& data) :
    m_Data(data)
  {
  }

  XII_FORCE_INLINE void WorldData::ReadMarker::Lock()
  {
    XII_ASSERT_DEV(m_Data.m_WriteThreadID == (xiiThreadID)0 || m_Data.m_WriteThreadID == xiiThreadUtils::GetCurrentThreadID(),
                   "World '{0}' cannot be marked for reading because it is already marked for writing by another thread.", m_Data.m_sName);
    m_Data.m_iReadCounter.Increment();
  }

  XII_ALWAYS_INLINE void WorldData::ReadMarker::Unlock() { m_Data.m_iReadCounter.Decrement(); }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  XII_ALWAYS_INLINE WorldData::WriteMarker::WriteMarker(WorldData& data) :
    m_Data(data)
  {
  }

  XII_FORCE_INLINE void WorldData::WriteMarker::Lock()
  {
    // already locked by this thread?
    if (m_Data.m_WriteThreadID != xiiThreadUtils::GetCurrentThreadID())
    {
      XII_ASSERT_DEV(m_Data.m_iReadCounter == 0, "World '{0}' cannot be marked for writing because it is already marked for reading.", m_Data.m_sName);
      XII_ASSERT_DEV(m_Data.m_WriteThreadID == (xiiThreadID)0,
                     "World '{0}' cannot be marked for writing because it is already marked for writing by another thread.", m_Data.m_sName);

      m_Data.m_WriteThreadID = xiiThreadUtils::GetCurrentThreadID();
      m_Data.m_iReadCounter.Increment(); // allow reading as well
    }

    m_Data.m_iWriteCounter++;
  }

  XII_FORCE_INLINE void WorldData::WriteMarker::Unlock()
  {
    m_Data.m_iWriteCounter--;

    if (m_Data.m_iWriteCounter == 0)
    {
      m_Data.m_iReadCounter.Decrement();
      m_Data.m_WriteThreadID = (xiiThreadID)0;
    }
  }
} // namespace xiiInternal
