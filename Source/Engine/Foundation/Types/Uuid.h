
#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

class xiiStreamReader;
class xiiStreamWriter;

/// \brief This data type is the abstraction for 128-bit Uuid (also known as GUID) instances.
class XII_FOUNDATION_DLL xiiUuid
{
public:
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor. Constructed Uuid will be invalid. [tested]
  XII_ALWAYS_INLINE xiiUuid();

  /// \brief Constructs the Uuid from existing values
  XII_ALWAYS_INLINE xiiUuid(xiiUInt64 uiLow, xiiUInt64 uiHigh)
  {
    m_uiLow  = uiLow;
    m_uiHigh = uiHigh;
  }

  /// \brief Comparison operator. [tested]
  XII_ALWAYS_INLINE bool operator==(const xiiUuid& Other) const;

  /// \brief Comparison operator. [tested]
  XII_ALWAYS_INLINE bool operator!=(const xiiUuid& Other) const;

  /// \brief Comparison operator.
  XII_ALWAYS_INLINE bool operator<(const xiiUuid& Other) const;

  /// \brief Returns true if this is a valid Uuid.
  XII_ALWAYS_INLINE bool IsValid() const;

  /// \brief Sets the Uuid to be invalid
  XII_ALWAYS_INLINE void SetInvalid();

  /// \brief Creates a new Uuid and stores is it in this object.
  void CreateNewUuid();

  /// \brief Returns the internal 128 Bit of data
  void GetValues(xiiUInt64& uiLow, xiiUInt64& uiHigh) const
  {
    uiHigh = m_uiHigh;
    uiLow  = m_uiLow;
  }

  /// \brief Creates a uuid from a string. The result is always the same for the same string.
  static xiiUuid StableUuidForString(const char* szString);

  /// \brief Creates a uuid from an integer. The result is always the same for the same input.
  static xiiUuid StableUuidForInt(xiiInt64 iInt);

  /// \brief Adds the given seed value to this guid, creating a new guid. The process is reversible.
  XII_ALWAYS_INLINE void CombineWithSeed(const xiiUuid& seed);

  /// \brief Subtracts the given seed from this guid, restoring the original guid.
  XII_ALWAYS_INLINE void RevertCombinationWithSeed(const xiiUuid& seed);

  /// \brief Combines two guids using hashing, irreversible and order dependent.
  XII_ALWAYS_INLINE void HashCombine(const xiiUuid& hash);

private:
  friend XII_FOUNDATION_DLL_FRIEND void operator>>(xiiStreamReader& Stream, xiiUuid& Value);
  friend XII_FOUNDATION_DLL_FRIEND void operator<<(xiiStreamWriter& Stream, const xiiUuid& Value);

  xiiUInt64 m_uiHigh;
  xiiUInt64 m_uiLow;
};

#include <Foundation/Types/Implementation/Uuid_inl.h>
