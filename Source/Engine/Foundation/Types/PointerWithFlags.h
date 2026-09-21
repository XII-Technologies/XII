/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// A wrapper around a raw pointer that allows to use the lower N bits for flags
///
/// When accessing the pointer, the lower N bits are masked off.
/// Typically one can safely store 3 bits in the lower bits of a pointer as most data is 8 byte aligned,
/// especially when it was heap allocated.
template <typename PtrType, xiiUInt8 NumFlagBits = 2>
class xiiPointerWithFlags
{
private:
  enum : size_t
  {
    AllOnes   = (std::size_t)(-1),
    PtrBits   = sizeof(void*) * 8,
    FlagsMask = (AllOnes >> (PtrBits - NumFlagBits)),
    PtrMask   = ~FlagsMask,
  };

  void* m_pPtr = nullptr;

public:
  /// Initializes the pointer and flags with zero.
  xiiPointerWithFlags() = default;

  /// Initializes the pointer and flags.
  explicit xiiPointerWithFlags(PtrType* pPtr, xiiUInt8 uiFlags = 0) { SetPtrAndFlags(pPtr, uiFlags); }

  /// Changes the pointer and flags.
  void SetPtrAndFlags(PtrType* pPtr, xiiUInt8 uiFlags)
  {
    const std::uintptr_t isrc = *reinterpret_cast<std::uintptr_t*>(&pPtr);
    std::uintptr_t&      iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);

    iptr = (isrc & PtrMask) | (uiFlags & FlagsMask);
  }

  /// Returns the masked off pointer value.
  const PtrType* GetPtr() const
  {
    const std::uintptr_t& iptr = *reinterpret_cast<const std::uintptr_t*>(&m_pPtr);
    return reinterpret_cast<const PtrType*>(iptr & PtrMask); // mask off lower N bits
  }

  /// Returns the masked off pointer value.
  PtrType* GetPtr()
  {
    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);
    return reinterpret_cast<PtrType*>(iptr & PtrMask); // mask off lower N bits
  }

  /// Changes the pointer value only. Flags stay unchanged.
  void SetPtr(PtrType* pPtr)
  {
    const std::uintptr_t isrc = *reinterpret_cast<std::uintptr_t*>(&pPtr);
    XII_ASSERT_DEBUG((isrc & FlagsMask) == 0, "The given pointer does not have an {} byte alignment and thus cannot be stored lossless.", 1u << NumFlagBits);

    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);

    iptr = (isrc & PtrMask) | (iptr & FlagsMask);
  }

  /// Returns the flags value only.
  xiiUInt8 GetFlags() const
  {
    const std::uintptr_t& iptr = *reinterpret_cast<const std::uintptr_t*>(&m_pPtr);
    return static_cast<xiiUInt8>(iptr & FlagsMask);
  }

  /// Changes only the flags value. The given value must fit into the reserved bits.
  void SetFlags(xiiUInt8 uiFlags)
  {
    XII_ASSERT_DEBUG(uiFlags <= FlagsMask, "The flag value {} requires more than {} bits", uiFlags, NumFlagBits);

    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);

    iptr = (iptr & PtrMask) | (uiFlags & FlagsMask);
  }

  /// Returns the masked off pointer value.
  operator PtrType*() { return GetPtr(); }

  /// Returns the masked off pointer value.
  operator const PtrType*() const { return GetPtr(); }

  /// Changes the pointer value only. Flags stay unchanged.
  void operator=(PtrType* pPtr) { SetPtr(pPtr); }

  /// Compares the pointer part for equality (flags are ignored).
  template <typename = typename std::enable_if<std::is_const<PtrType>::value == false>>
  bool operator==(const PtrType* pPtr) const
  {
    return GetPtr() == pPtr;
  }

  bool operator==(const xiiPointerWithFlags<PtrType, NumFlagBits>& rhs) const
  {
    return GetPtr() == rhs.GetPtr();
  }

  /// Compares the pointer part for equality (flags are ignored)
  bool operator==(PtrType* pPtr) const { return GetPtr() == pPtr; }

  /// Compares the pointer part for equality (flags are ignored)
  bool operator==(std::nullptr_t) const { return GetPtr() == nullptr; }

  /// Checks whether the pointer part is not nullptr (flags are ignored)
  explicit operator bool() const { return GetPtr() != nullptr; }

  /// Dereferences the pointer.
  const PtrType* operator->() const { return GetPtr(); }

  /// Dereferences the pointer.
  PtrType* operator->() { return GetPtr(); }

  /// Dereferences the pointer.
  const PtrType& operator*() const { return *GetPtr(); }

  /// Dereferences the pointer.
  PtrType& operator*() { return *GetPtr(); }
};
