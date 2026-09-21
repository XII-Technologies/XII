/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

class xiiRawMemoryStreamReader;

/// Compression modes for xiiArchive file entries
enum class xiiArchiveCompressionMode : xiiUInt8
{
  Uncompressed = 0U,
  Compressed_zstd,
};

/// Data for a single file entry in a xiiArchive file
class XII_FOUNDATION_DLL xiiArchiveEntry
{
public:
  xiiUInt64                 m_uiDataStartOffset      = 0; ///< Byte offset for where the file's (compressed) data stream starts in the xiiArchive
  xiiUInt64                 m_uiUncompressedDataSize = 0; ///< Size of the original uncompressed data.
  xiiUInt64                 m_uiStoredDataSize       = 0; ///< The amount of (compressed) bytes actually stored in the xiiArchive.
  xiiUInt32                 m_uiPathStringOffset     = 0; ///< Byte offset into xiiArchiveTOC::m_AllPathStrings where the path string for this entry resides.
  xiiArchiveCompressionMode m_CompressionMode        = xiiArchiveCompressionMode::Uncompressed;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

/// Helper class to store a hashed string for quick lookup in the archive TOC
///
/// Stores a hash of the lower case string for quick comparison.
/// Additionally stores an offset into the xiiArchiveTOC::m_AllPathStrings array for final validation, to prevent hash collisions.
/// The proper string lookup with hash collision check only works together with xiiArchiveLookupString, which has the necessary context
/// to index the xiiArchiveTOC::m_AllPathStrings array.
class XII_FOUNDATION_DLL xiiArchiveStoredString
{
public:
  XII_DECLARE_POD_TYPE();

  xiiArchiveStoredString() = default;

  xiiArchiveStoredString(xiiUInt64 uiLowerCaseHash, xiiUInt32 uiSrcStringOffset) :
    m_uiLowerCaseHash(xiiHashingUtils::StringHashTo32(uiLowerCaseHash)), m_uiSrcStringOffset(uiSrcStringOffset)
  {
  }

  xiiUInt32 m_uiLowerCaseHash;
  xiiUInt32 m_uiSrcStringOffset;
};

void operator<<(xiiStreamWriter& ref_stream, const xiiArchiveStoredString& value);
void operator>>(xiiStreamReader& ref_stream, xiiArchiveStoredString& value);

/// Helper class for looking up path strings in xiiArchiveTOC::FindEntry()
///
/// Only works together with xiiArchiveStoredString.
class xiiArchiveLookupString
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiArchiveLookupString);

public:
  XII_DECLARE_POD_TYPE();

  xiiArchiveLookupString(xiiUInt64 uiLowerCaseHash, xiiStringView sString, const xiiDynamicArray<xiiUInt8>& archiveAllPathStrings) :
    m_uiLowerCaseHash(xiiHashingUtils::StringHashTo32(uiLowerCaseHash)), m_sString(sString), m_ArchiveAllPathStrings(archiveAllPathStrings)
  {
  }

  xiiUInt32                        m_uiLowerCaseHash;
  xiiStringView                    m_sString;
  const xiiDynamicArray<xiiUInt8>& m_ArchiveAllPathStrings;
};

/// Functions to enable xiiHashTable to 1) store xiiArchiveStoredString and 2) lookup strings efficiently with a xiiArchiveLookupString
template <>
struct xiiHashHelper<xiiArchiveStoredString>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiArchiveStoredString& hs) { return hs.m_uiLowerCaseHash; }
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiArchiveLookupString& hs) { return hs.m_uiLowerCaseHash; }

  XII_ALWAYS_INLINE static bool Equal(const xiiArchiveStoredString& a, const xiiArchiveStoredString& b) { return a.m_uiSrcStringOffset == b.m_uiSrcStringOffset; }

  XII_ALWAYS_INLINE static bool Equal(const xiiArchiveStoredString& a, const xiiArchiveLookupString& b)
  {
    // in case that we want to lookup a string using a xiiArchiveLookupString, we validate
    // that the stored string is actually equal to the lookup string, to enable handling of hash collisions
    return b.m_sString.IsEqual_NoCase(reinterpret_cast<const char*>(&b.m_ArchiveAllPathStrings[a.m_uiSrcStringOffset]));
  }
};

/// Table-of-contents for a xiiArchive file
class XII_FOUNDATION_DLL xiiArchiveTOC
{
public:
  /// all files stored in the xiiArchive
  xiiDynamicArray<xiiArchiveEntry> m_Entries;
  /// allows to map a hashed string to the index of the file entry for the file path
  xiiHashTable<xiiArchiveStoredString, xiiUInt32> m_PathToEntryIndex;
  /// one large array holding all path strings for the file entries, to reduce allocations
  xiiDynamicArray<xiiUInt8> m_AllPathStrings;

  /// Returns the entry index for the given file or xiiInvalidIndex, if not found.
  xiiUInt32 FindEntry(xiiStringView sFile) const;

  xiiUInt32 AddPathString(xiiStringView sPathString);

  void RebuildPathToEntryHashes();

  xiiStringView GetEntryPathString(xiiUInt32 uiEntryIdx) const;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream, xiiUInt8 uiArchiveVersion);
};
