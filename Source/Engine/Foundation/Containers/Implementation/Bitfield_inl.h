/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <class Container>
XII_ALWAYS_INLINE xiiUInt32 xiiBitfield<Container>::GetBitInt(xiiUInt32 uiBitIndex) const
{
  return (uiBitIndex >> 5); // div 32
}

template <class Container>
XII_ALWAYS_INLINE xiiUInt32 xiiBitfield<Container>::GetBitMask(xiiUInt32 uiBitIndex) const
{
  return 1 << (uiBitIndex & 0x1F); // modulo 32, shifted to bit position
}

template <class Container>
XII_ALWAYS_INLINE xiiUInt32 xiiBitfield<Container>::GetCount() const
{
  return m_uiCount;
}

template <class Container>
template <typename> // Second template needed so that the compiler only instantiates it when called. Needed to prevent errors with containers that do not support this.
void xiiBitfield<Container>::SetCountUninitialized(xiiUInt32 uiBitCount)
{
  const xiiUInt32 uiInts = (uiBitCount + 31) >> 5;
  m_Container.SetCountUninitialized(uiInts);

  m_uiCount = uiBitCount;
}

template <class Container>
void xiiBitfield<Container>::SetCount(xiiUInt32 uiBitCount, bool bSetNew)
{
  if (m_uiCount == uiBitCount)
    return;

  const xiiUInt32 uiOldBits = m_uiCount;

  SetCountUninitialized(uiBitCount);

  // if there are new bits, initialize them
  if (uiBitCount > uiOldBits)
  {
    if (bSetNew)
    {
      SetBitRange(uiOldBits, uiBitCount - uiOldBits);
    }
    else
    {
      ClearBitRange(uiOldBits, uiBitCount - uiOldBits);
    }
  }
}

template <class Container>
XII_ALWAYS_INLINE bool xiiBitfield<Container>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <class Container>
bool xiiBitfield<Container>::IsAnyBitSet(xiiUInt32 uiFirstBit /*= 0*/, xiiUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  XII_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const xiiUInt32 uiLastBit = xiiMath::Min<xiiUInt32>(uiFirstBit + uiNumBits, m_uiCount) - 1;

  const xiiUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const xiiUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (xiiUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }
  else
  {
    const xiiUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const xiiUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (xiiUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }

    // check the bits in the ints in between with one operation
    for (xiiUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if ((m_Container[i] & 0xFFFFFFFF) != 0)
        return true;
    }

    // check the bits in the last int individually
    for (xiiUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }

  return false;
}

template <class Container>
XII_ALWAYS_INLINE bool xiiBitfield<Container>::IsNoBitSet(xiiUInt32 uiFirstBit /*= 0*/, xiiUInt32 uiLastBit /*= 0xFFFFFFFF*/) const
{
  return !IsAnyBitSet(uiFirstBit, uiLastBit);
}

template <class Container>
bool xiiBitfield<Container>::AreAllBitsSet(xiiUInt32 uiFirstBit /*= 0*/, xiiUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  XII_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const xiiUInt32 uiLastBit = xiiMath::Min<xiiUInt32>(uiFirstBit + uiNumBits, m_uiCount - 1);

  const xiiUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const xiiUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (xiiUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }
  else
  {
    const xiiUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const xiiUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (xiiUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }

    // check the bits in the ints in between with one operation
    for (xiiUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if (m_Container[i] != 0xFFFFFFFF)
        return false;
    }

    // check the bits in the last int individually
    for (xiiUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }

  return true;
}

template <class Container>
XII_ALWAYS_INLINE void xiiBitfield<Container>::Clear()
{
  m_uiCount = 0;
  m_Container.Clear();
}

template <class Container>
void xiiBitfield<Container>::SetBit(xiiUInt32 uiBit)
{
  XII_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] |= GetBitMask(uiBit);
}

template <class Container>
void xiiBitfield<Container>::ClearBit(xiiUInt32 uiBit)
{
  XII_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] &= ~GetBitMask(uiBit);
}

template <class Container>
void xiiBitfield<Container>::FlipBit(xiiUInt32 uiBit)
{
  XII_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] ^= GetBitMask(uiBit);
}

template <class Container>
XII_ALWAYS_INLINE void xiiBitfield<Container>::SetBitValue(xiiUInt32 uiBit, bool bValue)
{
  if (bValue)
  {
    SetBit(uiBit);
  }
  else
  {
    ClearBit(uiBit);
  }
}

template <class Container>
bool xiiBitfield<Container>::IsBitSet(xiiUInt32 uiBit) const
{
  XII_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  return (m_Container[GetBitInt(uiBit)] & GetBitMask(uiBit)) != 0;
}

