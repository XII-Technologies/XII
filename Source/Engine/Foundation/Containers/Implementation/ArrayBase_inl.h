
template <typename T, typename Derived>
xiiArrayBase<T, Derived>::xiiArrayBase() = default;

template <typename T, typename Derived>
xiiArrayBase<T, Derived>::~xiiArrayBase()
{
  XII_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  XII_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::operator=(const xiiArrayPtr<const T>& rhs)
{
  if (this->GetData() == rhs.GetPtr())
  {
    if (m_uiCount == rhs.GetCount())
      return;

    XII_ASSERT_DEV(m_uiCount > rhs.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");

    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    xiiMemoryUtils::Destruct(pElements + rhs.GetCount(), m_uiCount - rhs.GetCount());
    m_uiCount = rhs.GetCount();
    return;
  }

  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = rhs.GetCount();

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    xiiMemoryUtils::Copy(pElements, rhs.GetPtr(), uiOldCount);
    xiiMemoryUtils::CopyConstructArray(pElements + uiOldCount, rhs.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    xiiMemoryUtils::Copy(pElements, rhs.GetPtr(), uiNewCount);
    xiiMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiNewCount;
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiArrayBase<T, Derived>::operator xiiArrayPtr<const T>() const
{
  return xiiArrayPtr<const T>(static_cast<const Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiArrayBase<T, Derived>::operator xiiArrayPtr<T>()
{
  return xiiArrayPtr<T>(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
bool xiiArrayBase<T, Derived>::operator==(const xiiArrayBase<T, Derived>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return xiiMemoryUtils::IsEqual(static_cast<const Derived*>(this)->GetElementsPtr(), rhs.GetData(), m_uiCount);
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE bool xiiArrayBase<T, Derived>::operator<(const xiiArrayBase<T, Derived>& rhs) const
{
  return GetArrayPtr() < rhs.GetArrayPtr();
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE bool xiiArrayBase<T, Derived>::operator<(const xiiArrayPtr<const T>& rhs) const
{
  return GetArrayPtr() < rhs;
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE const T& xiiArrayBase<T, Derived>::operator[](const xiiUInt32 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);

  return static_cast<const Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE T& xiiArrayBase<T, Derived>::operator[](const xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);

  return static_cast<Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::SetCount(xiiUInt32 uiCount)
{
  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    xiiMemoryUtils::Construct<ConstructAll>(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    xiiMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::SetCount(xiiUInt32 uiCount, const T& fillValue)
{
  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    xiiMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    xiiMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::EnsureCount(xiiUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, typename Derived>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
// early.
void xiiArrayBase<T, Derived>::SetCountUninitialized(xiiUInt32 uiCount)
{
  static_assert(xiiIsPodType<T>::value == xiiTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");
  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);

    // We already assert above that T is a POD type. Do not construct anything and leave the memory untouched.
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiUInt32 xiiArrayBase<T, Derived>::GetCount() const
{
  return m_uiCount;
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE bool xiiArrayBase<T, Derived>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::Clear()
{
  xiiMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, typename Derived>
bool xiiArrayBase<T, Derived>::Contains(const T& value) const
{
  return IndexOf(value) != xiiInvalidIndex;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::InsertAt(xiiUInt32 uiIndex, const T& value)
{
  XII_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  xiiMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::InsertAt(xiiUInt32 uiIndex, T&& value)
{
  XII_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  xiiMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::InsertRange(const xiiArrayPtr<const T>& range, xiiUInt32 uiIndex)
{
  const xiiUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  xiiMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, range.GetPtr(), uiRangeCount, m_uiCount - uiIndex);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
bool xiiArrayBase<T, Derived>::RemoveAndCopy(const T& value)
{
  xiiUInt32 uiIndex = IndexOf(value);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, typename Derived>
bool xiiArrayBase<T, Derived>::RemoveAndSwap(const T& value)
{
  xiiUInt32 uiIndex = IndexOf(value);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::RemoveAtAndCopy(xiiUInt32 uiIndex, xiiUInt32 uiNumElements /*= 1*/)
{
  XII_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  m_uiCount -= uiNumElements;
  xiiMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::RemoveAtAndSwap(xiiUInt32 uiIndex, xiiUInt32 uiNumElements /*= 1*/)
{
  XII_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

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

template <typename T, typename Derived>
xiiUInt32 xiiArrayBase<T, Derived>::IndexOf(const T& value, xiiUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (xiiUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (xiiMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return xiiInvalidIndex;
}

template <typename T, typename Derived>
xiiUInt32 xiiArrayBase<T, Derived>::LastIndexOf(const T& value, xiiUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (xiiUInt32 i = xiiMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (xiiMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return xiiInvalidIndex;
}

template <typename T, typename Derived>
T& xiiArrayBase<T, Derived>::ExpandAndGetRef()
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  xiiMemoryUtils::Construct<SkipTrivialTypes>(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, typename Derived>
T* xiiArrayBase<T, Derived>::ExpandBy(xiiUInt32 uiNumNewItems)
{
  this->SetCount(this->GetCount() + uiNumNewItems);
  return GetArrayPtr().GetEndPtr() - uiNumNewItems;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::PushBack(const T& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  xiiMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::PushBack(T&& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  xiiMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::PushBackUnchecked(const T& value)
{
  XII_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  xiiMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::PushBackUnchecked(T&& value)
{
  XII_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  xiiMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::PushBackRange(const xiiArrayPtr<const T>& range)
{
  const xiiUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  xiiMemoryUtils::CopyConstructArray(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::PopBack(xiiUInt32 uiCountToRemove /* = 1 */)
{
  XII_ASSERT_DEV(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  xiiMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, typename Derived>
XII_FORCE_INLINE T& xiiArrayBase<T, Derived>::PeekBack()
{
  XII_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");

  return static_cast<Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
XII_FORCE_INLINE const T& xiiArrayBase<T, Derived>::PeekBack() const
{
  XII_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");

  return static_cast<const Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
template <typename Comparer>
void xiiArrayBase<T, Derived>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    xiiArrayPtr<T> ar = *this;
    xiiSorting::QuickSort(ar, comparer);
  }
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::Sort()
{
  if (m_uiCount > 1)
  {
    xiiArrayPtr<T> ar = *this;
    xiiSorting::QuickSort(ar, xiiCompareHelper<T>());
  }
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE T* xiiArrayBase<T, Derived>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return static_cast<Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE const T* xiiArrayBase<T, Derived>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return static_cast<const Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiArrayPtr<T> xiiArrayBase<T, Derived>::GetArrayPtr()
{
  return xiiArrayPtr<T>(GetData(), GetCount());
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiArrayPtr<const T> xiiArrayBase<T, Derived>::GetArrayPtr() const
{
  return xiiArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiArrayPtr<typename xiiArrayPtr<T>::ByteType> xiiArrayBase<T, Derived>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
XII_ALWAYS_INLINE xiiArrayPtr<typename xiiArrayPtr<const T>::ByteType> xiiArrayBase<T, Derived>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
void xiiArrayBase<T, Derived>::DoSwap(xiiArrayBase<T, Derived>& other)
{
  xiiMath::Swap(this->m_pElements, other.m_pElements);
  xiiMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  xiiMath::Swap(this->m_uiCount, other.m_uiCount);
}
