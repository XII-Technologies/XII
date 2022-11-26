#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>

template <typename T>
XII_ALWAYS_INLINE xiiHashableStruct<T>::xiiHashableStruct()
{
  xiiMemoryUtils::ZeroFill<T>(static_cast<T*>(this), 1);
}

template <typename T>
XII_ALWAYS_INLINE xiiHashableStruct<T>::xiiHashableStruct(const xiiHashableStruct<T>& other)
{
  xiiMemoryUtils::RawByteCopy(this, &other, sizeof(T));
}

template <typename T>
XII_ALWAYS_INLINE void xiiHashableStruct<T>::operator=(const xiiHashableStruct<T>& other)
{
  if (this != &other)
  {
    xiiMemoryUtils::RawByteCopy(this, &other, sizeof(T));
  }
}

template <typename T>
XII_ALWAYS_INLINE xiiUInt32 xiiHashableStruct<T>::CalculateHash() const
{
  return xiiHashingUtils::xxHash32(this, sizeof(T));
}