template <class Container>
void xiiBitfield<Container>::ClearAllBits()
{
  for (xiiUInt32 i = 0; i < m_Container.GetCount(); ++i)
  {
    m_Container[i] = 0;
  }
}

template <class Container>
void xiiBitfield<Container>::SetAllBits()
{
  for (xiiUInt32 i = 0; i < m_Container.GetCount(); ++i)
  {
    m_Container[i] = 0xFFFFFFFF;
  }
}

template <class Container>
void xiiBitfield<Container>::SetBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  XII_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const xiiUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const xiiUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const xiiUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (xiiUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      SetBit(i);
    }

    return;
  }

  const xiiUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const xiiUInt32 uiPrevIntBit = uiLastInt * 32;

  // set the bits in the first int individually
  for (xiiUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
  {
    SetBit(i);
  }

  // set the bits in the ints in between with one operation
  for (xiiUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
  {
    m_Container[i] = 0xFFFFFFFF;
  }

  // set the bits in the last int individually
  for (xiiUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
  {
    SetBit(i);
  }
}

template <class Container>
void xiiBitfield<Container>::ClearBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  XII_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const xiiUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const xiiUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const xiiUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (xiiUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      ClearBit(i);
    }

    return;
  }

  const xiiUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const xiiUInt32 uiPrevIntBit = uiLastInt * 32;

  // set the bits in the first int individually
  for (xiiUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
  {
    ClearBit(i);
  }

  // set the bits in the ints in between with one operation
  for (xiiUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
  {
    m_Container[i] = 0;
  }

  // set the bits in the last int individually
  for (xiiUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
  {
    ClearBit(i);
  }
}

template <class Container>
void xiiBitfield<Container>::FlipBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  XII_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const xiiUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const xiiUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const xiiUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // All within the same int.
  if (uiFirstInt == uiLastInt)
  {
    for (xiiUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      FlipBit(i);
    }
    return;
  }

  const xiiUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const xiiUInt32 uiPrevIntBit = uiLastInt * 32;

  // Flip the bits in the first int individually.
  for (xiiUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
  {
    FlipBit(i);
  }

  // Flip the bits in the ints in between with one operation.
  for (xiiUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
  {
    m_Container[i] = ~m_Container[i];
  }

  // Flip the bits in the last int individually.
  for (xiiUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
  {
    FlipBit(i);
  }
}

template <class Container>
void xiiBitfield<Container>::Swap(xiiBitfield<Container>& other)
{
  xiiMath::Swap(m_uiCount, other.m_uiCount);
  m_Container.Swap(other.m_Container);
}

template <class Container>
XII_ALWAYS_INLINE typename xiiBitfield<Container>::ConstIterator xiiBitfield<Container>::GetIterator() const
{
  return ConstIterator(*this);
};

template <class Container>
XII_ALWAYS_INLINE typename xiiBitfield<Container>::ConstIterator xiiBitfield<Container>::GetEndIterator() const
{
  return ConstIterator();
};

//////////////////////////////////////////////////////////////////////////
// xiiBitfield<Container>::ConstIterator

template <class Container>
xiiBitfield<Container>::ConstIterator::ConstIterator(const xiiBitfield<Container>& bitfield)
{
  m_pBitfield = &bitfield;
  FindNextChunk(0);
}

template <class Container>
XII_ALWAYS_INLINE bool xiiBitfield<Container>::ConstIterator::IsValid() const
{
  return m_pBitfield != nullptr;
}

template <class Container>
XII_ALWAYS_INLINE xiiUInt32 xiiBitfield<Container>::ConstIterator::Value() const
{
  return *m_Iterator + (m_uiChunk << 5);
}

template <class Container>
XII_ALWAYS_INLINE void xiiBitfield<Container>::ConstIterator::Next()
{
  ++m_Iterator;
  if (!m_Iterator.IsValid())
  {
    FindNextChunk(m_uiChunk + 1);
  }
}

template <class Container>
XII_ALWAYS_INLINE bool xiiBitfield<Container>::ConstIterator::operator==(const ConstIterator& other) const
{
  return m_pBitfield == other.m_pBitfield && m_Iterator == other.m_Iterator && m_uiChunk == other.m_uiChunk;
}

template <class Container>
XII_ALWAYS_INLINE xiiUInt32 xiiBitfield<Container>::ConstIterator::operator*() const
{
  return Value();
}

template <class Container>
XII_ALWAYS_INLINE void xiiBitfield<Container>::ConstIterator::operator++()
{
  Next();
}

template <class Container>
void xiiBitfield<Container>::ConstIterator::FindNextChunk(xiiUInt32 uiStartChunk)
{
  if (uiStartChunk < m_pBitfield->m_Container.GetCount())
  {
    const xiiUInt32 uiLastChunk = m_pBitfield->m_Container.GetCount() - 1;
    for (xiiUInt32 i = uiStartChunk; i < uiLastChunk; ++i)
    {
      if (m_pBitfield->m_Container[i] != 0)
      {
        m_uiChunk  = i;
        m_Iterator = sub_iterator(m_pBitfield->m_Container[i]);
        return;
      }
    }

    const xiiUInt32 uiMask = 0xFFFFFFFF >> (32 - (m_pBitfield->m_uiCount - (uiLastChunk << 5)));
    if ((m_pBitfield->m_Container[uiLastChunk] & uiMask) != 0)
    {
      m_uiChunk  = uiLastChunk;
      m_Iterator = sub_iterator(m_pBitfield->m_Container[uiLastChunk] & uiMask);
      return;
    }
  }

  // End iterator.
  m_pBitfield = nullptr;
  m_uiChunk   = 0;
  m_Iterator  = sub_iterator();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
XII_ALWAYS_INLINE xiiStaticBitfield<T>::xiiStaticBitfield()
{
  static_assert(std::is_unsigned<T>::value, "Storage type must be unsigned");
}

template <typename T>
XII_ALWAYS_INLINE xiiStaticBitfield<T> xiiStaticBitfield<T>::MakeFromMask(StorageType bits)
{
  return xiiStaticBitfield<T>(bits);
}

template <typename T>
XII_ALWAYS_INLINE bool xiiStaticBitfield<T>::IsAnyBitSet() const
{
  return m_Storage != 0;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiStaticBitfield<T>::IsNoBitSet() const
{
  return m_Storage == 0;
}

template <typename T>
bool xiiStaticBitfield<T>::AreAllBitsSet() const
{
  const T inv = ~m_Storage;
  return inv == 0;
}

template <typename T>
void xiiStaticBitfield<T>::ClearBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits)
{
  XII_ASSERT_DEBUG(uiFirstBit < GetStorageTypeBitCount(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetStorageTypeBitCount());

  T mask = (uiNumBits / 8 >= sizeof(T)) ? (~static_cast<T>(0)) : ((static_cast<T>(1) << uiNumBits) - 1);
  mask <<= uiFirstBit;
  mask = ~mask;
  m_Storage &= mask;
}

template <typename T>
void xiiStaticBitfield<T>::SetBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits)
{
  XII_ASSERT_DEBUG(uiFirstBit < GetStorageTypeBitCount(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetStorageTypeBitCount());

  T mask = (uiNumBits / 8 >= sizeof(T)) ? (~static_cast<T>(0)) : ((static_cast<T>(1) << uiNumBits) - 1);
  mask <<= uiFirstBit;
  m_Storage |= mask;
}

template <typename T>
XII_ALWAYS_INLINE xiiUInt32 xiiStaticBitfield<T>::GetNumBitsSet() const
{
  return xiiMath::CountBits(m_Storage);
}

template <typename T>
XII_ALWAYS_INLINE xiiUInt32 xiiStaticBitfield<T>::GetHighestBitSet() const
{
  return m_Storage == 0 ? GetStorageTypeBitCount() : xiiMath::FirstBitHigh(m_Storage);
}

template <typename T>
XII_ALWAYS_INLINE xiiUInt32 xiiStaticBitfield<T>::GetLowestBitSet() const
{
  return m_Storage == 0 ? GetStorageTypeBitCount() : xiiMath::FirstBitLow(m_Storage);
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::SetAllBits()
{
  m_Storage = xiiMath::MaxValue<T>(); // possible because we assert that T is unsigned
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::ClearAllBits()
{
  m_Storage = 0;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiStaticBitfield<T>::IsBitSet(xiiUInt32 uiBit) const
{
  XII_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  return (m_Storage & (static_cast<T>(1u) << uiBit)) != 0;
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::ClearBit(xiiUInt32 uiBit)
{
  XII_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  m_Storage &= ~(static_cast<T>(1u) << uiBit);
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::SetBitValue(xiiUInt32 uiBit, bool bValue)
{
  if (bValue)
  {
    SetBit(uiBit);
  }
  else
  {
    ClearBit(uiBit);
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::SetBit(xiiUInt32 uiBit)
{
  XII_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  m_Storage |= static_cast<T>(1u) << uiBit;
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::SetValue(T value)
{
  m_Storage = value;
}

template <typename T>
XII_ALWAYS_INLINE T xiiStaticBitfield<T>::GetValue() const
{
  return m_Storage;
}

template <typename T>
XII_ALWAYS_INLINE void xiiStaticBitfield<T>::Swap(xiiStaticBitfield<T>& other)
{
  xiiMath::Swap(m_Storage, other.m_Storage);
}
