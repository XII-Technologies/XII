#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>

// Template specialization to be able to use xiiTagSet properties as XII_SET_MEMBER_PROPERTY.
template <typename T>
struct xiiContainerSubTypeResolver<xiiTagSetTemplate<T>>
{
  using Type = xiiStringView;
};

// Template specialization to be able to use xiiTagSet properties as XII_SET_MEMBER_PROPERTY.
template <typename Class>
class xiiMemberSetProperty<Class, xiiTagSet, xiiStringView> : public xiiTypedSetProperty<typename xiiTypeTraits<xiiStringView>::NonConstReferenceType>
{
public:
  using Container             = xiiTagSet;
  using Type                  = xiiStringView;
  using RealType              = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class*);
  using GetContainerFunc      = Container& (*)(Class*);

  xiiMemberSetProperty(xiiStringView sPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter) :
    xiiTypedSetProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an set property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter      = getter;

    if (m_Getter == nullptr)
    {
      xiiAbstractSetProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
    }
  }

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).SetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).RemoveByName(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, xiiDynamicArray<xiiVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : m_ConstGetter(static_cast<const Class*>(pInstance)))
    {
      out_keys.PushBack(xiiVariant(value.GetTagString()));
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc      m_Getter;
};

// Template specialization to be able to use xiiTagSet properties as XII_SET_ACCESSOR_PROPERTY.
template <typename Class>
class xiiAccessorSetProperty<Class, xiiStringView, const xiiTagSet&> : public xiiTypedSetProperty<xiiStringView>
{
public:
  using Container = const xiiTagSet&;
  using Type      = xiiStringView;

  using ContainerType = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using RealType      = typename xiiTypeTraits<Type>::NonConstReferenceType;

  using InsertFunc    = void (Class::*)(Type value);
  using RemoveFunc    = void (Class::*)(Type value);
  using GetValuesFunc = Container (Class::*)() const;

  xiiAccessorSetProperty(xiiStringView sPropertyName, GetValuesFunc getValues, InsertFunc insert, RemoveFunc remove) :
    xiiTypedSetProperty<Type>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getValues != nullptr, "The get values function of an set property cannot be nullptr.");

    m_GetValues = getValues;
    m_Insert    = insert;
    m_Remove    = remove;

    if (m_Insert == nullptr || m_Remove == nullptr)
    {
      xiiAbstractSetProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
    }
  }


  virtual bool IsEmpty(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is read-only", xiiAbstractProperty::GetPropertyName());

    // We must not cache the container c here as the Remove can make it invalid
    // e.g. xiiArrayPtr by value.
    while (!IsEmpty(pInstance))
    {
      // this should be decltype(auto) c = ...; but MSVC 16 is too dumb for that (MSVC 15 works fine)
      decltype((static_cast<const Class*>(pInstance)->*m_GetValues)()) c     = (static_cast<const Class*>(pInstance)->*m_GetValues)();
      auto                                                             it    = cbegin(c);
      const xiiTag&                                                    value = *it;
      Remove(pInstance, value.GetTagString().GetData());
    }
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Insert)(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Remove)(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, xiiDynamicArray<xiiVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : (static_cast<const Class*>(pInstance)->*m_GetValues)())
    {
      out_keys.PushBack(xiiVariant(value.GetTagString()));
    }
  }

private:
  GetValuesFunc m_GetValues;
  InsertFunc    m_Insert;
  RemoveFunc    m_Remove;
};


template <typename BlockStorageAllocator>
xiiTagSetTemplate<BlockStorageAllocator>::Iterator::Iterator(const xiiTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd) :
  m_pTagSet(pSet)
{
  if (!bEnd)
  {
    m_uiIndex = m_pTagSet->GetTagBlockStart() * (sizeof(xiiTagSetBlockStorage) * 8);

    if (m_pTagSet->IsEmpty())
    {
      m_uiIndex = 0xFFFFFFFFU;
    }
    else
    {
      if (!IsBitSet())
      {
        operator++();
      }
    }
  }
  else
  {
    m_uiIndex = 0xFFFFFFFFU;
  }
}

