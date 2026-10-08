/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>

/// Declares an id type, see generic id below how to use this
#define XII_DECLARE_ID_TYPE(name, instanceIndexBits, generationBits)                                           \
  static constexpr StorageType MAX_INSTANCES             = (1ULL << instanceIndexBits);                        \
  static constexpr StorageType INVALID_INSTANCE_INDEX    = MAX_INSTANCES - 1;                                  \
  static constexpr StorageType INDEX_AND_GENERATION_MASK = (1ULL << (instanceIndexBits + generationBits)) - 1; \
  XII_DECLARE_POD_TYPE();                                                                                      \
  XII_ALWAYS_INLINE name()                                                                                     \
  {                                                                                                            \
    m_Data = INVALID_INSTANCE_INDEX;                                                                           \
  }                                                                                                            \
  XII_ALWAYS_INLINE explicit name(StorageType internalData)                                                    \
  {                                                                                                            \
    m_Data = internalData;                                                                                     \
  }                                                                                                            \
  XII_ALWAYS_INLINE bool operator==(const name other) const                                                    \
  {                                                                                                            \
    return m_Data == other.m_Data;                                                                             \
  }                                                                                                            \
  XII_ALWAYS_INLINE bool operator<(const name other) const                                                     \
  {                                                                                                            \
    return m_Data < other.m_Data;                                                                              \
  }                                                                                                            \
  XII_ALWAYS_INLINE void Invalidate()                                                                          \
  {                                                                                                            \
    m_Data = INVALID_INSTANCE_INDEX;                                                                           \
  }                                                                                                            \
  XII_ALWAYS_INLINE bool IsInvalidated() const                                                                 \
  {                                                                                                            \
    return m_Data == INVALID_INSTANCE_INDEX;                                                                   \
  }                                                                                                            \
  XII_ALWAYS_INLINE bool IsIndexAndGenerationEqual(const name other) const                                     \
  {                                                                                                            \
    return (m_Data & INDEX_AND_GENERATION_MASK) == (other.m_Data & INDEX_AND_GENERATION_MASK);                 \
  }



/// A compact generational identifier that combines an instance index and a generation counter.
/// A generational identifier uniquely identifies an instance within an indexed collection while providing a mechanism for detecting stale references when instance indices are reused.
///
/// The identifier consists of two components:
/// - Instance Index: Identifies the slot occupied by an instance within a collection.
/// - Generation: Distinguishes successive instances that occupy the same slot.
///
/// When an instance is removed, its index may be reused for a new instance.
/// By incrementing the generation counter associated with that slot, previously issued identifiers can be distinguished from identifiers referring to the newly allocated instance.
///
/// Both components are packed into a single integral storage type, allowing the identifier to be stored, copied, and compared efficiently.
///
/// \tparam InstanceIndexBits Number of bits reserved for the instance index.
/// \tparam GenerationBits    Number of bits reserved for the generation counter.
///
/// \note The generation counter provides stale-identifier detection only when the owning collection maintains and validates generation values. Generation counter wraparound may eventually cause an old identifier to become valid again.
///
/// \note The bit-field layout is implementation-defined. The packed representation should not be assumed to have a portable binary layout across different compilers or platforms.
///
/// \see XII_DECLARE_ID_TYPE
template <xiiUInt32 InstanceIndexBits, xiiUInt32 GenerationBits>
struct xiiGenericId
{
  /// Number of bytes requested for the underlying integral storage type.
  enum
  {
    STORAGE_SIZE = ((InstanceIndexBits + GenerationBits - 1) / 8) + 1
  };

  /// Integral type used to store the packed identifier.
  using StorageType = typename xiiSizeToType<STORAGE_SIZE>::Type;

  XII_DECLARE_ID_TYPE(xiiGenericId, InstanceIndexBits, GenerationBits);

  /// Constructs an identifier from an instance index and generation counter.
  ///
  /// \param instanceIndex The index of the instance within its owning collection.
  /// \param generation    The generation associated with the instance.
  ///
  /// \note Both values must fit within their respective bit widths. Values exceeding the allocated widths may be truncated when assigned to the bit fields.
  XII_ALWAYS_INLINE xiiGenericId(StorageType instanceIndex, StorageType generation)
  {
    m_Data          = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation    = generation;
  }

  union
  {
    StorageType m_Data; ///< Packed representation containing both identifier components.

    struct
    {
      StorageType m_InstanceIndex : InstanceIndexBits; ///< Index of the referenced instance.
      StorageType m_Generation : GenerationBits;       ///< Generation associated with the instance.
    };
  };
};

#define XII_DECLARE_HANDLE_TYPE(name, idType)                                   \
public:                                                                         \
  XII_DECLARE_POD_TYPE();                                                       \
  XII_ALWAYS_INLINE name() {}                                                   \
  XII_ALWAYS_INLINE explicit name(idType internalId) : m_InternalId(internalId) \
  {                                                                             \
  }                                                                             \
  XII_ALWAYS_INLINE bool operator==(const name other) const                     \
  {                                                                             \
    return m_InternalId == other.m_InternalId;                                  \
  }                                                                             \
  XII_ALWAYS_INLINE bool operator<(const name other) const                      \
  {                                                                             \
    return m_InternalId < other.m_InternalId;                                   \
  }                                                                             \
  XII_ALWAYS_INLINE void Invalidate()                                           \
  {                                                                             \
    m_InternalId.Invalidate();                                                  \
  }                                                                             \
  XII_ALWAYS_INLINE bool IsInvalidated() const                                  \
  {                                                                             \
    return m_InternalId.IsInvalidated();                                        \
  }                                                                             \
  XII_ALWAYS_INLINE idType GetInternalID() const                                \
  {                                                                             \
    return m_InternalId;                                                        \
  }                                                                             \
  using IdType = idType;                                                        \
                                                                                \
protected:                                                                      \
  idType m_InternalId;                                                          \
         operator idType()                                                      \
  {                                                                             \
    return m_InternalId;                                                        \
  }                                                                             \
  operator const idType() const                                                 \
  {                                                                             \
    return m_InternalId;                                                        \
  }
