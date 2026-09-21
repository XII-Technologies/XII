/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/HashedString.h>

using xiiTagSetBlockStorage = xiiUInt64;

/// The tag class stores the necessary lookup information for a single tag which can be used in conjunction with the tag set.
///
/// A tag is the storage for a small amount of lookup information for a single tag. Instances
/// of xiiTag can be used in checks with the tag set. Note that fetching information for the tag needs to access
/// the global tag registry which involves a mutex lock. It is thus
/// recommended to fetch tag instances early and reuse them for the actual tests and to avoid querying the tag registry
/// all the time (e.g. due to tag instances being kept on the stack).
class XII_FOUNDATION_DLL xiiTag
{
public:
  XII_ALWAYS_INLINE xiiTag();

  XII_ALWAYS_INLINE bool operator==(const xiiTag& rhs) const; // [tested]

  XII_ALWAYS_INLINE bool operator<(const xiiTag& rhs) const;

  XII_ALWAYS_INLINE const xiiString& GetTagString() const; // [tested]

  XII_ALWAYS_INLINE bool IsValid() const; // [tested]

private:
  template <typename BlockStorageAllocator>
  friend class xiiTagSetTemplate;
  friend class xiiTagRegistry;

  xiiHashedString m_sTagString;

  xiiUInt32 m_uiBitIndex   = 0xFFFFFFFEu;
  xiiUInt32 m_uiBlockIndex = 0xFFFFFFFEu;
};

#include <Foundation/Types/TagSet.h>

#include <Foundation/Types/Implementation/Tag_inl.h>
