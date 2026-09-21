/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// This class provides implementations of different hashing algorithms.
class XII_FOUNDATION_DLL xiiHashingUtils
{
public:
  /// Calculates the CRC32 checksum of the given key.
  static xiiUInt32 CRC32Hash(const void* pKey, size_t uiSizeInBytes); // [tested]

  /// Calculates the 32bit murmur hash of the given key.
  static xiiUInt32 MurmurHash32(const void* pKey, size_t uiSizeInByte, xiiUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit murmur hash of the given key.
  static xiiUInt64 MurmurHash64(const void* pKey, size_t uiSizeInByte, xiiUInt64 uiSeed = 0); // [tested]

  /// Calculates the 32bit murmur hash of a string constant at compile time. Encoding does not matter here.
  template <size_t N>
  constexpr static xiiUInt32 MurmurHash32String(const char (&str)[N], xiiUInt32 uiSeed = 0); // [tested]

  /// Calculates the 32bit murmur hash of a string pointer during runtime. Encoding does not matter here.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static xiiUInt32 MurmurHash32String(xiiStringView sStr, xiiUInt32 uiSeed = 0); // [tested]

  /// Calculates the 32bit xxHash of the given key.
  static xiiUInt32 xxHash32(const void* pKey, size_t uiSizeInByte, xiiUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit xxHash of the given key.
  static xiiUInt64 xxHash64(const void* pKey, size_t uiSizeInByte, xiiUInt64 uiSeed = 0); // [tested]

  /// Calculates the 32bit xxHash of the given string literal at compile time.
  template <size_t N>
  constexpr static xiiUInt32 xxHash32String(const char (&str)[N], xiiUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit xxHash of the given string literal at compile time.
  template <size_t N>
  constexpr static xiiUInt64 xxHash64String(const char (&str)[N], xiiUInt64 uiSeed = 0); // [tested]

  /// Calculates the 32bit xxHash of a string pointer during runtime.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static xiiUInt32 xxHash32String(xiiStringView sStr, xiiUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit xxHash of a string pointer during runtime.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static xiiUInt64 xxHash64String(xiiStringView sStr, xiiUInt64 uiSeed = 0); // [tested]

  /// Calculates the hash of the given string literal at compile time.
  template <size_t N>
  constexpr static xiiUInt64 StringHash(const char (&str)[N], xiiUInt64 uiSeed = 0); // [tested]

  /// Calculates the hash of a string pointer at runtime.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static xiiUInt64 StringHash(xiiStringView sStr, xiiUInt64 uiSeed = 0); // [tested]

  /// Truncates a 64 bit string hash to 32 bit.
  ///
  /// This is necessary when a 64 bit string hash is used in a hash table (which only uses 32 bit indices).
  constexpr static xiiUInt32 StringHashTo32(xiiUInt64 uiHash);

  /// Combines two 32 bit hash values into one.
  constexpr static xiiUInt32 CombineHashValues32(xiiUInt32 ui0, xiiUInt32 ui1);
};

/// Helper struct to calculate the Hash of different types.
///
/// This struct can be used to provide a custom hash function for xiiHashTable. The default implementation uses the xxHash function.
template <typename T>
struct xiiHashHelper
{
  template <typename U>
  static xiiUInt32 Hash(const U& value);

  template <typename U>
  static bool Equal(const T& a, const U& b);
};

#include <Foundation/Algorithm/Implementation/HashingMurmur_inl.h>
#include <Foundation/Algorithm/Implementation/HashingUtils_inl.h>
#include <Foundation/Algorithm/Implementation/HashingXxHash_inl.h>
