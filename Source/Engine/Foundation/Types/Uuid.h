/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

class xiiStreamReader;
class xiiStreamWriter;

/// This data type is the abstraction for 128-bit Uuid (also known as GUID) instances.
class XII_FOUNDATION_DLL xiiUuid
{
public:
  XII_DECLARE_POD_TYPE();

  /// Default constructor. Constructed Uuid will be invalid.
  XII_ALWAYS_INLINE xiiUuid() = default; // [tested]

  /// Constructs the Uuid from existing values
  XII_ALWAYS_INLINE constexpr xiiUuid(xiiUInt64 uiLow, xiiUInt64 uiHigh) :
    m_uiHigh(uiHigh), m_uiLow(uiLow)
  {
  }

  /// Comparison operator. [tested]
  XII_ALWAYS_INLINE bool operator==(const xiiUuid& other) const;

  /// Comparison operator.
  XII_ALWAYS_INLINE bool operator<(const xiiUuid& other) const;

  /// Returns true if this is a valid Uuid.
  XII_ALWAYS_INLINE bool IsValid() const;

  /// Returns an invalid UUID.
  [[nodiscard]] XII_ALWAYS_INLINE static xiiUuid MakeInvalid() { return xiiUuid(0, 0); }

  /// Returns a new Uuid.
  [[nodiscard]] static xiiUuid MakeUuid();

  /// Returns the internal 128 Bit of data
  void GetValues(xiiUInt64& ref_uiLow, xiiUInt64& ref_uiHigh) const
  {
    ref_uiHigh = m_uiHigh;
    ref_uiLow  = m_uiLow;
  }

  /// Creates a uuid from a string. The result is always the same for the same string.
  [[nodiscard]] static xiiUuid MakeStableUuidFromString(xiiStringView sString);

  /// Creates a uuid from an integer. The result is always the same for the same input.
  [[nodiscard]] static xiiUuid MakeStableUuidFromInt(xiiInt64 iInt);

  /// Adds the given seed value to this guid, creating a new guid. The process is reversible.
  XII_ALWAYS_INLINE void CombineWithSeed(const xiiUuid& seed);

  /// Subtracts the given seed from this guid, restoring the original guid.
  XII_ALWAYS_INLINE void RevertCombinationWithSeed(const xiiUuid& seed);

  /// Combines two guids using hashing, irreversible and order dependent.
  XII_ALWAYS_INLINE void HashCombine(const xiiUuid& hash);

private:
  friend XII_FOUNDATION_DLL_FRIEND void operator>>(xiiStreamReader& ref_stream, xiiUuid& ref_value);
  friend XII_FOUNDATION_DLL_FRIEND void operator<<(xiiStreamWriter& ref_stream, const xiiUuid& value);

  xiiUInt64 m_uiHigh = 0U;
  xiiUInt64 m_uiLow  = 0U;
};

#include <Foundation/Types/Implementation/Uuid_inl.h>
