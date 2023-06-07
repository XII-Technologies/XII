
#pragma once

#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagRegistry.h>

class xiiTag;
using xiiTagSetBlockStorage = xiiUInt64;

/// \brief A dynamic collection of tags featuring fast lookups.
///
/// This class can be used to store a (dynamic) collection of tags. Tags are registered within
/// the global tag registry and allocated a bit index. The tag set allows comparatively fast lookups
/// to check if a given tag is in the set or not.
/// Adding a tag may have some overhead depending whether the block storage for the tag
/// bit indices needs to be expanded or not (if the storage needs to be expanded the hybrid array will be resized).
/// Typical storage requirements for a given tag set instance should be small since the block storage is a sliding
/// window. The standard class which can be used is xiiTagSet, usage of xiiTagSetTemplate is only necessary
/// if the allocator needs to be overridden.
template <typename BlockStorageAllocator = xiiDefaultAllocatorWrapper>
class xiiTagSetTemplate
{
public:
  xiiTagSetTemplate();

  bool operator==(const xiiTagSetTemplate& other) const;
  bool operator!=(const xiiTagSetTemplate& other) const;

  /// \brief Adds the given tag to the set.
  void Set(const xiiTag& tag); // [tested]

  /// \brief Removes the given tag.
  void Remove(const xiiTag& tag); // [tested]

  /// \brief Returns true, if the given tag is in the set.
  bool IsSet(const xiiTag& tag) const; // [tested]

  /// \brief Returns true if this tag set contains any tag set in the given other tag set.
  bool IsAnySet(const xiiTagSetTemplate& otherSet) const; // [tested]

  /// \brief Returns how many tags are in this set.
  xiiUInt32 GetNumTagsSet() const;

  /// \brief True if the tag set never contained any tag or was cleared.
  bool IsEmpty() const;

  /// \brief Removes all tags from the set
  void Clear();

  /// \brief Adds the tag with the given name. If the tag does not exist, it will be registered.
  void SetByName(const char* szTag);

  /// \brief Removes the given tag. If it doesn't exist, nothing happens.
  void RemoveByName(const char* szTag);

  /// \brief Checks whether the named tag is part of this set. Returns false if the tag does not exist.
  bool IsSetByName(const char* szTag) const;

  /// \brief Allows to iterate over all tags in this set
  class Iterator
  {
  public:
    Iterator(const xiiTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd = false);

    /// \brief Returns a reference to the current tag
    const xiiTag& operator*() const;

    /// \brief Returns a pointer to the current tag
    const xiiTag* operator->() const;

    /// \brief Returns whether the iterator is still pointing to a valid item
    XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != 0xFFFFFFFF; }

    XII_ALWAYS_INLINE bool operator!=(const Iterator& rhs) const { return m_pTagSet != rhs.m_pTagSet || m_uiIndex != rhs.m_uiIndex; }

    /// \brief Advances the iterator to the next item
    void operator++();

  private:
    bool IsBitSet() const;

    const xiiTagSetTemplate<BlockStorageAllocator>* m_pTagSet;
    xiiUInt32                                       m_uiIndex = 0;
  };

  /// \brief Returns an iterator to list all tags in this set
  Iterator GetIterator() const { return Iterator(this); }

  /// \brief Writes the tag set state to a stream. Tags itself are serialized as strings.
  void Save(xiiStreamWriter& ref_stream) const;

  /// \brief Reads the tag set state from a stream and registers the tags with the given registry.
  void Load(xiiStreamReader& ref_stream, xiiTagRegistry& ref_registry);

private:
  friend class Iterator;

  bool IsTagInAllocatedRange(const xiiTag& Tag) const;

  void Reallocate(xiiUInt32 uiNewTagBlockStart, xiiUInt32 uiNewMaxBlockIndex);

  xiiSmallArray<xiiTagSetBlockStorage, 1, BlockStorageAllocator> m_TagBlocks;

  struct UserData
  {
    xiiUInt16 m_uiTagBlockStart;
    xiiUInt16 m_uiTagCount;
  };

  xiiUInt16 GetTagBlockStart() const;
  xiiUInt16 GetTagBlockEnd() const;
  void      SetTagBlockStart(xiiUInt16 uiTagBlockStart);

  xiiUInt16 GetTagCount() const;
  void      SetTagCount(xiiUInt16 uiTagCount);
  void      IncreaseTagCount();
  void      DecreaseTagCount();
};

/// Default tag set, uses xiiDefaultAllocatorWrapper for allocations.
using xiiTagSet = xiiTagSetTemplate<>;

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTagSet);

template <typename BlockStorageAllocator>
typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator cbegin(const xiiTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator(&cont);
}

template <typename BlockStorageAllocator>
typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator cend(const xiiTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true);
}

template <typename BlockStorageAllocator>
typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator begin(const xiiTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator(&cont);
}

template <typename BlockStorageAllocator>
typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator end(const xiiTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename xiiTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true);
}

#include <Foundation/Types/Implementation/TagSet_inl.h>
