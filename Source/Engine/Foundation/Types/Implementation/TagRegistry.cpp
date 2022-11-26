#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>

static xiiTagRegistry s_GlobalRegistry;

xiiTagRegistry::xiiTagRegistry() {}

xiiTagRegistry& xiiTagRegistry::GetGlobalRegistry()
{
  return s_GlobalRegistry;
}

const xiiTag& xiiTagRegistry::RegisterTag(xiiStringView sTagString)
{
  xiiHashedString TagString;
  TagString.Assign(sTagString);

  return RegisterTag(TagString);
}

const xiiTag& xiiTagRegistry::RegisterTag(const xiiHashedString& TagString)
{
  XII_LOCK(m_TagRegistryMutex);

  // Early out if the tag is already registered
  const xiiTag* pResult = GetTagByName(TagString);

  if (pResult != nullptr)
    return *pResult;

  const xiiUInt32 uiNextTagIndex = m_TagsByIndex.GetCount();

  // Build temp tag
  xiiTag TempTag;
  TempTag.m_uiBlockIndex = uiNextTagIndex / (sizeof(xiiTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex   = uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(xiiTagSetBlockStorage) * 8);
  TempTag.m_sTagString   = TagString;

  // Store the tag
  auto it = m_RegisteredTags.Insert(TagString, TempTag);

  m_TagsByIndex.PushBack(&it.Value());

  xiiLog::Debug("Registered Tag '{0}'", TagString);
  return *m_TagsByIndex.PeekBack();
}

const xiiTag* xiiTagRegistry::GetTagByName(const xiiTempHashedString& TagString) const
{
  XII_LOCK(m_TagRegistryMutex);

  auto It = m_RegisteredTags.Find(TagString);
  if (It.IsValid())
  {
    return &It.Value();
  }

  return nullptr;
}

const xiiTag* xiiTagRegistry::GetTagByMurmurHash(xiiUInt32 uiMurmurHash) const
{
  XII_LOCK(m_TagRegistryMutex);

  for (xiiTag* pTag : m_TagsByIndex)
  {
    if (xiiHashingUtils::MurmurHash32String(pTag->GetTagString()) == uiMurmurHash)
    {
      return pTag;
    }
  }

  return nullptr;
}

const xiiTag* xiiTagRegistry::GetTagByIndex(xiiUInt32 uiIndex) const
{
  XII_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex[uiIndex];
}

xiiUInt32 xiiTagRegistry::GetNumTags() const
{
  XII_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex.GetCount();
}

xiiResult xiiTagRegistry::Load(xiiStreamReader& stream)
{
  XII_LOCK(m_TagRegistryMutex);

  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  if (uiVersion != 1)
  {
    xiiLog::Error("Invalid xiiTagRegistry version {0}", uiVersion);
    return XII_FAILURE;
  }

  xiiUInt32 uiNumTags = 0;
  stream >> uiNumTags;

  if (uiNumTags > 16 * 1024)
  {
    xiiLog::Error("xiiTagRegistry::Load, unreasonable amount of tags {0}, cancelling load.", uiNumTags);
    return XII_FAILURE;
  }

  xiiStringBuilder temp;
  for (xiiUInt32 i = 0; i < uiNumTags; ++i)
  {
    stream >> temp;

    RegisterTag(temp);
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_TagRegistry);
