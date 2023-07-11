namespace xiiInternal
{
  template <typename AllocationPolicy, xiiUInt32 TrackingFlags>
  class xiiAllocatorImpl : public xiiAllocatorBase
  {
  public:
    xiiAllocatorImpl(xiiStringView sName, xiiAllocatorBase* pParent);
    ~xiiAllocatorImpl();

    // xiiAllocatorBase implementation
    virtual void*          Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc = nullptr) override;
    virtual void           Deallocate(void* pPtr) override;
    virtual size_t         AllocatedSize(const void* pPtr) override;
    virtual xiiAllocatorId GetId() const override;
    virtual Stats          GetStats() const override;

    xiiAllocatorBase* GetParent() const;

  protected:
    AllocationPolicy m_allocator;

    xiiAllocatorId m_Id;
    xiiThreadID    m_ThreadID;
  };

  template <typename AllocationPolicy, xiiUInt32 TrackingFlags, bool HasReallocate>
  class xiiAllocatorMixinReallocate : public xiiAllocatorImpl<AllocationPolicy, TrackingFlags>
  {
  public:
    xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent);
  };

  template <typename AllocationPolicy, xiiUInt32 TrackingFlags>
  class xiiAllocatorMixinReallocate<AllocationPolicy, TrackingFlags, true> : public xiiAllocatorImpl<AllocationPolicy, TrackingFlags>
  {
  public:
    xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent);
    virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
}; // namespace xiiInternal

template <typename A, xiiUInt32 TrackingFlags>
XII_FORCE_INLINE xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::xiiAllocatorImpl(xiiStringView sName, xiiAllocatorBase* pParent /* = nullptr */) :
  m_allocator(pParent), m_ThreadID(xiiThreadUtils::GetCurrentThreadID())
{
  if ((TrackingFlags & xiiMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    XII_CHECK_AT_COMPILETIME_MSG((TrackingFlags & ~xiiMemoryTrackingFlags::All) == 0, "Invalid tracking flags");
    const xiiUInt32                     uiTrackingFlags = TrackingFlags;
    xiiBitflags<xiiMemoryTrackingFlags> flags           = *reinterpret_cast<const xiiBitflags<xiiMemoryTrackingFlags>*>(&uiTrackingFlags);
    this->m_Id                                          = xiiMemoryTracker::RegisterAllocator(sName, flags, pParent != nullptr ? pParent->GetId() : xiiAllocatorId());
  }
}

template <typename A, xiiUInt32 TrackingFlags>
xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::~xiiAllocatorImpl()
{
  // XII_ASSERT_RELEASE(m_ThreadID == xiiThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");

  if ((TrackingFlags & xiiMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    xiiMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, xiiUInt32 TrackingFlags>
void* xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc)
{
  // Zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored).
  if (uiSize == 0)
    return nullptr;

  XII_ASSERT_DEBUG(xiiMath::IsPowerOf2((xiiUInt32)uiAlign), "Alignment must be power of two");

  xiiTime fAllocationTime = xiiTime::Now();

  void* pPtr = m_allocator.Allocate(uiSize, uiAlign);
  XII_ASSERT_DEV(pPtr != nullptr, "Could not allocate {0} bytes. Out of memory?", uiSize);

  if ((TrackingFlags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiBitflags<xiiMemoryTrackingFlags> flags;
    flags.SetValue(TrackingFlags);

    xiiMemoryTracker::AddAllocation(this->m_Id, flags, pPtr, uiSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return pPtr;
}

template <typename A, xiiUInt32 TrackingFlags>
void xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::Deallocate(void* pPtr)
{
  if ((TrackingFlags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  m_allocator.Deallocate(pPtr);
}

template <typename A, xiiUInt32 TrackingFlags>
size_t xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::AllocatedSize(const void* pPtr)
{
  if ((TrackingFlags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    return xiiMemoryTracker::GetAllocationInfo(this->m_Id, pPtr).m_uiSize;
  }

  return 0;
}

template <typename A, xiiUInt32 TrackingFlags>
xiiAllocatorId xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::GetId() const
{
  return this->m_Id;
}

template <typename A, xiiUInt32 TrackingFlags>
xiiAllocatorBase::Stats xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::GetStats() const
{
  if ((TrackingFlags & xiiMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    return xiiMemoryTracker::GetAllocatorStats(this->m_Id);
  }

  return Stats();
}

template <typename A, xiiUInt32 TrackingFlags>
XII_ALWAYS_INLINE xiiAllocatorBase* xiiInternal::xiiAllocatorImpl<A, TrackingFlags>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, xiiUInt32 TrackingFlags, bool HasReallocate>
xiiInternal::xiiAllocatorMixinReallocate<A, TrackingFlags, HasReallocate>::xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent) :
  xiiAllocatorImpl<A, TrackingFlags>(sName, pParent)
{
}

template <typename A, xiiUInt32 TrackingFlags>
xiiInternal::xiiAllocatorMixinReallocate<A, TrackingFlags, true>::xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent) :
  xiiAllocatorImpl<A, TrackingFlags>(sName, pParent)
{
}

template <typename A, xiiUInt32 TrackingFlags>
void* xiiInternal::xiiAllocatorMixinReallocate<A, TrackingFlags, true>::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  if ((TrackingFlags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  xiiTime fAllocationTime = xiiTime::Now();

  void* pNewMem = this->m_allocator.Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);

  if ((TrackingFlags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiBitflags<xiiMemoryTrackingFlags> flags;
    flags.SetValue(TrackingFlags);

    xiiMemoryTracker::AddAllocation(this->m_Id, flags, pNewMem, uiNewSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }
  return pNewMem;
}
