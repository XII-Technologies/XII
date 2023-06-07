
template <typename T>
xiiDynamicArrayBase<T>::xiiDynamicArrayBase(xiiAllocatorBase* pAllocator)
{
  m_pAllocator = pAllocator;
}

template <typename T>
xiiDynamicArrayBase<T>::xiiDynamicArrayBase(T* pInplaceStorage, xiiUInt32 uiCapacity, xiiAllocatorBase* pAllocator)
{
  m_pAllocator = pAllocator;
  m_pAllocator.SetFlags(Storage::External);
  this->m_uiCapacity = uiCapacity;
  this->m_pElements  = reinterpret_cast<T*>(reinterpret_cast<intptr_t>(pInplaceStorage) - reinterpret_cast<intptr_t>(this)); // store as an offset
}

template <typename T>
xiiDynamicArrayBase<T>::xiiDynamicArrayBase(const xiiDynamicArrayBase<T>& other, xiiAllocatorBase* pAllocator)
{
  m_pAllocator = pAllocator;

  xiiArrayBase<T, xiiDynamicArrayBase<T>>::operator=((xiiArrayPtr<const T>)other); // redirect this to the xiiArrayPtr version
}

template <typename T>
xiiDynamicArrayBase<T>::xiiDynamicArrayBase(xiiDynamicArrayBase<T>&& other, xiiAllocatorBase* pAllocator)
{
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename T>
xiiDynamicArrayBase<T>::xiiDynamicArrayBase(const xiiArrayPtr<const T>& other, xiiAllocatorBase* pAllocator)
{
  m_pAllocator = pAllocator;

  xiiArrayBase<T, xiiDynamicArrayBase<T>>::operator=(other);
}

template <typename T>
xiiDynamicArrayBase<T>::~xiiDynamicArrayBase()
{
  this->Clear();

  if (m_pAllocator.GetFlags() == Storage::Owned)
  {
    // only delete our storage, if we own it
    XII_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
  }

  this->m_uiCapacity = 0;
  this->m_pElements  = nullptr;
}

template <typename T>
XII_ALWAYS_INLINE void xiiDynamicArrayBase<T>::operator=(const xiiDynamicArrayBase<T>& rhs)
{
  xiiArrayBase<T, xiiDynamicArrayBase<T>>::operator=((xiiArrayPtr<const T>)rhs); // redirect this to the xiiArrayPtr version
}

template <typename T>
inline void xiiDynamicArrayBase<T>::operator=(xiiDynamicArrayBase<T>&& rhs) noexcept
{
  // Clear any existing data (calls destructors if necessary)
  this->Clear();

  if (this->m_pAllocator == rhs.m_pAllocator && rhs.m_pAllocator.GetFlags() == Storage::Owned) // only move the storage of rhs, if it owns it
  {
    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      // only delete our storage, if we own it
      XII_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    }

    // we now own this storage
    this->m_pAllocator.SetFlags(Storage::Owned);

    // move the data over from the other array
    this->m_uiCount    = rhs.m_uiCount;
    this->m_uiCapacity = rhs.m_uiCapacity;
    this->m_pElements  = rhs.m_pElements;

    // reset the other array to not reference the data anymore
    rhs.m_pElements  = nullptr;
    rhs.m_uiCount    = 0;
    rhs.m_uiCapacity = 0;
  }
  else
  {
    // Ensure we have enough data.
    this->Reserve(rhs.m_uiCount);
    this->m_uiCount = rhs.m_uiCount;

    xiiMemoryUtils::RelocateConstruct(
      this->GetElementsPtr(), rhs.GetElementsPtr() /* vital to remap rhs.m_pElements to absolute ptr */, rhs.m_uiCount);

    rhs.m_uiCount = 0;
  }
}

template <typename T>
void xiiDynamicArrayBase<T>::Swap(xiiDynamicArrayBase<T>& other)
{
  if (this->m_pAllocator.GetFlags() == Storage::External && other.m_pAllocator.GetFlags() == Storage::External)
  {
    constexpr xiiUInt32 InplaceStorageSize = 64;

    struct alignas(XII_ALIGNMENT_OF(T)) Tmp
    {
      xiiUInt8 m_StaticData[InplaceStorageSize * sizeof(T)];
    };

    const xiiUInt32 localSize      = this->m_uiCount;
    const xiiUInt32 otherLocalSize = other.m_uiCount;

    if (localSize <= InplaceStorageSize && otherLocalSize <= InplaceStorageSize && localSize <= other.m_uiCapacity &&
        otherLocalSize <= this->m_uiCapacity)
    {

      Tmp tmp;
      xiiMemoryUtils::RelocateConstruct(reinterpret_cast<T*>(tmp.m_StaticData), this->GetElementsPtr(), localSize);
      xiiMemoryUtils::RelocateConstruct(this->GetElementsPtr(), other.GetElementsPtr(), otherLocalSize);
      xiiMemoryUtils::RelocateConstruct(other.GetElementsPtr(), reinterpret_cast<T*>(tmp.m_StaticData), localSize);

      xiiMath::Swap(this->m_pAllocator, other.m_pAllocator);
      xiiMath::Swap(this->m_uiCount, other.m_uiCount);

      return; // successfully swapped in place
    }

    // temp buffer was insufficient -> fallthrough
  }

  if (this->m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    this->Reserve(this->m_uiCapacity + 1);
  }

  if (other.m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    other.Reserve(other.m_uiCapacity + 1);
  }

  // no external storage involved -> swap pointers
  xiiMath::Swap(this->m_pAllocator, other.m_pAllocator);
  this->DoSwap(other);
}

