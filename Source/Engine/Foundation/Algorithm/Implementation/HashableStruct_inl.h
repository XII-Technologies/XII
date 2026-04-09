#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>

template <typename DERIVED>
XII_ALWAYS_INLINE constexpr xiiHashableStruct<DERIVED>::xiiHashableStruct() noexcept
{
  xiiMemoryUtils::ZeroFill<DERIVED>(static_cast<DERIVED*>(this), 1U);
}

template <typename DERIVED>
XII_ALWAYS_INLINE xiiHashableStruct<DERIVED>::xiiHashableStruct(const xiiHashableStruct& other) noexcept
{
  xiiMemoryUtils::RawByteCopy(this, &other, sizeof(DERIVED));
}

template <typename DERIVED>
XII_ALWAYS_INLINE xiiHashableStruct<DERIVED>& xiiHashableStruct<DERIVED>::operator=(const xiiHashableStruct& other) noexcept
{
  if (this != &other)
  {
    xiiMemoryUtils::RawByteCopy(this, &other, sizeof(DERIVED));
  }
  return *this;
}

template <typename DERIVED>
XII_ALWAYS_INLINE bool xiiHashableStruct<DERIVED>::operator==(const xiiHashableStruct& other) const noexcept
{
  return xiiMemoryUtils::RawByteCompare(this, &other, sizeof(DERIVED)) == 0;
}

template <typename DERIVED>
XII_ALWAYS_INLINE std::strong_ordering xiiHashableStruct<DERIVED>::operator<=>(const xiiHashableStruct& other) const noexcept
{
  return xiiMemoryUtils::RawByteCompare(this, &other, sizeof(DERIVED)) <=> 0;
}

template <typename DERIVED>
XII_ALWAYS_INLINE xiiUInt32 xiiHashableStruct<DERIVED>::CalculateHash() const noexcept
{
  return xiiHashingUtils::xxHash32(this, sizeof(DERIVED));
}

template <typename DERIVED>
XII_ALWAYS_INLINE void xiiHashableStruct<DERIVED>::Clear() noexcept
{
  xiiMemoryUtils::ZeroFill<DERIVED>(static_cast<DERIVED*>(this), 1U);
}

template <typename DERIVED>
XII_ALWAYS_INLINE bool xiiHashableStruct<DERIVED>::IsZero() const noexcept
{
  const DERIVED zero{};
  return xiiMemoryUtils::RawByteCompare(this, &zero, sizeof(DERIVED)) == 0;
}
