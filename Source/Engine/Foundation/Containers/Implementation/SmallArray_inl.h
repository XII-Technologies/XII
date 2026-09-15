/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T, xiiUInt16 Size>
xiiSmallArrayBase<T, Size>::xiiSmallArrayBase() = default;

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiSmallArrayBase<T, Size>::xiiSmallArrayBase(const xiiSmallArrayBase<T, Size>& other, xiiAllocator* pAllocator)
{
  CopyFrom((xiiArrayPtr<const T>)other, pAllocator);
  m_uiUserData = other.m_uiUserData;
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiSmallArrayBase<T, Size>::xiiSmallArrayBase(const xiiArrayPtr<const T>& other, xiiAllocator* pAllocator)
{
  CopyFrom(other, pAllocator);
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiSmallArrayBase<T, Size>::xiiSmallArrayBase(xiiSmallArrayBase<T, Size>&& other, xiiAllocator* pAllocator)
{
  MoveFrom(std::move(other), pAllocator);
}

template <typename T, xiiUInt16 Size>
XII_FORCE_INLINE xiiSmallArrayBase<T, Size>::~xiiSmallArrayBase()
{
  XII_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  XII_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::CopyFrom(const xiiArrayPtr<const T>& other, xiiAllocator* pAllocator)
{
  XII_ASSERT_DEV(other.GetCount() <= xiiSmallInvalidIndex, "Can't copy {} elements to small array. Maximum count is {}", other.GetCount(), xiiSmallInvalidIndex);

  if (GetData() == other.GetPtr())
  {
    if (m_uiCount == other.GetCount())
      return;

    XII_ASSERT_DEV(m_uiCount > other.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");

    T* pElements = GetElementsPtr();
    xiiMemoryUtils::Destruct(pElements + other.GetCount(), m_uiCount - other.GetCount());
    m_uiCount = static_cast<xiiUInt16>(other.GetCount());
    return;
  }

  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = other.GetCount();

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<xiiUInt16>(uiNewCount), pAllocator);

    T* pElements = GetElementsPtr();
    xiiMemoryUtils::Copy(pElements, other.GetPtr(), uiOldCount);
    xiiMemoryUtils::CopyConstructArray(pElements + uiOldCount, other.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = GetElementsPtr();
    xiiMemoryUtils::Copy(pElements, other.GetPtr(), uiNewCount);
    xiiMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = static_cast<xiiUInt16>(uiNewCount);
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::MoveFrom(xiiSmallArrayBase<T, Size>&& other, xiiAllocator* pAllocator)
{
  Clear();

  if (other.m_uiCapacity > Size)
  {
    if (m_uiCapacity > Size)
    {
      // only delete our own external storage
      XII_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = other.m_uiCapacity;
    m_pElements  = other.m_pElements;
  }
  else
  {
    xiiMemoryUtils::RelocateConstruct(GetElementsPtr(), other.GetElementsPtr(), other.m_uiCount);
  }

  m_uiCount    = other.m_uiCount;
  m_uiUserData = other.m_uiUserData;

  // reset the other array to not reference the data anymore
  other.m_pElements  = nullptr;
  other.m_uiCount    = 0;
  other.m_uiCapacity = 0;
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiSmallArrayBase<T, Size>::operator xiiArrayPtr<const T>() const
{
  return xiiArrayPtr<const T>(GetElementsPtr(), m_uiCount);
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiSmallArrayBase<T, Size>::operator xiiArrayPtr<T>()
{
  return xiiArrayPtr<T>(GetElementsPtr(), m_uiCount);
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE bool xiiSmallArrayBase<T, Size>::operator==(const xiiSmallArrayBase<T, Size>& rhs) const
{
  return *this == rhs.GetArrayPtr();
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE bool xiiSmallArrayBase<T, Size>::operator<(const xiiSmallArrayBase<T, Size>& rhs) const
{
  return GetArrayPtr() < rhs.GetArrayPtr();
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE bool xiiSmallArrayBase<T, Size>::operator<(const xiiArrayPtr<const T>& rhs) const
{
  return GetArrayPtr() < rhs;
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE const T& xiiSmallArrayBase<T, Size>::operator[](const xiiUInt32 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE T& xiiSmallArrayBase<T, Size>::operator[](const xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::SetCount(xiiUInt16 uiCount, xiiAllocator* pAllocator)
{
  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<xiiUInt16>(uiNewCount), pAllocator);
    xiiMemoryUtils::Construct<ConstructAll>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    xiiMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::SetCount(xiiUInt16 uiCount, const T& fillValue, xiiAllocator* pAllocator)
{
  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiCount, pAllocator);
    xiiMemoryUtils::CopyConstruct(GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    xiiMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::EnsureCount(xiiUInt16 uiCount, xiiAllocator* pAllocator)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount, pAllocator);
  }
}

template <typename T, xiiUInt16 Size>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
void xiiSmallArrayBase<T, Size>::SetCountUninitialized(xiiUInt16 uiCount, xiiAllocator* pAllocator)
{
  static_assert(xiiIsPodType<T>::value == xiiTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");
  const xiiUInt16 uiOldCount = m_uiCount;
  const xiiUInt16 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiNewCount, pAllocator);
    xiiMemoryUtils::Construct<SkipTrivialTypes>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    xiiMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiUInt32 xiiSmallArrayBase<T, Size>::GetCount() const
{
  return m_uiCount;
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE bool xiiSmallArrayBase<T, Size>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::Clear()
{
  xiiMemoryUtils::Destruct(GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, xiiUInt16 Size>
bool xiiSmallArrayBase<T, Size>::Contains(const T& value) const
{
  return IndexOf(value) != xiiInvalidIndex;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::Insert(const T& value, xiiUInt32 uiIndex, xiiAllocator* pAllocator)
{
  XII_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  xiiMemoryUtils::Prepend(GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::Insert(T&& value, xiiUInt32 uiIndex, xiiAllocator* pAllocator)
{
  XII_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  xiiMemoryUtils::Prepend(GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, xiiUInt16 Size>
bool xiiSmallArrayBase<T, Size>::RemoveAndCopy(const T& value)
{
  xiiUInt32 uiIndex = IndexOf(value);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, xiiUInt16 Size>
bool xiiSmallArrayBase<T, Size>::RemoveAndSwap(const T& value)
{
  xiiUInt32 uiIndex = IndexOf(value);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::RemoveAtAndCopy(xiiUInt32 uiIndex, xiiUInt16 uiNumElements /*= 1*/)
{
  XII_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

  m_uiCount -= uiNumElements;
  xiiMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::RemoveAtAndSwap(xiiUInt32 uiIndex, xiiUInt16 uiNumElements /*= 1*/)
{
  XII_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

  for (xiiUInt32 i = 0; i < uiNumElements; ++i)
  {
    m_uiCount--;

    if (m_uiCount != uiIndex)
    {
      pElements[uiIndex] = std::move(pElements[m_uiCount]);
    }
    xiiMemoryUtils::Destruct(pElements + m_uiCount, 1);
    ++uiIndex;
  }
}

template <typename T, xiiUInt16 Size>
xiiUInt32 xiiSmallArrayBase<T, Size>::IndexOf(const T& value, xiiUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (xiiUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (xiiMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return xiiInvalidIndex;
}

template <typename T, xiiUInt16 Size>
xiiUInt32 xiiSmallArrayBase<T, Size>::LastIndexOf(const T& value, xiiUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (xiiUInt32 i = xiiMath::Min<xiiUInt32>(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (xiiMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return xiiInvalidIndex;
}

template <typename T, xiiUInt16 Size>
T& xiiSmallArrayBase<T, Size>::ExpandAndGetRef(xiiAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  T* pElements = GetElementsPtr();

  xiiMemoryUtils::Construct<SkipTrivialTypes>(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::PushBack(const T& value, xiiAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  xiiMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::PushBack(T&& value, xiiAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  xiiMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::PushBackUnchecked(const T& value)
{
  XII_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  xiiMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::PushBackUnchecked(T&& value)
{
  XII_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  xiiMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::PushBackRange(const xiiArrayPtr<const T>& range, xiiAllocator* pAllocator)
{
  const xiiUInt32 uiRangeCount = range.GetCount();
  Reserve(m_uiCount + uiRangeCount, pAllocator);

  xiiMemoryUtils::CopyConstructArray(GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::PopBack(xiiUInt32 uiCountToRemove /* = 1 */)
{
  XII_ASSERT_DEBUG(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= static_cast<xiiUInt16>(uiCountToRemove);
  xiiMemoryUtils::Destruct(GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, xiiUInt16 Size>
XII_FORCE_INLINE T& xiiSmallArrayBase<T, Size>::PeekBack()
{
  XII_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, xiiUInt16 Size>
XII_FORCE_INLINE const T& xiiSmallArrayBase<T, Size>::PeekBack() const
{
  XII_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, xiiUInt16 Size>
template <typename Comparer>
void xiiSmallArrayBase<T, Size>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    xiiArrayPtr<T> ar = GetArrayPtr();
    xiiSorting::QuickSort(ar, comparer);
  }
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::Sort()
{
  if (m_uiCount > 1)
  {
    xiiArrayPtr<T> ar = GetArrayPtr();
    xiiSorting::QuickSort(ar, xiiCompareHelper<T>());
  }
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE T* xiiSmallArrayBase<T, Size>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE const T* xiiSmallArrayBase<T, Size>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiArrayPtr<T> xiiSmallArrayBase<T, Size>::GetArrayPtr()
{
  return xiiArrayPtr<T>(GetData(), GetCount());
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiArrayPtr<const T> xiiSmallArrayBase<T, Size>::GetArrayPtr() const
{
  return xiiArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiArrayPtr<typename xiiArrayPtr<T>::ByteType> xiiSmallArrayBase<T, Size>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiArrayPtr<typename xiiArrayPtr<const T>::ByteType> xiiSmallArrayBase<T, Size>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::Reserve(xiiUInt16 uiCapacity, xiiAllocator* pAllocator)
{
  if (m_uiCapacity >= uiCapacity)
    return;

  const xiiUInt32 uiCurCap      = static_cast<xiiUInt32>(m_uiCapacity);
  xiiUInt32       uiNewCapacity = uiCurCap + (uiCurCap / 2);

  uiNewCapacity = xiiMath::Max<xiiUInt32>(uiNewCapacity, uiCapacity);
  uiNewCapacity = xiiMemoryUtils::AlignSize<xiiUInt32>(uiNewCapacity, CAPACITY_ALIGNMENT);
  uiNewCapacity = xiiMath::Min<xiiUInt32>(uiNewCapacity, 0xFFFFu);

  SetCapacity(static_cast<xiiUInt16>(uiNewCapacity), pAllocator);
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::Compact(xiiAllocator* pAllocator)
{
  if (IsEmpty())
  {
    if (m_uiCapacity > Size)
    {
      // completely deallocate all data, if the array is empty.
      XII_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = Size;
    m_pElements  = nullptr;
  }
  else if (m_uiCapacity > Size)
  {
    xiiUInt32 uiNewCapacity = xiiMemoryUtils::AlignSize<xiiUInt32>(m_uiCount, CAPACITY_ALIGNMENT);
    uiNewCapacity           = xiiMath::Min<xiiUInt32>(uiNewCapacity, 0xFFFFu);

    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(static_cast<xiiUInt16>(uiNewCapacity), pAllocator);
  }
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE xiiUInt64 xiiSmallArrayBase<T, Size>::GetHeapMemoryUsage() const
{
  return m_uiCapacity <= Size ? 0 : m_uiCapacity * sizeof(T);
}

template <typename T, xiiUInt16 Size>
template <typename U>
XII_ALWAYS_INLINE const U& xiiSmallArrayBase<T, Size>::GetUserData() const
{
  static_assert(sizeof(U) <= sizeof(xiiUInt32));
  return reinterpret_cast<const U&>(m_uiUserData);
}

template <typename T, xiiUInt16 Size>
template <typename U>
XII_ALWAYS_INLINE U& xiiSmallArrayBase<T, Size>::GetUserData()
{
  static_assert(sizeof(U) <= sizeof(xiiUInt32));
  return reinterpret_cast<U&>(m_uiUserData);
}

template <typename T, xiiUInt16 Size>
void xiiSmallArrayBase<T, Size>::SetCapacity(xiiUInt16 uiCapacity, xiiAllocator* pAllocator)
{
  if (m_uiCapacity > Size && uiCapacity > m_uiCapacity)
  {
    m_pElements  = XII_EXTEND_RAW_BUFFER(pAllocator, m_pElements, m_uiCount, uiCapacity);
    m_uiCapacity = uiCapacity;
  }
  else
  {
    // special case when migrating from in-place to external storage or shrinking
    T* pOldElements = GetElementsPtr();

    const xiiUInt32 uiOldCapacity = m_uiCapacity;
    const xiiUInt32 uiNewCapacity = uiCapacity;
    m_uiCapacity                  = xiiMath::Max(uiCapacity, Size);

    if (uiNewCapacity > Size)
    {
      // new external storage
      T* pNewElements = XII_NEW_RAW_BUFFER(pAllocator, T, uiCapacity);
      xiiMemoryUtils::RelocateConstruct(pNewElements, pOldElements, m_uiCount);
      m_pElements = pNewElements;
    }
    else
    {
      // Re-use inplace storage
      xiiMemoryUtils::RelocateConstruct(GetElementsPtr(), pOldElements, m_uiCount);
    }

    if (uiOldCapacity > Size)
    {
      XII_DELETE_RAW_BUFFER(pAllocator, pOldElements);
    }
  }
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE T* xiiSmallArrayBase<T, Size>::GetElementsPtr()
{
  return m_uiCapacity <= Size ? reinterpret_cast<T*>(m_StaticData) : m_pElements;
}

template <typename T, xiiUInt16 Size>
XII_ALWAYS_INLINE const T* xiiSmallArrayBase<T, Size>::GetElementsPtr() const
{
  return m_uiCapacity <= Size ? reinterpret_cast<const T*>(m_StaticData) : m_pElements;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiSmallArray<T, Size, AllocatorWrapper>::xiiSmallArray() = default;

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiSmallArray<T, Size, AllocatorWrapper>::xiiSmallArray(const xiiSmallArray<T, Size, AllocatorWrapper>& other) :
  SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiSmallArray<T, Size, AllocatorWrapper>::xiiSmallArray(const xiiArrayPtr<const T>& other) :
  SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiSmallArray<T, Size, AllocatorWrapper>::xiiSmallArray(xiiSmallArray<T, Size, AllocatorWrapper>&& other) :
  SUPER(static_cast<SUPER&&>(other), AllocatorWrapper::GetAllocator())
{
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiSmallArray<T, Size, AllocatorWrapper>::~xiiSmallArray()
{
  SUPER::Clear();
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::operator=(const xiiSmallArray<T, Size, AllocatorWrapper>& rhs)
{
  *this              = ((xiiArrayPtr<const T>)rhs); // redirect this to the xiiArrayPtr version
  this->m_uiUserData = rhs.m_uiUserData;
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::operator=(const xiiArrayPtr<const T>& rhs)
{
  SUPER::CopyFrom(rhs, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::operator=(xiiSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  SUPER::MoveFrom(std::move(rhs), AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::SetCount(xiiUInt16 uiCount)
{
  SUPER::SetCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::SetCount(xiiUInt16 uiCount, const T& fillValue)
{
  SUPER::SetCount(uiCount, fillValue, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::EnsureCount(xiiUInt16 uiCount)
{
  SUPER::EnsureCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::SetCountUninitialized(xiiUInt16 uiCount)
{
  SUPER::SetCountUninitialized(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::InsertAt(xiiUInt32 uiIndex, const T& value)
{
  SUPER::Insert(value, uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::InsertAt(xiiUInt32 uiIndex, T&& value)
{
  SUPER::Insert(std::move(value), uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE T& xiiSmallArray<T, Size, AllocatorWrapper>::ExpandAndGetRef()
{
  return SUPER::ExpandAndGetRef(AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::PushBack(const T& value)
{
  SUPER::PushBack(value, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::PushBack(T&& value)
{
  SUPER::PushBack(std::move(value), AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::PushBackRange(const xiiArrayPtr<const T>& range)
{
  SUPER::PushBackRange(range, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::Reserve(xiiUInt16 uiCapacity)
{
  SUPER::Reserve(uiCapacity, AllocatorWrapper::GetAllocator());
}

template <typename T, xiiUInt16 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiSmallArray<T, Size, AllocatorWrapper>::Compact()
{
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

//////////////////////////////////////////////////////////////////////////

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::iterator begin(xiiSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData();
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_iterator begin(const xiiSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_iterator cbegin(const xiiSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::reverse_iterator rbegin(xiiSmallArrayBase<T, Size>& ref_container)
{
  return typename xiiSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() + ref_container.GetCount() - 1);
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_reverse_iterator rbegin(const xiiSmallArrayBase<T, Size>& container)
{
  return typename xiiSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_reverse_iterator crbegin(const xiiSmallArrayBase<T, Size>& container)
{
  return typename xiiSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::iterator end(xiiSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData() + ref_container.GetCount();
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_iterator end(const xiiSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_iterator cend(const xiiSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::reverse_iterator rend(xiiSmallArrayBase<T, Size>& ref_container)
{
  return typename xiiSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() - 1);
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_reverse_iterator rend(const xiiSmallArrayBase<T, Size>& container)
{
  return typename xiiSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}

template <typename T, xiiUInt16 Size>
typename xiiSmallArrayBase<T, Size>::const_reverse_iterator crend(const xiiSmallArrayBase<T, Size>& container)
{
  return typename xiiSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}
