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


/// A generic id class that holds an id combined of an instance index and a generation counter.
///
/// \todo Document this better.
template <xiiUInt32 InstanceIndexBits, xiiUInt32 GenerationBits>
struct xiiGenericId
{
  enum
  {
    STORAGE_SIZE = ((InstanceIndexBits + GenerationBits - 1) / 8) + 1
  };
  using StorageType = typename xiiSizeToType<STORAGE_SIZE>::Type;

  XII_DECLARE_ID_TYPE(xiiGenericId, InstanceIndexBits, GenerationBits);

  XII_ALWAYS_INLINE xiiGenericId(StorageType instanceIndex, StorageType generation)
  {
    m_Data          = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation    = generation;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : InstanceIndexBits;
      StorageType m_Generation : GenerationBits;
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
