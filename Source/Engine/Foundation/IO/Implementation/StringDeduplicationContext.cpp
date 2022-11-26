#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StringDeduplicationContext.h>

static const xiiTypeVersion s_uiStringDeduplicationVersion = 1;

XII_IMPLEMENT_SERIALIZATION_CONTEXT(xiiStringDeduplicationWriteContext)

xiiStringDeduplicationWriteContext::xiiStringDeduplicationWriteContext(xiiStreamWriter& OriginalStream) :
  xiiSerializationContext(), m_OriginalStream(OriginalStream)
{
}

xiiStringDeduplicationWriteContext::~xiiStringDeduplicationWriteContext() = default;

xiiStreamWriter& xiiStringDeduplicationWriteContext::Begin()
{
  XII_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a string deduplication context.");

  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

xiiResult xiiStringDeduplicationWriteContext::End()
{
  // We set the context manual to null here since we need normal
  // string serialization to write the de-duplicated map
  SetContext(nullptr);

  m_OriginalStream.WriteVersion(s_uiStringDeduplicationVersion);

  const xiiUInt64 uiNumEntries = m_DeduplicatedStrings.GetCount();
  m_OriginalStream << uiNumEntries;

  xiiMap<xiiUInt32, xiiHybridString<64>> StringsSortedByIndex;

  // Build a new map from index to string so we can use a plain
  // array for serialization and lookup purposes
  for (const auto& it : m_DeduplicatedStrings)
  {
    StringsSortedByIndex.Insert(it.Value(), std::move(it.Key()));
  }

  // Write the new map entries, but just the strings since the indices are linear ascending
  for (const auto& it : StringsSortedByIndex)
  {
    m_OriginalStream << it.Value();
  }

  // Now append the original stream
  XII_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(m_OriginalStream));

  return XII_SUCCESS;
}

void xiiStringDeduplicationWriteContext::SerializeString(const xiiStringView& String, xiiStreamWriter& Writer)
{
  bool bAlreadDeduplicated = false;
  auto it                  = m_DeduplicatedStrings.FindOrAdd(String, &bAlreadDeduplicated);

  if (!bAlreadDeduplicated)
  {
    it.Value() = m_DeduplicatedStrings.GetCount() - 1;
  }

  Writer << it.Value();
}

xiiUInt32 xiiStringDeduplicationWriteContext::GetUniqueStringCount() const
{
  return m_DeduplicatedStrings.GetCount();
}


XII_IMPLEMENT_SERIALIZATION_CONTEXT(xiiStringDeduplicationReadContext)

xiiStringDeduplicationReadContext::xiiStringDeduplicationReadContext(xiiStreamReader& Stream) :
  xiiSerializationContext()
{
  // We set the context manually to nullptr to get the original string table
  SetContext(nullptr);

  // Read the string table first
  /*auto version =*/Stream.ReadVersion(s_uiStringDeduplicationVersion);

  xiiUInt64 uiNumEntries = 0;
  Stream >> uiNumEntries;

  for (xiiUInt64 i = 0; i < uiNumEntries; ++i)
  {
    xiiStringBuilder Builder;
    Stream >> Builder;

    m_DeduplicatedStrings.ExpandAndGetRef() = std::move(Builder);
  }

  SetContext(this);
}

xiiStringDeduplicationReadContext::~xiiStringDeduplicationReadContext() = default;

xiiStringView xiiStringDeduplicationReadContext::DeserializeString(xiiStreamReader& Reader)
{
  xiiUInt32 uiIndex;
  Reader >> uiIndex;

  return m_DeduplicatedStrings[uiIndex].GetView();
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StringDeduplicationContext);
