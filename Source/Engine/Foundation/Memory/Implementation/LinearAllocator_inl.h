template <xiiAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
xiiLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::xiiLinearAllocator(xiiStringView sName, xiiAllocator* pParent, xiiUInt32 uiInitialSize) :
  SUPER(sName, pParent), m_DestructData(pParent), m_PtrToDestructDataIndexTable(pParent)
{
  this->m_allocator.SetNextBucketSize(uiInitialSize);
}

template <xiiAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
xiiLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::~xiiLinearAllocator()
{
  Reset();
}

template <xiiAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
void* xiiLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc)
{
  XII_LOCK(m_Mutex);

  void* ptr = SUPER::Allocate(uiSize, uiAlign, destructorFunc);

  if (destructorFunc != nullptr)
  {
    xiiUInt32 uiIndex = m_DestructData.GetCount();
    m_PtrToDestructDataIndexTable.Insert(ptr, uiIndex);

    auto& data  = m_DestructData.ExpandAndGetRef();
    data.m_Func = destructorFunc;
    data.m_Ptr  = ptr;
  }

  return ptr;
}

template <xiiAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
void xiiLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::Deallocate(void* pPtr)
{
  XII_LOCK(m_Mutex);

  xiiUInt32 uiIndex;
  if (m_PtrToDestructDataIndexTable.Remove(pPtr, &uiIndex))
  {
    auto& data  = m_DestructData[uiIndex];
    data.m_Func = nullptr;
    data.m_Ptr  = nullptr;
  }

  SUPER::Deallocate(pPtr);
}

XII_MSVC_ANALYSIS_WARNING_PUSH

// Disable warning for incorrect operator (compiler complains about the TrackingMode bitwise and in the case that flags = None)
// even with the added guard of a check that it can't be 0.
XII_MSVC_ANALYSIS_WARNING_DISABLE(6313)

template <xiiAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
void xiiLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::Reset()
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = m_DestructData.GetCount(); i-- > 0;)
  {
    auto& data = m_DestructData[i];
    if (data.m_Func != nullptr)
    {
      data.m_Func(data.m_Ptr);
    }
  }

  m_DestructData.Clear();
  m_PtrToDestructDataIndexTable.Clear();

  this->m_allocator.Reset();
  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
  else if constexpr (TrackingMode >= xiiAllocatorTrackingMode::Basics)
  {
    xiiAllocator::Stats stats;
    this->m_allocator.FillStats(stats);

    xiiMemoryTracker::SetAllocatorStats(this->m_Id, stats);
  }
}
XII_MSVC_ANALYSIS_WARNING_POP
