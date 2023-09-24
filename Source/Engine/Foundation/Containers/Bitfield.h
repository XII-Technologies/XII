#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Constants.h>

/// \brief A template interface, that turns any array class into a bitfield.
///
/// This class provides an interface to work with single bits, to store true/false values.
/// The underlying container is configurable, though it must support random access and a 'SetCount' function and it must use elements of type
/// xiiUInt32. In most cases a dynamic array should be used. For this case the xiiDynamicBitfield alias is already available. There is also a
/// xiiHybridBitfield alias.
template <class Container>
class xiiBitfield
{
public:
  xiiBitfield() = default;

  /// \brief Returns the number of bits that this bitfield stores.
  xiiUInt32 GetCount() const; // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. This version does NOT initialize new bits!
  template <typename = void>                        // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(xiiUInt32 uiBitCount); // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. If \a bSetNew is true, new bits are set to 1, otherwise they are cleared to 0.
  void SetCount(xiiUInt32 uiBitCount, bool bSetNew = false); // [tested]

  /// \brief Returns true, if the bitfield does not store any bits.
  bool IsEmpty() const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and any bit is 1.
  bool IsAnyBitSet(xiiUInt32 uiFirstBit = 0, xiiUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Returns true, if the bitfield is empty or all bits are set to zero.
  bool IsNoBitSet(xiiUInt32 uiFirstBit = 0, xiiUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet(xiiUInt32 uiFirstBit = 0, xiiUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Discards all bits and sets count to zero.
  void Clear(); // [tested]

  /// \brief Sets the given bit to 1.
  void SetBit(xiiUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  void ClearBit(xiiUInt32 uiBit); // [tested]

  /// \brief Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(xiiUInt32 uiBit, bool bValue); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  bool IsBitSet(xiiUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0.
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits); // [tested]

private:
  xiiUInt32 GetBitInt(xiiUInt32 uiBitIndex) const;
  xiiUInt32 GetBitMask(xiiUInt32 uiBitIndex) const;

  xiiUInt32 m_uiCount = 0;
  Container m_Container;
};

/// \brief This should be the main type of bitfield to use, although other internal container types are possible.
using xiiDynamicBitfield = xiiBitfield<xiiDynamicArray<xiiUInt32>>;

/// \brief An xiiBitfield that uses a hybrid array as internal container.
template <xiiUInt32 BITS>
using xiiHybridBitfield = xiiBitfield<xiiHybridArray<xiiUInt32, (BITS + 31) / 32>>;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
class xiiStaticBitfield
{
public:
  using StorageType = T;
  static constexpr xiiUInt32 GetStorageTypeBitCount() { return xiiMath::NumBits<T>(); }

  /// \brief Initializes the bitfield to all zero.
  xiiStaticBitfield();

  static xiiStaticBitfield<T> FromMask(StorageType bits);

  /// \brief Returns true, if the bitfield is not zero.
  bool IsAnyBitSet() const; // [tested]

  /// \brief Returns true, if the bitfield is all zero.
  bool IsNoBitSet() const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet() const; // [tested]

  /// \brief Sets the given bit to 1.
  void SetBit(xiiUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  void ClearBit(xiiUInt32 uiBit); // [tested]

  /// \brief Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(xiiUInt32 uiBit, bool bValue); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  bool IsBitSet(xiiUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0. Same as Clear().
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(xiiUInt32 uiFirstBit, xiiUInt32 uiNumBits); // [tested]

  /// \brief Returns the index of the lowest bit that is set. Returns the max index+1 in case no bit is set, at all.
  xiiUInt32 GetLowestBitSet() const; // [tested]

  /// \brief Returns the index of the highest bit that is set. Returns the max index+1 in case no bit is set, at all.
  xiiUInt32 GetHighestBitSet() const; // [tested]

  /// \brief Returns the count of how many bits are set in total.
  xiiUInt32 GetNumBitsSet() const; // [tested]

  /// \brief Returns the raw uint that stores all bits.
  T GetValue() const; // [tested]

  /// \brief Sets the raw uint that stores all bits.
  void SetValue(T value); // [tested]

  /// \brief Modifies \a this to also contain the bits from \a rhs.
  XII_ALWAYS_INLINE void operator|=(const xiiStaticBitfield<T>& rhs) { m_Storage |= rhs.m_Storage; }

  /// \brief Modifies \a this to only contain the bits that were set in \a this and \a rhs.
  XII_ALWAYS_INLINE void operator&=(const xiiStaticBitfield<T>& rhs) { m_Storage &= rhs.m_Storage; }

  xiiResult Serialize(xiiStreamWriter& ref_writer) const
  {
    ref_writer.WriteVersion(s_Version);
    ref_writer << m_Storage;
    return XII_SUCCESS;
  }

  xiiResult Deserialize(xiiStreamReader& ref_reader)
  {
    /*auto version =*/ref_reader.ReadVersion(s_Version);
    ref_reader >> m_Storage;
    return XII_SUCCESS;
  }

private:
  static constexpr xiiTypeVersion s_Version = 1;

  xiiStaticBitfield(StorageType initValue) :
    m_Storage(initValue)
  {
  }

  template <typename U>
  friend xiiStaticBitfield<U> operator|(xiiStaticBitfield<U> lhs, xiiStaticBitfield<U> rhs);

  template <typename U>
  friend xiiStaticBitfield<U> operator&(xiiStaticBitfield<U> lhs, xiiStaticBitfield<U> rhs);

  template <typename U>
  friend xiiStaticBitfield<U> operator^(xiiStaticBitfield<U> lhs, xiiStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator==(xiiStaticBitfield<U> lhs, xiiStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator!=(xiiStaticBitfield<U> lhs, xiiStaticBitfield<U> rhs);

  StorageType m_Storage = 0;
};

template <typename T>
inline xiiStaticBitfield<T> operator|(xiiStaticBitfield<T> lhs, xiiStaticBitfield<T> rhs)
{
  return xiiStaticBitfield<T>(lhs.m_Storage | rhs.m_Storage);
}

template <typename T>
inline xiiStaticBitfield<T> operator&(xiiStaticBitfield<T> lhs, xiiStaticBitfield<T> rhs)
{
  return xiiStaticBitfield<T>(lhs.m_Storage & rhs.m_Storage);
}

template <typename T>
inline xiiStaticBitfield<T> operator^(xiiStaticBitfield<T> lhs, xiiStaticBitfield<T> rhs)
{
  return xiiStaticBitfield<T>(lhs.m_Storage ^ rhs.m_Storage);
}

template <typename T>
inline bool operator==(xiiStaticBitfield<T> lhs, xiiStaticBitfield<T> rhs)
{
  return lhs.m_Storage == rhs.m_Storage;
}

template <typename T>
inline bool operator!=(xiiStaticBitfield<T> lhs, xiiStaticBitfield<T> rhs)
{
  return lhs.m_Storage != rhs.m_Storage;
}

using xiiStaticBitfield8  = xiiStaticBitfield<xiiUInt8>;
using xiiStaticBitfield16 = xiiStaticBitfield<xiiUInt16>;
using xiiStaticBitfield32 = xiiStaticBitfield<xiiUInt32>;
using xiiStaticBitfield64 = xiiStaticBitfield<xiiUInt64>;

#include <Foundation/Containers/Implementation/Bitfield_inl.h>