template <typename BlockStorageAllocator>
bool xiiTagSetTemplate<BlockStorageAllocator>::Iterator::IsBitSet() const
{
  xiiTag TempTag;
  TempTag.m_uiBlockIndex = m_uiIndex / (sizeof(xiiTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex   = m_uiIndex - (TempTag.m_uiBlockIndex * sizeof(xiiTagSetBlockStorage) * 8);

  return m_pTagSet->IsSet(TempTag);
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::Iterator::operator++()
{
  const xiiUInt32 uiMax = m_pTagSet->GetTagBlockEnd() * (sizeof(xiiTagSetBlockStorage) * 8);

  do
  {
    ++m_uiIndex;
  } while (m_uiIndex < uiMax && !IsBitSet());

  if (m_uiIndex >= uiMax)
  {
    m_uiIndex = 0xFFFFFFFFU;
  }
}

template <typename BlockStorageAllocator>
const xiiTag& xiiTagSetTemplate<BlockStorageAllocator>::Iterator::operator*() const
{
  return *xiiTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
const xiiTag* xiiTagSetTemplate<BlockStorageAllocator>::Iterator::operator->() const
{
  return xiiTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
xiiTagSetTemplate<BlockStorageAllocator>::xiiTagSetTemplate()
{
  SetTagBlockStart(xiiSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
bool xiiTagSetTemplate<BlockStorageAllocator>::operator==(const xiiTagSetTemplate& other) const
{
  return m_TagBlocks == other.m_TagBlocks && m_TagBlocks.template GetUserData<xiiUInt32>() == other.m_TagBlocks.template GetUserData<xiiUInt32>();
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::Set(const xiiTag& tag)
{
  XII_ASSERT_DEV(tag.IsValid(), "Only valid tags can be set in a tag set!");

  if (m_TagBlocks.IsEmpty())
  {
    Reallocate(tag.m_uiBlockIndex, tag.m_uiBlockIndex);
  }
  else if (IsTagInAllocatedRange(tag) == false)
  {
    const xiiUInt32 uiNewBlockStart = xiiMath::Min<xiiUInt32>(tag.m_uiBlockIndex, GetTagBlockStart());
    const xiiUInt32 uiNewBlockEnd   = xiiMath::Max<xiiUInt32>(tag.m_uiBlockIndex, GetTagBlockEnd());

    Reallocate(uiNewBlockStart, uiNewBlockEnd);
  }

  xiiUInt64& tagBlock = m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()];

  const xiiUInt64 bitMask    = XII_BIT(tag.m_uiBitIndex);
  const bool      bBitWasSet = ((tagBlock & bitMask) != 0);

  tagBlock |= bitMask;

  if (!bBitWasSet)
  {
    IncreaseTagCount();
  }
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::Remove(const xiiTag& tag)
{
  XII_ASSERT_DEV(tag.IsValid(), "Only valid tags can be cleared from a tag set!");

  if (IsTagInAllocatedRange(tag))
  {
    xiiUInt64& tagBlock = m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()];

    const xiiUInt64 bitMask    = XII_BIT(tag.m_uiBitIndex);
    const bool      bBitWasSet = ((tagBlock & bitMask) != 0);

    tagBlock &= ~bitMask;

    if (bBitWasSet)
    {
      DecreaseTagCount();
    }
  }
}

template <typename BlockStorageAllocator>
bool xiiTagSetTemplate<BlockStorageAllocator>::IsSet(const xiiTag& tag) const
{
  XII_ASSERT_DEV(tag.IsValid(), "Only valid tags can be checked!");

  if (IsTagInAllocatedRange(tag))
  {
    return (m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()] & XII_BIT(tag.m_uiBitIndex)) != 0;
  }
  else
  {
    return false;
  }
}

template <typename BlockStorageAllocator>
bool xiiTagSetTemplate<BlockStorageAllocator>::IsAnySet(const xiiTagSetTemplate& otherSet) const
{
  // If any of the sets is empty nothing can match
  if (IsEmpty() || otherSet.IsEmpty())
    return false;

  // Calculate range to compare
  const xiiUInt32 uiMaxBlockStart = xiiMath::Max(GetTagBlockStart(), otherSet.GetTagBlockStart());
  const xiiUInt32 uiMinBlockEnd   = xiiMath::Min(GetTagBlockEnd(), otherSet.GetTagBlockEnd());

  if (uiMaxBlockStart > uiMinBlockEnd)
    return false;

  for (xiiUInt32 i = uiMaxBlockStart; i < uiMinBlockEnd; ++i)
  {
    const xiiUInt32 uiThisBlockStorageIndex  = i - GetTagBlockStart();
    const xiiUInt32 uiOtherBlockStorageIndex = i - otherSet.GetTagBlockStart();

    if ((m_TagBlocks[uiThisBlockStorageIndex] & otherSet.m_TagBlocks[uiOtherBlockStorageIndex]) != 0)
      return true;
  }
  return false;
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiUInt32 xiiTagSetTemplate<BlockStorageAllocator>::GetNumTagsSet() const
{
  return GetTagCount();
}

template <typename BlockStorageAllocator>
XII_ALWAYS_INLINE bool xiiTagSetTemplate<BlockStorageAllocator>::IsEmpty() const
{
  return GetTagCount() == 0;
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::Clear()
{
  m_TagBlocks.Clear();
  SetTagBlockStart(xiiSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::SetByName(xiiStringView sTag)
{
  const xiiTag& tag = xiiTagRegistry::GetGlobalRegistry().RegisterTag(sTag);
  Set(tag);
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::RemoveByName(xiiStringView sTag)
{
  if (const xiiTag* tag = xiiTagRegistry::GetGlobalRegistry().GetTagByName(xiiTempHashedString(sTag)))
  {
    Remove(*tag);
  }
}

template <typename BlockStorageAllocator>
bool xiiTagSetTemplate<BlockStorageAllocator>::IsSetByName(xiiStringView sTag) const
{
  if (const xiiTag* tag = xiiTagRegistry::GetGlobalRegistry().GetTagByName(xiiTempHashedString(sTag)))
  {
    return IsSet(*tag);
  }
  return false;
}

template <typename BlockStorageAllocator>
XII_ALWAYS_INLINE bool xiiTagSetTemplate<BlockStorageAllocator>::IsTagInAllocatedRange(const xiiTag& Tag) const
{
  return Tag.m_uiBlockIndex >= GetTagBlockStart() && Tag.m_uiBlockIndex < GetTagBlockEnd();
}

template <typename BlockStorageAllocator>
void xiiTagSetTemplate<BlockStorageAllocator>::Reallocate(xiiUInt32 uiNewTagBlockStart, xiiUInt32 uiNewMaxBlockIndex)
{
  XII_ASSERT_DEV(uiNewTagBlockStart < xiiSmallInvalidIndex, "Tag block start is too big");
  const xiiUInt16 uiNewBlockArraySize = static_cast<xiiUInt16>((uiNewMaxBlockIndex - uiNewTagBlockStart) + 1);

  // Early out for non-filled tag sets
  if (m_TagBlocks.IsEmpty())
  {
    m_TagBlocks.SetCount(uiNewBlockArraySize);
    SetTagBlockStart(static_cast<xiiUInt16>(uiNewTagBlockStart));

    return;
  }

  XII_ASSERT_DEBUG(uiNewTagBlockStart <= GetTagBlockStart(), "New block start must be smaller or equal to current block start!");

  xiiSmallArray<xiiUInt64, 32, BlockStorageAllocator> helperArray;
  helperArray.SetCount(uiNewBlockArraySize);

  const xiiUInt32 uiOldBlockStartOffset = GetTagBlockStart() - uiNewTagBlockStart;

  // Copy old data to the new array
  xiiMemoryUtils::Copy(helperArray.GetData() + uiOldBlockStartOffset, m_TagBlocks.GetData(), m_TagBlocks.GetCount());

  // Use array ptr copy assignment so it doesn't modify the user data in m_TagBlocks
  m_TagBlocks = helperArray.GetArrayPtr();
  SetTagBlockStart(static_cast<xiiUInt16>(uiNewTagBlockStart));
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiUInt16 xiiTagSetTemplate<BlockStorageAllocator>::GetTagBlockStart() const
{
  return m_TagBlocks.template GetUserData<UserData>().m_uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiUInt16 xiiTagSetTemplate<BlockStorageAllocator>::GetTagBlockEnd() const
{
  return static_cast<xiiUInt16>(GetTagBlockStart() + m_TagBlocks.GetCount());
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiTagSetTemplate<BlockStorageAllocator>::SetTagBlockStart(xiiUInt16 uiTagBlockStart)
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagBlockStart = uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE xiiUInt16 xiiTagSetTemplate<BlockStorageAllocator>::GetTagCount() const
{
  return m_TagBlocks.template GetUserData<UserData>().m_uiTagCount;
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiTagSetTemplate<BlockStorageAllocator>::SetTagCount(xiiUInt16 uiTagCount)
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount = uiTagCount;
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiTagSetTemplate<BlockStorageAllocator>::IncreaseTagCount()
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount++;
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
XII_ALWAYS_INLINE void xiiTagSetTemplate<BlockStorageAllocator>::DecreaseTagCount()
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount--;
}

static xiiTypeVersion s_TagSetVersion = 1;

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
void xiiTagSetTemplate<BlockStorageAllocator>::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt16 uiNumTags = static_cast<xiiUInt16>(GetNumTagsSet());
  ref_stream << uiNumTags;

  ref_stream.WriteVersion(s_TagSetVersion);

  for (Iterator it = GetIterator(); it.IsValid(); ++it)
  {
    const xiiTag& tag = *it;

    ref_stream << tag.m_sTagString;
  }
}

template <typename BlockStorageAllocator /*= xiiDefaultAllocatorWrapper*/>
void xiiTagSetTemplate<BlockStorageAllocator>::Load(xiiStreamReader& ref_stream, xiiTagRegistry& ref_registry)
{
  xiiUInt16 uiNumTags = 0;
  ref_stream >> uiNumTags;

  // Manually read version value since 0 can be a valid version here
  xiiTypeVersion version;
  ref_stream.ReadWordValue(&version).IgnoreResult();

  if (version == 0)
  {
    for (xiiUInt32 i = 0; i < uiNumTags; ++i)
    {
      xiiUInt32 uiTagMurmurHash = 0;
      ref_stream >> uiTagMurmurHash;

      if (const xiiTag* pTag = ref_registry.GetTagByMurmurHash(uiTagMurmurHash))
      {
        Set(*pTag);
      }
    }
  }
  else
  {
    for (xiiUInt32 i = 0; i < uiNumTags; ++i)
    {
      xiiHashedString tagString;
      ref_stream >> tagString;

      const xiiTag& tag = ref_registry.RegisterTag(tagString);
      Set(tag);
    }
  }
}
