namespace xiiInternal
{
  template <typename AllocationPolicy, xiiAllocatorTrackingMode TrackingMode>
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
    AllocationPolicy m_Allocator;

    xiiAllocatorId m_Id;
    xiiThreadID    m_ThreadID;
  };

  template <typename AllocationPolicy, xiiAllocatorTrackingMode TrackingMode, bool HasReallocate>
  class xiiAllocatorMixinReallocate : public xiiAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent);
  };

  template <typename AllocationPolicy, xiiAllocatorTrackingMode TrackingMode>
  class xiiAllocatorMixinReallocate<AllocationPolicy, TrackingMode, true> : public xiiAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent);
    virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
}; // namespace xiiInternal

template <typename A, xiiAllocatorTrackingMode TrackingMode>
XII_FORCE_INLINE xiiInternal::xiiAllocatorImpl<A, TrackingMode>::xiiAllocatorImpl(xiiStringView sName, xiiAllocatorBase* pParent /* = nullptr */) :
  m_Allocator(pParent), m_ThreadID(xiiThreadUtils::GetCurrentThreadID())
{
  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::Basics)
  {
    this->m_Id = xiiMemoryTracker::RegisterAllocator(sName, TrackingMode, pParent != nullptr ? pParent->GetId() : xiiAllocatorId());
  }
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
xiiInternal::xiiAllocatorImpl<A, TrackingMode>::~xiiAllocatorImpl()
{
  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::Basics)
  {
    xiiMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
void* xiiInternal::xiiAllocatorImpl<A, TrackingMode>::Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc)
{
  XII_IGNORE_UNUSED(destructorFunc);

  // zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored)
  if (uiSize == 0)
    return nullptr;

  XII_ASSERT_DEBUG(xiiMath::IsPowerOf2((xiiUInt32)uiAlign), "Alignment must be power of two");

  [[maybe_unused]] xiiTime fAllocationTime;

  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    fAllocationTime = xiiTime::Now();
  }

  void* ptr = m_Allocator.Allocate(uiSize, uiAlign);
  XII_ASSERT_DEV(ptr != nullptr, "Could not allocate {0} bytes. Out of memory?", uiSize);

  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::AddAllocation(this->m_Id, TrackingMode, ptr, uiSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
void xiiInternal::xiiAllocatorImpl<A, TrackingMode>::Deallocate(void* pPtr)
{
  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  m_Allocator.Deallocate(pPtr);
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
size_t xiiInternal::xiiAllocatorImpl<A, TrackingMode>::AllocatedSize(const void* pPtr)
{
  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    return xiiMemoryTracker::GetAllocationInfo(this->m_Id, pPtr).m_uiSize;
  }
  else
  {
    return 0;
  }
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
xiiAllocatorId xiiInternal::xiiAllocatorImpl<A, TrackingMode>::GetId() const
{
  return this->m_Id;
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
xiiAllocatorBase::Stats xiiInternal::xiiAllocatorImpl<A, TrackingMode>::GetStats() const
{
  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::Basics)
  {
    return xiiMemoryTracker::GetAllocatorStats(this->m_Id);
  }
  else
  {
    return Stats();
  }
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
XII_ALWAYS_INLINE xiiAllocatorBase* xiiInternal::xiiAllocatorImpl<A, TrackingMode>::GetParent() const
{
  return m_Allocator.GetParent();
}

template <typename A, xiiAllocatorTrackingMode TrackingMode, bool HasReallocate>
xiiInternal::xiiAllocatorMixinReallocate<A, TrackingMode, HasReallocate>::xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent) :
  xiiAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
xiiInternal::xiiAllocatorMixinReallocate<A, TrackingMode, true>::xiiAllocatorMixinReallocate(xiiStringView sName, xiiAllocatorBase* pParent) :
  xiiAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, xiiAllocatorTrackingMode TrackingMode>
void* xiiInternal::xiiAllocatorMixinReallocate<A, TrackingMode, true>::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  [[maybe_unused]] xiiTime fAllocationTime;

  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::RemoveAllocation(this->m_Id, pPtr);

    fAllocationTime = xiiTime::Now();
  }

  void* pNewMem = this->m_Allocator.Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);

  if constexpr (TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::AddAllocation(this->m_Id, TrackingMode, pNewMem, uiNewSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return pNewMem;
}
