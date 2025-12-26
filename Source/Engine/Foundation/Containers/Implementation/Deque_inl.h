#include <Foundation/Math/Math.h>

#define REDUCE_SIZE(iReduction)     \
  m_iReduceSizeTimer -= iReduction; \
  if (m_iReduceSizeTimer <= 0)      \
    ReduceSize(0);

#define RESERVE(uiCount)                                         \
  if (uiCount > m_uiCount)                                       \
  {                                                              \
    m_uiMaxCount = xiiMath::Max(m_uiMaxCount, uiCount);          \
    if ((m_uiFirstElement <= 0) || (GetCurMaxCount() < uiCount)) \
      Reserve(uiCount);                                          \
  }

#define CHUNK_SIZE(Type) (4096 / sizeof(Type) < 32 ? 32 : 4096 / sizeof(Type))
// (sizeof(Type) <= 8 ? 256 : (sizeof(Type) <= 16 ? 128 : (sizeof(Type) <= 32 ? 64 : 32))) // Although this is Pow(2), this is slower than just having larger chunks

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::Constructor(xiiAllocatorBase* pAllocator)
{
  m_pAllocator        = pAllocator;
  m_pChunks           = nullptr;
  m_uiChunks          = 0;
  m_uiFirstElement    = 0;
  m_uiCount           = 0;
  m_uiAllocatedChunks = 0;
  m_uiMaxCount        = 0;

  ResetReduceSizeCounter();

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  m_uiChunkSize = CHUNK_SIZE(T);
#endif
}

template <typename T, bool Construct>
xiiDequeBase<T, Construct>::xiiDequeBase(xiiAllocatorBase* pAllocator)
{
  Constructor(pAllocator);
}

template <typename T, bool Construct>
xiiDequeBase<T, Construct>::xiiDequeBase(const xiiDequeBase<T, Construct>& rhs, xiiAllocatorBase* pAllocator)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  Constructor(pAllocator);

  *this = rhs;
}

template <typename T, bool Construct>
xiiDequeBase<T, Construct>::xiiDequeBase(xiiDequeBase<T, Construct>&& rhs, xiiAllocatorBase* pAllocator)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  Constructor(pAllocator);

  *this = std::move(rhs);
}

template <typename T, bool Construct>
xiiDequeBase<T, Construct>::~xiiDequeBase()
{
  DeallocateAll();
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::operator=(const xiiDequeBase<T, Construct>& rhs)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  Clear();                // does not deallocate anything
  RESERVE(rhs.m_uiCount); // allocates data, if required
  m_uiCount = rhs.m_uiCount;

  // copy construct all the elements
  for (xiiUInt32 i = 0; i < rhs.m_uiCount; ++i)
  {
    xiiMemoryUtils::CopyConstruct(&ElementAt(i), rhs[i], 1);
  }
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::operator=(xiiDequeBase<T, Construct>&& rhs)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  if (m_pAllocator != rhs.m_pAllocator)
  {
    operator=(static_cast<xiiDequeBase<T, Construct>&>(rhs));
  }
  else
  {
    DeallocateAll();

    m_uiCount           = rhs.m_uiCount;
    m_iReduceSizeTimer  = rhs.m_iReduceSizeTimer;
    m_pChunks           = rhs.m_pChunks;
    m_uiAllocatedChunks = rhs.m_uiAllocatedChunks;
    m_uiChunks          = rhs.m_uiChunks;
    m_uiFirstElement    = rhs.m_uiFirstElement;
    m_uiMaxCount        = rhs.m_uiMaxCount;

    rhs.m_uiCount           = 0;
    rhs.m_pChunks           = nullptr;
    rhs.m_uiAllocatedChunks = 0;
    rhs.m_uiChunks          = 0;
    rhs.m_uiFirstElement    = 0;
    rhs.m_uiMaxCount        = 0;
  }
}