template <typename T>
void xiiDynamicArrayBase<T>::SetCapacity(xiiUInt32 uiCapacity)
{
  // do NOT early out here, it is vital that this function does its thing even if the old capacity would be sufficient

  if (this->m_pAllocator.GetFlags() == Storage::Owned && uiCapacity > this->m_uiCapacity)
  {
    this->m_pElements = XII_EXTEND_RAW_BUFFER(this->m_pAllocator, this->m_pElements, this->m_uiCount, uiCapacity);
  }
  else
  {
    T* pOldElements = GetElementsPtr();

    T* pNewElements = XII_NEW_RAW_BUFFER(this->m_pAllocator, T, uiCapacity);
    xiiMemoryUtils::RelocateConstruct(pNewElements, pOldElements, this->m_uiCount);

    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      XII_DELETE_RAW_BUFFER(this->m_pAllocator, pOldElements);
    }

    // after any resize, we definitely own the storage
    this->m_pAllocator.SetFlags(Storage::Owned);
    this->m_pElements = pNewElements;
  }

  this->m_uiCapacity = uiCapacity;
}

template <typename T>
void xiiDynamicArrayBase<T>::Reserve(xiiUInt32 uiCapacity)
{
  if (this->m_uiCapacity >= uiCapacity)
    return;

  const xiiUInt64 uiCurCap64      = static_cast<xiiUInt64>(this->m_uiCapacity);
  xiiUInt64       uiNewCapacity64 = uiCurCap64 + (uiCurCap64 / 2);

  uiNewCapacity64 = xiiMath::Max<xiiUInt64>(uiNewCapacity64, uiCapacity);

  constexpr xiiUInt64 uiMaxCapacity = 0xFFFFFFFFllu - (CAPACITY_ALIGNMENT - 1);

  // the maximum value must leave room for the capacity alignment computation below (without overflowing the 32 bit range)
  uiNewCapacity64 = xiiMath::Min<xiiUInt64>(uiNewCapacity64, uiMaxCapacity);

  uiNewCapacity64 = (uiNewCapacity64 + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);

  XII_ASSERT_DEV(uiCapacity <= uiNewCapacity64, "The requested capacity of {} elements exceeds the maximum possible capacity of {} elements.", uiCapacity, uiMaxCapacity);

  SetCapacity(static_cast<xiiUInt32>(uiNewCapacity64 & 0xFFFFFFFF));
}

template <typename T>
void xiiDynamicArrayBase<T>::Compact()
{
  if (m_pAllocator.GetFlags() == Storage::External)
    return;

  if (this->IsEmpty())
  {
    // completely deallocate all data, if the array is empty.
    XII_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    this->m_uiCapacity = 0;
  }
  else
  {
    const xiiUInt32 uiNewCapacity = (this->m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (this->m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename T>
XII_ALWAYS_INLINE T* xiiDynamicArrayBase<T>::GetElementsPtr()
{
  if (m_pAllocator.GetFlags() == Storage::External)
  {
    return reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + reinterpret_cast<intptr_t>(this->m_pElements));
  }

  return this->m_pElements;
}

template <typename T>
XII_ALWAYS_INLINE const T* xiiDynamicArrayBase<T>::GetElementsPtr() const
{
  if (m_pAllocator.GetFlags() == Storage::External)
  {
    return reinterpret_cast<const T*>(reinterpret_cast<intptr_t>(this) + reinterpret_cast<intptr_t>(this->m_pElements));
  }

  return this->m_pElements;
}

template <typename T>
xiiUInt64 xiiDynamicArrayBase<T>::GetHeapMemoryUsage() const
{
  if (this->m_pAllocator.GetFlags() == Storage::External)
    return 0;

  return (xiiUInt64)this->m_uiCapacity * (xiiUInt64)sizeof(T);
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray() :
  xiiDynamicArrayBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray(xiiAllocatorBase* pAllocator) :
  xiiDynamicArrayBase<T>(pAllocator)
{
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray(const xiiDynamicArray<T, A>& other) :
  xiiDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray(const xiiDynamicArrayBase<T>& other) :
  xiiDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray(const xiiArrayPtr<const T>& other) :
  xiiDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray(xiiDynamicArray<T, A>&& other) :
  xiiDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
xiiDynamicArray<T, A>::xiiDynamicArray(xiiDynamicArrayBase<T>&& other) :
  xiiDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
void xiiDynamicArray<T, A>::operator=(const xiiDynamicArray<T, A>& rhs)
{
  xiiDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void xiiDynamicArray<T, A>::operator=(const xiiDynamicArrayBase<T>& rhs)
{
  xiiDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void xiiDynamicArray<T, A>::operator=(const xiiArrayPtr<const T>& rhs)
{
  xiiArrayBase<T, xiiDynamicArrayBase<T>>::operator=(rhs);
}

template <typename T, typename A>
void xiiDynamicArray<T, A>::operator=(xiiDynamicArray<T, A>&& rhs) noexcept
{
  xiiDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename A>
void xiiDynamicArray<T, A>::operator=(xiiDynamicArrayBase<T>&& rhs) noexcept
{
  xiiDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename AllocatorWrapper>
xiiArrayPtr<const T* const> xiiMakeArrayPtr(const xiiDynamicArray<T*, AllocatorWrapper>& dynArray)
{
  return xiiArrayPtr<const T* const>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
xiiArrayPtr<const T> xiiMakeArrayPtr(const xiiDynamicArray<T, AllocatorWrapper>& dynArray)
{
  return xiiArrayPtr<const T>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
xiiArrayPtr<T> xiiMakeArrayPtr(xiiDynamicArray<T, AllocatorWrapper>& ref_dynArray)
{
  return xiiArrayPtr<T>(ref_dynArray.GetData(), ref_dynArray.GetCount());
}
