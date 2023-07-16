#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Logging/Log.h>

void operator<<(xiiStreamWriter& ref_stream, const xiiArchiveStoredString& value)
{
  ref_stream << value.m_uiLowerCaseHash;
  ref_stream << value.m_uiSrcStringOffset;
}

void operator>>(xiiStreamReader& ref_stream, xiiArchiveStoredString& value)
{
  ref_stream >> value.m_uiLowerCaseHash;
  ref_stream >> value.m_uiSrcStringOffset;
}

xiiUInt32 xiiArchiveTOC::FindEntry(xiiStringView sFile) const
{
  xiiStringBuilder sLowerCasePath = sFile;
  sLowerCasePath.ToLower();

  xiiUInt32 uiIndex;

  xiiArchiveLookupString lookup(xiiHashingUtils::StringHash(sLowerCasePath.GetView()), sLowerCasePath, m_AllPathStrings);

  if (!m_PathToEntryIndex.TryGetValue(lookup, uiIndex))
    return xiiInvalidIndex;

  XII_ASSERT_DEBUG(sFile.IsEqual_NoCase(GetEntryPathString(uiIndex)), "Hash table corruption detected.");

  return uiIndex;
}

xiiStringView xiiArchiveTOC::GetEntryPathString(xiiUInt32 uiEntryIdx) const
{
  return reinterpret_cast<const char*>(&m_AllPathStrings[m_Entries[uiEntryIdx].m_uiPathStringOffset]);
}

xiiResult xiiArchiveTOC::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(2);

  XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(m_Entries));

  // write the hash of a known string to the archive, to detect hash function changes
  xiiUInt64 uiStringHash = xiiHashingUtils::StringHash("xiiArchive");
  ref_stream << uiStringHash;

  XII_SUCCEED_OR_RETURN(ref_stream.WriteHashTable(m_PathToEntryIndex));

  XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(m_AllPathStrings));

  return XII_SUCCESS;
}

struct xiiOldTempHashedString
{
  xiiUInt32 m_uiHash = 0;

  xiiResult Deserialize(xiiStreamReader& r)
  {
    r >> m_uiHash;
    return XII_SUCCESS;
  }

  bool operator==(const xiiOldTempHashedString& rhs) const
  {
    return m_uiHash == rhs.m_uiHash;
  }
};

template <>
struct xiiHashHelper<xiiOldTempHashedString>
{
  static xiiUInt32 Hash(const xiiOldTempHashedString& value)
  {
    return value.m_uiHash;
  }

  static bool Equal(const xiiOldTempHashedString& a, const xiiOldTempHashedString& b) { return a == b; }
};

xiiResult xiiArchiveTOC::Deserialize(xiiStreamReader& ref_stream, xiiUInt8 uiArchiveVersion)
{
  XII_ASSERT_ALWAYS(uiArchiveVersion <= 4, "Unsupported archive version {}", uiArchiveVersion);

  // we don't use the TOC version anymore, but the archive version instead
  const xiiTypeVersion version = ref_stream.ReadVersion(2);

  XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(m_Entries));

  bool bRecreateStringHashes = true;

  if (version == 1)
  {
    // read and discard the data, it is regenerated below
    xiiHashTable<xiiOldTempHashedString, xiiUInt32> m_PathToIndex;
    XII_SUCCEED_OR_RETURN(ref_stream.ReadHashTable(m_PathToIndex));
  }
  else
  {
    if (uiArchiveVersion >= 4)
    {
      // read the hash of a known string from the archive, to detect hash function changes
      xiiUInt64 uiStringHash = 0;
      ref_stream >> uiStringHash;

      if (uiStringHash == xiiHashingUtils::StringHash("xiiArchive"))
      {
        bRecreateStringHashes = false;
      }
    }

    XII_SUCCEED_OR_RETURN(ref_stream.ReadHashTable(m_PathToEntryIndex));
  }

  XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(m_AllPathStrings));

  if (bRecreateStringHashes)
  {
    xiiLog::Info("Archive uses older string hashing, recomputing hashes.");

    // version 1 stores an older way for the path/hash -> entry lookup table, which is prone to hash collisions
    // in this case, rebuild the new hash table on the fly
    //
    // version 2 used MurmurHash
    // version 3 switched to 32 bit xxHash
    // version 4 switched to 64 bit hashes

    const xiiUInt32 uiNumEntries = m_Entries.GetCount();
    m_PathToEntryIndex.Clear();
    m_PathToEntryIndex.Reserve(uiNumEntries);

    xiiStringBuilder sLowerCasePath;

    for (xiiUInt32 i = 0; i < uiNumEntries; i++)
    {
      const xiiUInt32 uiSrcStringOffset = m_Entries[i].m_uiPathStringOffset;

      xiiStringView sEntryString = GetEntryPathString(i);

      sLowerCasePath = sEntryString;
      sLowerCasePath.ToLower();

      // cut off the upper 32 bit, we don't need them here
      const xiiUInt32 uiLowerCaseHash = xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sLowerCasePath.GetView()) & 0xFFFFFFFFllu);

      m_PathToEntryIndex.Insert(xiiArchiveStoredString(uiLowerCaseHash, uiSrcStringOffset), i);

      // Verify that the conversion worked
      XII_ASSERT_DEBUG(FindEntry(sEntryString) == i, "Hashed path retrieval did not yield inserted index");
    }
  }

  // path strings mustn't be empty and must be zero-terminated
  if (m_AllPathStrings.IsEmpty() || m_AllPathStrings.PeekBack() != '\0')
  {
    xiiLog::Error("Archive is corrupt. Invalid string data.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveEntry::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream << m_uiDataStartOffset;
  ref_stream << m_uiUncompressedDataSize;
  ref_stream << m_uiStoredDataSize;
  ref_stream << (xiiUInt8)m_CompressionMode;
  ref_stream << m_uiPathStringOffset;

  return XII_SUCCESS;
}

xiiResult xiiArchiveEntry::Deserialize(xiiStreamReader& ref_stream)
{
  ref_stream >> m_uiDataStartOffset;
  ref_stream >> m_uiUncompressedDataSize;
  ref_stream >> m_uiStoredDataSize;

  xiiUInt8 uiCompressionMode = 0;
  ref_stream >> uiCompressionMode;
  m_CompressionMode = (xiiArchiveCompressionMode)uiCompressionMode;
  ref_stream >> m_uiPathStringOffset;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_Archive);