template <typename T, bool Construct>
bool xiiDequeBase<T, Construct>::operator==(const xiiDequeBase<T, Construct>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < GetCount(); ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::Clear()
{
  if (Construct)
  {
    for (xiiUInt32 i = 0; i < m_uiCount; ++i)
    {
      xiiMemoryUtils::Destruct<T>(&operator[](i), 1);
    }
  }

  m_uiCount = 0;

  // since it is much more likely that data is appended at the back of the deque,
  // we do not use the center of the chunk index array, but instead set the first element
  // somewhere more at the front

  // set the first element to a position that allows to add elements at the front
  if (m_uiChunks > 30)
    m_uiFirstElement = CHUNK_SIZE(T) * 16;
  else if (m_uiChunks > 8)
    m_uiFirstElement = CHUNK_SIZE(T) * 4;
  else if (m_uiChunks > 1)
    m_uiFirstElement = CHUNK_SIZE(T) * 1;
  else if (m_uiChunks > 0)
    m_uiFirstElement = 1; // with the current implementation this case should not be possible.
  else
    m_uiFirstElement = 0; // must also work, if Clear is called on a deallocated (not yet allocated) deque
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::Reserve(xiiUInt32 uiCount)
{
  // This is the function where all the complicated stuff happens.
  // The basic idea is as follows:
  // * Do not do anything unless necessary
  // * If the index array (for the redirection) is already large enough to handle the 'address space', try to reuse it
  //   by moving data around (shift it left or right), if necessary
  // * If the chunk index array is not large enough to handle the required amount of redirections, allocate a new
  //   index array and move the old data over
  // This function does not allocate any of the chunks itself (that's what 'ElementAt' does), it only takes care
  // that the amount of reserved elements can be redirected once the deque is enlarged accordingly.

  // There is no need to change anything in this case.
  if (uiCount <= m_uiCount)
    return;

  // keeps track of the largest amount of used elements since the last memory reduction
  m_uiMaxCount = xiiMath::Max(m_uiMaxCount, uiCount);

  // if there is enough room to hold all requested elements AND one can prepend at least one element (PushFront)
  // do not reallocate
  if ((m_uiFirstElement > 0) && (GetCurMaxCount() >= uiCount))
    return;

  const xiiUInt32 uiCurFirstChunk  = GetFirstUsedChunk();
  const xiiUInt32 uiRequiredChunks = GetRequiredChunks(uiCount);

  // if we already have enough chunks, just rearrange them
  if (m_uiChunks > uiRequiredChunks + 1) // have at least one spare chunk for the front, and one for the back
  {
    const xiiUInt32 uiSpareChunks      = m_uiChunks - uiRequiredChunks;
    const xiiUInt32 uiSpareChunksStart = uiSpareChunks / 2;

    XII_ASSERT_DEBUG(uiSpareChunksStart > 0, "Implementation error.");

    // always leave one spare chunk at the front, to ensure that one can prepend elements

    XII_ASSERT_DEBUG(uiSpareChunksStart != uiCurFirstChunk, "No rearrangement possible.");

    // if the new first active chunk is to the left
    if (uiSpareChunksStart < uiCurFirstChunk)
    {
      MoveIndexChunksLeft(uiCurFirstChunk - uiSpareChunksStart);
    }
    else
    {
      MoveIndexChunksRight(uiSpareChunksStart - uiCurFirstChunk);
    }

    XII_ASSERT_DEBUG(m_uiFirstElement > 0, "Did not achieve the desired effect.");
    XII_ASSERT_DEBUG(GetCurMaxCount() >= uiCount, "Did not achieve the desired effect ({0} >= {1}).", GetCurMaxCount(), uiCount);
  }
  else
  {
    const xiiUInt32 uiReallocSize = 16 + uiRequiredChunks + 16;

    T** pNewChunksArray = XII_NEW_RAW_BUFFER(m_pAllocator, T*, uiReallocSize);
    xiiMemoryUtils::ZeroFill(pNewChunksArray, uiReallocSize);

    const xiiUInt32 uiFirstUsedChunk = m_uiFirstElement / CHUNK_SIZE(T);

    // move all old chunks over
    xiiUInt32 pos = 16;

    // first the used chunks at the start of the new array
    for (xiiUInt32 i = 0; i < m_uiChunks - uiFirstUsedChunk; ++i)
    {
      pNewChunksArray[pos] = m_pChunks[uiFirstUsedChunk + i];
      ++pos;
    }

    m_uiFirstElement -= uiFirstUsedChunk * CHUNK_SIZE(T);

    // then the unused chunks at the end of the new array
    for (xiiUInt32 i = 0; i < uiFirstUsedChunk; ++i)
    {
      pNewChunksArray[pos] = m_pChunks[i];
      ++pos;
    }

    m_uiFirstElement += 16 * CHUNK_SIZE(T);

    XII_ASSERT_DEBUG(m_uiFirstElement == (16 * CHUNK_SIZE(T)) + (m_uiFirstElement % CHUNK_SIZE(T)), "");

    XII_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);
    m_pChunks  = pNewChunksArray;
    m_uiChunks = uiReallocSize;
  }
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::Compact()
{
  ResetReduceSizeCounter();

  if (IsEmpty())
  {
    DeallocateAll();
    return;
  }

  // this will deallocate ALL unused chunks
  DeallocateUnusedChunks(GetRequiredChunks(m_uiCount));

  // reduces the size of the index array, but keeps some spare pointers, so that scaling up is still possible without reallocation
  CompactIndexArray(0);
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::Swap(xiiDequeBase<T, Construct>& other)
{
  xiiMath::Swap(this->m_pAllocator, other.m_pAllocator);
  xiiMath::Swap(this->m_pChunks, other.m_pChunks);
  xiiMath::Swap(this->m_uiChunks, other.m_uiChunks);
  xiiMath::Swap(this->m_uiFirstElement, other.m_uiFirstElement);
  xiiMath::Swap(this->m_uiCount, other.m_uiCount);
  xiiMath::Swap(this->m_uiAllocatedChunks, other.m_uiAllocatedChunks);
  xiiMath::Swap(this->m_iReduceSizeTimer, other.m_iReduceSizeTimer);
  xiiMath::Swap(this->m_uiMaxCount, other.m_uiMaxCount);
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::CompactIndexArray(xiiUInt32 uiMinChunksToKeep)
{
  const xiiUInt32 uiRequiredChunks = xiiMath::Max<xiiUInt32>(1, GetRequiredChunks(m_uiCount));
  uiMinChunksToKeep                = xiiMath::Max(uiRequiredChunks, uiMinChunksToKeep);

  // keep some spare pointers for scaling the deque up again
  const xiiUInt32 uiChunksToKeep = 16 + uiMinChunksToKeep + 16;

  // only reduce the index array, if we can reduce its size at least to half (the +4 is for the very small cases)
  if (uiChunksToKeep + 4 >= m_uiChunks / 2)
    return;

  T** pNewChunkArray = XII_NEW_RAW_BUFFER(m_pAllocator, T*, uiChunksToKeep);
  xiiMemoryUtils::ZeroFill<T*>(pNewChunkArray, uiChunksToKeep);

  const xiiUInt32 uiFirstChunk = GetFirstUsedChunk();

  // makes sure that no more than this amount of chunks is still allocated -> those can be copied over
  DeallocateUnusedChunks(uiChunksToKeep);

  // moves the used chunks into the new array
  for (xiiUInt32 i = 0; i < uiRequiredChunks; ++i)
  {
    pNewChunkArray[16 + i]      = m_pChunks[uiFirstChunk + i];
    m_pChunks[uiFirstChunk + i] = nullptr;
  }

  // copy all still allocated chunks over to the new index array
  // since we just deallocated enough chunks, all that are found can be copied over as spare chunks
  {
    xiiUInt32 iPos = 0;
    for (xiiUInt32 i = 0; i < uiFirstChunk; ++i)
    {
      if (m_pChunks[i])
      {
        XII_ASSERT_DEBUG(iPos < 16 || ((iPos >= 16 + uiRequiredChunks) && (iPos < uiChunksToKeep)), "Implementation error.");

        pNewChunkArray[iPos] = m_pChunks[i];
        m_pChunks[i]         = nullptr;
        ++iPos;

        if (iPos == 16)
        {
          iPos += uiRequiredChunks;
        }
      }
    }

    for (xiiUInt32 i = GetLastUsedChunk() + 1; i < m_uiChunks; ++i)
    {
      if (m_pChunks[i])
      {
        XII_ASSERT_DEBUG(iPos < 16 || ((iPos >= 16 + uiRequiredChunks) && (iPos < uiChunksToKeep)), "Implementation error.");

        pNewChunkArray[iPos] = m_pChunks[i];
        m_pChunks[i]         = nullptr;
        ++iPos;

        if (iPos == 16)
        {
          iPos += uiRequiredChunks;
        }
      }
    }
  }

  XII_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);
  m_pChunks        = pNewChunkArray;
  m_uiChunks       = uiChunksToKeep;
  m_uiFirstElement = (16 * CHUNK_SIZE(T)) + (m_uiFirstElement % CHUNK_SIZE(T));
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::SetCount(xiiUInt32 uiCount)
{
  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    // grow the deque

    RESERVE(uiNewCount);
    m_uiCount = uiNewCount;

    if (Construct)
    {
      // default construct the new elements
      for (xiiUInt32 i = uiOldCount; i < uiNewCount; ++i)
      {
        xiiMemoryUtils::Construct<ConstructAll>(&ElementAt(i), 1);
      }
    }
    else
    {
      for (xiiUInt32 i = uiOldCount; i < uiNewCount; ++i)
      {
        ElementAt(i);
      }
    }
  }
  else
  {
    if (Construct)
    {
      // destruct elements at the end of the deque
      for (xiiUInt32 i = uiNewCount; i < uiOldCount; ++i)
      {
        xiiMemoryUtils::Destruct(&operator[](i), 1);
      }
    }

    m_uiCount = uiNewCount;

    // if enough elements have been destructed, trigger a size reduction (the first time will not deallocate anything though)
    ReduceSize(uiOldCount - uiNewCount);
  }
}

template <typename T, bool Construct>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
// early.
void xiiDequeBase<T, Construct>::SetCountUninitialized(xiiUInt32 uiCount)
{
  static_assert(xiiIsPodType<T>::value == xiiTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");

  const xiiUInt32 uiOldCount = m_uiCount;
  const xiiUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    // grow the deque

    RESERVE(uiNewCount);
    m_uiCount = uiNewCount;

    for (xiiUInt32 i = uiOldCount; i < uiNewCount; ++i)
    {
      ElementAt(i);
    }
  }
  else
  {
    if (Construct)
    {
      // destruct elements at the end of the deque
      for (xiiUInt32 i = uiNewCount; i < uiOldCount; ++i)
      {
        xiiMemoryUtils::Destruct(&operator[](i), 1);
      }
    }

    m_uiCount = uiNewCount;

    // if enough elements have been destructed, trigger a size reduction (the first time will not deallocate anything though)
    ReduceSize(uiOldCount - uiNewCount);
  }
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::EnsureCount(xiiUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, bool Construct>
inline xiiUInt32 xiiDequeBase<T, Construct>::GetContiguousRange(xiiUInt32 uiIndex) const
{
  XII_ASSERT_DEV(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const xiiUInt32 uiChunkSize = CHUNK_SIZE(T);

  const xiiUInt32 uiRealIndex   = m_uiFirstElement + uiIndex;
  const xiiUInt32 uiChunkOffset = uiRealIndex % uiChunkSize;

  const xiiUInt32 uiRange = uiChunkSize - uiChunkOffset;

  return xiiMath::Min(uiRange, GetCount() - uiIndex);
}

template <typename T, bool Construct>
inline T& xiiDequeBase<T, Construct>::operator[](xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const xiiUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const xiiUInt32 uiChunkIndex  = uiRealIndex / CHUNK_SIZE(T);
  const xiiUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
inline const T& xiiDequeBase<T, Construct>::operator[](xiiUInt32 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const xiiUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const xiiUInt32 uiChunkIndex  = uiRealIndex / CHUNK_SIZE(T);
  const xiiUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
inline T& xiiDequeBase<T, Construct>::ExpandAndGetRef()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  T* pElement = &ElementAt(m_uiCount - 1);

  if (Construct)
  {
    xiiMemoryUtils::Construct<ConstructAll>(pElement, 1);
  }

  return *pElement;
}

template <typename T, bool Construct>
inline void xiiDequeBase<T, Construct>::PushBack()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  T* pElement = &ElementAt(m_uiCount - 1);

  if (Construct)
  {
    xiiMemoryUtils::Construct<ConstructAll>(pElement, 1);
  }
}

template <typename T, bool Construct>
inline void xiiDequeBase<T, Construct>::PushBack(const T& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  xiiMemoryUtils::CopyConstruct(&ElementAt(m_uiCount - 1), element, 1);
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::PushBack(T&& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  xiiMemoryUtils::MoveConstruct<T>(&ElementAt(m_uiCount - 1), std::move(element));
}

template <typename T, bool Construct>
inline void xiiDequeBase<T, Construct>::PopBack(xiiUInt32 uiElements)
{
  XII_ASSERT_DEV(uiElements <= GetCount(), "Cannot remove {0} elements, the deque only contains {1} elements.", uiElements, GetCount());

  for (xiiUInt32 i = 0; i < uiElements; ++i)
  {
    if (Construct)
    {
      xiiMemoryUtils::Destruct(&operator[](m_uiCount - 1), 1);
    }

    --m_uiCount;
  }

  // This may trigger a memory reduction.
  REDUCE_SIZE(uiElements);
}

template <typename T, bool Construct>
inline void xiiDequeBase<T, Construct>::PushFront(const T& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  xiiMemoryUtils::CopyConstruct(&ElementAt(0), element, 1);
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::PushFront(T&& element)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  xiiMemoryUtils::MoveConstruct<T>(&ElementAt(0), std::move(element));
}

template <typename T, bool Construct>
inline void xiiDequeBase<T, Construct>::PushFront()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  T* pElement = &ElementAt(0);

  if (Construct)
  {
    xiiMemoryUtils::Construct<SkipTrivialTypes>(pElement, 1);
  }
}

template <typename T, bool Construct>
inline void xiiDequeBase<T, Construct>::PopFront(xiiUInt32 uiElements)
{
  XII_ASSERT_DEV(uiElements <= GetCount(), "Cannot remove {0} elements, the deque only contains {1} elements.", uiElements, GetCount());

  for (xiiUInt32 i = 0; i < uiElements; ++i)
  {
    if (Construct)
    {
      xiiMemoryUtils::Destruct(&operator[](0), 1);
    }

    --m_uiCount;
    ++m_uiFirstElement;
  }

  // might trigger a memory reduction
  REDUCE_SIZE(uiElements);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE bool xiiDequeBase<T, Construct>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE xiiUInt32 xiiDequeBase<T, Construct>::GetCount() const
{
  return m_uiCount;
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE const T& xiiDequeBase<T, Construct>::PeekFront() const
{
  return operator[](0);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE T& xiiDequeBase<T, Construct>::PeekFront()
{
  return operator[](0);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE const T& xiiDequeBase<T, Construct>::PeekBack() const
{
  return operator[](m_uiCount - 1);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE T& xiiDequeBase<T, Construct>::PeekBack()
{
  return operator[](m_uiCount - 1);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE bool xiiDequeBase<T, Construct>::Contains(const T& value) const
{
  return IndexOf(value) != xiiInvalidIndex;
}

template <typename T, bool Construct>
xiiUInt32 xiiDequeBase<T, Construct>::IndexOf(const T& value, xiiUInt32 uiStartIndex) const
{
  for (xiiUInt32 i = uiStartIndex; i < m_uiCount; ++i)
  {
    if (xiiMemoryUtils::IsEqual(&operator[](i), &value))
      return i;
  }

  return xiiInvalidIndex;
}

template <typename T, bool Construct>
xiiUInt32 xiiDequeBase<T, Construct>::LastIndexOf(const T& value, xiiUInt32 uiStartIndex) const
{
  for (xiiUInt32 i = xiiMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (xiiMemoryUtils::IsEqual(&operator[](i), &value))
      return i;
  }
  return xiiInvalidIndex;
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::RemoveAtAndSwap(xiiUInt32 uiIndex)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  XII_ASSERT_DEV(uiIndex < m_uiCount, "Cannot remove element {0}, the deque only contains {1} elements.", uiIndex, m_uiCount);

  if (uiIndex + 1 < m_uiCount) // do not copy over the same element, if uiIndex is actually the last element
  {
    operator[](uiIndex) = PeekBack();
  }

  PopBack();
}

template <typename T, bool Construct>
XII_FORCE_INLINE void xiiDequeBase<T, Construct>::MoveIndexChunksLeft(xiiUInt32 uiChunkDiff)
{
  const xiiUInt32 uiCurFirstChunk   = GetFirstUsedChunk();
  const xiiUInt32 uiRemainingChunks = m_uiChunks - uiCurFirstChunk;
  const xiiUInt32 uiNewFirstChunk   = uiCurFirstChunk - uiChunkDiff;

  // ripple the chunks from the back to the front (in place)
  for (xiiUInt32 front = 0; front < uiRemainingChunks; ++front)
  {
    xiiMath::Swap(m_pChunks[uiNewFirstChunk + front], m_pChunks[front + uiCurFirstChunk]);
  }

  // just ensures that the following subtraction is possible
  XII_ASSERT_DEBUG(m_uiFirstElement > uiChunkDiff * CHUNK_SIZE(T), "");

  // adjust which element is the first by how much the index array has been moved
  m_uiFirstElement -= uiChunkDiff * CHUNK_SIZE(T);
}

template <typename T, bool Construct>
XII_FORCE_INLINE void xiiDequeBase<T, Construct>::MoveIndexChunksRight(xiiUInt32 uiChunkDiff)
{
  const xiiUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const xiiUInt32 uiLastChunk     = (m_uiCount == 0) ? (m_uiFirstElement / CHUNK_SIZE(T)) : ((m_uiFirstElement + m_uiCount - 1) / CHUNK_SIZE(T));
  const xiiUInt32 uiCopyChunks    = (uiLastChunk - uiCurFirstChunk) + 1;

  // ripple the chunks from the front to the back (in place)
  for (xiiUInt32 i = 0; i < uiCopyChunks; ++i)
  {
    xiiMath::Swap(m_pChunks[uiLastChunk - i], m_pChunks[uiLastChunk + uiChunkDiff - i]);
  }

  // adjust which element is the first by how much the index array has been moved
  m_uiFirstElement += uiChunkDiff * CHUNK_SIZE(T);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE xiiUInt32 xiiDequeBase<T, Construct>::GetFirstUsedChunk() const
{
  return m_uiFirstElement / CHUNK_SIZE(T);
}

template <typename T, bool Construct>
XII_FORCE_INLINE xiiUInt32 xiiDequeBase<T, Construct>::GetLastUsedChunk(xiiUInt32 uiAtSize) const
{
  if (uiAtSize == 0)
    return GetFirstUsedChunk();

  return (m_uiFirstElement + uiAtSize - 1) / CHUNK_SIZE(T);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE xiiUInt32 xiiDequeBase<T, Construct>::GetLastUsedChunk() const
{
  return GetLastUsedChunk(m_uiCount);
}

template <typename T, bool Construct>
XII_FORCE_INLINE xiiUInt32 xiiDequeBase<T, Construct>::GetRequiredChunks(xiiUInt32 uiAtSize) const
{
  if (uiAtSize == 0)
    return 0;

  return GetLastUsedChunk(uiAtSize) - GetFirstUsedChunk() + 1;
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::DeallocateUnusedChunks(xiiUInt32 uiMaxChunks)
{
  if (m_uiAllocatedChunks <= uiMaxChunks)
    return;

  // check all unused chunks at the end, deallocate all that are allocated
  for (xiiUInt32 i = GetLastUsedChunk() + 1; i < m_uiChunks; ++i)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      XII_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);

      if (m_uiAllocatedChunks <= uiMaxChunks)
        return;
    }
  }

  // check all unused chunks at the front, deallocate all that are allocated
  const xiiUInt32 uiFirstChunk = GetFirstUsedChunk();

  for (xiiUInt32 i = 0; i < uiFirstChunk; ++i)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      XII_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);

      if (m_uiAllocatedChunks <= uiMaxChunks)
        return;
    }
  }
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE void xiiDequeBase<T, Construct>::ResetReduceSizeCounter()
{
  m_iReduceSizeTimer = CHUNK_SIZE(T) * 8; // every time 8 chunks might be unused -> check whether to reduce the deque's size
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::ReduceSize(xiiInt32 iReduction)
{
  m_iReduceSizeTimer -= iReduction;

  // only trigger the size reduction every once in a while (after enough size reduction that actually a few chunks might be unused)
  if (m_iReduceSizeTimer > 0)
    return;

  ResetReduceSizeCounter();

  // we keep this amount of chunks
  // m_uiMaxCount will be adjusted over time
  // if the deque is shrunk and operates in this state long enough, m_uiMaxCount will be reduced more and more
  const xiiUInt32 uiMaxChunks = (m_uiMaxCount / CHUNK_SIZE(T)) + 3; // +1 because of rounding, +2 spare chunks

  XII_ASSERT_DEBUG(uiMaxChunks >= GetRequiredChunks(m_uiCount), "Implementation Error.");

  DeallocateUnusedChunks(uiMaxChunks);

  // lerp between the current MaxCount and the actually active number of elements
  // m_uiMaxCount is never smaller than m_uiCount, but m_uiCount might be smaller
  // thus m_uiMaxCount might be reduced over time
  m_uiMaxCount = xiiMath::Max(m_uiCount, (m_uiMaxCount / 2) + (m_uiCount / 2));

  // Should we really adjust the size of the index array here?
  CompactIndexArray(uiMaxChunks);
}

template <typename T, bool Construct>
XII_ALWAYS_INLINE xiiUInt32 xiiDequeBase<T, Construct>::GetCurMaxCount() const
{
  return m_uiChunks * CHUNK_SIZE(T) - m_uiFirstElement;
}

template <typename T, bool Construct>
XII_FORCE_INLINE T* xiiDequeBase<T, Construct>::GetUnusedChunk()
{
  // first search for an unused, but already allocated, chunk and reuse it, if possible
  const xiiUInt32 uiCurFirstChunk = GetFirstUsedChunk();

  // search the unused blocks at the start
  for (xiiUInt32 i = 0; i < uiCurFirstChunk; ++i)
  {
    if (m_pChunks[i])
    {
      T* pChunk    = m_pChunks[i];
      m_pChunks[i] = nullptr;
      return pChunk;
    }
  }

  const xiiUInt32 uiCurLastChunk = GetLastUsedChunk();

  // search the unused blocks at the end
  for (xiiUInt32 i = m_uiChunks - 1; i > uiCurLastChunk; --i)
  {
    if (m_pChunks[i])
    {
      T* pChunk    = m_pChunks[i];
      m_pChunks[i] = nullptr;
      return pChunk;
    }
  }

  // nothing unused found, allocate a new block
  ResetReduceSizeCounter();
  ++m_uiAllocatedChunks;
  return XII_NEW_RAW_BUFFER(m_pAllocator, T, CHUNK_SIZE(T));
}

template <typename T, bool Construct>
T& xiiDequeBase<T, Construct>::ElementAt(xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "");

  const xiiUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const xiiUInt32 uiChunkIndex  = uiRealIndex / CHUNK_SIZE(T);
  const xiiUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  XII_ASSERT_DEBUG(uiChunkIndex < m_uiChunks, "");

  if (m_pChunks[uiChunkIndex] == nullptr)
    m_pChunks[uiChunkIndex] = GetUnusedChunk();

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::DeallocateAll()
{
  Clear();

  xiiUInt32 i = 0;
  while (m_uiAllocatedChunks > 0)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      XII_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);
    }

    ++i;
  }

  XII_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);

  Constructor(m_pAllocator);
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::RemoveAtAndCopy(xiiUInt32 uiIndex)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  XII_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex);

  for (xiiUInt32 i = uiIndex + 1; i < m_uiCount; ++i)
  {
    xiiMemoryUtils::CopyOverlapped(&operator[](i - 1), &operator[](i), 1);
  }

  PopBack();
}

template <typename T, bool Construct>
bool xiiDequeBase<T, Construct>::RemoveAndCopy(const T& value)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  xiiUInt32 uiIndex = IndexOf(value);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, bool Construct>
bool xiiDequeBase<T, Construct>::RemoveAndSwap(const T& value)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  xiiUInt32 uiIndex = IndexOf(value);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::InsertAt(xiiUInt32 uiIndex, const T& value)
{
  static_assert(Construct, "This function is not supported on Deques that do not construct their data.");

  // Index 0 inserts before the first element, Index m_uiCount inserts after the last element.
  XII_ASSERT_DEV(uiIndex <= m_uiCount, "The deque has {0} elements. Cannot insert an element at index {1}.", m_uiCount, uiIndex);

  PushBack();

  for (xiiUInt32 i = m_uiCount - 1; i > uiIndex; --i)
  {
    xiiMemoryUtils::Copy(&operator[](i), &operator[](i - 1), 1);
  }

  xiiMemoryUtils::Copy(&operator[](uiIndex), &value, 1);
}

template <typename T, bool Construct>
template <typename Comparer>
void xiiDequeBase<T, Construct>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
    xiiSorting::QuickSort(*this, comparer);
}

template <typename T, bool Construct>
void xiiDequeBase<T, Construct>::Sort()
{
  if (m_uiCount > 1)
    xiiSorting::QuickSort(*this, xiiCompareHelper<T>());
}

template <typename T, bool Construct>
xiiUInt64 xiiDequeBase<T, Construct>::GetHeapMemoryUsage() const
{
  if (m_pChunks == nullptr)
    return 0;

  xiiUInt64 res = m_uiChunks * sizeof(T*);

  for (xiiUInt32 i = 0; i < m_uiChunks; ++i)
  {
    if (m_pChunks[i] != nullptr)
    {
      res += (xiiUInt64)(CHUNK_SIZE(T)) * (xiiUInt64)sizeof(T);
    }
  }

  return res;
}

#undef REDUCE_SIZE
#undef RESERVE


template <typename T, typename A, bool Construct>
xiiDeque<T, A, Construct>::xiiDeque() :
  xiiDequeBase<T, Construct>(A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
xiiDeque<T, A, Construct>::xiiDeque(xiiAllocatorBase* pAllocator) :
  xiiDequeBase<T, Construct>(pAllocator)
{
}

template <typename T, typename A, bool Construct>
xiiDeque<T, A, Construct>::xiiDeque(const xiiDeque<T, A, Construct>& other) :
  xiiDequeBase<T, Construct>(other, A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
xiiDeque<T, A, Construct>::xiiDeque(xiiDeque<T, A, Construct>&& other) :
  xiiDequeBase<T, Construct>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A, bool Construct>
xiiDeque<T, A, Construct>::xiiDeque(const xiiDequeBase<T, Construct>& other) :
  xiiDequeBase<T, Construct>(other, A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
xiiDeque<T, A, Construct>::xiiDeque(xiiDequeBase<T, Construct>&& other) :
  xiiDequeBase<T, Construct>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A, bool Construct>
void xiiDeque<T, A, Construct>::operator=(const xiiDeque<T, A, Construct>& rhs)
{
  xiiDequeBase<T, Construct>::operator=(rhs);
}

template <typename T, typename A, bool Construct>
void xiiDeque<T, A, Construct>::operator=(xiiDeque<T, A, Construct>&& rhs)
{
  xiiDequeBase<T, Construct>::operator=(std::move(rhs));
}

template <typename T, typename A, bool Construct>
void xiiDeque<T, A, Construct>::operator=(const xiiDequeBase<T, Construct>& rhs)
{
  xiiDequeBase<T, Construct>::operator=(rhs);
}

template <typename T, typename A, bool Construct>
void xiiDeque<T, A, Construct>::operator=(xiiDequeBase<T, Construct>&& rhs)
{
  xiiDequeBase<T, Construct>::operator=(std::move(rhs));
}
