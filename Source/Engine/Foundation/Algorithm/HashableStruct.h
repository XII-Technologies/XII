/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// Curiously Recurring Template Pattern (CRTP) base for making trivially copyable structs hashable and comparable.
///
/// This class provides automatic zero-initialization, bitwise equality, ordering, hashing, and a set of utility functions.
/// It is designed for descriptor-style types that contain raw data (including pointers) and no dynamic allocation or polymorphic behavior.
///
/// \note Requires derived types to be trivially copyable and standard-layout.
///
/// \tparam DERIVED The derived struct type using CRTP.
template <typename DERIVED>
class xiiHashableStruct
{
public:
  /// Default constructor. Initializes all bytes to zero.
  constexpr xiiHashableStruct() noexcept;

  /// Bitwise copy constructor.
  xiiHashableStruct(const xiiHashableStruct& other) noexcept;

  /// Bitwise assignment operator.
  xiiHashableStruct& operator=(const xiiHashableStruct& other) noexcept;

  /// Compares equality via raw byte comparison.
  bool operator==(const xiiHashableStruct& other) const noexcept;

  /// Compares ordering via raw byte comparison.
  std::strong_ordering operator<=>(const xiiHashableStruct& other) const noexcept;

  /// Calculates a 32-bit hash from raw bytes of the struct.
  xiiUInt32 CalculateHash() const noexcept;

  /// Fills all bytes with zero.
  void Clear() noexcept;

  /// Returns true if all bytes are zero.
  bool IsZero() const noexcept;

private:
  /// Deleted virtual destructor to prevent polymorphism.
  struct NoVTable
  {
    virtual ~NoVTable() = delete;
  };
};

/// For use as the Hasher in STL unordered_map/unordered_set (e.g., xiiHash<MyType>).
template <typename T, typename = void>
struct xiiHash;

template <typename T>
struct xiiHash<T, typename std::enable_if<std::is_base_of<xiiHashableStruct<T>, T>::value>::type>
{
  size_t operator()(const T& v) const noexcept
  {
    return static_cast<size_t>(v.CalculateHash());
  }
};

#include <Foundation/Algorithm/Implementation/HashableStruct_inl.h>
