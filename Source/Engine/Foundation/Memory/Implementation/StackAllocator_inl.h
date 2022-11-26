template <xiiUInt32 TrackingFlags>
xiiStackAllocator<TrackingFlags>::xiiStackAllocator(const char* szName, xiiAllocatorBase* pParent) :
  xiiAllocator<xiiMemoryPolicies::xiiStackAllocation, TrackingFlags>(szName, pParent), m_DestructData(pParent), m_PtrToDestructDataIndexTable(pParent)
{
}

template <xiiUInt32 TrackingFlags>
xiiStackAllocator<TrackingFlags>::~xiiStackAllocator()
{
  Reset();
}

template <xiiUInt32 TrackingFlags>
void* xiiStackAllocator<TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc)
{
  XII_LOCK(m_Mutex);

  void* ptr = xiiAllocator<xiiMemoryPolicies::xiiStackAllocation, TrackingFlags>::Allocate(uiSize, uiAlign, destructorFunc);

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

template <xiiUInt32 TrackingFlags>
void xiiStackAllocator<TrackingFlags>::Deallocate(void* ptr)
{
  XII_LOCK(m_Mutex);

  xiiUInt32 uiIndex;
  if (m_PtrToDestructDataIndexTable.Remove(ptr, &uiIndex))
  {
    auto& data  = m_DestructData[uiIndex];
    data.m_Func = nullptr;
    data.m_Ptr  = nullptr;
  }

  xiiAllocator<xiiMemoryPolicies::xiiStackAllocation, TrackingFlags>::Deallocate(ptr);
}

XII_MSVC_ANALYSIS_WARNING_PUSH

// Disable warning for incorrect operator (compiler complains about the TrackingFlags bitwise and in the case that flags = None)
// even with the added guard of a check that it can't be 0.
XII_MSVC_ANALYSIS_WARNING_DISABLE(6313)

template <xiiUInt32 TrackingFlags>
void xiiStackAllocator<TrackingFlags>::Reset()
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = m_DestructData.GetCount(); i-- > 0;)
  {
    auto& data = m_DestructData[i];
    if (data.m_Func != nullptr)
      data.m_Func(data.m_Ptr);
  }
  m_DestructData.Clear();
  m_PtrToDestructDataIndexTable.Clear();

  this->m_allocator.Reset();
  if ((TrackingFlags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
  else if ((TrackingFlags & xiiMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    xiiAllocatorBase::Stats stats;
    this->m_allocator.FillStats(stats);

    xiiMemoryTracker::SetAllocatorStats(this->m_Id, stats);
  }
}
XII_MSVC_ANALYSIS_WARNING_POP
